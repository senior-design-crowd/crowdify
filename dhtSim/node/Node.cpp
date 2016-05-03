#include <mpi.h>
#include <math.h>
#include <limits>
#include <vector>
#include <algorithm>
#include <chrono>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>

using namespace std;

#include "DHTArea.h"
#include "nodeMessages.h"

#include "Node.h"

Node::Node(int mpiRank, int rootRank, ofstream* fp)
	: 
	// when a new node joins the network and asks us to split our DHT area in half,
	// we split it along either the x or y axis. we alternate the axis that we split
	// our area on so that we don't end up with a very thin and long area after many
	// splits, but rather either a perfect square or a rectangle with one
	// side length being only half the other
	m_nextSplitAxis(-1),
	// for the simulation, we receive a signal from the root node to
	// either come alive and join the network or just die without
	// notifying the other nodes
	m_bAlive(false),
	// the rectangle coordinates representing our DHT area in the network
	m_dhtArea(),
	// each node needs a unique port number to listen on
	m_blockTransferer(fp, 50000 + mpiRank),
	// the random number generator and distribution to generate random
	// coordinates within the DHT network
	m_randGenerator((unsigned int)chrono::system_clock::now().time_since_epoch().count()),
	m_randomAreaCoordGenerator(0.0f, 1.0f),
	m_mpiRank(mpiRank),
	m_rootRank(rootRank),
	m_fp(*fp),
	// how long to wait after sending an update to our neighbors
	// until we send the next update
	m_timeUntilUpdate(std::chrono::seconds(2)),
	// the timeout when a node will consider its neighbor dead if it hasn't
	// received an update by then
	m_timeUntilNeighborConsideredOffline(std::chrono::seconds(5)),
	// when a node times out, we wait a certain amount of time
	// proportional to our current DHT area until we
	// send all that node's neighbors a DHT area takeover
	// message to seal the hole in the network. this is
	// that proportionality constant, i.e. we will multiply
	// this constant by our current DHT area and wait that
	// amount of time until sending the takeover message
	m_takeoverTimerPerArea(std::chrono::seconds(2)),
	m_edgeAbutEpsilon(1e-4f),
	m_bSeenDeath(false)
{
	stringstream ss;
	ss << "tcp://127.0.0.1:" << m_mpiRank + 60000;
	m_ownBlockStoreAddr = ss.str();

	int mpiSize;
	MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

	for (int i = 0; i < mpiSize - 1; ++i) {
		ss.str("");
		ss << "tcp://127.0.0.1:" << i + 50000;
		m_vNeighborNodeAddrs.push_back(ss.str());
	}
}

const DHTArea& Node::GetDHTArea() const
{
	return m_dhtArea;
}

bool Node::SplitDHTArea(int newNode, DHTArea& newDHTArea, std::vector<NodeNeighbor>& r_vNewNodeNeighbors, vector<vector<NodeNeighbor>>& r_vNewNodeNeighborsOfNeighbors)
{
	m_fp << LogOutputHeader() << "\tSplitting node DHT area." << endl;

	newDHTArea = m_dhtArea.SplitRegion(m_nextSplitAxis);

	// split the next axis
	m_nextSplitAxis = (m_nextSplitAxis + 1) % 2;

	m_fp << LogOutputHeader() << "\tMy new DHT area:" << endl
		<< m_dhtArea
		<< endl
		<< "\tNew Node DHT area:" << endl
		<< newDHTArea
		<< endl;

	r_vNewNodeNeighbors.clear();
	r_vNewNodeNeighborsOfNeighbors.clear();

	m_fp << LogOutputHeader() << "\tFinding new node neighbors:" << endl;

	// add all the neighbors
	vector<vector<NodeNeighbor>>::const_iterator j = m_vNeighborsOfNeighbors.begin();
	for (vector<NodeNeighbor>::const_iterator i = m_vNeighbors.begin(); i != m_vNeighbors.end(); ++i, ++j) {
		m_fp << LogOutputHeader() << "\t\tNode #: " << i->nodeNum << endl
			<< i->neighborArea
			<< endl;

		if (newDHTArea.IsNeighbor(i->neighborArea)) {
			m_fp << LogOutputHeader() << "Is neighbor." << endl;
			r_vNewNodeNeighbors.push_back(*i);
			r_vNewNodeNeighborsOfNeighbors.push_back(*j);
		} else {
			m_fp << LogOutputHeader() << "Is not neighbor." << endl;
		}
	}

	// add the newly created node to our neighbors
	NodeNeighbor newNeighbor;
	newNeighbor.nodeNum = newNode;
	newNeighbor.neighborArea = newDHTArea;
	m_vNeighbors.push_back(newNeighbor);
	m_vNeighborsOfNeighbors.push_back(r_vNewNodeNeighbors);
	m_neighborTimeSinceLastUpdate.push_back(chrono::high_resolution_clock::now());

	r_vNewNodeNeighbors.push_back({ m_dhtArea, m_mpiRank });
	r_vNewNodeNeighborsOfNeighbors.push_back(m_vNeighbors);

	m_fp << LogOutputHeader() << "\tNew node neighbors:" << endl;
	for (vector<NodeNeighbor>::const_iterator i = r_vNewNodeNeighbors.begin(); i != r_vNewNodeNeighbors.end(); ++i) {
		m_fp << endl
			<< "\t\tNode #: " << i->nodeNum << endl
			<< i->neighborArea
			<< endl;
	}

	SendNeighborsDHTUpdate();
	PruneNeighbors();

	return true;
}

