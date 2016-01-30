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

					MPI_Ssend((void*)&dhtArea, sizeof(NodeDHTArea) / sizeof(int), MPI_INT, rootRank, NodeToRootMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD);
				} else {
					alive = false;
					continue;
				}
			}			
		}

		if (alive) {
			// queue for any new messages
			int newMsg = 0;

			MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			while (newMsg == 1) {
				fp << node.LogOutputHeader() << "Got MPI message from " << mpiStatus.MPI_SOURCE << ": NODE_TAKEOVER_REQUEST" << endl;
				fp.flush();

				InterNodeMessage::NodeTakeoverRequest nodeTakeoverRequest;
				MPI_Recv((void*)&nodeTakeoverRequest, sizeof(InterNodeMessage::NodeTakeoverRequest) / sizeof(int), MPI_INT, MPI_ANY_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD, &mpiStatus);

				node.ResolveNodeTakeoverRequest(mpiStatus.MPI_SOURCE, &nodeTakeoverRequest);
			}

			newMsg = 0;
			MPI_Iprobe(MPI_ANY_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &newMsg, &mpiStatus);
			while (newMsg == 1) {
				fp << node.LogOutputHeader() << "Got MPI message from " << mpiStatus.MPI_SOURCE << ": NEIGHBOR_UPDATE" << endl;
				fp.flush();

				fp << "\tGetting neighbor area." << endl;
				fp.flush();
				NodeDHTArea neighborDHTArea;
				MPI_Recv((void*)&neighborDHTArea, sizeof(NodeDHTArea) / sizeof(int), MPI_INT, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &mpiStatus);

				fp << "\tGetting # neighbors." << endl;
				fp.flush();
				int numNeighborsOfNeighbor;
				MPI_Recv((void*)&numNeighborsOfNeighbor, 1, MPI_INT, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &mpiStatus);

				fp << "\tGetting neighbors of neighbor." << endl;
				fp.flush();
				vector<NodeNeighbor> neighborsOfNeighbor;
				neighborsOfNeighbor.resize(numNeighborsOfNeighbor);
				MPI_Recv((void*)&neighborsOfNeighbor[0], numNeighborsOfNeighbor * sizeof(NodeNeighbor) / sizeof(int), MPI_INT, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &mpiStatus);

				fp << "\tUpdating neighbor info." << endl;
				fp.flush();
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
					sizeof(InterNodeMessage::DHTAreaResponse) / sizeof(int),
					MPI_INT,
					mpiStatus.MPI_SOURCE,
					InterNodeMessageTags::RESPONSE_DHT_AREA,
					MPI_COMM_WORLD);

				MPI_Send((void*)&vNewNodeNeighbors[0],
					vNewNodeNeighbors.size() * sizeof(NodeNeighbor) / sizeof(int),
					MPI_INT,
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
							numNeighborsOfNeighbor * sizeof(NodeNeighbor) / sizeof(int),
							MPI_INT,
							mpiStatus.MPI_SOURCE,
							InterNodeMessageTags::RESPONSE_DHT_AREA,
							MPI_COMM_WORLD);
					}
				}

				NotifyRootOfMsg(NodeToNodeMsgTypes::SENDING, mpiStatus.MPI_SOURCE, rootRank);

				NodeDHTArea dhtArea = node.GetDHTArea();

				fp << endl << "\tUpdated root with new DHT area." << endl;

				MPI_Ssend((void*)&dhtArea, sizeof(NodeDHTArea) / sizeof(int), MPI_INT, rootRank, NodeToRootMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD);

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
				MPI_Recv((void*)&pointOwnerRequest, sizeof(InterNodeMessage::DHTPointOwnerRequest) / sizeof(int), MPI_INT, MPI_ANY_SOURCE, InterNodeMessageTags::REQUEST_DHT_POINT_OWNER, MPI_COMM_WORLD, &mpiStatus);

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
					MPI_Send((void*)&pointOwnerResponse, sizeof(InterNodeMessage::DHTPointOwnerResponse) / sizeof(int), MPI_INT, pointOwnerRequest.origRequestingNode, InterNodeMessageTags::RESPONSE_DHT_POINT_OWNER, MPI_COMM_WORLD);

					NotifyRootOfMsg(NodeToNodeMsgTypes::SENDING, pointOwnerRequest.origRequestingNode, rootRank);
				} else {
					fp << pointOwnerResponse.pointOwnerNode << endl;
					MPI_Send((void*)&pointOwnerRequest, sizeof(InterNodeMessage::DHTPointOwnerRequest) / sizeof(int), MPI_INT, pointOwnerResponse.pointOwnerNode, InterNodeMessageTags::REQUEST_DHT_POINT_OWNER, MPI_COMM_WORLD);

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