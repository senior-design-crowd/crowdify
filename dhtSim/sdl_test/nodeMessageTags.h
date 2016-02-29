#ifndef __NODE_MSG_TAGS__
#define __NODE_MSG_TAGS__

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

	extern MPI_Datatype MPI_NodeDHTArea;
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

#endif