bool Node::InitializeDHTArea(int aliveNode)
{
	// we're the only alive node in the network
	if (aliveNode == -1) {
		DHTRegion entireNetwork = { 0.0f, 1.0f, 0.0f, 1.0f };
		m_dhtArea.AddRegion(entireNetwork);
		m_vNeighborsOfNeighbors.push_back(vector<NodeNeighbor>());
		return true;
	}

	InterNodeMessage::DHTPointOwnerRequest dhtPoint = { m_randomAreaCoordGenerator(m_randGenerator), m_randomAreaCoordGenerator(m_randGenerator), m_mpiRank };
	MPI_Send((void*)&dhtPoint, 1, InterNodeMessage::MPI_DHTPointOwnerRequest, aliveNode, InterNodeMessageTags::REQUEST_DHT_POINT_OWNER, MPI_COMM_WORLD);

	NotifyRootOfMsg(NodeToNodeMsgTypes::SENDING, aliveNode, m_rootRank);

	m_fp << LogOutputHeader() << "Requested DHT point owner of (" << dhtPoint.x << ", " << dhtPoint.y << ") from: " << aliveNode << endl;

	MPI_Status mpiStatus;
	InterNodeMessage::DHTPointOwnerResponse dhtPointOwner;
	MPI_Recv((void*)&dhtPointOwner, 1, InterNodeMessage::MPI_DHTPointOwnerResponse, MPI_ANY_SOURCE, InterNodeMessageTags::RESPONSE_DHT_POINT_OWNER, MPI_COMM_WORLD, &mpiStatus);

	NotifyRootOfMsg(NodeToNodeMsgTypes::RECEIVING, mpiStatus.MPI_SOURCE, m_rootRank);

	m_fp << LogOutputHeader() << "Got DHT point owner response from " << mpiStatus.MPI_SOURCE << '.' << endl;
	m_fp << LogOutputHeader() << "DHT point owner: " << dhtPointOwner.pointOwnerNode << endl;

	{
		// sending dummy data as this message doesn't require any data
		int tmp = 0;
		MPI_Send((void*)&tmp, 1, MPI_INT, dhtPointOwner.pointOwnerNode, InterNodeMessageTags::REQUEST_DHT_AREA, MPI_COMM_WORLD);

		NotifyRootOfMsg(NodeToNodeMsgTypes::SENDING, dhtPointOwner.pointOwnerNode, m_rootRank);

		m_fp << LogOutputHeader() << "Sent DHT area request to: " << dhtPointOwner.pointOwnerNode << endl;
	}

	NotifyRootOfMsg(NodeToNodeMsgTypes::RECEIVING, dhtPointOwner.pointOwnerNode, m_rootRank);

	InterNodeMessage::DHTAreaResponse dhtAreaResponse;

	m_fp << LogOutputHeader() << "Receiving DHT area response." << endl;

	MPI_Recv((void*)&dhtAreaResponse, 1, InterNodeMessage::MPI_DHTAreaResponse, dhtPointOwner.pointOwnerNode, InterNodeMessageTags::RESPONSE_DHT_AREA, MPI_COMM_WORLD, &mpiStatus);

	m_fp << LogOutputHeader() << "\tReceived " << dhtAreaResponse.numNeighbors << " neighbors." << endl
		<< "\tNext split axis: " << dhtAreaResponse.nextAxisToSplit << endl;

	if (!m_dhtArea.RecvOverMPI(dhtPointOwner.pointOwnerNode, InterNodeMessageTags::RESPONSE_DHT_AREA)) {
		m_fp << LogOutputHeader() << "Failed to receive DHT area." << endl;
		return false;
	}

	m_vNeighbors.resize(dhtAreaResponse.numNeighbors);
	m_neighborTimeSinceLastUpdate.resize(dhtAreaResponse.numNeighbors, chrono::high_resolution_clock::now());

	RecvNeighborsOverMPI(m_vNeighbors, dhtPointOwner.pointOwnerNode, InterNodeMessageTags::RESPONSE_DHT_AREA);

	m_vNeighborsOfNeighbors.clear();

	for (int i = 0; i < dhtAreaResponse.numNeighbors; ++i) {
		int numNeighborsOfNeighbor;
		MPI_Recv((void*)&numNeighborsOfNeighbor,
			1,
			MPI_INT,
			mpiStatus.MPI_SOURCE,
			InterNodeMessageTags::RESPONSE_DHT_AREA,
			MPI_COMM_WORLD,
			&mpiStatus);

		m_vNeighborsOfNeighbors.push_back(vector<NodeNeighbor>(numNeighborsOfNeighbor));
		
		if (numNeighborsOfNeighbor > 0)
		{
			RecvNeighborsOverMPI(m_vNeighborsOfNeighbors[m_vNeighborsOfNeighbors.size() - 1], dhtPointOwner.pointOwnerNode, InterNodeMessageTags::RESPONSE_DHT_AREA);
		}
	}

	m_nextSplitAxis = dhtAreaResponse.nextAxisToSplit;

	m_fp << LogOutputHeader() << "Got DHT area response from " << dhtPointOwner.pointOwnerNode << ":" << endl
		<< "\tNew DHT Area:" << endl
		<< m_dhtArea << endl
		<< "\t" << dhtAreaResponse.numNeighbors << " Neighbor(s):" << endl;
	
	for (size_t i = 0; i < m_vNeighbors.size(); ++i) {
		m_fp << "\t\tNeighbor " << i << endl
			<< "\t\t\tNode #: " << m_vNeighbors[i].nodeNum << endl
			<< m_vNeighbors[i].neighborArea << endl
			<< "\t\t\t\t" << m_vNeighborsOfNeighbors[i].size() << " Neighbors: ";

		for (vector<NodeNeighbor>::const_iterator j = m_vNeighborsOfNeighbors[i].begin(); j != m_vNeighborsOfNeighbors[i].end(); ++j) {
			m_fp << j->nodeNum << " ";
		}

		m_fp << endl;	
	}

	m_fp << endl << endl;

	m_timeOfLastNeighborUpdate = chrono::high_resolution_clock::now();
	SendNeighborsDHTUpdate();
	SendRootNeighborsUpdate();

	return true;
}

const int Node::GetNextSplitAxis() const
{
	return m_nextSplitAxis;
}

