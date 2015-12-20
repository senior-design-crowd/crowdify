#ifndef __NODE_MSG_TAGS__
#define __NODE_MSG_TAGS__

namespace NodeMessageTags
{
	enum NodeMessageTag
	{
		NODE_ALIVE,
		DHT_AREA_UPDATE,
		NODE_TO_NODE_MSG
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

#endif