#ifndef __NODE_MSG_TAGS__
#define __NODE_MSG_TAGS__

#include "header.h"

namespace NodeMessageTags
{
	enum NodeMessageTag
	{
		NODE_ALIVE = 0,
		DHT_AREA_UPDATE,
		NODE_TO_NODE_MSG,
		NUM_NODE_MSG_TAGS
	};
};

namespace NodeAliveStates
{
	enum NodeAliveState
	{
		ALIVE,
		DEAD,
		ONLY_NODE_IN_NETWORK
	};
};

namespace InterNodeMessageTags
{
	enum InterNodeMessageTag
	{
		REQUEST_DHT_POINT_OWNER = NodeMessageTags::NUM_NODE_MSG_TAGS,
		RESPONSE_DHT_POINT_OWNER,
		REQUEST_DHT_AREA,
		RESPONSE_DHT_AREA
	};
};

namespace InterNodeMessage
{
	typedef struct
	{
		float	x, y;
		int		origRequestingNode;
	} DHTPointOwnerRequest;

	typedef struct
	{
		int pointOwnerNode;
	} DHTPointOwnerResponse;

	typedef struct
	{
		NodeDHTArea		newDHTArea;
		NodeNeighbor	neighbors[4];
		int				numNeighbors;
	} DHTAreaResponse;
};

#endif