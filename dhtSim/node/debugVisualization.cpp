#include <mpi.h>

#include "nodeMessages.h"

void NotifyRootOfMsg(NodeToNodeMsgTypes::NodeToNodeMsgType msgType, int otherNode, int rootRank)
{
	NodeToRootMessage::NodeToNodeMsg nodeToNodeMsg;
	nodeToNodeMsg.msgType = msgType;
	nodeToNodeMsg.otherNode = otherNode;

	MPI_Send((void*)&nodeToNodeMsg, 1, NodeToRootMessage::MPI_NodeToNodeMsg, rootRank, NodeToRootMessageTags::NODE_TO_NODE_MSG, MPI_COMM_WORLD);
}