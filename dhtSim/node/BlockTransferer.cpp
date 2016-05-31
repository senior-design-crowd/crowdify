#include <zmq.h>
#include <zmq.hpp>
#include <sstream>
#include <fstream>

using namespace std;

#include "BlockTransferer.h"

BlockTransferer::BlockTransferer(ofstream* fp, int port)
	: m_fp(*fp), m_port(port)
{
	m_context = new zmq::context_t(2);
	m_servSocket = new zmq::socket_t(*m_context, ZMQ_REP);

	std::stringstream ss;
	ss << "tcp://*:" << port;

	m_servSocket->bind(ss.str());
	m_fp << "BlockTransferer - Bound to " << ss.str() << endl;
}

BlockTransferer::~BlockTransferer()
{
	if(m_servSocket) {
		delete m_servSocket;
		m_servSocket = 0;
	}

	if (m_context) {
		delete m_context;
		m_context = 0;
	}
}

size_t	BlockTransferer::GetNumMsgsAvail()
{
	if (m_msgQueue.size() > 0) {
		m_fp << "Block transferer (port " << m_port << ") - GetNumMsgsAvail() called with packets available." << endl;
	}

	return m_msgQueue.size();
}

BlockMsg	BlockTransferer::GetMsg()
{
	BlockMsg msg = m_msgQueue.front();
	m_msgQueue.pop();
	return msg;
}

void BlockTransferer::Update()
{
	zmq::pollitem_t items[] = {
		{ *m_servSocket, 0, ZMQ_POLLIN, 0 }
	};
	
	try
	{
	  zmq::poll(&items[0], 1, 0);
	}
	catch(zmq::error_t e) {
	  if(e.num() != EINTR) {
	    m_fp << "Got error on poll: " << e.what() << endl
		<< "num: " << e.num() << endl;
	  }
	}

	while (items[0].revents & ZMQ_POLLIN) {
		m_fp << "Got message." << endl;
		BlockMsg msg;
		memset((void*)&msg, 0, sizeof(BlockMsg));

		bool successFullRecv = true;

		zmq::message_t initialMsg;
		
		try
		{
		  m_servSocket->recv(&initialMsg);
		}
		catch(zmq::error_t e) {
		  m_fp << "Got error on recv 1: " << e.what() << endl;
		}

		if (initialMsg.size() == sizeof(BlockMsg::InitialBlock)) {
			memcpy((void*)&msg.initial, (void*)initialMsg.data(), sizeof(BlockMsg::InitialBlock));
		} else {
			m_fp << "Failed to receive initial message." << endl;
			successFullRecv = false;
		}

		m_fp << "Hash length: " << msg.initial.hashLen << endl;

		zmq::message_t hashMsg;
		try
		{
		  m_servSocket->recv(&hashMsg);
		}
		catch(zmq::error_t e) {
		  m_fp << "Got error on recv 2: " << e.what() << endl;
		}
		

		if (hashMsg.size() == msg.initial.hashLen) {
			msg.hash = (char*)malloc(msg.initial.hashLen + 1);
			memcpy(msg.hash, (void*)hashMsg.data(), msg.initial.hashLen);
			msg.hash[msg.initial.hashLen] = 0;
		} else {
			successFullRecv = false;
			m_fp << "Failed to receive hash." << endl;
		}

		zmq::message_t blockLenMsg;
		
		try
		{
		  m_servSocket->recv(&blockLenMsg);
		}
		catch(zmq::error_t e) {
		  m_fp << "Got error on recv 3: " << e.what() << endl;
		}

		if (blockLenMsg.size() == sizeof(uint64_t)) {
			memcpy((void*)&msg.blockLen, (void*)blockLenMsg.data(), sizeof(uint64_t));
		} else {
			successFullRecv = false;
			m_fp << "Failed to receive block length." << endl;
		}

		m_fp << "Block length: " << msg.blockLen << endl;

		zmq::message_t blockMsg;
		
		try
		{
		  m_servSocket->recv(&blockMsg);
		}
		catch(zmq::error_t e) {
		  m_fp << "Got error on recv 4: " << e.what() << endl;
		}

		if (blockMsg.size() == msg.blockLen) {
			msg.block = (char*)malloc(msg.blockLen + 1);
			memcpy((void*)msg.block, (void*)blockMsg.data(), msg.blockLen);
			msg.block[msg.blockLen] = 0;
		} else {
			successFullRecv = false;
			m_fp << "Failed to receive block." << endl;
		}

		zmq::message_t addressLenMsg;
		
		try
		{
		  m_servSocket->recv(&addressLenMsg);
		}
		catch(zmq::error_t e) {
		  m_fp << "Got error on recv 5: " << e.what() << endl;
		}

		if (addressLenMsg.size() == sizeof(uint32_t)) {
			memcpy((void*)&msg.addressLen, (void*)addressLenMsg.data(), sizeof(uint32_t));
		} else {
			successFullRecv = false;
			m_fp << "Failed to receive address length." << endl;
		}

		m_fp << "Address length: " << msg.addressLen << endl;

		zmq::message_t addressMsg;
		
		try
		{
		  m_servSocket->recv(&addressMsg);
		}
		catch(zmq::error_t e) {
		  m_fp << "Got error on recv 6: " << e.what() << endl;
		}

		if (addressMsg.size() == msg.addressLen) {
			msg.address = (char*)malloc(msg.addressLen + 1);
			memcpy((void*)msg.address, (void*)addressMsg.data(), msg.blockLen);
			msg.address[msg.addressLen] = 0;
		} else {
			successFullRecv = false;
			m_fp << "Failed to receive address." << endl;
		}

		if (successFullRecv) {
			m_fp << "Successful received message." << endl;
			m_msgQueue.push(msg);
		}

		uint8_t response = successFullRecv ? 1 : 0;
		zmq::message_t responseMsg(sizeof(uint8_t));
		memcpy((void*)responseMsg.data(), (void*)&response, sizeof(uint8_t));

		m_fp << "Sending response." << endl;

		try
		{
		  m_servSocket->send(responseMsg);
		}
		catch(zmq::error_t e) {
		  m_fp << "Got error on send: " << e.what() << endl;
		}

		m_fp << "Sent response." << endl;
		
		try
		{
		  zmq::poll(&items[0], 1, 0);
		}
		catch(zmq::error_t e) {
		  if(e.num() != EINTR) {
		    m_fp << "Got error on poll 2: " << e.what() << endl;
		  }
		}
	}
}

