#include <mpi.h>
#include <math.h>
#include <limits>
#include <vector>
#include <algorithm>
#include <chrono>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

#include "header.h"
#include "nodeMessageTags.h"

#include "Node.h"

float PointDistToRect(float x, float y, float left, float right, float top, float bottom);

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
	m_dhtArea({0.0f, 1.0f, 0.0f, 1.0f}),
	// the random number generator and distribution to generate random
	// coordinates within the DHT network
	m_randGenerator((unsigned int)chrono::system_clock::now().time_since_epoch().count()),
	m_randomAreaCoordGenerator(0.0f, 1.0f),
	m_mpiRank(mpiRank), m_rootRank(rootRank), m_fp(*fp),
	// how long to wait after sending an update to our neighbors
	// until we send the next update
	m_timeUntilUpdateNeighbors(std::chrono::seconds(2)),
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
	m_edgeAbutEpsilon(1e-4f)
{}

const NodeDHTArea& Node::GetDHTArea()
{
	return m_dhtArea;
}

bool Node::SplitDHTArea(int newNode, NodeDHTArea& newNodeDHTArea, std::vector<NodeNeighbor>& r_vNewNodeNeighbors, vector<vector<NodeNeighbor>>& r_vNewNodeNeighborsOfNeighbors)
{
	m_fp << LogOutputHeader() << "\tSplitting node DHT area." << endl;

	// split the x axis
	if (m_nextSplitAxis == 0) {
		m_fp << "\tSplitting x axis." << endl;

		newNodeDHTArea.top = m_dhtArea.top;
		newNodeDHTArea.bottom = m_dhtArea.bottom;

		// split our width and put the new node
		// to the right of us
		float newHorizEdgeLength = (m_dhtArea.right - m_dhtArea.left) * 0.5f;
		newNodeDHTArea.left = m_dhtArea.left + newHorizEdgeLength;
		newNodeDHTArea.right = newNodeDHTArea.left + newHorizEdgeLength;
		m_dhtArea.right = newNodeDHTArea.left;
	} else { // split the y axis
		m_fp << "\tSplitting y axis." << endl;

		newNodeDHTArea.left = m_dhtArea.left;
		newNodeDHTArea.right = m_dhtArea.right;

		// split our height and put the new node
		// underneath us
		float newVertEdgeLength = (m_dhtArea.bottom - m_dhtArea.top) * 0.5f;
		newNodeDHTArea.top = m_dhtArea.top + newVertEdgeLength;
		newNodeDHTArea.bottom = newNodeDHTArea.top + newVertEdgeLength;
		m_dhtArea.bottom = newNodeDHTArea.top;
	}

	// split the next axis
	m_nextSplitAxis = (m_nextSplitAxis + 1) % 2;

	m_fp << LogOutputHeader() << "\tMy new DHT area:" << endl
		<< "\t\tLeft: " << m_dhtArea.left << endl
		<< "\t\tRight: " << m_dhtArea.right << endl
		<< "\t\tTop: " << m_dhtArea.top << endl
		<< "\t\tBottom: " << m_dhtArea.bottom << endl
		<< endl
		<< "\tNew Node DHT area:" << endl
		<< "\t\tLeft: " << newNodeDHTArea.left << endl
		<< "\t\tRight: " << newNodeDHTArea.right << endl
		<< "\t\tTop: " << newNodeDHTArea.top << endl
		<< "\t\tBottom: " << newNodeDHTArea.bottom << endl
		<< endl;

	r_vNewNodeNeighbors.clear();
	r_vNewNodeNeighborsOfNeighbors.clear();

	m_fp << LogOutputHeader() << "\tFinding new node neighbors:" << endl;

	// add all the neighbors that share an edge
	vector<vector<NodeNeighbor>>::const_iterator j = m_vNeighborsOfNeighbors.begin();
	for (vector<NodeNeighbor>::const_iterator i = m_vNeighbors.begin(); i != m_vNeighbors.end(); ++i, ++j) {
		bool edgeShared = false;

		m_fp << LogOutputHeader() << "\t\tNode #: " << i->nodeNum << endl
			<< "\t\t\tLeft: " << i->neighborArea.left << endl
			<< "\t\t\tRight: " << i->neighborArea.right << endl
			<< "\t\t\tTop: " << i->neighborArea.top << endl
			<< "\t\t\tBottom: " << i->neighborArea.bottom << endl
			<< endl;

		if (AreNeighbors(newNodeDHTArea, i->neighborArea)) {
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
	newNeighbor.neighborArea = newNodeDHTArea;
	m_vNeighbors.push_back(newNeighbor);
	m_vNeighborsOfNeighbors.push_back(r_vNewNodeNeighbors);
	m_neighborTimeSinceLastUpdate.push_back(chrono::high_resolution_clock::now());

	r_vNewNodeNeighbors.push_back({ m_dhtArea, m_mpiRank });
	r_vNewNodeNeighborsOfNeighbors.push_back(m_vNeighbors);

	m_fp << LogOutputHeader() << "\tNew node neighbors:" << endl;
	for (vector<NodeNeighbor>::const_iterator i = r_vNewNodeNeighbors.begin(); i != r_vNewNodeNeighbors.end(); ++i) {
		m_fp << endl
			<< "\t\tNode #: " << i->nodeNum << endl
			<< "\t\t\tLeft: " << i->neighborArea.left << endl
			<< "\t\t\tRight: " << i->neighborArea.right << endl
			<< "\t\t\tTop: " << i->neighborArea.top << endl
			<< "\t\t\tBottom: " << i->neighborArea.bottom << endl
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
		m_dhtArea = { 0.0f, 1.0f, 0.0f, 1.0f };
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
	MPI_Recv((void*)&dhtAreaResponse, 1, InterNodeMessage::MPI_DHTAreaResponse, dhtPointOwner.pointOwnerNode, InterNodeMessageTags::RESPONSE_DHT_AREA, MPI_COMM_WORLD, &mpiStatus);

	m_vNeighbors.resize(dhtAreaResponse.numNeighbors);
	m_neighborTimeSinceLastUpdate.resize(dhtAreaResponse.numNeighbors, chrono::high_resolution_clock::now());

	MPI_Recv((void*)&m_vNeighbors[0],
		dhtAreaResponse.numNeighbors,
		InterNodeMessage::MPI_NodeNeighbor,
		dhtPointOwner.pointOwnerNode,
		InterNodeMessageTags::RESPONSE_DHT_AREA,
		MPI_COMM_WORLD,
		&mpiStatus);

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
			vector<NodeNeighbor>& neighborNeighbors = m_vNeighborsOfNeighbors[m_vNeighborsOfNeighbors.size() - 1];

			MPI_Recv((void*)&neighborNeighbors[0],
				numNeighborsOfNeighbor,
				InterNodeMessage::MPI_NodeNeighbor,
				mpiStatus.MPI_SOURCE,
				InterNodeMessageTags::RESPONSE_DHT_AREA,
				MPI_COMM_WORLD,
				&mpiStatus);
		}
	}

	m_nextSplitAxis = dhtAreaResponse.nextAxisToSplit;
	m_dhtArea = dhtAreaResponse.newDHTArea;

	m_fp << LogOutputHeader() << "Got DHT area response from " << dhtPointOwner.pointOwnerNode << ":" << endl
		<< "\tNew DHT Area:" << endl
		<< "\t\tLeft: " << dhtAreaResponse.newDHTArea.left << endl
		<< "\t\tRight: " << dhtAreaResponse.newDHTArea.right << endl
		<< "\t\tTop: " << dhtAreaResponse.newDHTArea.top << endl
		<< "\t\tBottom: " << dhtAreaResponse.newDHTArea.bottom << endl
		<< endl
		<< "\t" << dhtAreaResponse.numNeighbors << " Neighbor(s):" << endl;
	
	for (size_t i = 0; i < m_vNeighbors.size(); ++i) {
		m_fp << "\t\tNeighbor " << i << endl
			<< "\t\t\tNode #: " << m_vNeighbors[i].nodeNum << endl
			<< "\t\t\t\tLeft: " << m_vNeighbors[i].neighborArea.left << endl
			<< "\t\t\t\tRight: " << m_vNeighbors[i].neighborArea.right << endl
			<< "\t\t\t\tTop: " << m_vNeighbors[i].neighborArea.top << endl
			<< "\t\t\t\tBottom: " << m_vNeighbors[i].neighborArea.bottom << endl
			<< "\t\t\t\tNeighbors: ";

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

float PointDistToRect(float x, float y, float left, float right, float top, float bottom)
{
	float xDist = 0.0f, yDist = 0.0f;

	// assuming the rectangles here are axially-aligned.
	// so the x distance from the rectangle is just the
	// distance to the closer vertical edge and the y distance
	// is similarly the distance to the closer horizontal edge

	if (x < left || x > right) {
		float xDistFromLeft = abs(x - left);
		float xDistFromRight = abs(x - right);

		xDist = xDistFromLeft > xDistFromRight ? xDistFromRight : xDistFromLeft;
	}

	if (y < top || y > bottom) {
		float yDistFromTop = abs(y - top);
		float yDistFromBottom = abs(y - bottom);

		yDist = yDistFromTop > yDistFromBottom ? yDistFromBottom : yDistFromTop;
	}

	// return the squared distance to avoid
	// the square root that is unnecessary
	// if we're just comparing distances
	return xDist*xDist + yDist*yDist;
}

const int Node::GetNextSplitAxis()
{
	return m_nextSplitAxis;
}

int Node::GetNearestNeighbor(float x, float y)
{
	// return -1 if point is inside our DHT area
	if (x >= m_dhtArea.left && x <= m_dhtArea.right && y >= m_dhtArea.top && y <= m_dhtArea.bottom) {
		m_fp << LogOutputHeader() << "\t\tPoint is inside our DHT area." << endl;
		m_fp.flush();
		return -1;
	}

	vector<NodeNeighbor>::const_iterator	closestNeighbor = m_vNeighbors.end();
	float									closestNeighborDist = FLT_MAX;

	for (vector<NodeNeighbor>::const_iterator i = m_vNeighbors.begin(); i != m_vNeighbors.end(); ++i) {
		float neighborDist = PointDistToRect(x, y, i->neighborArea.left, i->neighborArea.right, i->neighborArea.top, i->neighborArea.bottom);

		m_fp << LogOutputHeader() << "\t\tPoint dist from node " << i->nodeNum << ": " << neighborDist << endl
			<< "\t\t\tLeft: " << i->neighborArea.left << endl
			<< "\t\t\tRight: " << i->neighborArea.right << endl
			<< "\t\t\tTop: " << i->neighborArea.top << endl
			<< "\t\t\tBottom: " << i->neighborArea.bottom << endl
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
		return -2;
	}

	return closestNeighbor->nodeNum;
}

bool Node::UpdateNeighborDHTArea(int nodeNum, const NodeDHTArea& dhtArea, const vector<NodeNeighbor>& neighborsOfNeighbor)
{
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

	return true;
}

void Node::SendNeighborsDHTUpdate()
{
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
		m_fp << LogOutputHeader() << "\tSending neighbor #" << neighborUpdateIndex << ": my DHT area" << endl;
		m_fp.flush();
		MPI_Send((void*)&m_dhtArea, 1, InterNodeMessage::MPI_NodeDHTArea, i->nodeNum, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD);

		m_fp << LogOutputHeader() << "\tSending neighbor #" << neighborUpdateIndex << ": my # neighbors" << endl;
		m_fp.flush();
		MPI_Send((void*)&numNeighbors, 1, MPI_INT, i->nodeNum, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD);

		m_fp << LogOutputHeader() << "\tSending neighbor #" << neighborUpdateIndex << ": neighbors" << endl;
		m_fp.flush();

		MPI_Send((void*)&m_vNeighbors[0], numNeighbors, InterNodeMessage::MPI_NodeNeighbor, i->nodeNum, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD);

		NotifyRootOfMsg(NodeToNodeMsgTypes::SENDING, i->nodeNum, m_rootRank);
	}

	m_fp << LogOutputHeader() << "\tDone." << endl;
	m_fp.flush();
}

void Node::UpdateNeighbors()
{
	chrono::high_resolution_clock::time_point timeNow = chrono::high_resolution_clock::now();

	// check if its time to update our neighbors again
	chrono::milliseconds timeSinceLastUpdate = chrono::duration_cast<chrono::milliseconds>(timeNow - m_timeOfLastNeighborUpdate);
	if (timeSinceLastUpdate >= m_timeUntilUpdateNeighbors) {
		SendNeighborsDHTUpdate();
	}

	// check if any of our neighbors may be offline
	int neighborNum = 0;

	for (size_t neighborNum = 0; neighborNum < m_vNeighbors.size(); ++neighborNum) {
		vector<chrono::high_resolution_clock::time_point>::const_iterator timerIt = m_neighborTimeSinceLastUpdate.begin() + neighborNum;

		timeNow = chrono::high_resolution_clock::now();

		// if we haven't received a neighbor update within a certain amount of time from one
		// of our neighbors, its considered offline and the DHT area takeover process begins
		chrono::milliseconds timeSinceUpdate = chrono::duration_cast<chrono::milliseconds>(timeNow - *timerIt);

		if (timeSinceUpdate >= m_timeUntilNeighborConsideredOffline) {
			vector<NodeNeighbor>::const_iterator nodeIt = m_vNeighbors.begin() + neighborNum;
			vector<vector<NodeNeighbor>>::const_iterator nodeNeighborsIt = m_vNeighborsOfNeighbors.begin() + neighborNum;

			DeadNeighbor deadNeighbor;
			deadNeighbor.neighbor = *nodeIt;
			deadNeighbor.timeOfDeath = chrono::high_resolution_clock::now();
			deadNeighbor.neighborNeighbors = *nodeNeighborsIt;
			deadNeighbor.neighborsAllowingTakeover = vector<bool>(deadNeighbor.neighborNeighbors.size(), false);
			deadNeighbor.state = DeadNeighbor::INITIAL;

			// make sure it is set that we are allowing ourselves to take over this node
			for (size_t neighborIndex = 0; neighborIndex < deadNeighbor.neighborNeighbors.size(); ++neighborIndex) {
				if (deadNeighbor.neighborNeighbors[neighborIndex].nodeNum == m_mpiRank) {
					deadNeighbor.neighborsAllowingTakeover[neighborIndex] = true;
				}
			}

			float totalDHTArea = (m_dhtArea.right - m_dhtArea.left) * (m_dhtArea.bottom - m_dhtArea.top);
			deadNeighbor.timeUntilTakeover = m_takeoverTimerPerArea * totalDHTArea;

			m_fp << LogOutputHeader() << "Neighbor node #" << deadNeighbor.neighbor.nodeNum << " died." << endl
				<< "\tHasn't updated us for " << timeSinceUpdate.count() << " ms." << endl
				<< "\tThis node has DHT area of size " << totalDHTArea << endl
				<< "\tTaking over node in " << deadNeighbor.timeUntilTakeover.count() << " ms." << endl;
			
			m_vDeadNeighbors.push_back(deadNeighbor);
			m_vNeighbors.erase(nodeIt);
			m_vNeighborsOfNeighbors.erase(nodeNeighborsIt);
			m_neighborTimeSinceLastUpdate.erase(timerIt);
		}
	}

	// check if the takeover timer for any of our dead neighbors
	// has hit their limit yet, and if so, begin the takeover process
	for (size_t i = 0; i < m_vDeadNeighbors.size(); ++i) {
		if(m_vDeadNeighbors[i].state == DeadNeighbor::INITIAL) {
			chrono::milliseconds timeSinceDeath = chrono::duration_cast<chrono::milliseconds>(timeNow - m_vDeadNeighbors[i].timeOfDeath);

			if (timeSinceDeath >= m_vDeadNeighbors[i].timeUntilTakeover) {
				m_fp << LogOutputHeader() << "Taking over neighbor node #" << m_vDeadNeighbors[i].neighbor.nodeNum << "." << endl
					<< '\t' << timeSinceDeath.count() << " ms has passed since its death." << endl;

				InterNodeMessage::NodeTakeoverInfo nodeTakeoverRequest;
				nodeTakeoverRequest.nodeNum = m_vDeadNeighbors[i].neighbor.nodeNum;
				nodeTakeoverRequest.myDHTArea = m_dhtArea;

				m_fp << "\tNode takeover request:" << endl
					<< "\t\tNode num: " << nodeTakeoverRequest.nodeNum << endl
					<< "\t\tMy DHT Area: " << endl
					<< "\t\t\tLeft: " << nodeTakeoverRequest.myDHTArea.left << endl
					<< "\t\t\tRight: " << nodeTakeoverRequest.myDHTArea.right << endl
					<< "\t\t\tTop: " << nodeTakeoverRequest.myDHTArea.top << endl
					<< "\t\t\tBottom: " << nodeTakeoverRequest.myDHTArea.bottom << endl;

				vector<NodeNeighbor>& neighbors = m_vDeadNeighbors[i].neighborNeighbors;
				for (vector<NodeNeighbor>::const_iterator j = neighbors.begin(); j != neighbors.end(); ++j) {
					if(j->nodeNum != m_mpiRank) {
						m_fp << LogOutputHeader() << "Sending node takeover request to " << j->nodeNum << "." << endl;
						MPI_Send((void*)&nodeTakeoverRequest, 1, InterNodeMessage::MPI_NodeTakeoverInfo, j->nodeNum, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD);
					}					
				}

				m_fp << LogOutputHeader() << "Waiting for response now." << endl;
				m_fp.flush();

				m_vDeadNeighbors[i].state = DeadNeighbor::TIMER_OFF;
			}
		}
	}
}

void Node::ResolveNodeTakeoverRequest(int srcNode, InterNodeMessage::NodeTakeoverInfo& nodeTakeoverRequest)
{
	vector<DeadNeighbor>::iterator deadNeighborIt = m_vDeadNeighbors.begin();
	for (; deadNeighborIt != m_vDeadNeighbors.end(); ++deadNeighborIt) {
		if (deadNeighborIt->neighbor.nodeNum == nodeTakeoverRequest.nodeNum) {
			break;
		}
	}

	m_fp << LogOutputHeader() << "Resolving node takeover request from " << srcNode << " for " << nodeTakeoverRequest.nodeNum << endl;

	InterNodeMessage::NodeTakeoverResponse response;
	memset((void*)&response, 0, sizeof(InterNodeMessage::NodeTakeoverResponse));
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

				m_vDeadNeighbors.push_back(deadNeighbor);
				m_vNeighbors.erase(vNeighborIt);
				m_vNeighborsOfNeighbors.erase(m_vNeighborsOfNeighbors.begin() + neighborIndex);
				m_neighborTimeSinceLastUpdate.erase(m_neighborTimeSinceLastUpdate.begin() + neighborIndex);

				deadNeighborIt = m_vDeadNeighbors.end() - 1;
			} else {
				m_fp << "\tNode " << nodeTakeoverRequest.nodeNum << " didn't hit timeout." << endl
					<< "\tInforming node " << srcNode << " that node " << nodeTakeoverRequest.nodeNum << " is still alive." << endl;

				response.response = InterNodeMessage::NodeTakeoverResponse::NODE_STILL_ALIVE;
				MPI_Send((void*)&response, 1, InterNodeMessage::MPI_NodeTakeoverResponse, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
				return;
			}
		} else {
			m_fp << "\tNode " << nodeTakeoverRequest.nodeNum << " is not one of our neighbors, informing node " << srcNode << " of this." << endl;
			// somehow srcNode thought we were neighbors with this node when
			// we're not, as they're neither in our array of alive or dead neighbors
			response.response = InterNodeMessage::NodeTakeoverResponse::NOT_MY_NEIGHBOR;
			MPI_Send((void*)&response, 1, InterNodeMessage::MPI_NodeTakeoverResponse, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
			return;
		}
	} else {
		m_fp << "\tNode " << nodeTakeoverRequest.nodeNum << " was detected as a dead neighbor." << endl;
	}

	float myDHTArea = (m_dhtArea.right - m_dhtArea.left) * (m_dhtArea.bottom - m_dhtArea.top);
	float nodeDHTArea = (nodeTakeoverRequest.myDHTArea.right - nodeTakeoverRequest.myDHTArea.left) *
		(nodeTakeoverRequest.myDHTArea.bottom - nodeTakeoverRequest.myDHTArea.top);

	m_fp << LogOutputHeader() << "\tComparing our DHT area with that of node " << srcNode << endl;

	// if our area is less than their than we should takeover the node,
	// so send out our own node takeover request to srcNode and all the neighbors
	if (myDHTArea < nodeDHTArea) {
		m_fp << "\tNode " << srcNode << " has a greater DHT area than us, informing them of this." << endl;

		response.response = InterNodeMessage::NodeTakeoverResponse::SMALLER_AREA;
		response.myDHTArea = m_dhtArea;
		response.nodeNum = nodeTakeoverRequest.nodeNum;
		MPI_Send((void*)&response, 1, InterNodeMessage::MPI_NodeTakeoverResponse, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);

		m_fp << "\tNow sending out our own node takeover request to node " << nodeTakeoverRequest.nodeNum << "'s neighbors." << endl;

		InterNodeMessage::NodeTakeoverInfo newTakeoverReq;
		newTakeoverReq.nodeNum = nodeTakeoverRequest.nodeNum;
		newTakeoverReq.myDHTArea = m_dhtArea;

		for (vector<NodeNeighbor>::const_iterator neighbor = deadNeighborIt->neighborNeighbors.begin();
			neighbor != deadNeighborIt->neighborNeighbors.end(); ++neighbor) {
				if(neighbor->nodeNum != m_mpiRank) {
					m_fp << "\tSending our new node takeover request to node " << neighbor->nodeNum << "." << endl;
					MPI_Send((void*)&newTakeoverReq, 1, InterNodeMessage::MPI_NodeTakeoverInfo, neighbor->nodeNum, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD);
				}
		}
	} else {
		m_fp << "\tNode " << srcNode << " has a lesser or equal DHT area than us, responding to them with CAN_TAKEOVER message." << endl;

		// they have a smaller or equal area, so let them takeover the node
		response.response = InterNodeMessage::NodeTakeoverResponse::CAN_TAKEOVER;
		MPI_Send((void*)&response, 1, InterNodeMessage::MPI_NodeTakeoverResponse, srcNode, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD);
	}
}

void Node::ResolveNodeTakeoverResponse(int srcNode, InterNodeMessage::NodeTakeoverResponse& nodeTakeoverResponse)
{
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
				m_fp << "\tUnknown neighbor " << srcNode << " claims to not be node " << deadNeighborIt->neighbor.nodeNum << "'s neighbor, ignoring." << endl;
			}
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
						MPI_Send((void*)&takeoverNotification, 1, InterNodeMessage::MPI_NodeTakeoverInfo, j->nodeNum, InterNodeMessageTags::NODE_TAKEOVER_NOTIFY, MPI_COMM_WORLD);
						m_fp << LogOutputHeader() << "\tSent node takeover notification to " << j->nodeNum << endl;
					}
				}

				m_fp << LogOutputHeader() << "\tTaking over the DHT area of node " << deadNeighborIt->neighbor.nodeNum << ": " << endl
					<< "\t\tLeft: " << deadNeighborIt->neighbor.neighborArea.left << endl
					<< "\t\tRight: " << deadNeighborIt->neighbor.neighborArea.right << endl
					<< "\t\tTop: " << deadNeighborIt->neighbor.neighborArea.top << endl
					<< "\t\tBottom: " << deadNeighborIt->neighbor.neighborArea.bottom << endl << endl;

				// add all of the node's neighbors as our neighbors.
				// they'll be pruned after we takeover the DHT area
				// in TakeoverDHTArea()
				for (vector<NodeNeighbor>::const_iterator j = deadNeighborIt->neighborNeighbors.begin(); j != deadNeighborIt->neighborNeighbors.end(); ++j) {
					// make sure we don't add duplicate neighbors
					if (find(m_vNeighbors.begin(), m_vNeighbors.end(), *j) == m_vNeighbors.end() && j->nodeNum != m_mpiRank) {
						m_vNeighbors.push_back(*j);

						m_fp << LogOutputHeader() << "\t\tAcquired new neighbor:" << endl
							<< "\t\t\t\tNode num: " << j->nodeNum << endl
							<< "\t\t\t\tNode DHT Area: " << endl
							<< "\t\t\t\t\tLeft: " << j->neighborArea.left << endl
							<< "\t\t\t\t\tRight: " << j->neighborArea.right << endl
							<< "\t\t\t\t\tTop: " << j->neighborArea.top << endl
							<< "\t\t\t\t\tBottom: " << j->neighborArea.bottom << endl;
					}
				}

				m_fp << LogOutputHeader() << "\t\tCoalescing my node and dead node's DHT area." << endl;
				TakeoverDHTArea(deadNeighborIt->neighbor.neighborArea);

				m_fp << LogOutputHeader() << "\t\tErasing dead node from dead neighbors list." << endl;
				m_vDeadNeighbors.erase(deadNeighborIt);
			}
		} else if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::NODE_STILL_ALIVE) {
			m_fp << "\tNode " << deadNeighborIt->neighbor.nodeNum << " is still alive, adding back to alive neighbors." << endl;

			// if the node is still alive then add back to our neighbors vector.
			// this effectively resets the timeout timer for this node.
			m_vNeighbors.push_back(deadNeighborIt->neighbor);
			m_vNeighborsOfNeighbors.push_back(deadNeighborIt->neighborNeighbors);
			m_neighborTimeSinceLastUpdate.push_back(chrono::high_resolution_clock::now());
			
			// then remove it from this vector as its no longer considered dead.
			m_vDeadNeighbors.erase(deadNeighborIt);
		} else if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::SMALLER_AREA) {
			// not sure how to response to this yet except log it
			m_fp << "\tNode " << srcNode << " claims to have a smaller DHT area:" << endl
				<< "\t\tLeft: " << neighborIt->neighborArea.left << endl
				<< "\t\tRight: " << neighborIt->neighborArea.right << endl
				<< "\t\tTop: " << neighborIt->neighborArea.top << endl
				<< "\t\tBottom: " << neighborIt->neighborArea.bottom << endl;
		} else {
			m_fp << "\tUnknown NodeTakeoverResponse." << endl;
		}
	}
}

