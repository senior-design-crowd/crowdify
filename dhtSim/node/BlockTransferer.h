#ifndef __BLOCK_TRANSFER_LISTENER_H__
#define __BLOCK_TRANSFER_LISTENER_H__

#include <queue>
#include <stdint.h>
#include <fstream>

typedef struct {
	struct InitialBlock {
		char		msgType;
		uint64_t	hashLen;
	} initial;

	char*		hash;
	uint64_t	blockLen;
	char*		block;
	uint32_t	addressLen;
	char*		address;
} BlockMsg;

namespace zmq {
	class socket_t;
	class context_t;
};

class BlockTransferer
{
public:
	BlockTransferer(std::ofstream* fp, int port = 50000);
	~BlockTransferer();

	void					Update();
	size_t					GetNumMsgsAvail();
	BlockMsg				GetMsg();

	bool SendBlockTransferMsg(const char* address, const BlockMsg& msg);

private:
	std::queue<BlockMsg>	m_msgQueue;
	zmq::socket_t*			m_servSocket;
	zmq::context_t*			m_context;
	std::ofstream&			m_fp;
	int						m_port;
};

#endif