bool BlockTransferer::SendBlockTransferMsg(const char* address, const BlockMsg& msg)
{
	m_fp << "Sending message:" << endl
		<< "\tMessage type: " << msg.initial.msgType << endl
		<< "\tTo Address: " << address << endl
		<< "\tOriginal Sending Address: " << msg.address << endl
		<< "\tAddress Length: " << msg.addressLen << endl
		<< "\tBlock length: " << msg.blockLen << endl
		<< "\tHash Length: " << msg.initial.hashLen << endl;

	m_fp << "\tConnecting." << endl;

	zmq::socket_t clientSocket(*m_context, ZMQ_REQ);
	m_fp << "\tSocket created." << endl;

	try {
		clientSocket.connect(address);
	}
	catch (zmq::error_t e) {
		m_fp << "\tZeroMQ error: " << e.what() << endl;
		m_fp.flush();
		throw e;
	}
	catch (...) {
		m_fp << "\tUnknown exception." << endl;
		m_fp.flush();
		throw;
	}

	m_fp << "\tConnected." << endl;

	zmq::message_t initialMsg(sizeof(BlockMsg::InitialBlock));
	memcpy(initialMsg.data(), (void*)&msg.initial, sizeof(BlockMsg::InitialBlock));

	clientSocket.send(initialMsg, ZMQ_SNDMORE);

	m_fp << "\tSent 1st part." << endl;
	
	zmq::message_t hashMsg(msg.initial.hashLen);
	memcpy(hashMsg.data(), (void*)msg.hash, msg.initial.hashLen);

	clientSocket.send(hashMsg, ZMQ_SNDMORE);

	m_fp << "\tSent 2nd part." << endl;
	
	zmq::message_t blockLenMsg(sizeof(uint64_t));
	memcpy(blockLenMsg.data(), (void*)&msg.blockLen, sizeof(uint64_t));

	clientSocket.send(blockLenMsg, ZMQ_SNDMORE);
	
	m_fp << "\tSent 3rd part." << endl;

	zmq::message_t msgMsg(msg.blockLen);
	memcpy(msgMsg.data(), (void*)msg.block, msg.blockLen);

	clientSocket.send(msgMsg, ZMQ_SNDMORE);

	m_fp << "\tSent 4th part." << endl;

	zmq::message_t addressLenMsg(sizeof(uint32_t));
	memcpy(addressLenMsg.data(), (void*)&msg.addressLen, sizeof(uint32_t));

	clientSocket.send(addressLenMsg, ZMQ_SNDMORE);

	m_fp << "\tSent 5th part." << endl;
	
	zmq::message_t addressMsg(msg.addressLen);
	memcpy(addressMsg.data(), (void*)msg.address, msg.addressLen);

	clientSocket.send(addressMsg);

	m_fp << endl
		<< "\tMessage sent." << endl;

	m_fp.flush();

	uint8_t serverOk = 0;
	zmq::message_t serverMsg;

	m_fp << "\tReceiving server response." << endl;

	try {
		clientSocket.recv(&serverMsg);
	} catch (zmq::error_t e) {
		m_fp << "\tZeroMQ error: " << e.what() << endl;
		m_fp.flush();
		throw e;
	} catch (...) {
		m_fp << "\tUnknown exception." << endl;
		m_fp.flush();
		throw;
	}

	if (serverMsg.size() == sizeof(uint8_t)) {
		m_fp << "\tChecking server response." << endl;
		memcpy((void*)&serverOk, (void*)serverMsg.data(), sizeof(uint8_t));
	}

	m_fp << "\tServer response: ";

	if (serverOk != 1) {
		m_fp << "FAIL" << endl;
		m_fp.flush();
		return false;
	}

	m_fp << "OK" << endl;
	m_fp.flush();
	
	return true;
}