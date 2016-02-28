#include <SDL.h>
#include <mpi.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdio.h>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>

#include "header.h"
#include "SDLEngine.h"
#include "nodeMessageTags.h"

using namespace std;

typedef chrono::high_resolution_clock timer;

int main(int argc, char* argv[])
{
	// initialize MPI
	int mpiRank, mpiNumNodes;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpiNumNodes);

	if (argc > 1 && strcmp(argv[1], "-d") == 0) {
		while (!IsDebuggerPresent()) {
			this_thread::sleep_for(chrono::milliseconds(200));
		}
	}

	// send all the other nodes my rank
	for (int i = 0; i < mpiNumNodes; ++i) {
		if (i == mpiRank) {
			continue;
		}

		MPI_Send((void*)&mpiRank, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
	}

	// this node doesn't count since its the renderer
	int numNetworkNodes = mpiNumNodes - 1;

	bool quit = false;

	NodeState defaultState;
	defaultState.receivingNode = -1;
	defaultState.sendingNode = -1;
	defaultState.routing = false;
	defaultState.isAlive = false;

	vector<set<int>>		nodeEdges(numNetworkNodes);
	vector<NodeState>		nodeStates(numNetworkNodes, defaultState);
	int						numAliveNodes = 0;

	SDLEngine		engine;

	SDL_Texture*	graphTexture = NULL;
	SDL_Rect		graphRect;

	int				graphFrame = 0, dhtFrame = 0;
	bool			dhtChanged = true;

	vector<FontTexture>	nodeLabels;

	bool success = engine.CreateSDLWindow("Node Transfer");

	if(success) {
		success = engine.CreateSDLWindow("DHT");
	}

	if (!success) {
		printf("Failed to initialize SDL!\n");
	} else {
		vector<string>	nodeLabelStrings;
		stringstream	nodeLabelSS;
		SDL_Color		blackFontColor = { 0, 0, 0, 255 };

		for (int i = 0; i < numNetworkNodes; ++i) {
			nodeLabelSS.str("");
			nodeLabelSS << i;
			nodeLabelStrings.push_back(nodeLabelSS.str());
		}

		engine.CreateFontTextures("arial.ttf", 36, &blackFontColor, 1, nodeLabelStrings, nodeLabels);

		MPI_Status	mpiStatus;
		bool		graphStateChanged = true;

		ofstream fp("masterNode.txt");
		fp << "my rank: " << mpiRank << endl;

		NodeDHTArea			defaultDHTArea = { 0.0f, 0.0f, 0.0f, 0.0f};
		vector<NodeDHTArea> dhtAreas(numNetworkNodes, defaultDHTArea);

		// tell one of the nodes to come alive to start the
		// network
		{
			NodeAliveStates::NodeAliveState aliveState = NodeAliveStates::ONLY_NODE_IN_NETWORK;
			MPI_Send((void*)&aliveState, 1, MPI_INT, 0, NodeToRootMessageTags::NODE_ALIVE, MPI_COMM_WORLD);
			nodeStates[0].isAlive = true;
			++numAliveNodes;

			fp << "Sending ALIVE msg to node 0" << endl
				<< "\tNode is only alive node in network" << endl;
		}

		vector<timer::time_point> lastNodeMsgStateChanges(numNetworkNodes);
		const chrono::milliseconds milliTimeoutUntilNodeIdleState = chrono::milliseconds(300);

		timer::time_point lastNodeAliveStateChange = timer::now();
		const chrono::milliseconds milliUntilNextNodeBroughtAlive = chrono::seconds(1);
		//const chrono::milliseconds milliUntilNextNodeAliveStateChange = chrono::seconds(120);
		const chrono::milliseconds milliUntilNextNodeAliveStateChange = chrono::seconds(3);

		int nodeChangingStage = 0;

		default_random_engine randGenerator((unsigned int)chrono::system_clock::now().time_since_epoch().count());
		uniform_int_distribution<int> randomNodeGenerator(0, numNetworkNodes - 1);

		// the main event loop
		while (!quit) {
			// if the program is quitting
			if (engine.ProcessEvents()) {
				break;
			}

			// check if its time to tell a new node to flip its
			// alive state
			bool timeToSwitchState = false;
			if (nodeChangingStage == 0) {
				if (chrono::duration_cast<chrono::milliseconds>(timer::now() - lastNodeAliveStateChange) >= milliUntilNextNodeBroughtAlive) {
					timeToSwitchState = true;
				}
			} else if (nodeChangingStage == 1) {
				if (chrono::duration_cast<chrono::milliseconds>(timer::now() - lastNodeAliveStateChange) >= milliUntilNextNodeAliveStateChange) {
					timeToSwitchState = true;
				}
			}

			if (timeToSwitchState) {
				int nodeToChange = randomNodeGenerator(randGenerator);

				if (nodeChangingStage == 0) {
					while (nodeToChange == mpiRank || nodeStates[nodeToChange].isAlive) {
						nodeToChange = randomNodeGenerator(randGenerator);
					}

					nodeStates[nodeToChange].isAlive = true;
				} else if (nodeChangingStage == 1) {
					nodeToChange = 0;
					while (nodeToChange == mpiRank) {
						nodeToChange = randomNodeGenerator(randGenerator);
					}

					// flip its alive state
					nodeStates[nodeToChange].isAlive = !nodeStates[nodeToChange].isAlive;
				}

				fp << "Changing node " << nodeToChange << "'s alive state." << endl;

				graphStateChanged = true;

				NodeAliveStates::NodeAliveState newAliveState;
				int								aliveNode = -1;

				if (nodeStates[nodeToChange].isAlive) {
					fp << "Sending ALIVE msg to node " << nodeToChange << endl;
					++numAliveNodes;

					fp << numAliveNodes << " out of " << numNetworkNodes << " alive." << endl;

					if (numAliveNodes == numNetworkNodes) {
						fp << "Switching to node changing stage 1, where alive states are continually flipped." << endl;
						nodeChangingStage = 1;
					}

					if (numAliveNodes == 1) {
						newAliveState = NodeAliveStates::ONLY_NODE_IN_NETWORK;
						fp << "\tNode is only alive node in network";
					} else {
						newAliveState = NodeAliveStates::ALIVE;
					}
				} else {
					fp << "Sending KILL msg to node " << nodeToChange << endl;
					newAliveState = NodeAliveStates::DEAD;
					--numAliveNodes;

					dhtAreas[nodeToChange].left = 0.0f;
					dhtAreas[nodeToChange].right = 0.0f;
					dhtAreas[nodeToChange].top = 0.0f;
					dhtAreas[nodeToChange].bottom = 0.0f;

					nodeStates[nodeToChange].isAlive = false;
					graphStateChanged = true;

					if (numAliveNodes < 2) {
						fp << "Switching to node changing stage 0, where nodes are all brought to life." << endl;
						nodeChangingStage = 0;
					}


					// TEMPORARY FIX TO ONLY ALLOW ONE NODE TO BE KILLED AT A TIME
					nodeChangingStage = 2;
				}

				if (newAliveState == NodeAliveStates::ALIVE) {
					// find the first alive node
					for (aliveNode = 0; aliveNode < numNetworkNodes; ++aliveNode) {
						if (nodeStates[aliveNode].isAlive && aliveNode != nodeToChange) {
							break;
						}
					}

					fp << "\tSending it alive node: " << aliveNode << endl;
				}

				fp.flush();

				MPI_Send((void*)&newAliveState, 1, MPI_INT, nodeToChange, NodeToRootMessageTags::NODE_ALIVE, MPI_COMM_WORLD);

				if (newAliveState == NodeAliveStates::ALIVE) {
					MPI_Send((void*)&aliveNode, 1, MPI_INT, nodeToChange, NodeToRootMessageTags::NODE_ALIVE, MPI_COMM_WORLD);
				}

				lastNodeAliveStateChange = timer::now();
			}

			// check if any new messages have been received

			int probeFlag;

			// update the node DHT areas
			//while (probeFlag) {
			for (int nodeNum = 0; nodeNum < numNetworkNodes; ++nodeNum) {
				probeFlag = 0;
				MPI_Iprobe(nodeNum, NodeToRootMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD, &probeFlag, &mpiStatus);

				if(probeFlag) {
					NodeDHTArea newArea;
					MPI_Recv((void*)&newArea, sizeof(NodeDHTArea) / sizeof(int), MPI_INT, MPI_ANY_SOURCE, NodeToRootMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD, &mpiStatus);

					fp << "Got DHT_AREA_UPDATE from " << mpiStatus.MPI_SOURCE << ":" << endl
						<< "\tLeft: " << newArea.left << endl
						<< "\tRight: " << newArea.right << endl
						<< "\tTop: " << newArea.top << endl
						<< "\tBottom: " << newArea.bottom << endl;

					int src = mpiStatus.MPI_SOURCE;
					dhtAreas[src] = newArea;
					dhtChanged = true;
				}
			}

			// update the node message status
			MPI_Iprobe(MPI_ANY_SOURCE, NodeToRootMessageTags::NODE_TO_NODE_MSG, MPI_COMM_WORLD, &probeFlag, &mpiStatus);

			//while (probeFlag) {
			if (probeFlag) {
				NodeToRootMessage::NodeToNodeMsg msg;
				MPI_Recv((void*)&msg, sizeof(NodeToRootMessage::NodeToNodeMsg) / sizeof(int), MPI_INT, MPI_ANY_SOURCE, NodeToRootMessageTags::NODE_TO_NODE_MSG, MPI_COMM_WORLD, &mpiStatus);

				if (msg.msgType == NodeToNodeMsgTypes::SENDING) {
					fp << "Got message from " << mpiStatus.MPI_SOURCE << " to " << msg.otherNode << endl;
					fp.flush();

					nodeStates[mpiStatus.MPI_SOURCE].receivingNode = msg.otherNode;
				} else if (msg.msgType == NodeToNodeMsgTypes::RECEIVING) {
					fp << "Got message from " << msg.otherNode << " to " << mpiStatus.MPI_SOURCE << endl;
					fp.flush();

					nodeStates[mpiStatus.MPI_SOURCE].sendingNode = msg.otherNode;
				} else if (msg.msgType == NodeToNodeMsgTypes::ROUTING) {
					fp << "Routing message from " << mpiStatus.MPI_SOURCE << " to " << msg.otherNode << endl;
					fp.flush();

					nodeStates[mpiStatus.MPI_SOURCE].receivingNode = msg.otherNode;
					nodeStates[mpiStatus.MPI_SOURCE].routing = true;
				} else if (msg.msgType == NodeToNodeMsgTypes::IDLE) {
					nodeStates[mpiStatus.MPI_SOURCE].routing = false;
					nodeStates[mpiStatus.MPI_SOURCE].sendingNode = -1;
					nodeStates[mpiStatus.MPI_SOURCE].receivingNode = -1;
				}

				lastNodeMsgStateChanges[mpiStatus.MPI_SOURCE] = timer::now();
				
				graphStateChanged = true;
				probeFlag = 0;

				//MPI_Iprobe(MPI_ANY_SOURCE, NodeToRootMessageTags::NODE_TO_NODE_MSG, MPI_COMM_WORLD, &probeFlag, &mpiStatus);
			}

			// check if any nodes have gone idle
			{
				vector<NodeState>::iterator state = nodeStates.begin();
				for (vector<timer::time_point>::iterator i = lastNodeMsgStateChanges.begin(); i != lastNodeMsgStateChanges.end(); ++i, ++state) {
					bool nodeIsIdle = state->receivingNode == -1 && state->sendingNode == -1;
					if (chrono::duration_cast<chrono::milliseconds>(timer::now() - *i) >= milliTimeoutUntilNodeIdleState && state->isAlive && !nodeIsIdle) {
						state->receivingNode = -1;
						state->sendingNode = -1;
						*i = timer::now();
						graphStateChanged = true;
					}
				}
			}

			MPI_Iprobe(MPI_ANY_SOURCE, NodeToRootMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &probeFlag, &mpiStatus);

			if (probeFlag) {
				NodeToRootMessage::NodeNeighborUpdate neighborUpdate;
				MPI_Recv((void*)&neighborUpdate, sizeof(NodeToRootMessage::NodeNeighborUpdate)/sizeof(int), MPI_INT, MPI_ANY_SOURCE, NodeToRootMessageTags::NEIGHBOR_UPDATE, MPI_COMM_WORLD, &mpiStatus);
				
				// update node edges
				nodeEdges[mpiStatus.MPI_SOURCE].clear();
				for (int i = 0; i < neighborUpdate.numNeighbors; ++i) {
					// don't insert edge if it will create a cycle, i.e. if the node we're pointing to
					// already has an edge that points to us
					if (nodeEdges[neighborUpdate.neighbors[i]].find(mpiStatus.MPI_SOURCE) == nodeEdges[neighborUpdate.neighbors[i]].end()) {
						nodeEdges[mpiStatus.MPI_SOURCE].insert(neighborUpdate.neighbors[i]);
					}
				}

				graphStateChanged = true;
			}

			// if the node states have changed
			// then rerender the graph
			if(graphStateChanged) {
				// generate the graph DOT file
				string graphDotStr = GenerateGraph(nodeStates, nodeEdges, &engine, 0);

				if (graphTexture) {
					SDL_DestroyTexture(graphTexture);
				}

				// create the surface for the graph
				if (!RenderGraph(graphDotStr, &engine, 0, &graphTexture, graphRect.w, graphRect.h))
				{
					printf("Failed to render graph!\n");
					quit = true;
					break;
				}

				graphStateChanged = false;
			}

			// fill background with white
			engine.ClearWindow(0);
			engine.ClearWindow(1);

			// render graph png image
			// in the center of the screen
			const Window&	nodeTransferWin = engine.GetWindow(0);
			SDL_Rect		screenCenter, srcRect;

			screenCenter.x = (nodeTransferWin.width - graphRect.w) / 2;
			screenCenter.y = (nodeTransferWin.height - graphRect.h) / 2;
			screenCenter.w = graphRect.w;
			screenCenter.h = graphRect.h;

			srcRect.x = 0;
			srcRect.y = 0;
			srcRect.w = graphRect.w;
			srcRect.h = graphRect.h;

			engine.RenderTexture(0, graphTexture, &screenCenter, &srcRect);
			engine.PresentWindow(0);

			RenderDHT(dhtAreas, nodeLabels, &engine, 1);
			engine.PresentWindow(1);

			++graphFrame;
			
			if (dhtChanged) {
				++dhtFrame;

				stringstream ss;
				ss << "Images\\dht_frame" << dhtFrame << ".bmp";

				engine.TakeScreenshot(1, ss.str().c_str());
				dhtChanged = false;
			}

			this_thread::sleep_for(chrono::milliseconds(20));
		}
	}

	MPI_Abort(MPI_COMM_WORLD, 0);
	MPI_Finalize();

	return 0;
}