#include <SDL.h>
#include <mpi.h>

#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#include <random>

#include "header.h"
#include "nodeMessageTags.h"

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

typedef chrono::high_resolution_clock timer;

int main(int argc, char* argv[])
{
	// initialize MPI
	int mpiRank, mpiNumNodes;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpiNumNodes);

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

	vector<pair<int, int>>	nodeEdges;
	vector<NodeState>		nodeStates(numNetworkNodes, defaultState);
	int						numAliveNodes = 0;

	SDL_Window* nodeTransferWin = NULL;
	int			nodeTransferWinID;
	SDL_Surface* nodeTransferWinSurface = NULL;
	SDL_Renderer* nodeTransferWinRenderer = NULL;
	SDL_Rect	nodeTransferWinRect;
	SDL_Window* dhtWin = NULL;
	int			dhtWinID;
	SDL_Surface* dhtWinSurface = NULL;
	SDL_Renderer* dhtWinRenderer = NULL;
	SDL_Rect	dhtWinRect;
	SDL_Texture* graphTexture = NULL;
	SDL_Rect	graphRect;

	bool success = sdl_init();
	
	if (success) {
		success = sdl_createWindow("Node Transfer", &nodeTransferWin, &nodeTransferWinRenderer, &nodeTransferWinSurface);
	}

	if(success) {
		success = sdl_createWindow("DHT", &dhtWin, &dhtWinRenderer, &dhtWinSurface);
	}

	if (!success) {
		printf("Failed to initialize SDL!\n");
	} else {
		SDL_Event	e;
		MPI_Status	mpiStatus;
		bool		graphStateChanged = true;

		nodeTransferWinRect.w = SCREEN_WIDTH;
		nodeTransferWinRect.h = SCREEN_HEIGHT;
		nodeTransferWinID = SDL_GetWindowID(nodeTransferWin);
		
		dhtWinRect.w = SCREEN_WIDTH;
		dhtWinRect.h = SCREEN_HEIGHT;
		dhtWinID = SDL_GetWindowID(dhtWin);

		FILE* fp = fopen("masterNode.txt", "w");
		fprintf(fp, "my rank: %d\n", mpiRank);

		NodeDHTArea			defaultDHTArea = { 0.0f, 0.0f, 0.0f, 0.0f};
		vector<NodeDHTArea> dhtAreas(numNetworkNodes, defaultDHTArea);

		// tell one of the nodes to come alive to start the
		// network
		{
			NodeAliveStates::NodeAliveState aliveState = NodeAliveStates::ONLY_NODE_IN_NETWORK;
			MPI_Send((void*)&aliveState, 1, MPI_INT, 0, NodeMessageTags::NODE_ALIVE, MPI_COMM_WORLD);
			nodeStates[0].isAlive = true;
			++numAliveNodes;
		}

		timer::time_point lastNodeAliveStateChange = timer::now();
		const chrono::milliseconds milliUntilNextNodeAliveStateChange = chrono::seconds(10);
		default_random_engine randGenerator(chrono::system_clock::now().time_since_epoch().count());
		uniform_int_distribution<int> randomNodeGenerator(0, mpiNumNodes - 1);

		// the main event loop
		while (!quit) {
			while (SDL_PollEvent(&e) != 0) {
				if (e.type == SDL_QUIT) {
					quit = true;
				} else if (e.type == SDL_WINDOWEVENT) {
					SDL_Window* window = NULL;
					SDL_Rect* windowRect = NULL;
					SDL_Renderer* windowRenderer = NULL;

					if (e.window.windowID == nodeTransferWinID) {
						window = nodeTransferWin;
						windowRect = &nodeTransferWinRect;
						windowRenderer = nodeTransferWinRenderer;
					} else if (e.window.windowID == dhtWinID) {
						window = dhtWin;
						windowRect = &dhtWinRect;
						windowRenderer = dhtWinRenderer;
					}

					if (e.window.type == SDL_WINDOWEVENT_SIZE_CHANGED) {
						SDL_GetWindowSize(window, &windowRect->w, &windowRect->h);
						SDL_RenderPresent(windowRenderer);
					} else if (e.window.type == SDL_WINDOWEVENT_EXPOSED) {
						SDL_RenderPresent(windowRenderer);
					} else if (e.window.type == SDL_WINDOWEVENT_CLOSE) {
						quit = true;
						break;
					}
				}
			}

			if (quit) {
				break;
			}

			// check if its time to tell a new node to flip its
			// alive state
			if (chrono::duration_cast<chrono::milliseconds>(timer::now() - lastNodeAliveStateChange) >= milliUntilNextNodeAliveStateChange) {
				int nodeToChange = randomNodeGenerator(randGenerator);

				while (nodeToChange == mpiRank) {
					nodeToChange = randomNodeGenerator(randGenerator);
				}

				// if the node is already alive then kill it,
				// otherwise bring it back to life again
				//nodeStates[nodeToChange].isAlive = !nodeStates[nodeToChange].isAlive;

				if(!nodeStates[nodeToChange].isAlive)
				{
					nodeStates[nodeToChange].isAlive = true;

					NodeAliveStates::NodeAliveState newAliveState;
					int								aliveNode = -1;

					// if we're killing a node, we have to remove
					// its DHT area

					//
					// NOTE: don't kill off any nodes for now
					//
					/*if (!nodeStates[nodeToChange].isAlive) {
					dhtAreas[nodeToChange] = defaultDHTArea;
					--numAliveNodes;
					newAliveState = NodeAliveStates::DEAD;
					} else {*/
					++numAliveNodes;

					if (numAliveNodes == 1) {
						newAliveState = NodeAliveStates::ONLY_NODE_IN_NETWORK;
					} else {
						newAliveState = NodeAliveStates::ALIVE;

						// find the first alive node
						for (aliveNode = 0; aliveNode < numNetworkNodes; ++aliveNode) {
							if (nodeStates[aliveNode].isAlive && aliveNode != nodeToChange) {
								break;
							}
						}
					}
					//}

					MPI_Send((void*)&newAliveState, 1, MPI_INT, nodeToChange, NodeMessageTags::NODE_ALIVE, MPI_COMM_WORLD);

					if (aliveNode != -1) {
						MPI_Send((void*)&aliveNode, 1, MPI_INT, nodeToChange, NodeMessageTags::NODE_ALIVE, MPI_COMM_WORLD);
					}
				}
			}

			// check if any new messages have been received

			// update the node message status
			int probeFlag;
			MPI_Iprobe(MPI_ANY_SOURCE, NodeMessageTags::NODE_TO_NODE_MSG, MPI_COMM_WORLD, &probeFlag, &mpiStatus);

			if (probeFlag) {
				int src, dest;
				MPI_Recv((void*)&dest, 1, MPI_INT, MPI_ANY_SOURCE, NodeMessageTags::NODE_TO_NODE_MSG, MPI_COMM_WORLD, &mpiStatus);
				src = mpiStatus.MPI_SOURCE;

				if(dest >= 0) {
					fprintf(fp, "Got message from %d to %d\n", src, dest);
					fflush(fp);

					nodeStates[src].receivingNode = dest;
					nodeStates[dest].sendingNode = src;
				} else {
					nodeStates[src].receivingNode = -1;
					nodeStates[src].sendingNode = -1;
				}
				
				graphStateChanged = true;
				probeFlag = 0;
			}

			// update the node DHT areas
			MPI_Iprobe(MPI_ANY_SOURCE, NodeMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD, &probeFlag, &mpiStatus);

			if (probeFlag) {
				NodeDHTArea newArea;
				MPI_Recv((void*)&newArea, 4, MPI_FLOAT, MPI_ANY_SOURCE, NodeMessageTags::DHT_AREA_UPDATE, MPI_COMM_WORLD, &mpiStatus);
				int src = mpiStatus.MPI_SOURCE;
				dhtAreas[src] = newArea;
			}

			// if the node states have changed
			// then rerender the graph
			if(graphStateChanged) {
				// generate the graph DOT file
				string graphDotStr = generateGraph(nodeStates, nodeEdges, &nodeTransferWinRect);
				SDL_Surface* graphSurface;

				// create the surface for the graph
				if (!renderGraph(graphDotStr, nodeTransferWinSurface, &graphSurface))
				{
					printf("Failed to render graph!\n");
					quit = true;
					break;
				}

				graphRect.w = graphSurface->w;
				graphRect.h = graphSurface->h;

				graphTexture = SDL_CreateTextureFromSurface(nodeTransferWinRenderer, graphSurface);
				SDL_FreeSurface(graphSurface);

				graphStateChanged = false;
			}

			// fill background with white
			SDL_SetRenderDrawColor(nodeTransferWinRenderer, 255, 255, 255, 255);
			SDL_RenderClear(nodeTransferWinRenderer);
			SDL_SetRenderDrawColor(dhtWinRenderer, 255, 255, 255, 255);
			SDL_RenderClear(dhtWinRenderer);

			// render graph png image
			// in the center of the screen
			SDL_Rect screenCenter;
			screenCenter.x = (nodeTransferWinRect.w - graphRect.w) / 2;
			screenCenter.y = (nodeTransferWinRect.h - graphRect.h) / 2;
			screenCenter.w = graphRect.w;
			screenCenter.h = graphRect.h;

			SDL_RenderCopy(nodeTransferWinRenderer, graphTexture, NULL, &screenCenter);
			SDL_RenderPresent(nodeTransferWinRenderer);

			RenderDHT(dhtAreas, dhtWinRenderer, &dhtWinRect);
			SDL_RenderPresent(dhtWinRenderer);
		}
	}
	
	sdl_close(&nodeTransferWin);
	MPI_Abort(MPI_COMM_WORLD, 0);
	MPI_Finalize();

	return 0;
}