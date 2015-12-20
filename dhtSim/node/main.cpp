#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#include <chrono>
#include <thread>
#include <random>
#include <vector>

#include "header.h"
#include "nodeMessageTags.h"
#include "Node.h"

using namespace std;

int main(int argc, char* argv[])
{
	bool alive = false;
	int mpiRank, mpiSize;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);

	MPI_Status	mpiStatus;
	int			rootRank;
	MPI_Recv((void*)&rootRank, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &mpiStatus);

	char filename[128];
	sprintf(filename, "node%d.txt", mpiRank);
	
	ofstream fp(filename);

	fp << "root rank: " << rootRank << endl;

	Node node(mpiRank, &fp);

	while (true) {
		int newAliveStateAvailable;
		MPI_Iprobe(rootRank, NodeMessageTags::NODE_ALIVE, MPI_COMM_WORLD, &newAliveStateAvailable, &mpiStatus);

		if (!alive || newAliveStateAvailable) {
			// block until we receive a new alive state
			NodeAliveStates::NodeAliveState newAliveState;
			MPI_Recv((void*)&newAliveState, 1, MPI_INT, rootRank, NodeMessageTags::NODE_ALIVE, MPI_COMM_WORLD, &mpiStatus);
			
			fp << "Got new alive state: ";

			if (newAliveState == NodeAliveStates::ALIVE) {
				fp << "ALIVE" << endl;
			} else if (newAliveState == NodeAliveStates::DEAD) {
				fp << "DEAD" << endl;
			} else if (newAliveState == NodeAliveStates::ONLY_NODE_IN_NETWORK) {
				fp << "ONLY_NODE_IN_NETWORK" << endl;
			}

			fp.flush();

			// initialize our area in the DHT
			if (newAliveState != NodeAliveStates::DEAD) {
				fp << "Initializing node DHT area." << endl;
				fp.flush();

				if (newAliveState == NodeAliveStates::ONLY_NODE_IN_NETWORK) {
					node.initializeDHTArea(-1);
				} else if (newAliveState == NodeAliveStates::ALIVE) {
					int aliveNode = -1;
					MPI_Recv((void*)&aliveNode, 1, MPI_INT, rootRank, NodeMessageTags::NODE_ALIVE, MPI_COMM_WORLD, &mpiStatus);

					fp << "Alive node: " << aliveNode << endl;
					fp.flush();

					node.initializeDHTArea(aliveNode);
				}

				alive = true;

				// update the root with our new area
				NodeDHTArea dhtArea = node.getDHTArea();

				fp << "New DHT area:" << endl;
				fp << "\tLeft: " << dhtArea.left << endl;
				fp << "\tRight: " << dhtArea.right << endl;
				fp << "\tTop: " << dhtArea.top << endl;
				fp << "\tBottom: " << dhtArea.bottom << endl;
				fp.flush();

				MPI_Send((void*)&dhtArea, sizeof(NodeDHTArea) / sizeof(float), MPI_FLOAT, rootRank, NodeMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD);
			} else {
				alive = false;
				continue;
			}			
		}

		if (alive) {
			// queue for any new messages
			int newMsg = 0;
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &newMsg, &mpiStatus);

			if (newMsg == 1) {
				fp << "Got MPI message from " << mpiStatus.MPI_SOURCE << ": ";

				if (mpiStatus.MPI_TAG == InterNodeMessageTags::REQUEST_DHT_POINT_OWNER) {
					fp << "REQUEST_DHT_POINT_OWNER" << endl;
					fp.flush();

					InterNodeMessage::DHTPointOwnerRequest pointOwnerRequest;
					MPI_Recv((void*)&pointOwnerRequest, sizeof(InterNodeMessage::DHTPointOwnerRequest) / sizeof(int), MPI_INT, MPI_ANY_SOURCE, InterNodeMessageTags::REQUEST_DHT_POINT_OWNER, MPI_COMM_WORLD, &mpiStatus);

					fp << "\tPoint ownership request originally from " << pointOwnerRequest.origRequestingNode << ": (" << pointOwnerRequest.x << ", " << pointOwnerRequest.y << ")" << endl;

					InterNodeMessage::DHTPointOwnerResponse pointOwnerResponse;
					pointOwnerResponse.pointOwnerNode = node.getNearestNeighbor(pointOwnerRequest.x, pointOwnerRequest.y);

					fp << "\tNearest neighbor: ";

					// if the point is inside our DHT area then send
					// the response back to the original node that made the request.
					// otherwise forward the request on to our closest neighbor.
					if (pointOwnerResponse.pointOwnerNode == -1) {
						fp << "self" << endl;
						pointOwnerResponse.pointOwnerNode = mpiRank;
						MPI_Send((void*)&pointOwnerResponse, sizeof(InterNodeMessage::DHTPointOwnerResponse) / sizeof(int), MPI_INT, pointOwnerRequest.origRequestingNode, InterNodeMessageTags::RESPONSE_DHT_POINT_OWNER, MPI_COMM_WORLD);
					} else {
						fp << pointOwnerResponse.pointOwnerNode << endl;
						MPI_Send((void*)&pointOwnerRequest, sizeof(InterNodeMessage::DHTPointOwnerRequest) / sizeof(int), MPI_INT, pointOwnerResponse.pointOwnerNode, InterNodeMessageTags::REQUEST_DHT_POINT_OWNER, MPI_COMM_WORLD);
					}

					fp.flush();
				} else if (mpiStatus.MPI_TAG == InterNodeMessageTags::REQUEST_DHT_AREA) {
					fp << "REQUEST_DHT_AREA" << endl;
					fp.flush();

					int tmp;
					MPI_Recv((void*)&tmp, 1, MPI_INT, MPI_ANY_SOURCE, InterNodeMessageTags::REQUEST_DHT_AREA, MPI_COMM_WORLD, &mpiStatus);
					
					InterNodeMessage::DHTAreaResponse dhtAreaResponse;

					vector<NodeNeighbor>	vNewNodeNeighbors;
					node.splitDHTArea(mpiStatus.MPI_SOURCE, dhtAreaResponse.newDHTArea, vNewNodeNeighbors);

					dhtAreaResponse.numNeighbors = vNewNodeNeighbors.size() > 4 ? 4 : vNewNodeNeighbors.size();
					memcpy(dhtAreaResponse.neighbors, &vNewNodeNeighbors[0], sizeof(NodeNeighbor) * dhtAreaResponse.numNeighbors);

					fp << "Sending new DHT area to: " << mpiStatus.MPI_SOURCE << endl;

					MPI_Send((void*)&dhtAreaResponse,
						sizeof(InterNodeMessage::DHTAreaResponse) / sizeof(int),
						MPI_INT,
						mpiStatus.MPI_SOURCE,
						InterNodeMessageTags::RESPONSE_DHT_AREA,
						MPI_COMM_WORLD);

					NodeDHTArea dhtArea = node.getDHTArea();
					MPI_Send((void*)&dhtArea, sizeof(NodeDHTArea) / sizeof(float), MPI_FLOAT, rootRank, NodeMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD);
				} else {
					fp << "Unknown" << endl;
					fp.flush();
				}
			}

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
	}

	MPI_Finalize();

	return 0;
}