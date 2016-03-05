#include "DHTArea.h"
#include "nodeMessages.h"

#include <limits>
#include <algorithm>
#include <vector>

#include <mpi.h>

using namespace std;

bool operator==(const DHTRegion& a, const DHTRegion& b);

DHTArea::DHTArea()
	: edgeAbutEpsilon(1e-4f)
{
}

DHTArea& DHTArea::operator=(const DHTArea& a)
{
	m_vDHTRegions = a.m_vDHTRegions;
	return *this;
}

const std::vector<DHTRegion>& DHTArea::GetRegions() const
{
	return m_vDHTRegions;
}

bool DHTArea::AddArea(const DHTArea& area)
{
	bool success = true;

	for (vector<DHTRegion>::const_iterator it = area.m_vDHTRegions.begin(); it != area.m_vDHTRegions.end(); ++it) {
		if (!AddRegion(*it)) {
			success = false;
		}
	}
	
	return success;
}

bool DHTArea::AddRegion(const DHTRegion& region)
{
	if (m_vDHTRegions.empty()) {
		m_vDHTRegions.push_back(region);
		return true;
	}

	//bool isNeighborToRegion = false;

	// go through each region and try to find one that we can coalesce with.
	// but if we can't coalesce with any of them then just add the new region.
	// if we just add the region, make sure its at least a neighbor to one of the regions.
	//
	// note sure if this last restriction is relevant at the moment, but will reconsider it later
	// TODO: think this through :P
	for (vector<DHTRegion>::iterator it = m_vDHTRegions.begin(); it != m_vDHTRegions.end(); ++it) {
		if (IsNeighbor(*it, region)) {
			//isNeighborToRegion = true;

			bool leftEdgeAbutted = abs(region.right - it->left) < edgeAbutEpsilon;
			bool rightEdgeAbutted = abs(region.left - it->right) < edgeAbutEpsilon;
			bool topEdgeAbutted = abs(region.bottom - it->top) < edgeAbutEpsilon;
			bool bottomEdgeAbutted = abs(region.top - it->bottom) < edgeAbutEpsilon;

			float myWidth = it->right - it->left;
			float myHeight = it->bottom - it->top;
			float regionWidth = region.right - region.left;
			float regionHeight = region.bottom - region.top;

			// if these DHT areas are adjacent and their
			// abutting edge is the same length, then
			// they can be combined. otherwise we'll
			// have to have two separate DHT areas.
			if ((leftEdgeAbutted || rightEdgeAbutted) && myHeight == regionHeight) {
				it->left = it->left > region.left ? region.left : it->left;
				it->right = it->right > region.right ? it->right : region.right;
				return true;
			} else if ((topEdgeAbutted || bottomEdgeAbutted) && myWidth == regionWidth) {
				it->top = it->top > region.top ? region.top : it->top;
				it->bottom = it->bottom > region.bottom ? it->bottom : region.bottom;
				return true;
			}
		}
	}

	// no edges we can combine along, have to have seperate
	// dht areas.
	m_vDHTRegions.push_back(region);
	return true;
}

DHTArea	DHTArea::SplitRegion(int splitAxis)
{
	DHTArea			newDHTArea;
	DHTRegion	newDHTRegion;

	// sort the DHT regions in ascending order of their total area
	sort(m_vDHTRegions.begin(), m_vDHTRegions.end(),
		[](const DHTRegion& a, const DHTRegion& b) -> bool
	{
		float areaOfA = (a.right - a.left) * (a.bottom - a.top);
		float areaOfB = (b.right - b.left) * (b.bottom - b.top);
		return areaOfA < areaOfB;
	});

	DHTRegion&	regionToSplit = *m_vDHTRegions.begin();

	// split the x axis
	if (splitAxis == 0) {
		newDHTRegion.top = regionToSplit.top;
		newDHTRegion.bottom = regionToSplit.bottom;

		// split our width and put the new node
		// to the right of us
		float newHorizEdgeLength = (regionToSplit.right - regionToSplit.left) * 0.5f;
		newDHTRegion.left = regionToSplit.left + newHorizEdgeLength;
		newDHTRegion.right = newDHTRegion.left + newHorizEdgeLength;
		regionToSplit.right = newDHTRegion.left;
	} else { // split the y axis
		newDHTRegion.left = regionToSplit.left;
		newDHTRegion.right = regionToSplit.right;

		// split our height and put the new node
		// underneath us
		float newVertEdgeLength = (regionToSplit.bottom - regionToSplit.top) * 0.5f;
		newDHTRegion.top = regionToSplit.top + newVertEdgeLength;
		newDHTRegion.bottom = newDHTRegion.top + newVertEdgeLength;
		regionToSplit.bottom = newDHTRegion.top;
	}

	newDHTArea.m_vDHTRegions.push_back(newDHTRegion);
	return newDHTArea;
}

bool	DHTArea::IsNeighbor(const DHTRegion& a, const DHTRegion& b) const
{
	bool horzEdgeAbutted = abs(b.right - a.left) < edgeAbutEpsilon
		|| abs(b.left - a.right) < edgeAbutEpsilon;
	bool vertEdgeAbutted = abs(b.bottom - a.top) < edgeAbutEpsilon
		|| abs(b.top - a.bottom) < edgeAbutEpsilon;

	bool horzEdgeOverlapping =
		(b.left < a.left && a.left < b.right)
		|| (b.left < a.right && a.right < b.right)
		|| (a.left < b.left && b.left < a.right)
		|| (a.left < b.right && b.right < a.right)
		|| (a.left == b.left && a.right == b.right);
	bool vertEdgeOverlapping =
		(b.top < a.top && a.top < b.bottom)
		|| (b.top < a.bottom && a.bottom < b.bottom)
		|| (a.top < b.top && b.top < a.bottom)
		|| (a.top < b.bottom && b.bottom < a.bottom)
		|| (a.top == b.top && a.bottom == b.bottom);

	if ((horzEdgeAbutted && vertEdgeOverlapping) || (vertEdgeAbutted && horzEdgeOverlapping)) {
		return true;
	} else {
		return false;
	}
}