int Node::GetNearestNeighbor(float x, float y) const
{
	// return -1 if point is inside our DHT area
	if (m_dhtArea.DistanceFromPt(x,y) == 0.0f) {
		m_fp << LogOutputHeader() << "\t\tPoint is inside our DHT area." << endl;
		m_fp.flush();
		return -1;
	}

	m_fp << LogOutputHeader() << "\t\tPoint is not inside our DHT area." << endl;

	vector<NodeNeighbor>::const_iterator	closestNeighbor = m_vNeighbors.end();
	float									closestNeighborDist = FLT_MAX;

	for (vector<NodeNeighbor>::const_iterator i = m_vNeighbors.begin(); i != m_vNeighbors.end(); ++i) {
		m_fp << LogOutputHeader() << "\t\tPoint dist from node " << i->nodeNum << ": ";
		
		float neighborDist = i->neighborArea.DistanceFromPt(x, y);

		m_fp << neighborDist << endl
			<< i->neighborArea
			<< endl;

		if (neighborDist < closestNeighborDist) {
			m_fp << LogOutputHeader() << "\t\tNode is new closest node." << endl;
			closestNeighbor = i;
			closestNeighborDist = neighborDist;
		}

		m_fp.flush();
	}

	// if we have no neighbors and the point isn't
	// inside our DHT area then we have an issue.
	// can't return -1 because that would indicate
	// its inside our own DHT area. so returning -2.
	if (closestNeighbor == m_vNeighbors.end()) {
		m_fp << LogOutputHeader() << "\t\tERROR: Couldn't find a neighbor that contains the point." << endl;
		m_fp.flush();
		return -2;
	}

	m_fp << LogOutputHeader() << "\t\tNode #";
	m_fp.flush();
	m_fp << closestNeighbor->nodeNum << " is new closest node." << endl;
	m_fp.flush();

	return closestNeighbor->nodeNum;
}

bool Node::UpdateNeighborDHTArea(int nodeNum, const DHTArea& dhtArea, const vector<NodeNeighbor>& neighborsOfNeighbor)
{
	OutputDebugMsg("In UpdateNeighborDHTArea");
	vector<NodeNeighbor>::iterator i = m_vNeighbors.begin();

	// is this a new neighbor or a current one?
	int neighborNum = 0;
	for (; i != m_vNeighbors.end(); ++i, ++neighborNum) {
		if (i->nodeNum == nodeNum) {
			break;
		}
	}

	if (i != m_vNeighbors.end()) {
		i->neighborArea = dhtArea;
		m_vNeighborsOfNeighbors[neighborNum] = neighborsOfNeighbor;
		m_neighborTimeSinceLastUpdate[neighborNum] = chrono::high_resolution_clock::now();

		// prune the neighbors in case
		// this neighbor is no longer our neighbor
		PruneNeighbors();
	} else {
		NodeNeighbor newNeighbor;
		newNeighbor.nodeNum = nodeNum;
		newNeighbor.neighborArea = dhtArea;

		m_vNeighbors.push_back(newNeighbor);
		m_vNeighborsOfNeighbors.push_back(neighborsOfNeighbor);
		m_neighborTimeSinceLastUpdate.push_back(chrono::high_resolution_clock::now());
		SendRootNeighborsUpdate();
	}

	OutputDebugMsg("Reached end of UpdateNeighborDHTArea");

	return true;
}

void Node::SendNeighborsDHTUpdate()
{
	OutputDebugMsg("In SendNeighborsDHTUpdate");

	int neighborUpdateIndex = 0;
	int numNeighbors = m_vNeighbors.size();

	m_fp << LogOutputHeader() << "Sending " << numNeighbors << " neighbors update of my DHT area." << endl;
	m_fp.flush();

	// set the timer to the point before the update is sent to the first neighbor.
	// that way we send the next update before the timeout hits for the first node
	// rather than setting the timer after the messages are sent so we send it
	// too late for the first node and in time for the last node.
	//
	// this is all based on the fact that it takes time to send the update to all
	// the neighbors
	m_timeOfLastNeighborUpdate = chrono::high_resolution_clock::now();

	// send each of our neighbors our DHT area and the DHT area and node number of all of our neighbors
	for (vector<NodeNeighbor>::const_iterator i = m_vNeighbors.begin(); i != m_vNeighbors.end(); ++i, ++neighborUpdateIndex) {
		m_fp << LogOutputHeader() << "\tSending neighbor Node #" << i->nodeNum << ": my DHT area" << endl;
		m_fp.flush();
		m_dhtArea.SendOverMPI(i->nodeNum, InterNodeMessageTags::NEIGHBOR_UPDATE);

		m_fp << LogOutputHeader() << "\tSending neighbor Node #" << i->nodeNum << ": neighbors (" << m_vNeighbors.size() << ")" << endl;
		m_fp.flush();

		SendNeighborsOverMPI(m_vNeighbors, i->nodeNum, InterNodeMessageTags::NEIGHBOR_UPDATE);
		NotifyRootOfMsg(NodeToNodeMsgTypes::SENDING, i->nodeNum, m_rootRank);
	}

	m_fp << LogOutputHeader() << "\tDone." << endl;
	m_fp.flush();

	OutputDebugMsg("Reached end of SendNeighborsDHTUpdate");
}

