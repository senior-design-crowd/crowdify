#include <graphviz\cgraph.h>
#include <graphviz\gvc.h>
#include <SDL.h>

#include <string>
#include <vector>
#include <set>
#include <sstream>

using namespace std;

#include "header.h"
#include "SDLEngine.h"

string GenerateGraph(const vector<NodeState>& nodeStates, const vector<set<int>>& nodeEdges, SDLEngine* engine, int windowIndex)
{
	const Window& window = engine->GetWindow(windowIndex);

	string attributes[] = {
		" [fillcolor=white]",
		" [fillcolor=green]",
		" [fillcolor=red]",
		" [fillcolor=yellow]"
	};

	stringstream ss;

	std::string graphDot;
	ss << "digraph G {" << endl
		<< "\tdpi=60.0;" << endl
		<< "\tsize=\"" << window.height / 100 << ',' << window.width / 100 << "\";" << endl;
		//<< "\tratio=compress;" << endl;

	size_t nodeNum = 0;
	for (vector<NodeState>::const_iterator i = nodeStates.begin(); i != nodeStates.end(); ++i, ++nodeNum) {
		if (!i->isAlive) {
			continue;
		}

		ss << '\t' << nodeNum << " [shape=none label=< <table border='0' cellspacing='0'>" << endl
			<< "\t\t\t<tr><td border='1'><font point-size=\"24\">" << nodeNum << "</font></td></tr>" << endl
			<< "\t\t\t<tr><td border='1' bgcolor='";
		
		if (i->sendingNode >= 0) {
			ss << "green'>" << i->sendingNode;
		} else {
			ss << "white'>R";
		}

		ss << "</td></tr>" << endl
			<< "\t\t\t<tr><td border='1' bgcolor='";
		
		if (i->receivingNode >= 0) {
			if (i->routing) {
				ss << "yellow'>" << i->receivingNode;
			} else {
				ss << "red'>" << i->receivingNode;
			}
		} else {
			ss << "white'>T";
		}

		ss << "</td></tr>" << endl
			<< "\t\t</table>" << endl
			<< "\t\t>" << endl
			<< "\t];" << endl;
	}

	nodeNum = 0;
	for (vector<set<int>>::const_iterator i = nodeEdges.begin(); i != nodeEdges.end(); ++i, ++nodeNum) {
		for (set<int>::const_iterator j = i->begin(); j != i->end(); ++j) {
			ss << '\t' << nodeNum << " -> " << *j << " [shape=none];" << endl;
		}
	}

	ss << '}';

	FILE* fp = fopen("output.txt", "w");
	fwrite(ss.str().c_str(), 1, ss.str().length() + 1, fp);
	fclose(fp);

	return ss.str();
}

bool RenderGraph(const string& graphDotStr, SDLEngine* engine, int windowIndex, SDL_Texture** r_pngTexture, int& width, int& height)
{
	bool success = true;

	// parse DOT input to create graph
	Agraph_t* g = agmemread(graphDotStr.c_str());

	if (!g) {
		printf("Error generating graph image.\n");
		return false;
	}

	// render the graph as a PNG image
	GVC_t*	gvc = gvContext();
	char*	graphRenderingBuf;
	size_t	graphRenderingBufSize;

	gvLayout(gvc, g, "dot");
	gvRenderData(gvc, g, "png", &graphRenderingBuf, &graphRenderingBufSize);
	gvFreeLayout(gvc, g);
	agclose(g);

	if (gvFreeContext(gvc) != 0) {
		printf("Error generating graph image.\n");
		if (graphRenderingBuf) {
			gvFreeRenderData(graphRenderingBuf);
		}

		return false;
	}

	// load the PNG image into a surface
	*r_pngTexture = engine->LoadSDLImage(graphRenderingBuf, graphRenderingBufSize, windowIndex, width, height);
	if (*r_pngTexture == NULL)
	{
		printf("Failed to load graph PNG image!\n");
		success = false;
	}

	gvFreeRenderData(graphRenderingBuf);

	return success;
}