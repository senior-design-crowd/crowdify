#include <graphviz\cgraph.h>
#include <graphviz\gvc.h>
#include <SDL.h>

#include <string>
#include <vector>
#include <sstream>

using namespace std;

#include "header.h"

string generateGraph(const vector<NodeState>& nodeStates, const vector<pair<int, int>>& nodeEdges, SDL_Rect* screenRect)
{
	string attributes[] = {
		" [fillcolor=white]",
		" [fillcolor=green]",
		" [fillcolor=red]",
		" [fillcolor=yellow]"
	};

	stringstream ss;

	std::string graphDot;
	ss << "digraph G {" << endl
		<< "\tdpi=100.0;" << endl
		<< "\tsize=\"" << screenRect->h / 100 << ',' << screenRect->w / 100 << "\";" << endl
		<< "\tratio=compress;" << endl;

	size_t nodeNum = 0;
	for (vector<NodeState>::const_iterator i = nodeStates.begin(); i != nodeStates.end(); ++i, ++nodeNum) {
		ss << '\t' << nodeNum << " [shape=none label=< <table border='0' cellspacing='0'>" << endl
			<< "\t\t\t<tr><td border='1' bgcolor='";
		
		if (i->sendingNode >= 0) {
			ss << "green'>" << i->sendingNode;
		} else {
			ss << "white'>T";
		}

		ss << "</td></tr>" << endl
			<< "\t\t\t<tr><td border='1' bgcolor='";
		
		if (i->receivingNode >= 0) {
			ss << "red'>" << i->receivingNode;
		} else {
			ss << "white'>R";
		}

		ss << "</td></tr>" << endl
			<< "\t\t</table>" << endl
			<< "\t\t>" << endl
			<< "\t];" << endl;
	}

	for (vector<pair<int, int>>::const_iterator i = nodeEdges.begin(); i != nodeEdges.end(); ++i) {
		ss << '\t' << i->first << " -> " << i->second << ';' << endl;
	}

	ss << '}';

	FILE* fp = fopen("output.txt", "w");
	fwrite(ss.str().c_str(), 1, ss.str().length() + 1, fp);
	fclose(fp);

	return ss.str();
}

bool renderGraph(const string& graphDotStr, SDL_Surface* screenSurface, SDL_Surface** r_pngSurface)
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
	*r_pngSurface = loadSurface(graphRenderingBuf, graphRenderingBufSize, screenSurface);
	if (*r_pngSurface == NULL)
	{
		printf("Failed to load graph PNG image!\n");
		success = false;
	}

	return success;
}