#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <chrono>
#include <thread>
#include <random>
#include <vector>

#include "header.h"
#include "nodeMessageTags.h"
#include "Node.h"

using namespace std;

void NotifyRootOfMsg(NodeToNodeMsgTypes::NodeToNodeMsgType msgType, int otherNode, int rootRank);

namespace NodeToRootMessage {
	MPI_Datatype MPI_NodeToNodeMsg;
	MPI_Datatype MPI_NodeNeighborUpdate;
}

namespace InterNodeMessage {
	MPI_Datatype MPI_NodeDHTArea;
	MPI_Datatype MPI_NodeNeighbor;
	MPI_Datatype MPI_DHTPointOwnerRequest;
	MPI_Datatype MPI_DHTPointOwnerResponse;
	MPI_Datatype MPI_DHTAreaResponse;
	MPI_Datatype MPI_NodeTakeoverInfo;
	MPI_Datatype MPI_NodeTakeoverResponse;
}

bool InitMPITypes()
{
	int ret = -1;

	// MPI_NodeDHTArea
	{
		/*
		typedef struct _NodeDHTArea {
			float left, right, top, bottom;
		} NodeDHTArea;
		*/

		MPI_Datatype types[] = { MPI_FLOAT };
		int blockSizes[] = { 4 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0)
		};

		ret = MPI_Type_create_struct(1, blockSizes, displacements, types, &InterNodeMessage::MPI_NodeDHTArea);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_NodeDHTArea);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_NodeToNodeMsg
	{
		/*typedef struct {
		int										otherNode;
		NodeToNodeMsgTypes::NodeToNodeMsgType	msgType;
		} NodeToNodeMsg;*/

		MPI_Datatype types[] = { MPI_INT };
		int blockSizes[] = { 2 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0)
		};

		ret = MPI_Type_create_struct(1, blockSizes, displacements, types, &NodeToRootMessage::MPI_NodeToNodeMsg);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&NodeToRootMessage::MPI_NodeToNodeMsg);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_NodeNeighborUpdate
	{
		/*typedef struct {
		int neighbors[10];
		int numNeighbors;
		} NodeNeighborUpdate;*/

		MPI_Datatype types[] = { MPI_INT };
		int blockSizes[] = { 11 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0)
		};

		ret = MPI_Type_create_struct(1, blockSizes, displacements, types, &NodeToRootMessage::MPI_NodeNeighborUpdate);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&NodeToRootMessage::MPI_NodeNeighborUpdate);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_NodeNeighbor
	{
		/*
		typedef struct {
			int			nodeNum;
			NodeDHTArea neighborArea;
		} NodeNeighbor;
		*/

		MPI_Datatype types[] = { InterNodeMessage::MPI_NodeDHTArea, MPI_INT };
		int blockSizes[] = { 1, 1 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0),
			static_cast<MPI_Aint>(0)
		};

		MPI_Aint lb;
		ret = MPI_Type_get_extent(InterNodeMessage::MPI_NodeDHTArea, &lb, &displacements[1]);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_create_struct(2, blockSizes, displacements, types, &InterNodeMessage::MPI_NodeNeighbor);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_NodeNeighbor);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_DHTPointOwnerRequest
	{
		/*
		typedef struct
		{
			float	x, y;
			int		origRequestingNode;
		} DHTPointOwnerRequest;
		*/

		MPI_Datatype types[] = { MPI_FLOAT, MPI_INT };
		int blockSizes[] = { 2, 1 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0),
			static_cast<MPI_Aint>(sizeof(float) * 2)
		};

		ret = MPI_Type_create_struct(2, blockSizes, displacements, types, &InterNodeMessage::MPI_DHTPointOwnerRequest);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_DHTPointOwnerRequest);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_DHTPointOwnerResponse
	{
		/*
		typedef struct
		{
			int pointOwnerNode;
		} DHTPointOwnerResponse;
		*/

		MPI_Datatype types[] = { MPI_INT };
		int blockSizes[] = { 1 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0)
		};

		ret = MPI_Type_create_struct(1, blockSizes, displacements, types, &InterNodeMessage::MPI_DHTPointOwnerResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_DHTPointOwnerResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_DHTAreaResponse
	{
		/*
		typedef struct {
			NodeDHTArea		newDHTArea;
			int				numNeighbors;
			int				nextAxisToSplit;
		} DHTAreaResponse;
		*/
		
		MPI_Datatype types[] = { InterNodeMessage::MPI_NodeDHTArea, MPI_INT };
		int blockSizes[] = { 1, 2 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0),
			static_cast<MPI_Aint>(0)
		};

		MPI_Aint lb;
		ret = MPI_Type_get_extent(InterNodeMessage::MPI_NodeDHTArea, &lb, &displacements[1]);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_create_struct(2, blockSizes, displacements, types, &InterNodeMessage::MPI_DHTAreaResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_DHTAreaResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_NodeTakeoverInfo
	{
		/*
		typedef struct
		{
			int				nodeNum;
			NodeDHTArea		myDHTArea;
		} NodeTakeoverInfo;
		*/

		MPI_Datatype types[] = { MPI_INT, InterNodeMessage::MPI_NodeDHTArea };
		int blockSizes[] = { 1, 1 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0),
			static_cast<MPI_Aint>(sizeof(int))
		};

		ret = MPI_Type_create_struct(2, blockSizes, displacements, types, &InterNodeMessage::MPI_NodeTakeoverInfo);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_NodeTakeoverInfo);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	// MPI_NodeTakeoverResponse
	{
		/*
		typedef struct
		{
			NodeDHTArea		myDHTArea;
			int				nodeNum;

			enum ResponseType : unsigned int {
				CAN_TAKEOVER = 1,
				SMALLER_AREA,
				NOT_MY_NEIGHBOR,
				NODE_STILL_ALIVE
			};

			ResponseType	response;
		} NodeTakeoverResponse;
		*/

		MPI_Datatype types[] = { InterNodeMessage::MPI_NodeDHTArea, MPI_LONG };
		int blockSizes[] = { 1, 2 };
		MPI_Aint displacements[] = {
			static_cast<MPI_Aint>(0),
			static_cast<MPI_Aint>(0)
		};

		MPI_Aint lb;
		ret = MPI_Type_get_extent(InterNodeMessage::MPI_NodeDHTArea, &lb, &displacements[1]);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_create_struct(2, blockSizes, displacements, types, &InterNodeMessage::MPI_NodeTakeoverResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}

		ret = MPI_Type_commit(&InterNodeMessage::MPI_NodeTakeoverResponse);
		if (ret != MPI_SUCCESS) {
			return false;
		}
	}

	return true;
}

