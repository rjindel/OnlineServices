#include "stdafx.h"

#include "QosSocket.h"

uint32_t QosSocket::instanceCounter = 0;

QosSocket::QosSocket() : instanceId(instanceCounter++) 
	, exit(false)
	,packetsSent(0)
	//,QosThread( std::thread(&QosSocket::Measure, this) )
{
}

void QosSocket::StartMeasuringQos()
{
	QueryPerformanceFrequency(&frequency);

	socketInfo.thisPtr = this;
	memset(&socketInfo.overlapped, 0, sizeof(WSAOVERLAPPED));
	socketInfo.overlapped.hEvent = WSACreateEvent();
	WSAResetEvent(socketInfo.overlapped.hEvent);

	packet.instanceId = instanceId;

	QosThread = std::thread(&QosSocket::Measure, this);

	//QosReceiveThread = std::thread(&QosSocket::ReceiveFunction, this);
	//QosSendThread = std::thread(&QosSocket::SendFunction, this);
}

void QosSocket::StopMeasuring()
{
	exit = true;
	QosThread.join();
	
	//QosSendThread.join();
	//QosReceiveThread.join();
	WSACloseEvent(socketInfo.overlapped.hEvent);
}

void QosSocket::SendFunction()
{
	DWORD timeOut = 5000;
	packet.packetId = packetsSent;

	WSABUF wsaBuffer = { sizeof(packet), (char*)&packet };

	WSAResetEvent(socketInfo.overlapped.hEvent);
	QueryPerformanceCounter(&packet.startTime);

	WSASend(connectedSocket, &wsaBuffer, 1, nullptr, 0, &socketInfo.overlapped, 0);
	auto err = WSAGetLastError();
	PrintError("Sending packet: ");
	packetsSent++;

	DWORD result = WSAWaitForMultipleEvents(1, &socketInfo.overlapped.hEvent, FALSE, timeOut, false);
	//WSAGetOverlappedResult(connectedSocket, &socketInfo.overlapped, &recvBytes, true, &flags);
}

void QosSocket::ReceiveFunction()
{
	DWORD timeOut = 5000;
	DWORD recvBytes = 0, flags = 0;

	QosPacket packet;
	WSABUF wsaBuffer = { sizeof(packet), (char*)&packet };
	WSAResetEvent(socketInfo.overlapped.hEvent);
	int err = 0;

	do {
		WSARecv(connectedSocket, &wsaBuffer, 1, &recvBytes, &flags, &socketInfo.overlapped, nullptr);
		auto err = WSAGetLastError();
	} while (err == WSA_IO_PENDING);
	PrintError("Receiving packet: ");

	auto result = WSAWaitForMultipleEvents(1, &socketInfo.overlapped.hEvent, FALSE, timeOut, true);
	//PrintError("Receiving packet: ");
	QueryPerformanceCounter(&endTime);

	accumulator.QuadPart += endTime.QuadPart - startTime.QuadPart;
}

void QosSocket::Measure()
{
	LARGE_INTEGER endTime = { 0 };
	DWORD timeOut = 500;
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
		//WSAGetOverlappedResult(connectedSocket, &socketInfo.overlapped, &recvBytes, true, &flags);
		WSAResetEvent(socketInfo.overlapped.hEvent);

		if (result != WSA_WAIT_TIMEOUT)
		{
			do {
				WSARecv(connectedSocket, &wsaBuffer, 1, &recvBytes, &flags, &socketInfo.overlapped, nullptr);
				err = WSAGetLastError();
			} while (err == WSA_IO_PENDING);
			PrintError("Receiving packet: ");

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

void QosSocket::RecvCallback(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	auto err = WSAGetLastError();
	if (dwError == WSA_WAIT_IO_COMPLETION || dwError == 0)
	{
		SocketInfo *socketInfo = (SocketInfo*)overlapped;
		QueryPerformanceCounter(&socketInfo->thisPtr->endTime);

		socketInfo->thisPtr->accumulator.QuadPart += socketInfo->thisPtr->endTime.QuadPart - socketInfo->thisPtr->startTime.QuadPart;
	}
}

uint32_t QosSocket::GetPacketsLost() 
{ 
	std::lock_guard<>(QosMutex);
	uint32_t packetsLost = packetIds.size();
	return packetsLost;
}

uint32_t QosSocket::GetAveragePing() 
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

void QosSocket::PrintSocketOptions()
{
	DWORD receiveBuffer = 0;
	DWORD sendBuffer = 0;
	DWORD datatgramSize = 0;
	int optionSize = sizeof(receiveBuffer);

	getsockopt(connectedSocket, SOL_SOCKET, SO_RCVBUF, (char*)&receiveBuffer, &optionSize);
	getsockopt(connectedSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBuffer, &optionSize);
	//getsockopt(connectedSocket, SOL_SOCKET, SO_MAXDG, (char*)&datatgramSize, &optionSize);

	printf("Buffer size. Recieve: %i , Send: %i \n", receiveBuffer, sendBuffer);
	//printf("maximum datagram size is %i \n", datatgramSize);
}

