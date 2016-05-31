#ifndef __NODE_H__
#define __NODE_H__

#include <vector>

//#ifdef __GNUC__
#include <tr1/random>
#define randns std::tr1

typedef randns::mt19937 generator_t;
typedef randns::uniform_real<float> distribution_t;
typedef randns::variate_generator<generator_t, distribution_t> variate_t;
//#else
//#include <random>
//#define randns std
//#endif

#include <fstream>
#include <string>

#include "nodeMessages.h"
#include "HashEval.h"
#include "BlockTransferer.h"

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
	void	Update();

	const DHTArea&						GetDHTArea() const;
	const std::vector<NodeNeighbor>&	GetNeighbors() const;
	int									GetNearestNeighbor(float x, float y) const;
	const int							GetNextSplitAxis() const;

	void								ResolveNodeTakeoverNotification(int srcNode, InterNodeMessage::NodeTakeoverInfo& nodeTakeoverNotification);
	void								ResolveNodeTakeoverRequest(int srcNode, InterNodeMessage::NodeTakeoverInfo& nodeTakeoverRequest);
	void								ResolveNodeTakeoverResponse(int srcNode, InterNodeMessage::NodeTakeoverResponse& nodeTakeoverResponse);

	std::string		LogOutputHeader() const;

	static bool		SendNeighborsOverMPI(const std::vector<NodeNeighbor>& neighbors, int rank, int tag, std::ofstream& fp);
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
	generator_t													m_randGenerator;
	distribution_t							m_randomAreaCoordGenerator;
	variate_t							m_randVariate;
	int																m_nextSplitAxis;

	int																m_mpiRank;
	int																m_rootRank;
	std::ofstream&													m_fp;

	const std::chrono::duration<int, std::milli>					m_timeUntilUpdate;
	const std::chrono::duration<int, std::milli>					m_timeUntilNeighborConsideredOffline;
	const std::chrono::duration<float, std::milli>					m_takeoverTimerPerArea;
	const float														m_edgeAbutEpsilon;
	bool															m_bSeenDeath;

	BlockTransferer													m_blockTransferer;
	std::string														m_ownBlockStoreAddr;
	std::vector<std::string>										m_vNeighborNodeAddrs;
	HashEval														m_hashEval;
};

#endif