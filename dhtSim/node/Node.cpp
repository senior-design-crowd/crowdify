#include <mpi.h>
#include <math.h>
#include <limits>
#include <vector>
#include <chrono>
#include <fstream>

using namespace std;

#include "header.h"
#include "nodeMessageTags.h"

#include "Node.h"

float ptDistToRect(float x, float y, float left, float right, float top, float bottom);

Node::Node(int mpiRank, ofstream* fp)
	: m_lastSplitAxis(-1), m_bAlive(false), m_dhtArea({0.0f, 1.0f, 0.0f, 1.0f}),
	m_randGenerator(chrono::system_clock::now().time_since_epoch().count()),
	m_randomAreaCoordGenerator(0.0f, 1.0f), m_mpiRank(mpiRank), m_fp(*fp)
{}

const NodeDHTArea& Node::getDHTArea()
{
	return m_dhtArea;
}

bool Node::initializeDHTArea(int aliveNode)
{
	if (aliveNode == -1) {
		m_dhtArea = { 0.0f, 1.0f, 0.0f, 1.0f };
		return true;
	}

	InterNodeMessage::DHTPointOwnerRequest dhtPoint = { m_randomAreaCoordGenerator(m_randGenerator), m_randomAreaCoordGenerator(m_randGenerator), m_mpiRank };
	MPI_Send((void*)&dhtPoint, sizeof(InterNodeMessage::DHTPointOwnerRequest) / sizeof(int), MPI_INT, aliveNode, InterNodeMessageTags::REQUEST_DHT_POINT_OWNER, MPI_COMM_WORLD);

	m_fp << "Requested DHT point owner of (" << dhtPoint.x << ", " << dhtPoint.y << ") from: " << aliveNode << endl;

	MPI_Status mpiStatus;
	InterNodeMessage::DHTPointOwnerResponse dhtPointOwner;
	MPI_Recv((void*)&dhtPointOwner, sizeof(InterNodeMessage::DHTPointOwnerResponse) / sizeof(int), MPI_INT, MPI_ANY_SOURCE, InterNodeMessageTags::RESPONSE_DHT_POINT_OWNER, MPI_COMM_WORLD, &mpiStatus);
	m_fp << "Got DHT point owner response from " << mpiStatus.MPI_SOURCE << '.' << endl;
	m_fp << "DHT point owner: " << dhtPointOwner.pointOwnerNode << endl;

	{
		// sending dummy data as this message doesn't require any data
		int tmp = 0;
		MPI_Send((void*)&tmp, 1, MPI_INT, dhtPointOwner.pointOwnerNode, InterNodeMessageTags::REQUEST_DHT_AREA, MPI_COMM_WORLD);
		m_fp << "Sent DHT area request to: " << dhtPointOwner.pointOwnerNode << endl;
	}

	InterNodeMessage::DHTAreaResponse dhtAreaResponse;
	MPI_Recv((void*)&dhtAreaResponse, sizeof(InterNodeMessage::DHTAreaResponse) / sizeof(int), MPI_INT, dhtPointOwner.pointOwnerNode, InterNodeMessageTags::RESPONSE_DHT_AREA, MPI_COMM_WORLD, &mpiStatus);

	m_fp << "Got DHT area response from " << dhtPointOwner.pointOwnerNode << ":" << endl
		<< "\tNew DHT Area:" << endl
		<< "\t\tLeft: " << dhtAreaResponse.newDHTArea.left << endl
		<< "\t\tRight: " << dhtAreaResponse.newDHTArea.right << endl
		<< "\t\tTop: " << dhtAreaResponse.newDHTArea.top << endl
		<< "\t\tBottom: " << dhtAreaResponse.newDHTArea.bottom << endl
		<< endl
		<< "\t" << dhtAreaResponse.numNeighbors << " Neighbor(s):" << endl;
	
	for (int i = 0; i < dhtAreaResponse.numNeighbors; ++i) {
		m_fp << "\t\tNeighbor " << i << endl
			<< "\t\t\tNode #: " << dhtAreaResponse.neighbors[i].nodeNum << endl
			<< "\t\t\t\tLeft: " << dhtAreaResponse.neighbors[i].neighborArea.left << endl
			<< "\t\t\t\tRight: " << dhtAreaResponse.neighbors[i].neighborArea.right << endl
			<< "\t\t\t\tTop: " << dhtAreaResponse.neighbors[i].neighborArea.top << endl
			<< "\t\t\t\tBottom: " << dhtAreaResponse.neighbors[i].neighborArea.bottom << endl;
	}

	m_fp << endl << endl;

	memcpy((void*)&m_dhtArea, (void*)&dhtAreaResponse.newDHTArea, sizeof(NodeDHTArea));

	m_vNeighbors.resize(dhtAreaResponse.numNeighbors);
	memcpy((void*)&m_vNeighbors[0], (void*)dhtAreaResponse.neighbors, sizeof(NodeNeighbor) * dhtAreaResponse.numNeighbors);

	return true;
}