void Node::TakeoverDHTArea(NodeDHTArea dhtArea)
{
	bool leftEdgeAbutted = abs(dhtArea.right - m_dhtArea.left) < m_edgeAbutEpsilon;
	bool rightEdgeAbutted = abs(dhtArea.left - m_dhtArea.right) < m_edgeAbutEpsilon;
	bool topEdgeAbutted = abs(dhtArea.bottom - m_dhtArea.top) < m_edgeAbutEpsilon;
	bool bottomEdgeAbutted = abs(dhtArea.top - m_dhtArea.bottom) < m_edgeAbutEpsilon;

	float myWidth = m_dhtArea.right - m_dhtArea.left;
	float myHeight = m_dhtArea.bottom - m_dhtArea.top;
	float nodeWidth = dhtArea.right - dhtArea.left;
	float nodeHeight = dhtArea.bottom - dhtArea.top;

	// if these DHT areas are adjacent and their
	// abutting edge is the same length, then
	// they can be combined. otherwise we'll
	// have to have two separate DHT areas.
	if ((leftEdgeAbutted || rightEdgeAbutted) && myHeight == nodeHeight) {
		m_dhtArea.left = m_dhtArea.left > dhtArea.left ? dhtArea.left : m_dhtArea.left;
		m_dhtArea.right = m_dhtArea.right > dhtArea.right ? m_dhtArea.right : dhtArea.right;

		m_fp << LogOutputHeader() << "\tCoalescing nodes along horizontal edge." << endl;
	} else if ((topEdgeAbutted || bottomEdgeAbutted) && myWidth == nodeWidth) {
		m_dhtArea.top = m_dhtArea.top > dhtArea.top ? dhtArea.top : m_dhtArea.top;
		m_dhtArea.bottom = m_dhtArea.bottom > dhtArea.bottom ? m_dhtArea.bottom : dhtArea.bottom;

		m_fp << LogOutputHeader() << "\tCoalescing nodes along vertical edge." << endl;
	} else {
		// no edges we can combine along, have to have seperate
		// dht areas.

		m_fp << LogOutputHeader() << "\tNodes can't be coalesced into a single DHT area." << endl;
	}

	m_fp << "\tNew DHT area:" << endl
		<< "\t\tLeft: " << m_dhtArea.left << endl
		<< "\t\tRight: " << m_dhtArea.right << endl
		<< "\t\tTop: " << m_dhtArea.top << endl
		<< "\t\tBottom: " << m_dhtArea.bottom << endl;

	m_fp.flush();

	PruneNeighbors();
	SendNeighborsDHTUpdate();
}

