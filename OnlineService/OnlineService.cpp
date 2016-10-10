// OnlineService.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

#include "Connections\SimpleConnection.h"
#include "Connections\QosConnection.h"
#include "Connections\AuthenticationConnection.h"
#include "Connections\APIConnection.h"

#include "Utils.h"

int main()
{
	WSADATA wsaData = { 0 };
	int err =	WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (err != 0)
	{
		printf("Error creating socket");
		return EXIT_FAILURE;
	}

	//Connect to authentication server

	//These hard coded value should idealy be read from file. 
	const int	authPort = 12704;
	const char* authUrl = "auth.rcr.experiments.fireteam.net";

	AuthenticationConnection authConnection;
	if ( !authConnection.CreateConnection(authUrl, std::to_string(authPort)) )
	{
		//Cleanup
		printf("Error creating connection");
		return EXIT_FAILURE;
	}

	//Get Authentication token
	const char* clientID = "45f67935-9286-4ba0-8b3b-70228e727ca2";
	const char*	clientSecret = "1eba4dac53c33ae135fe7b2a839eb30aec964f75";
	
	std::vector<char> authToken;
	if (!authConnection.GetAuthToken(clientID, clientSecret, authToken))
	{
		return EXIT_FAILURE;
	}
	
	//Connect to API server
	const int	apiPort = 12705;
	const char*	apiUrl = "api.rcr.experiments.fireteam.net";

	APIConnection apiConnection;
	if (!apiConnection.CreateConnection(apiUrl, std::to_string(apiPort)))
	{
		printf("Error creating connection");
		return EXIT_FAILURE;
	}

	//Authorize
	if (!apiConnection.Send(static_cast<uint16_t>(APIMessageType::Authorize), authToken))
	{
		return EXIT_FAILURE;
	}

	//Get servers for Qos measurements

	HANDLE packetReceivedThreshholdEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	uint32_t packetsToWaitFor = 50;
	DWORD timeOut = 5000;

	//Can not use std container, as copy constructor is deleted for thread\mutex class
	//Defining move constructor and using move semantics doesn't help.
	//using a smart pointer makes things more complicated
	QosConnection *qosServers = nullptr;
	uint32_t qosServerCount = 0;
	if (!apiConnection.GetQosServer(qosServers, qosServerCount))
	{
		return EXIT_FAILURE;
	}

	//Start measuring
	for (uint32_t i = 0; i < qosServerCount; i++)
	{
		qosServers[i].StartMeasuringQos();
		qosServers[i].SignalAfterPackets(packetsToWaitFor, packetReceivedThreshholdEvent);
	}

	//Output measurements to screen and wait for user input before exitting.
	//QosConnection::PrintQosData(qosServers, qosServerCount);

	WaitForSingleObject(packetReceivedThreshholdEvent, timeOut);

	for( uint32_t i = 0; i < qosServerCount; i++)
	{
		qosServers[i].RequestStopMeasuring();
	}

	uint32_t mostPacketsSent = 0;
	uint32_t qosServerIndex = 0;

	//Cleanup and determine most suitable server
	for( uint32_t i = 0; i < qosServerCount; i++)
	{
		qosServers[i].StopMeasuring();
		if ((qosServers[i].GetPacketsSent() - qosServers[i].GetPacketsLost()) > mostPacketsSent)
		{
			mostPacketsSent = qosServers[i].GetPacketsSent()- qosServers[i].GetPacketsLost();
			qosServerIndex = i;
		}
	}

	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	Utils::ClearScreen(consoleHandle);

	printf("Preferred data centre is %s. With an average latency of %u ms\n", qosServers[qosServerIndex].GetAddress().c_str(), qosServers[qosServerIndex].GetAveragePing());

	system("pause");

	delete[] qosServers;

	WSACleanup();

    return EXIT_SUCCESS;
}