int main(int argc, char* argv[])
{
	bool alive = false;
	int mpiRank, mpiSize;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);

	if (argc > 1 && strcmp(argv[1], "-d") == 0) {
		while (!IsDebuggerPresent()) {
			this_thread::sleep_for(chrono::milliseconds(200));
		}
	}

	InitMPITypes();

	MPI_Status	mpiStatus;
	int			rootRank;
	MPI_Recv((void*)&rootRank, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &mpiStatus);

	char filename[128];
	sprintf(filename, "node%d.txt", mpiRank);
	
	ofstream fp(filename);

	Node node(mpiRank, rootRank, &fp);

	fp << node.LogOutputHeader() << "root rank: " << rootRank << endl;

	while (true) {
		int newAliveStateAvailable;
		MPI_Iprobe(rootRank, NodeToRootMessageTags::NODE_ALIVE, MPI_COMM_WORLD, &newAliveStateAvailable, &mpiStatus);

		if (!alive || newAliveStateAvailable) {
			// block until we receive a new alive state
			NodeAliveStates::NodeAliveState newAliveState;
			MPI_Recv((void*)&newAliveState, 1, MPI_INT, rootRank, NodeToRootMessageTags::NODE_ALIVE, MPI_COMM_WORLD, &mpiStatus);
			
			fp << node.LogOutputHeader() << "Got new alive state: ";

			if (newAliveState == NodeAliveStates::ALIVE) {
				fp << "ALIVE" << endl;
			} else if (newAliveState == NodeAliveStates::DEAD) {
				fp << "DEAD" << endl;
			} else if (newAliveState == NodeAliveStates::ONLY_NODE_IN_NETWORK) {
				fp << "ONLY_NODE_IN_NETWORK" << endl;
			}

			if (alive && newAliveState != NodeAliveStates::DEAD) {
				fp << node.LogOutputHeader() << "\tAlive state is duplicate." << endl;
				fp.flush();
			} else {
				fp.flush();

				// initialize our area in the DHT
				if (newAliveState != NodeAliveStates::DEAD) {
					fp << node.LogOutputHeader() << "Initializing node DHT area." << endl;
					fp.flush();

					if (newAliveState == NodeAliveStates::ONLY_NODE_IN_NETWORK) {
						node.InitializeDHTArea(-1);
					} else if (newAliveState == NodeAliveStates::ALIVE) {
						int aliveNode = -1;
						MPI_Recv((void*)&aliveNode, 1, MPI_INT, rootRank, NodeToRootMessageTags::NODE_ALIVE, MPI_COMM_WORLD, &mpiStatus);

						fp << node.LogOutputHeader() << "Alive node: " << aliveNode << endl;
						fp.flush();

						node.InitializeDHTArea(aliveNode);
					}

					alive = true;

					// update the root with our new area
					NodeDHTArea dhtArea = node.GetDHTArea();

					fp << node.LogOutputHeader() << "New DHT area:" << endl
						<< "\tLeft: " << dhtArea.left << endl
						<< "\tRight: " << dhtArea.right << endl
						<< "\tTop: " << dhtArea.top << endl
						<< "\tBottom: " << dhtArea.bottom << endl
						<< endl << "\tUpdated root with new DHT area." << endl;
					fp.flush();

					MPI_Ssend((void*)&dhtArea, 1, InterNodeMessage::MPI_NodeDHTArea, rootRank, NodeToRootMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD);
				} else {
					alive = false;
					continue;
				}
			}			
		}

		if (alive) {
			// queue for any new messages
			int newMsg = 0;

			MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_NOTIFY, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			while (newMsg == 1) {
				fp << node.LogOutputHeader() << "Got MPI message from " << mpiStatus.MPI_SOURCE << ": NODE_TAKEOVER_NOTIFY" << endl;
				fp.flush();

				InterNodeMessage::NodeTakeoverInfo takeoverNotification;
				MPI_Recv((void*)&takeoverNotification, 1, InterNodeMessage::MPI_NodeTakeoverInfo, MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_NOTIFY, MPI_COMM_WORLD, &mpiStatus);

				fp << node.LogOutputHeader() << "\tNode takeover notification:" << endl
					<< "\t\tNode num: " << takeoverNotification.nodeNum << endl
					<< "\t\tDHT Area: " << endl
					<< "\t\t\tLeft: " << takeoverNotification.myDHTArea.left << endl
					<< "\t\t\tRight: " << takeoverNotification.myDHTArea.right << endl
					<< "\t\t\tTop: " << takeoverNotification.myDHTArea.top << endl
					<< "\t\t\tBottom: " << takeoverNotification.myDHTArea.bottom << endl;
				fp.flush();

				node.ResolveNodeTakeoverNotification(mpiStatus.MPI_SOURCE, takeoverNotification);

				newMsg = 0;
				MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_NOTIFY, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			}

			MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			while (newMsg == 1) {
				fp << node.LogOutputHeader() << "Got MPI message from " << mpiStatus.MPI_SOURCE << ": NODE_TAKEOVER_REQUEST" << endl;
				fp.flush();

				InterNodeMessage::NodeTakeoverInfo nodeTakeoverRequest;
				MPI_Recv((void*)&nodeTakeoverRequest, 1, InterNodeMessage::MPI_NodeTakeoverInfo, MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD, &mpiStatus);

				fp << node.LogOutputHeader() << "\tNode takeover request:" << endl
					<< "\t\tNode num: " << nodeTakeoverRequest.nodeNum << endl
					<< "\t\tDHT Area: " << endl
					<< "\t\t\tLeft: " << nodeTakeoverRequest.myDHTArea.left << endl
					<< "\t\t\tRight: " << nodeTakeoverRequest.myDHTArea.right << endl
					<< "\t\t\tTop: " << nodeTakeoverRequest.myDHTArea.top << endl
					<< "\t\t\tBottom: " << nodeTakeoverRequest.myDHTArea.bottom << endl;
				fp.flush();

				node.ResolveNodeTakeoverRequest(mpiStatus.MPI_SOURCE, nodeTakeoverRequest);

				newMsg = 0;
				MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			}

			newMsg = 0;
			MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD, &newMsg, &mpiStatus);

			while (newMsg == 1) {
				fp << node.LogOutputHeader() << "Got MPI message from " << mpiStatus.MPI_SOURCE << ": NODE_TAKEOVER_RESPONSE" << endl;
				fp.flush();

				InterNodeMessage::NodeTakeoverResponse nodeTakeoverResponse;
				memset((void*)&nodeTakeoverResponse, 0, sizeof(InterNodeMessage::NodeTakeoverResponse));
				MPI_Recv((void*)&nodeTakeoverResponse, 1, InterNodeMessage::MPI_NodeTakeoverResponse, MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD, &mpiStatus);

				fp << node.LogOutputHeader() << "\tNode takeover response:" << endl
					<< "\t\tNode num: " << nodeTakeoverResponse.nodeNum << endl
					<< "\t\tDHT Area: " << endl
					<< "\t\t\tLeft: " << nodeTakeoverResponse.myDHTArea.left << endl
					<< "\t\t\tRight: " << nodeTakeoverResponse.myDHTArea.right << endl
					<< "\t\t\tTop: " << nodeTakeoverResponse.myDHTArea.top << endl
					<< "\t\t\tBottom: " << nodeTakeoverResponse.myDHTArea.bottom << endl;

				/*CAN_TAKEOVER = 1,
					SMALLER_AREA,
					LOWER_NODE_ADDRESS,
					NOT_MY_NEIGHBOR,
					NODE_STILL_ALIVE,
					TAKEOVER_ERROR*/

				if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::CAN_TAKEOVER) {
					fp << "\t\tResponse: CAN_TAKEOVER" << endl;
				} else if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::SMALLER_AREA) {
					fp << "\t\tResponse: SMALLER_AREA" << endl;
				} else if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::LOWER_NODE_ADDRESS) {
					fp << "\t\tResponse: LOWER_NODE_ADDRESS" << endl;
				} else if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::NOT_MY_NEIGHBOR) {
					fp << "\t\tResponse: NOT_MY_NEIGHBOR" << endl;
				} else if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::NODE_STILL_ALIVE) {
					fp << "\t\tResponse: NODE_STILL_ALIVE" << endl;
				} else if (nodeTakeoverResponse.response == InterNodeMessage::NodeTakeoverResponse::TAKEOVER_ERROR) {
					fp << "\t\tResponse: TAKEOVER_ERROR" << endl;
				} else {
					fp << "\t\tResponse: UNKNOWN (" << nodeTakeoverResponse.response << ")" << endl;
				}

				node.ResolveNodeTakeoverResponse(mpiStatus.MPI_SOURCE, nodeTakeoverResponse);

				newMsg = 0;
				MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			}

			newMsg = 0;
			MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			while (newMsg == 1) {
				fp << node.LogOutputHeader() << "Got MPI message from " << mpiStatus.MPI_SOURCE << ": NEIGHBOR_UPDATE" << endl;
				fp.flush();

				/*fp << "\tGetting neighbor area." << endl;
				fp.flush();*/
				NodeDHTArea neighborDHTArea;
				MPI_Recv((void*)&neighborDHTArea, 1, InterNodeMessage::MPI_NodeDHTArea, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &mpiStatus);

				/*fp << "\tGetting # neighbors." << endl;
				fp.flush();*/
				int numNeighborsOfNeighbor;
				MPI_Recv((void*)&numNeighborsOfNeighbor, 1, MPI_INT, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &mpiStatus);

				/*fp << "\tGetting neighbors of neighbor." << endl;
				fp.flush();*/
				vector<NodeNeighbor> neighborsOfNeighbor;
				neighborsOfNeighbor.resize(numNeighborsOfNeighbor);
				MPI_Recv((void*)&neighborsOfNeighbor[0], numNeighborsOfNeighbor, InterNodeMessage::MPI_NodeNeighbor, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &mpiStatus);

				/*fp << "\tUpdating neighbor info." << endl;
				fp.flush();*/
				node.UpdateNeighborDHTArea(mpiStatus.MPI_SOURCE, neighborDHTArea, neighborsOfNeighbor);

				NotifyRootOfMsg(NodeToNodeMsgTypes::RECEIVING, mpiStatus.MPI_SOURCE, rootRank);

				fp << node.LogOutputHeader() << "\tNew neighbor DHT area:" << endl
					<< "\t\tLeft: " << neighborDHTArea.left << endl
					<< "\t\tRight: " << neighborDHTArea.right << endl
					<< "\t\tTop: " << neighborDHTArea.top << endl
					<< "\t\tBottom: " << neighborDHTArea.bottom << endl;

				for (vector<NodeNeighbor>::const_iterator i = neighborsOfNeighbor.begin(); i != neighborsOfNeighbor.end(); ++i) {
					fp << "\tNew neighbor neighbor:" << endl
						<< "\t\tNode num: " << i->nodeNum << endl
						<< "\t\tLeft: " << i->neighborArea.left << endl
						<< "\t\tRight: " << i->neighborArea.right << endl
						<< "\t\tTop: " << i->neighborArea.top << endl
						<< "\t\tBottom: " << i->neighborArea.bottom << endl;
				}

				newMsg = 0;
				MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			}

			newMsg = 0;
			MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::REQUEST_DHT_AREA, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			while (newMsg == 1) {
				fp << node.LogOutputHeader() << "Got MPI message from " << mpiStatus.MPI_SOURCE << ": REQUEST_DHT_AREA" << endl;
				fp.flush();

				// DHT area requests don't contain any content, but MPI requires us to send something. so we send a dummy variable.
				int tmp;
				MPI_Recv((void*)&tmp, 1, MPI_INT, MPI_ANY_SOURCE, InterNodeMessageTags::REQUEST_DHT_AREA, MPI_COMM_WORLD, &mpiStatus);

				NotifyRootOfMsg(NodeToNodeMsgTypes::RECEIVING, mpiStatus.MPI_SOURCE, rootRank);
					
				InterNodeMessage::DHTAreaResponse dhtAreaResponse;

				vector<NodeNeighbor>	vNewNodeNeighbors;
				vector<vector<NodeNeighbor>> vNewNodeNeighborsOfNeighbors;
				node.SplitDHTArea(mpiStatus.MPI_SOURCE, dhtAreaResponse.newDHTArea, vNewNodeNeighbors, vNewNodeNeighborsOfNeighbors);

				dhtAreaResponse.nextAxisToSplit = node.GetNextSplitAxis();
				dhtAreaResponse.numNeighbors = vNewNodeNeighbors.size();

				fp << node.LogOutputHeader() << "Sending new DHT area to: " << mpiStatus.MPI_SOURCE << endl;

				MPI_Send((void*)&dhtAreaResponse,
					1,
					InterNodeMessage::MPI_DHTAreaResponse,
					mpiStatus.MPI_SOURCE,
					InterNodeMessageTags::RESPONSE_DHT_AREA,
					MPI_COMM_WORLD);

				MPI_Send((void*)&vNewNodeNeighbors[0],
					vNewNodeNeighbors.size(),
					InterNodeMessage::MPI_NodeNeighbor,
					mpiStatus.MPI_SOURCE,
					InterNodeMessageTags::RESPONSE_DHT_AREA,
					MPI_COMM_WORLD);

				// for each of the new node's neighbors, send:
				//	o the number of neighbors that neighbor itself has
				//	o the DHT areas of all those neighbors
				for (size_t i = 0; i < vNewNodeNeighbors.size(); ++i) {					
					int numNeighborsOfNeighbor = vNewNodeNeighborsOfNeighbors[i].size();
					MPI_Send((void*)&numNeighborsOfNeighbor,
						1,
						MPI_INT,
						mpiStatus.MPI_SOURCE,
						InterNodeMessageTags::RESPONSE_DHT_AREA,
						MPI_COMM_WORLD);

					if(numNeighborsOfNeighbor > 0) {
						MPI_Send((void*)&vNewNodeNeighborsOfNeighbors[i][0],
							numNeighborsOfNeighbor,
							InterNodeMessage::MPI_NodeNeighbor,
							mpiStatus.MPI_SOURCE,
							InterNodeMessageTags::RESPONSE_DHT_AREA,
							MPI_COMM_WORLD);
					}
				}

				NotifyRootOfMsg(NodeToNodeMsgTypes::SENDING, mpiStatus.MPI_SOURCE, rootRank);

				NodeDHTArea dhtArea = node.GetDHTArea();

				fp << endl << "\tUpdated root with new DHT area." << endl;

				MPI_Ssend((void*)&dhtArea, 1, InterNodeMessage::MPI_NodeDHTArea, rootRank, NodeToRootMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD);

				newMsg = 0;
				MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::REQUEST_DHT_AREA, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			}

			// only want to process one routing request at a time
			// because neighbor updates are crucial to have an accurate
			// view of the network and to avoid routing cycles
			MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::REQUEST_DHT_POINT_OWNER, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			if (newMsg == 1) {
				fp << node.LogOutputHeader() << "Got MPI message from " << mpiStatus.MPI_SOURCE << ": REQUEST_DHT_POINT_OWNER" << endl;
				fp.flush();

				InterNodeMessage::DHTPointOwnerRequest pointOwnerRequest;
				MPI_Recv((void*)&pointOwnerRequest, 1, InterNodeMessage::MPI_DHTPointOwnerRequest, MPI_ANY_SOURCE, InterNodeMessageTags::REQUEST_DHT_POINT_OWNER, MPI_COMM_WORLD, &mpiStatus);

				fp << node.LogOutputHeader() << "\tPoint ownership request originally from " << pointOwnerRequest.origRequestingNode << ": (" << pointOwnerRequest.x << ", " << pointOwnerRequest.y << ")" << endl;

				NotifyRootOfMsg(NodeToNodeMsgTypes::RECEIVING, mpiStatus.MPI_SOURCE, rootRank);

				InterNodeMessage::DHTPointOwnerResponse pointOwnerResponse;
				pointOwnerResponse.pointOwnerNode = node.GetNearestNeighbor(pointOwnerRequest.x, pointOwnerRequest.y);

				fp << node.LogOutputHeader() << "\tNearest neighbor: ";

				// if the point is inside our DHT area then send
				// the response back to the original node that made the request.
				// otherwise forward the request on to our closest neighbor.
				if (pointOwnerResponse.pointOwnerNode == -1) {
					fp << "self" << endl;
					pointOwnerResponse.pointOwnerNode = mpiRank;
					MPI_Send((void*)&pointOwnerResponse, 1, InterNodeMessage::MPI_DHTPointOwnerResponse, pointOwnerRequest.origRequestingNode, InterNodeMessageTags::RESPONSE_DHT_POINT_OWNER, MPI_COMM_WORLD);

					NotifyRootOfMsg(NodeToNodeMsgTypes::SENDING, pointOwnerRequest.origRequestingNode, rootRank);
				} else {
					fp << pointOwnerResponse.pointOwnerNode << endl;
					MPI_Send((void*)&pointOwnerRequest, 1, InterNodeMessage::MPI_DHTPointOwnerRequest, pointOwnerResponse.pointOwnerNode, InterNodeMessageTags::REQUEST_DHT_POINT_OWNER, MPI_COMM_WORLD);

					NotifyRootOfMsg(NodeToNodeMsgTypes::ROUTING, pointOwnerResponse.pointOwnerNode, rootRank);
				}

				fp.flush();
			}

			// update neighbors if needed
			node.UpdateNeighbors();

			/*
			int rankToSend = rand() % mpiSize;
			fprintf(fp, "rank destination: %d\n", rankToSend);

			if (rankToSend != mpiRank && rankToSend != rootRank) {
				if (rankToSend > rootRank) {
					--rankToSend;
					fprintf(fp, "rank destination offset by 1, correction: %d\n", rankToSend);
				}

				// send the message and then delay so we don't spam the renderer
				MPI_Send((void*)&rankToSend, 1, MPI_INT, rootRank, 0, MPI_COMM_WORLD);
				this_thread::sleep_for(chrono::seconds(4));

				// stop the inter-node transfer
				rankToSend = -1;
				MPI_Send((void*)&rankToSend, 1, MPI_INT, rootRank, 0, MPI_COMM_WORLD);
				this_thread::sleep_for(chrono::seconds(2));
			}
			*/
		}

		fp.flush();
		this_thread::sleep_for(chrono::milliseconds(7));
	}

	MPI_Finalize();

	return 0;
}