#ifndef __NODE_MSG_TAGS__
#define __NODE_MSG_TAGS__

#include "header.h"

namespace NodeToRootMessageTags
{
	enum NodeToRootMessageTag
	{
		NODE_ALIVE = 0,
		DHT_AREA_UPDATE,
		NEIGHBOR_UPDATE,
		NODE_TO_NODE_MSG,
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
	extern MPI_Datatype MPI_NodeDHTArea;
	extern MPI_Datatype MPI_NodeNeighbor;

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
		NodeDHTArea		newDHTArea;
		int				numNeighbors;
		int				nextAxisToSplit;
	} DHTAreaResponse;

	extern MPI_Datatype MPI_DHTAreaResponse;

	typedef struct
	{
		int				nodeNum;
		NodeDHTArea		myDHTArea;
	} NodeTakeoverInfo;

	extern MPI_Datatype MPI_NodeTakeoverInfo;

	typedef struct
	{
		int				nodeNum;

		enum ResponseType : unsigned int {
			CAN_TAKEOVER = 1,
			SMALLER_AREA,
			NOT_MY_NEIGHBOR,
			NODE_STILL_ALIVE
		};

		int				response;
		NodeDHTArea		myDHTArea;
	} NodeTakeoverResponse;

	extern MPI_Datatype MPI_NodeTakeoverResponse;
};

void NotifyRootOfMsg(NodeToNodeMsgTypes::NodeToNodeMsgType msgType, int otherNode, int rootRank);

#endif