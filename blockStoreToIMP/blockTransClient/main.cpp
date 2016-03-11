#include <zmq.h>
#include <zmq.hpp>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

#include <stdint.h>

using namespace std;

typedef struct {
	struct InitialBlock {
		char		msgType;
		uint64_t	hashLen;
	} initial;

	char*		hash;
	uint64_t	blockLen;
	char*		block;
} BlockMsg;

BlockMsg GetUserMsg()
{
	BlockMsg msg;
	memset((void*)&msg, 0, sizeof(BlockMsg));

	cout << "Choose message type:" << endl
		<< "\ts - Block status" << endl
		<< "\tr - Block retrieve" << endl
		<< "\tt - Block store" << endl;

	while ('r' > msg.initial.msgType || 't' < msg.initial.msgType) {
		cout << "Enter message type: ";
		cin >> msg.initial.msgType;
	}

	string block, hash;
	
	cout << "Enter block: ";
	cin >> block;

	cout << "Enter hash: ";
	cin >> hash;

	msg.hash = _strdup(hash.c_str());
	msg.initial.hashLen = hash.length();

	msg.block = _strdup(block.c_str());
	msg.blockLen = block.length();

	return msg;
}

int main(int argc, char* argv[])
{
	zmq::context_t context(1);
	zmq::socket_t clientSocket(context, ZMQ_REQ);

	string addr = "tcp://localhost:50000";

	if (argc > 1) {
		addr = "tcp://";
		addr += argv[1];
		addr += ":50000";
	}

	cout << "Connecting to " << addr << endl;
	clientSocket.connect(addr.c_str());

	while (true) {
		BlockMsg block = GetUserMsg();
		zmq::message_t initialMsg(sizeof(BlockMsg::InitialBlock));
		memcpy(initialMsg.data(), (void*)&block.initial, sizeof(BlockMsg::InitialBlock));

		clientSocket.send(initialMsg, ZMQ_SNDMORE);
		cout << "Sent initial msg." << endl;

		zmq::message_t hashMsg(block.initial.hashLen);
		memcpy(hashMsg.data(), (void*)block.hash, block.initial.hashLen);

		clientSocket.send(hashMsg, ZMQ_SNDMORE);
		cout << "Sent hash." << endl;

		zmq::message_t blockLenMsg(sizeof(uint64_t));
		memcpy(blockLenMsg.data(), (void*)&block.blockLen, sizeof(uint64_t));

		clientSocket.send(blockLenMsg, ZMQ_SNDMORE);
		cout << "Sent block length." << endl;

		zmq::message_t blockMsg(block.blockLen);
		memcpy(blockMsg.data(), (void*)block.block, block.blockLen);

		clientSocket.send(blockMsg);
		cout << "Sent block." << endl;

		uint8_t serverOk = 0;
		zmq::message_t serverMsg;
		clientSocket.recv(&serverMsg);

		if (serverMsg.size() == sizeof(uint8_t)) {
			memcpy((void*)&serverOk, (void*)serverMsg.data(), sizeof(uint8_t));
		}

		cout << "Server response: ";
		if (serverOk == 1) {
			cout << "OK";
		} else {
			cout << "FAIL";
		}
		cout << endl;
	}

	return 0;
}