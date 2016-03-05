#ifndef __NODE_H__
#define __NODE_H__

#include <vector>
#include <random>
#include <fstream>
#include <string>

#include "nodeMessages.h"

typedef struct {
	NodeNeighbor									neighbor;
	std::vector<NodeNeighbor>						neighborNeighbors;
	std::vector<bool>								neighborsAllowingTakeover;
	std::chrono::high_resolution_clock::time_point	timeOfDeath;
	std::chrono::duration<float, std::milli>		timeUntilTakeover;

	enum DeadNeighborState
	{
		INITIAL = 0,
		TIMER_OFF
	} state;
}	DeadNeighbor;

class Node
{
public:
	Node(int mpiRank, int rootRank, std::ofstream* fp);

	bool	InitializeDHTArea(int aliveNode);
	bool	SplitDHTArea(int newNode, DHTArea& newDHTArea, std::vector<NodeNeighbor>& r_vNewNodeNeighbors, std::vector<std::vector<NodeNeighbor>>& r_vNewNodeNeighborsOfNeighbors);
	bool	UpdateNeighborDHTArea(int nodeNum, const DHTArea& dhtArea, const std::vector<NodeNeighbor>& neighborsOfNeighbor);
	void	UpdateNeighbors();

	const DHTArea&						GetDHTArea() const;
	const std::vector<NodeNeighbor>&	GetNeighbors() const;
	int									GetNearestNeighbor(float x, float y) const;
	const int							GetNextSplitAxis() const;

	void								ResolveNodeTakeoverNotification(int srcNode, InterNodeMessage::NodeTakeoverInfo& nodeTakeoverNotification);
	void								ResolveNodeTakeoverRequest(int srcNode, InterNodeMessage::NodeTakeoverInfo& nodeTakeoverRequest);
	void								ResolveNodeTakeoverResponse(int srcNode, InterNodeMessage::NodeTakeoverResponse& nodeTakeoverResponse);

	std::string		LogOutputHeader() const;

	static bool		SendNeighborsOverMPI(const std::vector<NodeNeighbor>& neighbors, int rank, int tag);
	static bool		RecvNeighborsOverMPI(std::vector<NodeNeighbor>& neighbors, int rank, int tag);

private:
	void		PruneNeighbors();
	void		SendNeighborsDHTUpdate();
	void		SendRootNeighborsUpdate();
	void		TakeoverDHTArea(DHTArea dhtArea);

	bool															m_bAlive;
	std::vector<NodeNeighbor>										m_vNeighbors;
	std::vector<std::vector<NodeNeighbor>>							m_vNeighborsOfNeighbors;
	std::vector<DeadNeighbor>										m_vDeadNeighbors;
	DHTArea															m_dhtArea;
	std::vector<std::chrono::high_resolution_clock::time_point>		m_neighborTimeSinceLastUpdate;
	std::chrono::high_resolution_clock::time_point					m_timeOfLastNeighborUpdate;
	std::mt19937													m_randGenerator;
	std::uniform_real_distribution<float>							m_randomAreaCoordGenerator;
	int																m_nextSplitAxis;
	int																m_mpiRank;
	int																m_rootRank;
	std::ofstream&													m_fp;

	const std::chrono::duration<int, std::milli>					m_timeUntilUpdateNeighbors;
	const std::chrono::duration<int, std::milli>					m_timeUntilNeighborConsideredOffline;
	const std::chrono::duration<float, std::milli>					m_takeoverTimerPerArea;
	const float														m_edgeAbutEpsilon;

	bool															m_bSeenDeath;
};

#endif