#include "..\stdafx.h"

#include "QosConnection.h"
#include "..\Utils.h"

uint32_t QosConnection::instanceCounter = 0;

QosConnection::QosConnection() : instanceId(instanceCounter++) 
	, exit(false)
	, packetsSent(0)
{
}

QosConnection::~QosConnection()
{
	if (QosThread.joinable())
	{
		printf("Destructor called while still measuring. Call StopMeasuring before destroying QosSocket. Blocking to wait for thread to complete.\n");
		StopMeasuring();
	}
}

void QosConnection::StartMeasuringQos()
{
	if (QosThread.joinable())
	{
		printf("Measuring already in progress. Call StopMeasuring first.\n");
		return;
	}
	
	QueryPerformanceFrequency(&frequency);
	exit = false;

	socketInfo.thisPtr = this;
	memset(&socketInfo.overlapped, 0, sizeof(WSAOVERLAPPED));
	socketInfo.overlapped.hEvent = WSACreateEvent();
	WSAResetEvent(socketInfo.overlapped.hEvent);

	packet.instanceId = instanceId;

	QosThread = std::thread(&QosConnection::Measure, this);
}

void QosConnection::StopMeasuring()
{
	exit = true;
	QosThread.join();
	
	WSACloseEvent(socketInfo.overlapped.hEvent);
}

void QosConnection::Measure()
{
	LARGE_INTEGER endTime = { 0 };
	DWORD timeOut = 1000;
	DWORD recvBytes = 0, flags = 0;
	packetIds.clear();

	static_assert(sizeof(packet) <= MAX_PAYLOADSIZE, "Packet exceeds max payload size");
	WSABUF wsaBuffer = { sizeof(packet), (char*)&packet };

	while (!exit)
	{
		WSAResetEvent(socketInfo.overlapped.hEvent);

		{
			std::lock_guard<>(QosMutex);
			packetIds.push_back(packetsSent);
			packet.packetId = packetsSent++;
		}
		QueryPerformanceCounter(&packet.startTime);

		WSASend(connectedSocket, &wsaBuffer, 1, nullptr, 0, &socketInfo.overlapped, 0);
		auto err = WSAGetLastError();
		PrintError("Sending packet: ");

		DWORD result = WSAWaitForMultipleEvents(1, &socketInfo.overlapped.hEvent, FALSE, timeOut, false);

		WSAResetEvent(socketInfo.overlapped.hEvent);

		if (result != WSA_WAIT_TIMEOUT)
		{
			do {
				WSARecv(connectedSocket, &wsaBuffer, 1, &recvBytes, &flags, &socketInfo.overlapped, nullptr);
				err = WSAGetLastError();
			} while (err == WSA_IO_PENDING);
			//PrintError("Receiving packet: ");

			result = WSAWaitForMultipleEvents(1, &socketInfo.overlapped.hEvent, FALSE, timeOut, true);
			QueryPerformanceCounter(&endTime);

			if (result == 0)
			{
				if (packet.header[0] != (char)0xff && packet.header[1] != (char)0xff)
				{
					packet.header[0] = packet.header[1] = (char)0xff;
					printf("Error bad packet received\n");
					continue;
				}
				if (packet.instanceId != instanceId)
				{
					packet.instanceId = instanceId;
					printf("Error packet contains wrong instance id");
					continue;
				}

				std::lock_guard<>(QosMutex);
				auto iter = std::find(packetIds.begin(), packetIds.end(), packet.packetId);
				if (iter != packetIds.end())
				{
					packetIds.erase(iter);
					accumulator.QuadPart += endTime.QuadPart - packet.startTime.QuadPart;
				}
			}
		}
	}
}

uint32_t QosConnection::GetPacketsLost() 
{ 
	std::lock_guard<>(QosMutex);
	uint32_t packetsLost = packetIds.size();
	return packetsLost;
}

uint32_t QosConnection::GetAveragePing() 
{
	std::lock_guard<>(QosMutex);
	uint32_t succesfullySentPackets = packetsSent - GetPacketsLost();
	if (succesfullySentPackets)
	{
		uint32_t averagePing = (uint32_t)(accumulator.QuadPart * 1000 / (frequency.QuadPart * succesfullySentPackets));
		return averagePing;
	}
	return 0;
}

void QosConnection::PrintQosData(QosConnection *qosServers, uint32_t qosServerCount)
{
	const int commentLines = 2;
	const int refreshTime = 10;
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	while (!(GetAsyncKeyState(VK_ESCAPE) & 1))
	{
		ClearScreen(consoleHandle, qosServerCount + commentLines);
		printf("Press Escape to exit\n");
		printf("IP Address:port \t Packets sent \t packets lost \t Average ping \n");
		for (uint32_t i = 0; i < qosServerCount; i++)
		{
			uint32_t packetSent = qosServers[i].GetPacketsSent();
			uint32_t packetLost = qosServers[i].GetPacketsLost();
			uint32_t averagePing = qosServers[i].GetAveragePing();
			printf("%s:%i \t\t %i \t\t %i \t\t %i \n", qosServers[i].GetAddress().c_str(),  std::stoi(qosServers[i].GetPort()), packetSent, packetLost, averagePing);
		}
		printf("\n\n");
		Sleep(refreshTime);
	}
}