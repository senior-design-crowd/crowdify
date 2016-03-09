#ifndef __DHTAREA_H__
#define __DHTAREA_H__

#include <vector>
#include <ostream>

typedef struct _DHTRegion {
	float left, right, top, bottom;
} DHTRegion;

std::ostream& operator<<(std::ostream& o, const DHTRegion& a);

class DHTArea
{
public:
	DHTArea();
	~DHTArea() {}
	DHTArea& operator=(const DHTArea& a);

	bool	AddArea(const DHTArea& area);
	bool	AddRegion(const DHTRegion& a);

	bool							IsNeighbor(const DHTRegion& a) const;
	bool							IsNeighbor(const DHTArea& a) const;
	float							DistanceFromPt(float x, float y) const;
	float							TotalArea() const;
	const std::vector<DHTRegion>&	GetRegions() const;

	DHTArea	SplitRegion(int splitAxis);

	bool	operator==(const DHTArea& a) const;
	bool	operator!=(const DHTArea& a) const;

	bool	SendOverMPI(int rank, int tag) const;
	bool	RecvOverMPI(int rank, int tag);

	friend std::ostream& operator<<(std::ostream& o, const DHTArea& a);

private:
	std::vector<DHTRegion>	m_vDHTRegions;

	bool	IsNeighbor(const DHTRegion& a, const DHTRegion& b) const;
	float	RegionDistanceFromPt(float x, float y, const DHTRegion& a) const;
	float	RegionTotalArea(const DHTRegion& a) const;

	const float	edgeAbutEpsilon;
};

typedef struct {
	DHTArea		neighborArea;
	int			nodeNum;
} NodeNeighbor;

bool operator==(const NodeNeighbor& a, const NodeNeighbor& b);

#endif