#ifndef __HEADER_H__
#define __HEADER_H__

typedef struct _NodeDHTArea {
	float left, right, top, bottom;
} NodeDHTArea;

typedef struct {
	NodeDHTArea neighborArea;
	int			nodeNum;
} NodeNeighbor;

bool operator==(const NodeDHTArea& a, const NodeDHTArea& b);
bool operator==(const NodeNeighbor& a, const NodeNeighbor& b);

#endif