#pragma once
#include "..\stdafx.h"
#include "SimpleConnection.h"

struct QosPacket
{
	QosPacket() 
		: instanceId(0)
		, packetId(0)
	{
		memset(header, 0xff, sizeof(header));
	}
	char			header[2];
	uint32_t		instanceId;
	uint32_t		packetId;
	LARGE_INTEGER	startTime = { 0 };
};

class QosConnection : public SimpleConnection
{
	static uint32_t			instanceCounter;
	uint32_t				instanceId;

	std::thread				QosThread;
	std::recursive_mutex	QosMutex;
	bool					exit;

	WSAOVERLAPPED			overlapped;
	LARGE_INTEGER			frequency = { 0 };
	LARGE_INTEGER			accumulator = { 0 };

	std::vector<uint32_t>	packetIds;
	uint32_t				packetsSent;

	DWORD					timeOut;
	static const int		MAX_PAYLOADSIZE = 256;
	QosPacket				packet;


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