void Node::Update()
{
	OutputDebugMsg("In Update");

	m_blockTransferer.Update();

	if (m_blockTransferer.GetNumMsgsAvail() > 0) {
		BlockMsg	msg = m_blockTransferer.GetMsg();

		m_fp << LogOutputHeader() << "Received block transfer msg." << endl;

		double		hashCoords[2];

		m_hashEval.GetHashPoint((unsigned char*)msg.hash, msg.initial.hashLen, &hashCoords[0], &hashCoords[1]);

		m_fp << LogOutputHeader() << "\tBlock at coordinates: (" << hashCoords[0] << ", " << hashCoords[1] << ")" << endl;

		// if the file is contained within our own DHT area then store it in
		// our own block store
		if (m_dhtArea.DistanceFromPt(hashCoords[0], hashCoords[1]) == 0.0f) {
			m_fp << LogOutputHeader() << "\tBlock is within our own DHT area." << endl
				<< "\tSending to dummy block store: " << m_ownBlockStoreAddr << endl;
			m_blockTransferer.SendBlockTransferMsg(m_ownBlockStoreAddr.c_str(), msg);
		} else {
			m_fp << LogOutputHeader() << "\tGetting closest neighbor to block." << endl;
			int nearestNeighbor = GetNearestNeighbor(hashCoords[0], hashCoords[1]);
			m_fp << LogOutputHeader() << "\tNearest neighbor: " << nearestNeighbor << endl;
			m_fp.flush();

			if(nearestNeighbor > 0 && nearestNeighbor < m_vNeighborNodeAddrs.size()) {
				m_fp << LogOutputHeader() << "\tValid nearest neighbor, forwarding msg to: " << m_vNeighborNodeAddrs[nearestNeighbor] << endl;
				m_blockTransferer.SendBlockTransferMsg(m_vNeighborNodeAddrs[nearestNeighbor].c_str(), msg);
			}
		}
	}

	chrono::high_resolution_clock::time_point timeNow = chrono::high_resolution_clock::now();

	// check if its time to update our neighbors again
	chrono::milliseconds timeSinceLastUpdate = chrono::duration_cast<chrono::milliseconds>(timeNow - m_timeOfLastNeighborUpdate);
	if (timeSinceLastUpdate >= m_timeUntilUpdate) {
		SendNeighborsDHTUpdate();
	}

	// check if any of our neighbors may be offline
	int neighborNum = 0;

	for (size_t neighborNum = 0; neighborNum < m_vNeighbors.size(); ++neighborNum) {
		vector<chrono::high_resolution_clock::time_point>::const_iterator timerIt = m_neighborTimeSinceLastUpdate.begin() + neighborNum;
		OutputDebugMsg("timerIt constructed");
		timeNow = chrono::high_resolution_clock::now();

		// if we haven't received a neighbor update within a certain amount of time from one
		// of our neighbors, its considered offline and the DHT area takeover process begins
		chrono::milliseconds timeSinceUpdate = chrono::duration_cast<chrono::milliseconds>(timeNow - *timerIt);
		OutputDebugMsg("timeSinceUpdate constructed");

		if (timeSinceUpdate >= m_timeUntilNeighborConsideredOffline) {
			m_bSeenDeath = true;

			vector<NodeNeighbor>::const_iterator nodeIt = m_vNeighbors.begin() + neighborNum;
			vector<vector<NodeNeighbor>>::const_iterator nodeNeighborsIt = m_vNeighborsOfNeighbors.begin() + neighborNum;

			DeadNeighbor deadNeighbor;
			deadNeighbor.neighbor = *nodeIt;
			deadNeighbor.timeOfDeath = chrono::high_resolution_clock::now();
			deadNeighbor.neighborNeighbors = *nodeNeighborsIt;
			deadNeighbor.neighborsAllowingTakeover = vector<bool>(deadNeighbor.neighborNeighbors.size(), false);
			deadNeighbor.state = DeadNeighbor::INITIAL;

			OutputDebugMsg("deadNeighbor created");

			// make sure it is set that we are allowing ourselves to take over this node
			for (size_t neighborIndex = 0; neighborIndex < deadNeighbor.neighborNeighbors.size(); ++neighborIndex) {
				if (deadNeighbor.neighborNeighbors[neighborIndex].nodeNum == m_mpiRank) {
					deadNeighbor.neighborsAllowingTakeover[neighborIndex] = true;
				}
			}

			float totalDHTArea = m_dhtArea.TotalArea();
			deadNeighbor.timeUntilTakeover = m_takeoverTimerPerArea * totalDHTArea;

			m_fp << LogOutputHeader() << "Neighbor node #" << deadNeighbor.neighbor.nodeNum << " died." << endl
				<< "\tHasn't updated us for " << timeSinceUpdate.count() << " ms." << endl
				<< "\tThis node has DHT area of size " << totalDHTArea << endl
				<< "\tTaking over node in " << deadNeighbor.timeUntilTakeover.count() << " ms." << endl;
			
			OutputVecOperation(true);
			m_vDeadNeighbors.push_back(deadNeighbor);
			OutputVecOperation(true);
			m_vNeighbors.erase(nodeIt);
			OutputVecOperation(true);
			m_vNeighborsOfNeighbors.erase(nodeNeighborsIt);
			OutputVecOperation(true);
			m_neighborTimeSinceLastUpdate.erase(timerIt);
			OutputVecOperation(false);
		}
	}

	OutputDebugMsg("Past second block");

	// check if the takeover timer for any of our dead neighbors
	// has hit their limit yet, and if so, begin the takeover process
	for (size_t i = 0; i < m_vDeadNeighbors.size(); ++i) {
		if(m_vDeadNeighbors[i].state == DeadNeighbor::INITIAL) {
			m_bSeenDeath = true;
			chrono::milliseconds timeSinceDeath = chrono::duration_cast<chrono::milliseconds>(timeNow - m_vDeadNeighbors[i].timeOfDeath);

			if (timeSinceDeath >= m_vDeadNeighbors[i].timeUntilTakeover) {
				m_fp << LogOutputHeader() << "Taking over neighbor node #" << m_vDeadNeighbors[i].neighbor.nodeNum << "." << endl
					<< '\t' << timeSinceDeath.count() << " ms has passed since its death." << endl;

				int takeoverNodeNum = m_vDeadNeighbors[i].neighbor.nodeNum;

				m_fp << "\tNode takeover request:" << endl
					<< "\t\tNode num: " << takeoverNodeNum << endl
					<< "\t\tMy DHT Area: " << endl
					<< m_dhtArea << endl;

				vector<NodeNeighbor>& neighbors = m_vDeadNeighbors[i].neighborNeighbors;
				for (vector<NodeNeighbor>::const_iterator j = neighbors.begin(); j != neighbors.end(); ++j) {
					if(j->nodeNum != m_mpiRank) {
						m_fp << LogOutputHeader() << "Sending node takeover request to " << j->nodeNum << "." << endl;
						MPI_Send((void*)&takeoverNodeNum, 1, MPI_INT, j->nodeNum, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD);
						m_dhtArea.SendOverMPI(j->nodeNum, InterNodeMessageTags::NODE_TAKEOVER_REQUEST);
					}
				}

				m_fp << LogOutputHeader() << "Waiting for response now." << endl;
				m_fp.flush();

				m_vDeadNeighbors[i].state = DeadNeighbor::TIMER_OFF;
			}
		}
	}

	OutputDebugMsg("Reached end of Update");
}

