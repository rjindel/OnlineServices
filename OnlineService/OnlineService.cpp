// OnlineService.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

#include "SimpleConnection.h"
#include "QosConnection.h"
#include "Utils.h"

//Message types for Authentication server
enum class AuthMessageType : uint16_t
{
	Error = 0,
	AuthTicketRequest = 1, 
	AuthTicketResponse = 2
};

//Message types for Api Server 
enum class APIMessageType : UINT16
{
	Error = 0,
	Authorize = 1, 
	GetServers = 2,
	GetServerResponse = 3,
};

//Get auth ticket from authentication server
//authsocket is a socket that is already connect to the authentication server
//clientID and clientSecret are null terminated string in hex form
//The returned authToken is a byte array of tokenLength. This is still "message packed", however there is no need to unpack this as it is needs to be 'packed' before sending to the API Server
bool GetAuthToken(SimpleConnection& authConnection, const char* clientID, const char* clientSecret, std::vector<char> authToken)
{
	const int MAX_PAYLOADSIZE = 256;
	const int UUID_LENGTH_IN_BYTES = 16;
	
	BYTE clientUUID[UUID_LENGTH_IN_BYTES];
	constexpr int maxSecretLength = MAX_PAYLOADSIZE - UUID_LENGTH_IN_BYTES;
	BYTE clientSecretBinary[maxSecretLength];
	memset(clientSecretBinary, 0, maxSecretLength);

	StringToHex(clientID, &clientUUID[0]);
	StringToHex(clientSecret, &clientSecretBinary[0]);
	int secretLength = 0;
	while (secretLength < maxSecretLength && clientSecretBinary[secretLength] != 0 ) secretLength++;

	if (secretLength == maxSecretLength)
	{
		printf("Client secret exceed max payload length");
		return false;
	}

	//MsgPack client id and client secret
	msgpack::sbuffer streamBuffer;
	msgpack::packer<msgpack::sbuffer> packer(&streamBuffer);
	packer.pack_bin(UUID_LENGTH_IN_BYTES);
	packer.pack_bin_body((const char *)clientUUID, UUID_LENGTH_IN_BYTES);
	packer.pack_bin(secretLength);
	packer.pack_bin_body((const char *)clientSecretBinary, secretLength);

	if (!authConnection.Send(static_cast<uint16_t>(AuthMessageType::AuthTicketRequest), streamBuffer))
	{
		printf("Error sending Authentication token");
		return false;
	}

	uint16_t messageType = 0;
	if(!authConnection.Receive(messageType, authToken))
	{
		printf("Error response expected");
		return false;
	}

	if ((AuthMessageType) messageType != AuthMessageType::AuthTicketResponse)
	{
		printf("Error expected Auth Token");
		return false;
	}

	return true;
}

bool GetQosServerNames(SimpleConnection& apiConnection, QosConnection*& qosServers, uint32_t& qosServerCount)
{
	if (!apiConnection.Send(static_cast<uint16_t>(APIMessageType::GetServers)))
	{
		return false;
	}
	
	uint16_t messageType = 0;
	std::vector<char> payload;
	if (!apiConnection.Receive(messageType, payload))
	{
		return false;
	}

	if ((APIMessageType)messageType != APIMessageType::GetServerResponse)
	{
		printf("expected server reponse");
		return false;
	}

	msgpack::object_handle objectHandle = msgpack::unpack((const char*)payload.data(), payload.size());
	msgpack::object serverObjects = objectHandle.get();
	
	qosServerCount = serverObjects.via.array.size;
	qosServers = new QosConnection[qosServerCount];

	for(uint32_t i = 0; i < qosServerCount ; i++)
	{
		msgpack::type::tuple<std::string, int> serverData;
		serverObjects.via.array.ptr[i].convert(serverData);

		std::string servername = serverData.get<0>();
		int port = serverData.get<1>();

		if (!qosServers[i].CreateConnection(servername, std::to_string(port), false))
		{
			return false;
		}
		qosServers[i].SetNonBlockingMode();
	}
	
	return true;
}

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

	SimpleConnection authConnection;
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
	if (!GetAuthToken(authConnection, clientID, clientSecret, authToken))
	{
		return -1;
	}
	
	//Connect to API server
	const int apiPort = 12705;
	const char* apiUrl = "api.rcr.experiments.fireteam.net";

	SimpleConnection apiConnection;
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
	if (!GetQosServerNames(apiConnection, qosServers, qosServerCount))
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