bool Node::AreNeighbors(const NodeDHTArea& a, const NodeDHTArea& b)
{
	bool horzEdgeAbutted = abs(b.right - a.left) < m_edgeAbutEpsilon
		|| abs(b.left - a.right) < m_edgeAbutEpsilon;
	bool vertEdgeAbutted = abs(b.bottom - a.top) < m_edgeAbutEpsilon
		|| abs(b.top - a.bottom) < m_edgeAbutEpsilon;

	bool horzEdgeOverlapping =
			(b.left < a.left && a.left < b.right)
		||	(b.left < a.right && a.right < b.right)
		||	(a.left < b.left && b.left < a.right)
		||	(a.left < b.right && b.right < a.right)
		||	(a.left == b.left && a.right == b.right);
	bool vertEdgeOverlapping =
			(b.top < a.top && a.top < b.bottom)
		||	(b.top < a.bottom && a.bottom < b.bottom)
		||	(a.top < b.top && b.top < a.bottom)
		||	(a.top < b.bottom && b.bottom < a.bottom)
		||	(a.top == b.top && a.bottom == b.bottom);

	m_fp << LogOutputHeader();

	if ((horzEdgeAbutted && vertEdgeOverlapping) || (vertEdgeAbutted && horzEdgeOverlapping)) {
		m_fp << "\t\tEdge overlapped and abutted." << endl;
		return true;
	} else {
		if (horzEdgeAbutted) {
			m_fp << "\t\tHorizontal edge abutted but no vertical edge overlapping." << endl;
		} else if (vertEdgeAbutted) {
			m_fp << "\t\tVertical edge abutted but no horizontal edge overlapping." << endl;
		} else if (horzEdgeOverlapping) {
			m_fp << "\t\tHorizontal edge overlapping but no vertical edge abutting." << endl;
		} else if (vertEdgeOverlapping) {
			m_fp << "\t\tVertical edge overlapping but no horizontal edge abutting." << endl;
		} else {
			m_fp << "\t\tEdges not abutting or overlapping on any dimension." << endl;
		}

		return false;
	}
}