void Node::ResolveNodeTakeoverNotification(int srcNode, InterNodeMessage::NodeTakeoverInfo& nodeTakeoverNotification)
{
	m_bSeenDeath = true;
	vector<DeadNeighbor>::iterator deadNeighborIt = m_vDeadNeighbors.begin();
	for (; deadNeighborIt != m_vDeadNeighbors.end(); ++deadNeighborIt) {
		if (deadNeighborIt->neighbor.nodeNum == nodeTakeoverNotification.nodeNum) {
			break;
		}
	}

	m_fp << LogOutputHeader() << "Resolving node takeover notification from " << srcNode << " for " << nodeTakeoverNotification.nodeNum << endl;

	if (deadNeighborIt != m_vDeadNeighbors.end()) {
		m_fp << "\tNode " << nodeTakeoverNotification.nodeNum << " is in our list of dead nodes, removing it." << endl;
		OutputVecOperation(true);
		m_vDeadNeighbors.erase(deadNeighborIt);
		OutputVecOperation(false);
	} else {
		m_fp << "\tNode " << nodeTakeoverNotification.nodeNum << " isn't in our list of dead nodes." << endl;

		size_t neighborNum = 0;
		vector<NodeNeighbor>::iterator neighborIt = m_vNeighbors.begin();
		for (; neighborIt != m_vNeighbors.end(); ++neighborIt, ++neighborNum) {
			if (neighborIt->nodeNum == nodeTakeoverNotification.nodeNum) {
				break;
			}
		}

		if (neighborIt != m_vNeighbors.end()) {
			m_fp << "\tNode " << nodeTakeoverNotification.nodeNum << " is in our list of neighbor nodes, removing it." << endl;
			OutputVecOperation(true);
			m_vNeighbors.erase(neighborIt);
			OutputVecOperation(true);
			m_vNeighborsOfNeighbors.erase(m_vNeighborsOfNeighbors.begin() + neighborNum);
			OutputVecOperation(true);
			m_neighborTimeSinceLastUpdate.erase(m_neighborTimeSinceLastUpdate.begin() + neighborNum);
			OutputVecOperation(false);
		} else {
			m_fp << "\tNode " << nodeTakeoverNotification.nodeNum << " isn't in our list of neighbor nodes, ignoring message." << endl;
		}
	}
}

