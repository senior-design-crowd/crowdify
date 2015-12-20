#ifndef __NODE_H__
#define __NODE_H__

#include <vector>
#include <random>
#include <fstream>

#include "header.h"

class Node
{
public:
	Node(int mpiRank, std::ofstream* fp);

	bool	initializeDHTArea(int aliveNode);
	bool	splitDHTArea(int newNode, NodeDHTArea& newNodeDHTArea, std::vector<NodeNeighbor>& r_vNewNodeNeighbors);

	const NodeDHTArea&	getDHTArea();
	int					getNearestNeighbor(float x, float y);

private:
	bool									m_bAlive;
	std::vector<NodeNeighbor>				m_vNeighbors;
	NodeDHTArea								m_dhtArea;
	std::mt19937							m_randGenerator;
	std::uniform_real_distribution<float>	m_randomAreaCoordGenerator;
	int										m_lastSplitAxis;
	int										m_mpiRank;
	std::ofstream&							m_fp;
};

#endif