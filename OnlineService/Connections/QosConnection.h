#pragma once
#include "stdafx.h"
#include "SimpleConnection.h"

class QosConnection;

struct SocketInfo
{
	WSAOVERLAPPED overlapped;
	QosConnection* thisPtr;
};

struct QosPacket
{
	char header[2] = { (char)0xff, (char)0xff };
	uint32_t instanceId;
	uint32_t packetId;
	//Timestamp
	LARGE_INTEGER startTime = { 0 };
};

class QosConnection : public SimpleConnection
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

	std::recursive_mutex QosMutex;

	bool exit;
public:
	QosConnection();
	virtual ~QosConnection();

	void StartMeasuringQos();
	void StopMeasuring();

	void Measure();

	uint32_t GetPacketsSent() { return packetsSent; }
	uint32_t GetPacketsLost();
	uint32_t GetAveragePing();

	static void PrintQosData(QosConnection *qosServers, uint32_t qosServerCount);
};
