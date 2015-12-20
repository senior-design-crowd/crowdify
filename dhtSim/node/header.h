#ifndef __HEADER_H__
#define __HEADER_H__

typedef struct _NodeDHTArea {
	float left, right, top, bottom;
} NodeDHTArea;

typedef struct _NodeNeighbor {
	NodeDHTArea neighborArea;
	int			nodeNum;
} NodeNeighbor;

#endif