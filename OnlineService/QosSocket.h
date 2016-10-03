#pragma once
#include "stdafx.h"
#include "SimpleSocket.h"

class QosSocket;

struct SocketInfo
{
	WSAOVERLAPPED overlapped;
	QosSocket* thisPtr;
};

struct QosPacket
{
	char header[2] = { (char)0xff, (char)0xff };
	uint32_t instanceId;
	uint32_t packetId;
	//Timestamp
	LARGE_INTEGER startTime = { 0 };
};

class QosSocket : public SimpleSocket
{
	uint32_t	packetsSent;

	std::vector<uint32_t> packetIds;

	LARGE_INTEGER startTime, endTime, frequency;
	LARGE_INTEGER accumulator = { 0 };
	static uint32_t instanceCounter;
	uint32_t instanceId;

	SocketInfo socketInfo;

	static const int MAX_PAYLOADSIZE = 256;
	QosPacket packet;

	std::thread QosThread;
	//std::thread QosReceiveThread;
	//std::thread QosSendThread;

	std::recursive_mutex QosMutex;

	bool exit;
public:
	QosSocket();
	virtual ~QosSocket();

	void StartMeasuringQos();
	void StopMeasuring();

	void SendFunction();
	void ReceiveFunction();

	void Measure();

	uint32_t GetPacketsSent() { return packetsSent; }
	uint32_t GetPacketsLost();
	uint32_t GetAveragePing();

	void PrintSocketOptions();
};
