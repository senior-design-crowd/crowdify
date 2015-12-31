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
		NODE_TAKEOVER_RESPONSE
	};
};

namespace InterNodeMessage
{
	typedef struct
	{
		float	x, y;
		int		origRequestingNode;
	} DHTPointOwnerRequest;

	typedef struct
	{
		int pointOwnerNode;
	} DHTPointOwnerResponse;

	typedef struct
	{
		NodeDHTArea		newDHTArea;
		int				numNeighbors;
		int				nextAxisToSplit;
	} DHTAreaResponse;

	typedef struct
	{
		int				nodeNum;
		NodeDHTArea		myDHTArea;
	} NodeTakeoverRequest;

	typedef struct
	{
		int				nodeNum;
		NodeDHTArea		myDHTArea;

		enum
		{
			SUCCESS,
			SMALLER_AREA,
			NOT_MY_NEIGHBOR,
			NODE_STILL_ALIVE
		} Response;
	} NodeTakeoverResponse;
};

void NotifyRootOfMsg(NodeToNodeMsgTypes::NodeToNodeMsgType msgType, int otherNode, int rootRank);

#endif