bool Node::splitDHTArea(int newNode, NodeDHTArea& newNodeDHTArea, std::vector<NodeNeighbor>& r_vNewNodeNeighbors)
{
	m_fp << "\tSplitting node DHT area." << endl;

	// split the next axis
	m_lastSplitAxis = (m_lastSplitAxis + 1) % 2;

	// split the x axis
	if (m_lastSplitAxis == 0) {
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

	m_fp << "\tMy new DHT area:" << endl
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
	r_vNewNodeNeighbors.push_back({m_dhtArea, m_mpiRank});

	m_fp << "\tFinding new node neighbors:" << endl;
	// add all the neighbors that share an edge
	for (vector<NodeNeighbor>::const_iterator i = m_vNeighbors.begin(); i != m_vNeighbors.end(); ++i) {
		bool edgeShared = false;

		m_fp << "\t\tNode #: " << i->nodeNum << endl
			<< "\t\t\tLeft: " << i->neighborArea.left << endl
			<< "\t\t\tRight: " << i->neighborArea.right << endl
			<< "\t\t\tTop: " << i->neighborArea.top << endl
			<< "\t\t\tBottom: " << i->neighborArea.bottom << endl
			<< endl;

		if (abs(i->neighborArea.left - newNodeDHTArea.right) < 1e-4) {
			edgeShared = true;
			m_fp << "\t\tShares new node's right edge." << endl;
		} else if (abs(i->neighborArea.right - newNodeDHTArea.left) < 1e-4) {
			edgeShared = true;
			m_fp << "\t\tShares new node's left edge." << endl;
		} else if (abs(i->neighborArea.top - newNodeDHTArea.bottom) < 1e-4) {
			edgeShared = true;
			m_fp << "\t\tShares new node's bottom edge." << endl;
		} else if (abs(i->neighborArea.bottom - newNodeDHTArea.top) < 1e-4) {
			edgeShared = true;
			m_fp << "\t\tShares new node's top edge." << endl;
		} else {
			m_fp << "\t\tNo edge shared with new node." << endl;
		}

		if (edgeShared) {
			r_vNewNodeNeighbors.push_back(*i);
		}
	}

	m_fp << "\tNew node neighbors:" << endl;
	for (vector<NodeNeighbor>::const_iterator i = r_vNewNodeNeighbors.begin(); i != r_vNewNodeNeighbors.end(); ++i) {
		m_fp << endl
			<< "\t\tNode #: " << i->nodeNum << endl
			<< "\t\t\tLeft: " << i->neighborArea.left << endl
			<< "\t\t\tRight: " << i->neighborArea.right << endl
			<< "\t\t\tTop: " << i->neighborArea.top << endl
			<< "\t\t\tBottom: " << i->neighborArea.bottom << endl
			<< endl;
	}

	// add the newly created node to our neighbors
	NodeNeighbor newNeighbor;
	newNeighbor.nodeNum = newNode;
	newNeighbor.neighborArea = newNodeDHTArea;
	m_vNeighbors.push_back(newNeighbor);

	return true;
}

float ptDistToRect(float x, float y, float left, float right, float top, float bottom)
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

		float yDist = yDistFromTop > yDistFromBottom ? yDistFromBottom : yDistFromTop;
	}

	// return the squared distance to avoid
	// the square root that is unnecessary
	// if we're just comparing distances
	return xDist*xDist + yDist*yDist;
}

int Node::getNearestNeighbor(float x, float y)
{
	// return -1 if point is inside our DHT area
	if (x >= m_dhtArea.left && x <= m_dhtArea.right && y >= m_dhtArea.top && y <= m_dhtArea.bottom) {
		m_fp << "\t\tPoint is inside our DHT area." << endl;
		m_fp.flush();
		return -1;
	}

	vector<NodeNeighbor>::const_iterator	closestNeighbor = m_vNeighbors.end();
	float									closestNeighborDist = FLT_MAX;

	for (vector<NodeNeighbor>::const_iterator i = m_vNeighbors.begin(); i != m_vNeighbors.end(); ++i) {
		float neighborDist = ptDistToRect(x, y, i->neighborArea.left, i->neighborArea.right, i->neighborArea.top, i->neighborArea.bottom);

		m_fp << "\t\tPoint dist from node " << i->nodeNum << ": " << neighborDist << endl;

		if (neighborDist < closestNeighborDist) {
			m_fp << "\t\tNode is new closest node." << endl;
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