void Node::PruneNeighbors()
{
	// remove all the neighbors we no longer
	// share an edge with (i.e. those that are no longer neighbors)
	m_fp << LogOutputHeader() << "\tPruning my neighbors:" << endl;
	for (size_t i = 0; i < m_vNeighbors.size();) {

		m_fp << LogOutputHeader() << "\t\tNode #: " << m_vNeighbors[i].nodeNum << endl
			<< "\t\t\tLeft: " << m_vNeighbors[i].neighborArea.left << endl
			<< "\t\t\tRight: " << m_vNeighbors[i].neighborArea.right << endl
			<< "\t\t\tTop: " << m_vNeighbors[i].neighborArea.top << endl
			<< "\t\t\tBottom: " << m_vNeighbors[i].neighborArea.bottom << endl
			<< endl;

		bool isMyNeighbor = AreNeighbors(m_dhtArea, m_vNeighbors[i].neighborArea);

		if(isMyNeighbor) {
			m_fp << LogOutputHeader() << "Is neighbor. Keeping it." << endl;
			++i;
		} else {
			m_fp << LogOutputHeader() << "Is not neighbor. Removing it." << endl;
			m_fp.flush();
			m_vNeighbors.erase(m_vNeighbors.begin() + i);
			m_fp << "1 ";
			m_fp.flush();
			m_neighborTimeSinceLastUpdate.erase(m_neighborTimeSinceLastUpdate.begin() + i);
			m_fp << "2 ";
			m_fp.flush();
			m_vNeighborsOfNeighbors.erase(m_vNeighborsOfNeighbors.begin() + i);
			m_fp << "3" << endl;
		}

		m_fp.flush();
	}

	SendRootNeighborsUpdate();
}

string Node::LogOutputHeader()
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
	MPI_Send((void*)&neighborUpdate, sizeof(NodeToRootMessage::NodeNeighborUpdate) / sizeof(int), MPI_INT, m_rootRank, NodeToRootMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD);
}

const vector<NodeNeighbor>&	Node::GetNeighbors()
{
	return m_vNeighbors;
}

bool operator==(const NodeNeighbor& a, const NodeNeighbor& b)
{
	return a.nodeNum == b.nodeNum && a.neighborArea == b.neighborArea;
}

bool operator==(const NodeDHTArea& a, const NodeDHTArea& b)
{
	return a.left == b.left && a.right == b.right && a.top == b.top && a.bottom == b.bottom;
}