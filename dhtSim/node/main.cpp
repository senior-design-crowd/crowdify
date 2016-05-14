#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#include <chrono>
#include <thread>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "PlatformDependent.h"
#include "nodeMessages.h"
#include "Node.h"

using namespace std;

void NotifyRootOfMsg(NodeToNodeMsgTypes::NodeToNodeMsgType msgType, int otherNode, int rootRank);

int mpiRank, mpiSize, rootRank;

void CreateDummyStorageDaemon(int mpiRank, ofstream& fp, bool debug = false)
{
	static bool hasRun = false;

	if (hasRun) {
		return;
	}

#ifdef _WIN32
	const char* procPath = "\"..\\Debug\\DummyStorageDaemon.exe\"";
#else
	const char* procPath = "\".DummyStorageDaemon\"";
#endif

	stringstream ss;
	ss << procPath << " " << mpiRank;

	if (debug) {
		ss << " -d";
	}

	string cmdLine = ss.str();

	fp << "\tCmdline: " << cmdLine << endl;
	fp.flush();

	RunProc(procPath, cmdLine.c_str(), fp);

	fp.flush();

	hasRun = true;
}

int main(int argc, char* argv[])
{
	bool alive = false, debug = false;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);

	if (argc > 1 && strcmp(argv[1], "-d") == 0) {
		while (!BeingDebugged()) {
			this_thread::sleep_for(chrono::milliseconds(200));
		}

		debug = true;
	}

	InitMPITypes();

	MPI_Status	mpiStatus;
	MPI_Recv((void*)&rootRank, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &mpiStatus);

	char filename[128];
	sprintf(filename, "node%d.txt", mpiRank);
	
	ofstream fp(filename);
	
	SetExceptionHooks(rootRank, &fp);

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
					DHTArea dhtArea = node.GetDHTArea();

					fp << node.LogOutputHeader() << "New DHT area:" << endl
						<< dhtArea
						<< endl << "\tUpdated root (" << rootRank << ")  with new DHT area." << endl;
					fp.flush();

					dhtArea.SendOverMPI(rootRank, NodeToRootMessageTags::DHT_AREA_UPDATE, fp);

					fp << node.LogOutputHeader() << "Launching storage daemon" << endl;
					CreateDummyStorageDaemon(mpiRank, fp, debug);
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
				MPI_Recv((void*)&takeoverNotification.nodeNum, 1, MPI_INT, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_NOTIFY, MPI_COMM_WORLD, &mpiStatus);
				takeoverNotification.myDHTArea.RecvOverMPI(mpiStatus.MPI_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_NOTIFY);

				fp << node.LogOutputHeader() << "\tNode takeover notification:" << endl
					<< "\t\tNode num: " << takeoverNotification.nodeNum << endl
					<< "\t\tDHT Area: " << endl
					<< takeoverNotification.myDHTArea << endl;
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
				MPI_Recv((void*)&nodeTakeoverRequest.nodeNum, 1, MPI_INT, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_REQUEST, MPI_COMM_WORLD, &mpiStatus);
				nodeTakeoverRequest.myDHTArea.RecvOverMPI(mpiStatus.MPI_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_REQUEST);

				fp << node.LogOutputHeader() << "\tNode takeover request:" << endl
					<< "\t\tNode num: " << nodeTakeoverRequest.nodeNum << endl
					<< "\t\tDHT Area: " << endl
					<< nodeTakeoverRequest.myDHTArea;
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
				MPI_Recv((void*)&nodeTakeoverResponse.nodeNum, 1, MPI_INT, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD, &mpiStatus);
				MPI_Recv((void*)&nodeTakeoverResponse.response, 1, MPI_INT, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE, MPI_COMM_WORLD, &mpiStatus);
				nodeTakeoverResponse.myDHTArea.RecvOverMPI(mpiStatus.MPI_SOURCE, InterNodeMessageTags::NODE_TAKEOVER_RESPONSE);

				fp << node.LogOutputHeader() << "\tNode takeover response:" << endl
					<< "\t\tNode num: " << nodeTakeoverResponse.nodeNum << endl
					<< "\t\tDHT Area: " << endl
					<< nodeTakeoverResponse.myDHTArea << endl;

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

				fp << "\tGetting neighbor area." << endl;
				fp.flush();
				DHTArea neighborDHTArea;
				if (!neighborDHTArea.RecvOverMPI(mpiStatus.MPI_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE)) {
					fp << node.LogOutputHeader() << "\tFailed to receive DHT area." << endl;
				}

				fp << "\tGetting neighbors of neighbor." << endl;
				fp.flush();
				vector<NodeNeighbor> neighborsOfNeighbor;
				if (!Node::RecvNeighborsOverMPI(neighborsOfNeighbor, mpiStatus.MPI_SOURCE, InterNodeMessageTags::NEIGHBOR_UPDATE)) {
					fp << node.LogOutputHeader() << "\tFailed to receive neighbors of neighbor." << endl;
				} else {
					fp << node.LogOutputHeader() << "\tGot " << neighborsOfNeighbor.size() << " neighbors." << endl;
				}

				fp << "\tUpdating neighbor info." << endl;
				fp.flush();
				node.UpdateNeighborDHTArea(mpiStatus.MPI_SOURCE, neighborDHTArea, neighborsOfNeighbor);

				NotifyRootOfMsg(NodeToNodeMsgTypes::RECEIVING, mpiStatus.MPI_SOURCE, rootRank);

				fp << node.LogOutputHeader() << "\tNew neighbor DHT area:" << endl
					<< neighborDHTArea << endl;

				for (vector<NodeNeighbor>::const_iterator i = neighborsOfNeighbor.begin(); i != neighborsOfNeighbor.end(); ++i) {
					fp << "\tNew neighbor neighbor:" << endl
						<< "\t\tNode num: " << i->nodeNum << endl
						<< i->neighborArea << endl;
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

				DHTArea							newDHTArea;
				vector<NodeNeighbor>			vNewNodeNeighbors;
				vector<vector<NodeNeighbor>>	vNewNodeNeighborsOfNeighbors;
				node.SplitDHTArea(mpiStatus.MPI_SOURCE, newDHTArea, vNewNodeNeighbors, vNewNodeNeighborsOfNeighbors);

				dhtAreaResponse.nextAxisToSplit = node.GetNextSplitAxis();
				dhtAreaResponse.numNeighbors = vNewNodeNeighbors.size();

				fp << node.LogOutputHeader() << "Sending new DHT area to: " << mpiStatus.MPI_SOURCE << endl;

				MPI_Send((void*)&dhtAreaResponse,
					1,
					InterNodeMessage::MPI_DHTAreaResponse,
					mpiStatus.MPI_SOURCE,
					InterNodeMessageTags::RESPONSE_DHT_AREA,
					MPI_COMM_WORLD);

				newDHTArea.SendOverMPI(mpiStatus.MPI_SOURCE, InterNodeMessageTags::RESPONSE_DHT_AREA, fp);

				Node::SendNeighborsOverMPI(vNewNodeNeighbors, mpiStatus.MPI_SOURCE, InterNodeMessageTags::RESPONSE_DHT_AREA, fp);

				// for each of the new node's neighbors, send:
				//	o the number of neighbors that neighbor itself has
				//	o the DHT areas of all those neighbors
				for (auto it = vNewNodeNeighborsOfNeighbors.begin(); it != vNewNodeNeighborsOfNeighbors.end(); ++it) {
					int numNeighborsOfNeighbor = it->size();

					MPI_Send((void*)&numNeighborsOfNeighbor, 1, MPI_INT, mpiStatus.MPI_SOURCE, InterNodeMessageTags::RESPONSE_DHT_AREA, MPI_COMM_WORLD);

					if(numNeighborsOfNeighbor > 0) {
						Node::SendNeighborsOverMPI(*it, mpiStatus.MPI_SOURCE, InterNodeMessageTags::RESPONSE_DHT_AREA, fp);
					}
				}

				NotifyRootOfMsg(NodeToNodeMsgTypes::SENDING, mpiStatus.MPI_SOURCE, rootRank);

				DHTArea dhtArea = node.GetDHTArea();

				fp << endl << "\tUpdated root (" << rootRank << ") with new DHT area." << endl;
				dhtArea.SendOverMPI(rootRank, NodeToRootMessageTags::DHT_AREA_UPDATE, fp);

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
			node.Update();

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