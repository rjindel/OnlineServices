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
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (err != 0)
	{
		printf("Error creating socket");
		return -1;
	}

	bool testLocalServer = false;
	if ( testLocalServer )
	{
		int port = 27015;
		std::string ipaddress = "127.0.0.1";
		const uint32_t qosServerCount = 3;
		QosConnection qosServers[qosServerCount];
		for (uint32_t i = 0; i < qosServerCount; i++)
		{
			qosServers[i].CreateConnection(ipaddress, std::to_string(port + i), false);
			qosServers[i].SetNonBlockingMode();
			qosServers[i].StartMeasuringQos();
		}

		QosConnection::PrintQosData(qosServers, qosServerCount);

		for( uint32_t i = 0; i < qosServerCount; i++)
		{
			qosServers[i].StopMeasuring();
		}

		WSACleanup();
		return 0;
	}
	
	//Connect to authentication server

	//These hard coded value should idealy be read from file. 
	const int authPort = 12704;
	const char* authUrl = "auth.rcr.experiments.fireteam.net";

	AuthenticationConnection authConnection;
	if ( !authConnection.CreateConnection(authUrl, std::to_string(authPort)) )
	{
		//Cleanup
		printf("Error creating connection");
		return -1;
	}

	//Get Authentication token
	const char * clientID = "45f67935-9286-4ba0-8b3b-70228e727ca2";
	const char * clientSecret = "1eba4dac53c33ae135fe7b2a839eb30aec964f75";
	
	std::vector<char> authToken;
	if (!authConnection.GetAuthToken(clientID, clientSecret, authToken))
	{
		return -1;
	}
	
	//Connect to API server
	const int apiPort = 12705;
	const char* apiUrl = "api.rcr.experiments.fireteam.net";

	APIConnection apiConnection;
	if (!apiConnection.CreateConnection(apiUrl, std::to_string(apiPort)))
	{
		//Cleanup
		printf("Error creating connection");
		return -1;
	}

	//Authorize
	if (!apiConnection.Send(static_cast<uint16_t>(APIMessageType::Authorize), authToken))
	{
		return -1;
	}

	//Get servers for Qos measurements

	//Can not use std container, as copy constructor is deleted for thread\mutex class
	//Defining move constructor and using move semantics doesn't help.
	QosConnection *qosServers;
	uint32_t qosServerCount;
	if (!apiConnection.GetQosServerNames(qosServers, qosServerCount))
	{
		return -1;
	}

	//Start measuring
	for (uint32_t i = 0; i < qosServerCount; i++)
	{
		qosServers[i].StartMeasuringQos();
	}

	//Output measurements to screen
	QosConnection::PrintQosData(qosServers, qosServerCount);

	//Cleanup
	for( uint32_t i = 0; i < qosServerCount; i++)
	{
		qosServers[i].StopMeasuring();
	}

	delete[] qosServers;

	WSACleanup();
    return 0;
}

