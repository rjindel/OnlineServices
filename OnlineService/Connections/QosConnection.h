//-------------------------------------------------------------------------
//
// File: QosConnection.h
//
// Connection class to handle qos
//
//--------------------------------------------------------------------------
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

	WSAOVERLAPPED			overlapped = { 0 };
	LARGE_INTEGER			frequency = { 0 };
	LARGE_INTEGER			accumulator = { 0 };

	std::vector<uint32_t>	packetIds;
	uint32_t				packetsSent;

	DWORD					timeOut;
	static const int		MAX_PAYLOADSIZE = 256;
	QosPacket				packet;

	uint32_t				packetsThreshhold;
	HANDLE					packetsThreshholdEvent;
	
	void		Measure();
public:
	QosConnection();
	virtual ~QosConnection();

	void		StartMeasuringQos();

	//Request measuring to stop immediately without blocking
	void		RequestStopMeasuring() 
	{
		exit = true;
	}
	//Stop measuring and wait for thread to finish. Blocking call.
	void		StopMeasuring();

	void		SignalAfterPackets(const uint32_t packetsToWaitFor, HANDLE event)
	{
		packetsThreshhold = packetsToWaitFor;
		packetsThreshholdEvent = event;
	}

	uint32_t	GetPacketsSent() const { return packetsSent; }
	uint32_t	GetPacketsLost();
	uint32_t	GetAveragePing();

	static void PrintQosData(QosConnection *qosServers, uint32_t qosServerCount);
};