bool	DHTArea::IsNeighbor(const DHTRegion& a) const
{
	for (vector<DHTRegion>::const_iterator it = m_vDHTRegions.begin(); it != m_vDHTRegions.end(); ++it) {
		if (IsNeighbor(*it, a)) {
			return true;
		}
	}

	return false;
}

bool	DHTArea::IsNeighbor(const DHTArea& a) const
{
	for (vector<DHTRegion>::const_iterator it = a.m_vDHTRegions.begin(); it != a.m_vDHTRegions.end(); ++it) {
		if (IsNeighbor(*it)) {
			return true;
		}
	}

	return false;
}

float	DHTArea::DistanceFromPt(float x, float y) const
{
	float minDist = numeric_limits<float>::max();

	for (vector<DHTRegion>::const_iterator it = m_vDHTRegions.begin(); it != m_vDHTRegions.end(); ++it) {
		float distFromRegion = RegionDistanceFromPt(x, y, *it);

		if (distFromRegion < minDist) {
			minDist = distFromRegion;
		}
	}

	return minDist;
}

float DHTArea::RegionTotalArea(const DHTRegion& a) const
{
	return (a.right - a.left) * (a.bottom - a.top);
}

float DHTArea::TotalArea() const
{
	float totalArea = 0.0f;

	for (vector<DHTRegion>::const_iterator it = m_vDHTRegions.begin(); it != m_vDHTRegions.end(); ++it) {
		totalArea += RegionTotalArea(*it);
	}

	return totalArea;
}

float DHTArea::RegionDistanceFromPt(float x, float y, const DHTRegion& a) const
{
	float xDist = 0.0f, yDist = 0.0f;

	// assuming the rectangles here are axially-aligned.
	// so the x distance from the rectangle is just the
	// distance to the closer vertical edge and the y distance
	// is similarly the distance to the closer horizontal edge

	if (x < a.left || x > a.right) {
		float xDistFromLeft = abs(x - a.left);
		float xDistFromRight = abs(x - a.right);

		xDist = xDistFromLeft > xDistFromRight ? xDistFromRight : xDistFromLeft;
	}

	if (y < a.top || y > a.bottom) {
		float yDistFromTop = abs(y - a.top);
		float yDistFromBottom = abs(y - a.bottom);

		yDist = yDistFromTop > yDistFromBottom ? yDistFromBottom : yDistFromTop;
	}

	// return the squared distance to avoid
	// the square root that is unnecessary
	// if we're just comparing distances
	return xDist*xDist + yDist*yDist;
}

bool DHTArea::operator==(const DHTArea& a) const
{
	for (vector<DHTRegion>::const_iterator i = m_vDHTRegions.begin(); i != m_vDHTRegions.end(); ++i) {
		bool match = false;

		for (vector<DHTRegion>::const_iterator j = m_vDHTRegions.begin(); j != m_vDHTRegions.end(); ++j) {
			if (*i == *j) {
				match = true;
				break;
			}
		}

		if (!match) {
			return false;
		}
	}

	return true;
}

bool DHTArea::operator!=(const DHTArea& a) const
{
	return !(a == *(const DHTArea*)this);
}

bool DHTArea::SendOverMPI(int rank, int tag) const
{
	int numRegions = m_vDHTRegions.size();
	MPI_Send((void*)&numRegions, 1, MPI_INT, rank, tag, MPI_COMM_WORLD);

	if(numRegions > 0) {
		MPI_Send((void*)&m_vDHTRegions[0], m_vDHTRegions.size(), InterNodeMessage::MPI_DHTRegion, rank, tag, MPI_COMM_WORLD);
	}

	return true;
}

bool DHTArea::RecvOverMPI(int rank, int tag)
{
	MPI_Status mpiStatus;

	int numRegions = 0;
	MPI_Recv((void*)&numRegions, 1, MPI_INT, rank, tag, MPI_COMM_WORLD, &mpiStatus);
	
	if (numRegions > 0) {
		m_vDHTRegions.resize(numRegions);
		MPI_Recv((void*)&m_vDHTRegions[0], numRegions, InterNodeMessage::MPI_DHTRegion, rank, tag, MPI_COMM_WORLD, &mpiStatus);
		return true;
	}

	return false;
}

std::ostream& operator<<(std::ostream& o, const DHTRegion& a)
{
	o << "Left: " << a.left << endl
		<< "Right: " << a.right << endl
		<< "Top: " << a.top << endl
		<< "Bottom: " << a.bottom << endl;

	return o;
}

std::ostream& operator<<(std::ostream& o, const DHTArea& a)
{
	size_t regionNum = 0;
	for (vector<DHTRegion>::const_iterator it = a.m_vDHTRegions.begin(); it != a.m_vDHTRegions.end(); ++it, ++regionNum) {
		o << "Region #" << regionNum << ":" << endl
			<< *it << endl;
	}

	return o;
}

bool operator==(const DHTRegion& a, const DHTRegion& b)
{
	return a.left == b.left && a.right == b.right && a.top == b.top && a.bottom == b.bottom;
}

bool operator==(const NodeNeighbor& a, const NodeNeighbor& b)
{
	return a.neighborArea == b.neighborArea && a.nodeNum == b.nodeNum;
}