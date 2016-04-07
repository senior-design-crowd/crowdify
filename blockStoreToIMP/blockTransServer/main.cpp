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

void OutputBlockMsg(const BlockMsg& msg)
{
	cout << "Msg Type: ";

	if (msg.initial.msgType == 's') {
		cout << "Block Status";
	} else if (msg.initial.msgType == 'r') {
		cout << "Block Retrieve";
	} else if (msg.initial.msgType == 't') {
		cout << "Block Store";
	}

	cout << endl
		<< "Hash: " << endl
		<< msg.hash << endl
		<< endl
		<< "Block: " << endl
		<< msg.block << endl
		<< endl;
}

int main(int argc, char* argv[])
{
	zmq::context_t context(1);
	zmq::socket_t servSocket(context, ZMQ_REP);

	servSocket.bind("tcp://*:50000");

	while (true) {
		BlockMsg msg;
		memset((void*)&msg, 0, sizeof(BlockMsg));

		bool successFullRecv = true;

		zmq::message_t initialMsg;
		servSocket.recv(&initialMsg);
		
		if (initialMsg.size() == sizeof(BlockMsg::InitialBlock)) {
			memcpy((void*)&msg.initial, (void*)initialMsg.data(), sizeof(BlockMsg::InitialBlock));
			cout << "Received initial msg." << endl;
		} else {
			successFullRecv = false;
		}

		zmq::message_t hashMsg;
		servSocket.recv(&hashMsg);

		if (hashMsg.size() == msg.initial.hashLen) {
			msg.hash = (char*)malloc(msg.initial.hashLen + 1);
			memcpy(msg.hash, (void*)hashMsg.data(), msg.initial.hashLen);
			msg.hash[msg.initial.hashLen] = 0;
			cout << "Received hash." << endl;
		} else {
			successFullRecv = false;
		}

		zmq::message_t blockLenMsg;
		servSocket.recv(&blockLenMsg);

		if (blockLenMsg.size() == sizeof(uint64_t)) {
			memcpy((void*)&msg.blockLen, (void*)blockLenMsg.data(), sizeof(uint64_t));
			cout << "Received block length." << endl;
		} else {
			successFullRecv = false;
		}

		zmq::message_t blockMsg;
		servSocket.recv(&blockMsg);

		if (blockMsg.size() == msg.blockLen) {
			msg.block = (char*)malloc(msg.blockLen + 1);
			memcpy((void*)msg.block, (void*)blockMsg.data(), msg.blockLen);
			msg.block[msg.blockLen] = 0;
			cout << "Received block." << endl;
		} else {
			successFullRecv = false;
		}

		if(successFullRecv) {
			OutputBlockMsg(msg);
		} else {
			cout << "Failure to receive." << endl;
		}

		uint8_t response = successFullRecv ? 1 : 0;
		zmq::message_t responseMsg(sizeof(uint8_t));
		memcpy((void*)responseMsg.data(), (void*)&response, sizeof(uint8_t));

		servSocket.send(responseMsg);
	}

	return 0;
}