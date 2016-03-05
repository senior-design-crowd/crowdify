#ifndef __NODE_MSG_TAGS__
#define __NODE_MSG_TAGS__

#include <string>
#include <stdio.h>
#include <string.h>
#include <mpi.h>

#include "DHTArea.h"

#define OutputVecOperation(isBefore)
//#define OutputVecOperation(isBefore) do { \
//std::string str = __FILE__; \
//int			lineNum = __LINE__; \
//bool		before = isBefore; \
//\
//MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, m_rootRank, NodeToRootMessageTags::VECTOR_OPERATION, MPI_COMM_WORLD); \
//MPI_Send((void*)&lineNum, 1, MPI_INT, m_rootRank, NodeToRootMessageTags::VECTOR_OPERATION, MPI_COMM_WORLD); \
//MPI_Send((void*)&before, sizeof(bool), MPI_CHAR, m_rootRank, NodeToRootMessageTags::VECTOR_OPERATION, MPI_COMM_WORLD); \
//} while (0);

#define OutputDebugMsg(formatStr, ...)
//#define OutputDebugMsg(formatStr, ...) do { \
//if(m_bSeenDeath) {\
//	char tmpBuf[256] = {0}; \
//	sprintf(tmpBuf, formatStr, __VA_ARGS__); \
//	std::string str = __FILE__; \
//	int			lineNum = __LINE__; \
//	\
//	MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, m_rootRank, NodeToRootMessageTags::DEBUG_MSG, MPI_COMM_WORLD); \
//	str = tmpBuf; \
//	MPI_Send((void*)str.c_str(), str.length(), MPI_CHAR, m_rootRank, NodeToRootMessageTags::DEBUG_MSG, MPI_COMM_WORLD); \
//	MPI_Send((void*)&lineNum, 1, MPI_INT, m_rootRank, NodeToRootMessageTags::DEBUG_MSG, MPI_COMM_WORLD); \
//}\
//} while (0);

namespace NodeToRootMessageTags
{
	enum NodeToRootMessageTag
	{
		NODE_ALIVE = 0,
		DHT_AREA_UPDATE,
		NEIGHBOR_UPDATE,
		NODE_TO_NODE_MSG,
		VECTOR_OPERATION,
		DEBUG_MSG,
		NUM_NODE_MSG_TAGS
	};
};

namespace NodeToNodeMsgTypes
{
	enum NodeToNodeMsgType {
		SENDING,
		RECEIVING,
		ROUTING,
		IDLE
	};
}

namespace NodeToRootMessage
{
	typedef struct {
		int										otherNode;
		NodeToNodeMsgTypes::NodeToNodeMsgType	msgType;
	} NodeToNodeMsg;

	typedef struct {
		int neighbors[10];
		int numNeighbors;
	} NodeNeighborUpdate;

	extern MPI_Datatype MPI_NodeToNodeMsg;
	extern MPI_Datatype MPI_NodeNeighborUpdate;
}

namespace NodeAliveStates
{
	enum NodeAliveState
	{
		ALIVE,
		DEAD,
		ONLY_NODE_IN_NETWORK
	};
};

namespace InterNodeMessageTags
{
	enum InterNodeMessageTag
	{
		REQUEST_DHT_POINT_OWNER = NodeToRootMessageTags::NUM_NODE_MSG_TAGS,
		RESPONSE_DHT_POINT_OWNER,
		REQUEST_DHT_AREA,
		RESPONSE_DHT_AREA,
		NEIGHBOR_UPDATE,
		NODE_TAKEOVER_REQUEST,
		NODE_TAKEOVER_RESPONSE,
		NODE_TAKEOVER_NOTIFY
	};
};

namespace InterNodeMessage
{
	extern MPI_Datatype MPI_DHTRegion;

	typedef struct
	{
		float	x, y;
		int		origRequestingNode;
	} DHTPointOwnerRequest;

	extern MPI_Datatype MPI_DHTPointOwnerRequest;

	typedef struct
	{
		int pointOwnerNode;
	} DHTPointOwnerResponse;

	extern MPI_Datatype MPI_DHTPointOwnerResponse;

	typedef struct
	{
		int				numNeighbors;
		int				nextAxisToSplit;
	} DHTAreaResponse;

	extern MPI_Datatype MPI_DHTAreaResponse;

	typedef struct {
		DHTArea			myDHTArea;
		int				nodeNum;
	} NodeTakeoverInfo;

	typedef struct
	{
		DHTArea			myDHTArea;
		int				nodeNum;

		enum ResponseType : unsigned int {
			CAN_TAKEOVER = 1,
			SMALLER_AREA,
			LOWER_NODE_ADDRESS,
			NOT_MY_NEIGHBOR,
			NODE_STILL_ALIVE,
			TAKEOVER_ERROR
		} response;
	} NodeTakeoverResponse;

	extern MPI_Datatype MPI_NodeTakeoverResponse;
};

bool InitMPITypes();
void NotifyRootOfMsg(NodeToNodeMsgTypes::NodeToNodeMsgType msgType, int otherNode, int rootRank);

#endif