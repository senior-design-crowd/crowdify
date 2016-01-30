#include <mpi.h>

#include "nodeMessageTags.h"
#include "header.h"

void NotifyRootOfMsg(NodeToNodeMsgTypes::NodeToNodeMsgType msgType, int otherNode, int rootRank)
{
	NodeToRootMessage::NodeToNodeMsg nodeToNodeMsg;
	nodeToNodeMsg.msgType = msgType;
	nodeToNodeMsg.otherNode = otherNode;

	MPI_Send((void*)&nodeToNodeMsg, sizeof(NodeToRootMessage::NodeToNodeMsg) / sizeof(int), MPI_INT, rootRank, NodeToRootMessageTags::NODE_TO_NODE_MSG, MPI_COMM_WORLD);
}