void Node::ResolveNodeTakeoverRequest(int srcNode, InterNodeMessage::NodeTakeoverInfo& nodeTakeoverRequest)
{
	m_bSeenDeath = true;
	vector<DeadNeighbor>::iterator deadNeighborIt = m_vDeadNeighbors.begin();
	for (; deadNeighborIt != m_vDeadNeighbors.end(); ++deadNeighborIt) {
		if (deadNeighborIt->neighbor.nodeNum == nodeTakeoverRequest.nodeNum) {
			break;
		}
	}

	m_fp << LogOutputHeader() << "Resolving node takeover request from " << srcNode << " for " << nodeTakeoverRequest.nodeNum << endl;

	InterNodeMessage::NodeTakeoverResponse response;
	response.nodeNum = nodeTakeoverRequest.nodeNum;

	if (deadNeighborIt == m_vDeadNeighbors.end()) {
		m_fp << "\t" << nodeTakeoverRequest.nodeNum << " was not detected as a dead neighbor." << endl;

		vector<NodeNeighbor>::iterator vNeighborIt = m_vNeighbors.begin();
		size_t neighborIndex = 0;
		for (; vNeighborIt != m_vNeighbors.end(); ++vNeighborIt, ++neighborIndex) {
			if (vNeighborIt->nodeNum == nodeTakeoverRequest.nodeNum) {
				break;
			}
		}

		if (vNeighborIt != m_vNeighbors.end()) {
			m_fp << "\t" << nodeTakeoverRequest.nodeNum << " is one of our neighbors." << endl;
			// srcNode observed a timeout from this neighbor but we didn't.
			// there's three possibilities:
			//	1. srcNode checked for a timeout before we did
			//	2. that neighbor failed to send a message to srcNode before the timeout,
			//		but did manage to for us
			//	3. we received a message from that neighbor after srcNode,
			//		so it hasn't triggered our timeout timer
			//
			//	So this neighbor is:
			//		o Alive, but its update failed to reach srcNode and succeeded in reaching us
			//		o Alive, but its update reached us after srcNode
			//		o Dead, but its update reached us after srcNode or we haven't checked it yet

			// so check our update timer and if it is within our
			// timeout window, send a not alive message. otherwise
			// consider it dead and resolve the takeover request
			if ((chrono::high_resolution_clock::now() -  m_neighborTimeSinceLastUpdate[neighborIndex]) >= m_timeUntilNeighborConsideredOffline) {
				m_fp << "\tNode " << nodeTakeoverRequest.nodeNum << " has hit timeout, we just haven't checked it yet." << endl
					<< "\tAdd it to our list of dead neighbors." << endl;

				DeadNeighbor deadNeighbor;
				deadNeighbor.state = DeadNeighbor::TIMER_OFF;
				deadNeighbor.neighbor = *vNeighborIt;
				deadNeighbor.neighborNeighbors = m_vNeighborsOfNeighbors[vNeighborIt - m_vNeighbors.begin()];
				deadNeighbor.neighborsAllowingTakeover = vector<bool>(deadNeighbor.neighborNeighbors.size(), false);

				// make sure it is set that we are allowing ourselves to take over this node
				for (size_t neighborIndex = 0; neighborIndex < deadNeighbor.neighborNeighbors.size(); ++neighborIndex) {
					if (deadNeighbor.neighborNeighbors[neighborIndex].nodeNum == m_mpiRank) {
						deadNeighbor.neighborsAllowingTakeover[neighborIndex] = true;
					}
				}

				OutputVecOperation(true);
				m_vDeadNeighbors.push_back(deadNeighbor);
				OutputVecOperation(true);
				m_vNeighbors.erase(vNeighborIt);
				OutputVecOperation(true);
				m_vNeighborsOfNeighbors.erase(m_vNeighborsOfNeighbors.begin() + neighborIndex);
				OutputVecOperation(true);
				m_neighborTimeSinceLastUpdate.erase(m_neighborTimeSinceLastUpdate.begin() + neighborIndex);
				OutputVecOperation(false);

				OutputVecOperation(true);
				deadNeighborIt = m_vDeadNeighbors.end() - 1;
				OutputVecOperation(false);
			} else {
				m_fp << "\tNode " << nodeTakeoverRequest.nodeNum << " didn't hit timeout." << endl
					<< "\tInforming node " << srcNode << " that node " << nodeTakeoverRequest.nodeNum << " is still alive." << endl;

				response.response = InterNodeMessage::NodeTakeoverResponse::NODE_STILL_ALIVE;
				
				MPI_Send((void*)&response.nodeNum, 1, MPI_INT, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
				MPI_Send((void*)&response.response, 1, MPI_INT, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
				m_dhtArea.SendOverMPI(srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE);
				return;
			}
		} else {
			m_fp << "\tNode " << nodeTakeoverRequest.nodeNum << " is not one of our neighbors, informing node " << srcNode << " of this." << endl;
			// somehow srcNode thought we were neighbors with this node when
			// we're not, as they're neither in our array of alive or dead neighbors
			response.response = InterNodeMessage::NodeTakeoverResponse::NOT_MY_NEIGHBOR;
			
			MPI_Send((void*)&response.nodeNum, 1, MPI_INT, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
			MPI_Send((void*)&response.response, 1, MPI_INT, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
			m_dhtArea.SendOverMPI(srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE);
			return;
		}
	} else {
		m_fp << "\tNode " << nodeTakeoverRequest.nodeNum << " was detected as a dead neighbor." << endl;
	}

	float myDHTArea = m_dhtArea.TotalArea();
	float DHTRegion = nodeTakeoverRequest.myDHTArea.TotalArea();

	m_fp << LogOutputHeader() << "\tComparing our DHT area with that of node " << srcNode << endl;

	// if our area is less than their than we should takeover the node,
	// so send out our own node takeover request to srcNode and all the neighbors
	if (myDHTArea < DHTRegion) {
		m_fp << "\tNode " << srcNode << " has a greater DHT area than us, informing them of this." << endl;

		response.response = InterNodeMessage::NodeTakeoverResponse::SMALLER_AREA;
		response.nodeNum = nodeTakeoverRequest.nodeNum;
		
		MPI_Send((void*)&response.nodeNum, 1, MPI_INT, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
		MPI_Send((void*)&response.response, 1, MPI_INT, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
		m_dhtArea.SendOverMPI(srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE);

		m_fp << "\tNow sending out our own node takeover request to node " << nodeTakeoverRequest.nodeNum << "'s neighbors." << endl;

		InterNodeMessage::NodeTakeoverInfo newTakeoverReq;
		newTakeoverReq.nodeNum = nodeTakeoverRequest.nodeNum;
		newTakeoverReq.myDHTArea = m_dhtArea;

		for (vector<NodeNeighbor>::const_iterator neighbor = deadNeighborIt->neighborNeighbors.begin();
			neighbor != deadNeighborIt->neighborNeighbors.end(); ++neighbor) {
				if(neighbor->nodeNum != m_mpiRank) {
					m_fp << "\tSending our new node takeover request to node " << neighbor->nodeNum << "." << endl;
					
					MPI_Send((void*)&response.nodeNum, 1, MPI_INT, srcNode, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD);
					m_dhtArea.SendOverMPI(srcNode, InterNodeMessageTags::NODE_TAKEOVER_REQUEST);
				}
		}
	} else {
		if (m_mpiRank < srcNode && myDHTArea == DHTRegion) {
			m_fp << "\tNode " << srcNode << " has an equal DHT area to us but a higher node number, responding to them with LOWER_NODE_ADDRESS message." << endl;

			// they have a smaller or equal area, so let them takeover the node
			response.response = InterNodeMessage::NodeTakeoverResponse::LOWER_NODE_ADDRESS;
		} else if (myDHTArea > DHTRegion || (myDHTArea == DHTRegion && m_mpiRank > srcNode)) {
			m_fp << "\tNode " << srcNode << " has a smaller DHT area or equal DHT area and lower node address than us, responding to them with CAN_TAKEOVER message." << endl;

			// they have a smaller or equal area, so let them takeover the node
			response.response = InterNodeMessage::NodeTakeoverResponse::CAN_TAKEOVER;
		} else {
			m_fp << "\tReached error state with node takeover, responding with ERROR message." << endl;

			response.response = InterNodeMessage::NodeTakeoverResponse::TAKEOVER_ERROR;
		}

		MPI_Send((void*)&response.nodeNum, 1, MPI_INT, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
		MPI_Send((void*)&response.response, 1, MPI_INT, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
		m_dhtArea.SendOverMPI(srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE);
	}
}

void Node::ResolveNodeTakeoverResponse(int srcNode, InterNodeMessage::NodeTakeoverResponse& nodeTakeoverResponse)
{
	m_bSeenDeath = true;
	vector<DeadNeighbor>::iterator deadNeighborIt = m_vDeadNeighbors.begin();
	for (; deadNeighborIt != m_vDeadNeighbors.end(); ++deadNeighborIt) {
		if (deadNeighborIt->neighbor.nodeNum == nodeTakeoverResponse.nodeNum) {
			break;
		}
	}

	if (deadNeighborIt != m_vDeadNeighbors.end()) {
		m_fp << LogOutputHeader() << "\tNode takeover response is for node " << deadNeighborIt->neighbor.nodeNum << endl;

		size_t neighborIndex = 0;
		vector<NodeNeighbor>::const_iterator neighborIt = deadNeighborIt->neighborNeighbors.begin();
		for (; neighborIt != deadNeighborIt->neighborNeighbors.end(); ++neighborIt, ++neighborIndex) {
			if (neighborIt->nodeNum == srcNode) {
				break;
			}
		}

		if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::NOT_MY_NEIGHBOR) {
			if (neighborIt != deadNeighborIt->neighborNeighbors.end()) {
				m_fp << "\tNode " << srcNode << " claims to not be node " << deadNeighborIt->neighbor.nodeNum << "'s neighbor,"
					<< " treating it as a CAN_TAKEOVER message." << endl;
				nodeTakeoverResponse.response = InterNodeMessage::NodeTakeoverResponse::CAN_TAKEOVER;
			} else {
				m_fp << "\tUnknown node " << srcNode << " claims to not be node " << deadNeighborIt->neighbor.nodeNum << "'s neighbor, ignoring." << endl;
			}
		}

		if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::TAKEOVER_ERROR) {
			m_fp << "\tNode " << srcNode << " gave ERROR NodeTakeoverResponse, treating it as a CAN_TAKEOVER message." << endl;
			nodeTakeoverResponse.response = InterNodeMessage::NodeTakeoverResponse::CAN_TAKEOVER;
		}

		if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::CAN_TAKEOVER) {
			m_fp << "\tNode " << srcNode << " is allowing the takeover." << endl;
			
			if (neighborIt != deadNeighborIt->neighborNeighbors.end()) {
				deadNeighborIt->neighborsAllowingTakeover[neighborIndex] = true;
			} else {
				m_fp << "\tUnable to find node " << srcNode << " in dead node's neighbor list." << endl;
			}

			int numNeigborsAllowing = count(deadNeighborIt->neighborsAllowingTakeover.begin(), deadNeighborIt->neighborsAllowingTakeover.end(), true);

			m_fp << "\t" << numNeigborsAllowing << " out of " << deadNeighborIt->neighborsAllowingTakeover.size() << " neighbors allowing takeover." << endl;

			// if all the neighbors are allowing takeover now, then notify them all that we're
			// taking over the node's DHT area and take it over
			if (find(deadNeighborIt->neighborsAllowingTakeover.begin(), deadNeighborIt->neighborsAllowingTakeover.end(), false) == deadNeighborIt->neighborsAllowingTakeover.end()) {
				m_fp << "\tAll nodes are allowing the takeover." << endl;

				InterNodeMessage::NodeTakeoverInfo takeoverNotification;
				takeoverNotification.myDHTArea = m_dhtArea;
				takeoverNotification.nodeNum = deadNeighborIt->neighbor.nodeNum;

				for (vector<NodeNeighbor>::const_iterator j = deadNeighborIt->neighborNeighbors.begin(); j != deadNeighborIt->neighborNeighbors.end(); ++j) {
					if(j->nodeNum != m_mpiRank) {
						MPI_Send((void*)&takeoverNotification.nodeNum, 1, MPI_INT, j->nodeNum, InterNodeMessageTags::NODE_TAKEOVER_NOTIFY, MPI_COMM_WORLD);
						m_dhtArea.SendOverMPI(j->nodeNum, InterNodeMessageTags::NODE_TAKEOVER_NOTIFY);
						m_fp << LogOutputHeader() << "\tSent node takeover notification to " << j->nodeNum << endl;
					}
				}

				m_fp << LogOutputHeader() << "\tTaking over the DHT area of node " << deadNeighborIt->neighbor.nodeNum << ": " << endl
					<< deadNeighborIt->neighbor.neighborArea << endl;

				// add all of the node's neighbors as our neighbors.
				// they'll be pruned after we takeover the DHT area
				// in TakeoverDHTArea()
				for (vector<NodeNeighbor>::const_iterator j = deadNeighborIt->neighborNeighbors.begin(); j != deadNeighborIt->neighborNeighbors.end(); ++j) {
					// make sure we don't add duplicate neighbors
					if (find(m_vNeighbors.begin(), m_vNeighbors.end(), *j) == m_vNeighbors.end() && j->nodeNum != m_mpiRank) {
						m_vNeighbors.push_back(*j);
						m_vNeighborsOfNeighbors.push_back(vector<NodeNeighbor>());
						m_neighborTimeSinceLastUpdate.push_back(chrono::high_resolution_clock::now());

						m_fp << LogOutputHeader() << "\t\tAcquired new neighbor:" << endl
							<< "\t\t\t\tNode num: " << j->nodeNum << endl
							<< "\t\t\t\tNode DHT Area: " << endl
							<< j->neighborArea << endl;
					}
				}

				try
				{
					m_fp << LogOutputHeader() << "\t\tCoalescing my node and dead node's DHT area." << endl;
					TakeoverDHTArea(deadNeighborIt->neighbor.neighborArea);

					m_fp << LogOutputHeader() << "\t\tErasing dead node from dead neighbors list." << endl;
					OutputVecOperation(true);
					m_vDeadNeighbors.erase(deadNeighborIt);
					OutputVecOperation(false);

					m_dhtArea.SendOverMPI(m_rootRank, NodeToRootMessageTags::DHT_AREA_UPDATE);
				}
				catch (const out_of_range& oor) {
					m_fp << "Out of range error: " << oor.what() << endl;
				}
			}
		} else if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::NODE_STILL_ALIVE) {
			m_fp << "\tNode " << deadNeighborIt->neighbor.nodeNum << " is still alive, adding back to alive neighbors." << endl;

			// if the node is still alive then add back to our neighbors vector.
			// this effectively resets the timeout timer for this node.
			m_vNeighbors.push_back(deadNeighborIt->neighbor);
			m_vNeighborsOfNeighbors.push_back(deadNeighborIt->neighborNeighbors);
			m_neighborTimeSinceLastUpdate.push_back(chrono::high_resolution_clock::now());
			
			// then remove it from this vector as its no longer considered dead.
			OutputVecOperation(true);
			m_vDeadNeighbors.erase(deadNeighborIt);
			OutputVecOperation(false);
		} else if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::SMALLER_AREA || nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::LOWER_NODE_ADDRESS) {
			// not sure how to response to this yet except log it
			m_fp << "\tNode " << srcNode << " claims to have a smaller DHT area or equal DHT area and lower node address:" << endl
				<< neighborIt->neighborArea << endl;
		} else {
			m_fp << "\tUnknown NodeTakeoverResponse." << endl;
		}
	}

	OutputDebugMsg("Reached end of ResolveNodeTakeoverResponse");
}

void Node::TakeoverDHTArea(DHTArea dhtArea)
{
	m_bSeenDeath = true;

	if(m_dhtArea.AddArea(dhtArea)) {
		m_fp << LogOutputHeader() << "\tDHT region successfully added to our area." << endl;
	} else {
		// no edges we can combine along, have to have seperate
		// dht areas.

		m_fp << LogOutputHeader() << "\tDHT area not a neighbor of my DHT area." << endl;
	}

	m_fp << "\tNew DHT area:" << endl
		<< m_dhtArea << endl;

	m_fp.flush();

	PruneNeighbors();
	SendNeighborsDHTUpdate();
}

void Node::PruneNeighbors()
{
	// remove all the neighbors we no longer
	// share an edge with (i.e. those that are no longer neighbors)
	m_fp << LogOutputHeader() << "\tPruning my neighbors:" << endl;
	for (size_t i = 0; i < m_vNeighbors.size();) {

		m_fp << LogOutputHeader() << "\t\tNode #: " << m_vNeighbors[i].nodeNum << endl
			<< m_vNeighbors[i].neighborArea << endl
			<< endl;

		bool isMyNeighbor = m_dhtArea.IsNeighbor(m_vNeighbors[i].neighborArea);

		if(isMyNeighbor) {
			m_fp << LogOutputHeader() << "Is neighbor. Keeping it." << endl;
			++i;
		} else {
			m_fp << LogOutputHeader() << "Is not neighbor. Removing it." << endl;
			m_fp.flush();
			OutputDebugMsg("m_vNeighbors size: %d", m_vNeighbors.size());
			OutputDebugMsg("m_neighborTimeSinceLastUpdate size: %d", m_neighborTimeSinceLastUpdate.size());
			OutputDebugMsg("m_vNeighborsOfNeighbors size: %d", m_vNeighborsOfNeighbors.size());
			OutputDebugMsg("i: %d", i);
			OutputVecOperation(true);
			m_vNeighbors.erase(m_vNeighbors.begin() + i);
			OutputVecOperation(true);
			m_neighborTimeSinceLastUpdate.erase(m_neighborTimeSinceLastUpdate.begin() + i);
			OutputVecOperation(true);
			m_vNeighborsOfNeighbors.erase(m_vNeighborsOfNeighbors.begin() + i);
			OutputVecOperation(false);
		}

		m_fp.flush();
	}

	SendRootNeighborsUpdate();
}

string Node::LogOutputHeader() const
{
	stringstream ss;
	ss << "TS - " << chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count() << " - ";

	return ss.str();
}

void Node::SendRootNeighborsUpdate()
{
	NodeToRootMessage::NodeNeighborUpdate neighborUpdate;
	neighborUpdate.numNeighbors = m_vNeighbors.size();
	int neighborUpdateIndex = 0;

	for (vector<NodeNeighbor>::const_iterator i = m_vNeighbors.begin(); i != m_vNeighbors.end(); ++i, ++neighborUpdateIndex) {
		neighborUpdate.neighbors[neighborUpdateIndex] = i->nodeNum;
	}

	// update the root node with our new neighbors
	MPI_Send((void*)&neighborUpdate, 1, NodeToRootMessage::MPI_NodeNeighborUpdate, m_rootRank, NodeToRootMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD);
}

const vector<NodeNeighbor>&	Node::GetNeighbors() const
{
	return m_vNeighbors;
}

bool Node::SendNeighborsOverMPI(const std::vector<NodeNeighbor>& neighbors, int rank, int tag)
{
	bool success = true;

	int numNeighbors = neighbors.size();
	MPI_Send((void*)&numNeighbors, 1, MPI_INT, rank, tag, MPI_COMM_WORLD);

	for (vector<NodeNeighbor>::const_iterator it = neighbors.begin(); it != neighbors.end(); ++it) {
		MPI_Send((void*)&it->nodeNum, 1, MPI_INT, rank, tag, MPI_COMM_WORLD);
		
		if (!it->neighborArea.SendOverMPI(rank, tag)) {
			success = false;
		}
	}

	return success;
}

bool Node::RecvNeighborsOverMPI(std::vector<NodeNeighbor>& neighbors, int rank, int tag)
{
	MPI_Status	mpiStatus;
	bool		success = true;

	int numNeighbors;
	MPI_Recv((void*)&numNeighbors, 1, MPI_INT, rank, tag, MPI_COMM_WORLD, &mpiStatus);

	neighbors.resize(numNeighbors);

	for (vector<NodeNeighbor>::iterator it = neighbors.begin(); it != neighbors.end(); ++it) {
		MPI_Recv((void*)&it->nodeNum, 1, MPI_INT, rank, tag, MPI_COMM_WORLD, &mpiStatus);
		
		if (!it->neighborArea.RecvOverMPI(rank, tag)) {
			success = false;
		}
	}

	return success;
}