// OnlineService.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

#include "SimpleSocket.h"
#include "QosSocket.h"

const int MAX_PAYLOADSIZE = 256;

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

//Convert a single ascii character to a 4 bit hex value
//If the character is outside outside the range [0-9a-fA-F] throw
char CharToHex(char character)
{
	character = tolower(character);
	if (!isalnum(character))
	{
		throw "invalid string";
	}

	char temp = 0;
	if (isdigit(character))
	{
		temp = character - '0';
	}
	else
	{
		temp = character - 'a' + 10;
	}

	return temp;
}

//Convert a null terminated string to a byte array
void StringToHex(const char* str, BYTE* hex)
{
	if (!str || !hex)
	{
		throw "invalid parameters";
	}

	while (*str != 0 && *str + 1 != 0)// && *hex != 0)
	{
		if (*str == '-')
		{
			//Handle uuid's. Ideally this belongs in a higher level function, but as we have simple use case, it's easier to handle here.
			//As the message headers are little-endian, we don't need to endian conversion
			str++;
			continue;
		}

		char hiNibble = CharToHex(*str);
		str++;

		char loNibble = CharToHex(*str);

		*hex = (hiNibble << 4) + loNibble;
		hex++;
		str++;
	}
}

//Get auth ticket from authentication server
//authsocket is a socket that is already connect to the authentication server
//clientID and clientSecret are null terminated string in hex form
//The returned authToken is a byte array of tokenLength. This is still "message packed", however there is no need to unpack this as it is needs to be 'packed' before sending to the API Server
bool GetAuthToken(SimpleSocket& authSocket, const char* clientID, const char* clientSecret, char* authToken, uint32_t& tokenLength)
{
	const int UUID_LENGTH_IN_BYTES = 16;
	
	BYTE clientUUID[UUID_LENGTH_IN_BYTES];
	constexpr int maxSecretLength = MAX_PAYLOADSIZE - UUID_LENGTH_IN_BYTES;
	BYTE clientSecretBinary[maxSecretLength];
	memset(clientSecretBinary, 0, maxSecretLength);

	StringToHex(clientID, &clientUUID[0]);
	StringToHex(clientSecret, &clientSecretBinary[0]);
	int secretLength = 0;
	while (clientSecretBinary[secretLength] != 0) secretLength++;

	//MsgPack client id and client secret
	msgpack::sbuffer streamBuffer;
	msgpack::packer<msgpack::sbuffer> packer(&streamBuffer);
	packer.pack_bin(UUID_LENGTH_IN_BYTES + secretLength);
	packer.pack_bin_body((const char *)clientUUID, UUID_LENGTH_IN_BYTES);
	packer.pack_bin_body((const char *)clientSecretBinary, secretLength);

	authSocket.Send(static_cast<uint16_t>(AuthMessageType::AuthTicketRequest), streamBuffer);

	uint16_t messageType = 0;
	if(!authSocket.Receive(messageType, authToken, tokenLength))
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

bool GetQosServerNames(SimpleSocket& apiSocket, QosSocket*& qosServers, uint32_t& qosServerCount)
//bool GetQosServerNames(SimpleSocket& apiSocket, std::vector<QosSocket>& qosServers)
{
	if (!apiSocket.Send(static_cast<uint16_t>(APIMessageType::GetServers), nullptr, 0))
	{
		return false;
	}
	
	uint16_t messageType = 0;
	uint32_t payloadSize = MAX_PAYLOADSIZE;
	char payload[MAX_PAYLOADSIZE];
	if (!apiSocket.Receive(messageType, payload, payloadSize))
	{
		return false;
	}

	if ((APIMessageType)messageType != APIMessageType::GetServerResponse)
	{
		printf("expected server reponse");
		return false;
	}

	msgpack::object_handle objectHandle = msgpack::unpack((const char*)&payload[0], payloadSize);
	msgpack::object serverObjects = objectHandle.get();
	
	qosServerCount = serverObjects.via.array.size;
	qosServers = new QosSocket[qosServerCount];

	for(uint32_t i = 0; i < qosServerCount ; i++)
	{
		msgpack::type::tuple<std::string, int> serverData;
		serverObjects.via.array.ptr[i].convert(serverData);

		std::string servername = serverData.get<0>();
		int port = serverData.get<1>();
		printf("Address %s : %i \n", servername.c_str(), port);

		qosServers[i].CreateConnection(servername, std::to_string(port), false);
		qosServers[i].SetNonBlockingMode();
		qosServers[i].StartMeasuringQos();
	}
	
	return true;
}

void ClearScreen(HANDLE consoleHandle, uint32_t lines = 0)
{
	COORD coords{0, 0};
	DWORD charsWritten, length;

	CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;
	GetConsoleScreenBufferInfo(consoleHandle, &consoleBufferInfo);

	if (lines == 0)
	{
		lines = consoleBufferInfo.dwSize.Y;
	}
	length = consoleBufferInfo.dwSize.X * lines;

	FillConsoleOutputCharacter(consoleHandle, _TCHAR(' '), length, coords, &charsWritten);
	//FillConsoleOutputAttribute

	SetConsoleCursorPosition(consoleHandle, coords);
}

int main()
{
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	//TODO: better error handling. Minimally output the type of error we got from err
	if (err != 0)
	{
		printf("Error creating socket");
		return -1;
	}

	bool testLocalServer = true;
	if ( testLocalServer )
	{
		int port = 27015;
		std::string ipaddress = "127.0.0.1";
		const uint32_t qosServerCount = 3;
		QosSocket qosServers[qosServerCount];
		for (uint32_t i = 0; i < qosServerCount; i++)
		{
			QosSocket& socket = qosServers[i];
			socket.CreateConnection(ipaddress, std::to_string(port + i), false);
			socket.SetNonBlockingMode();
			socket.PrintSocketOptions();
			socket.StartMeasuringQos();
		}

		HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		while (!(GetAsyncKeyState(VK_ESCAPE) & 1))
		{
			ClearScreen(consoleHandle, qosServerCount + 4);
			printf("Press Escape to exit\n");
			printf("IP Address:port \t Packets sent \t packets lost \t Average ping \n");
			for (uint32_t i = 0; i < qosServerCount; i++)
			{
				uint32_t packetSent = qosServers[i].GetPacketsSent();
				uint32_t packetLost = qosServers[i].GetPacketsLost();
				uint32_t averagePing = qosServers[i].GetAveragePing();
				printf("%s:%i \t\t %i \t\t %i \t\t %i \n",  qosServers[i].GetAddress().c_str(), std::stoi(qosServers[i].GetPort()), packetSent, packetLost, averagePing);
			}
			printf("\n\n");
			Sleep(10);
		}

		//for (auto socket : qosServers)	//Required copy constructor for QosSocket
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

	SimpleSocket authSocket;
	if ( !authSocket.CreateConnection(authUrl, std::to_string(authPort)) )
	{
		//Cleanup
		printf("Error creating connection");
		return -1;
	}

	//Get Authentication token
	const char * clientID = "45f67935-9286-4ba0-8b3b-70228e727ca2";
	const char * clientSecret = "1eba4dac53c33ae135fe7b2a839eb30aec964f75";
	
	uint32_t tokenLength = MAX_PAYLOADSIZE;
	char authToken[MAX_PAYLOADSIZE];
	GetAuthToken(authSocket, clientID, clientSecret, authToken, tokenLength);
	
	//Connect to API server
	const int apiPort = 12705;
	const char* apiUrl = "api.rcr.experiments.fireteam.net";

	SimpleSocket apiSocket;
	if (!apiSocket.CreateConnection(apiUrl, std::to_string(apiPort)))
	{
		//Cleanup
		printf("Error creating connection");
		return -1;
	}

	//Authorize
	if (!apiSocket.Send(static_cast<uint16_t>(APIMessageType::Authorize), authToken, tokenLength))
	{
		return -1;
	}

	//Get servers for Qos measurements

	//Can not use std container, as copy constructor is deleted for thread\mutex class
	//Defining move constructor and using move semantics doesn't help.
	QosSocket *qosServers;
	uint32_t qosServerCount;
	GetQosServerNames(apiSocket, qosServers, qosServerCount);

	//Start measuring
	for (uint32_t i = 0; i < qosServerCount; i++)
	{
		qosServers[i].StartMeasuringQos();
	}

	//Output measurements to screen
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	while (!(GetAsyncKeyState(VK_ESCAPE) & 1))
	{
		ClearScreen(consoleHandle, qosServerCount + 4);
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
		Sleep(10);
	}

	//Cleanup
	for( uint32_t i = 0; i < qosServerCount; i++)
	{
		qosServers[i].StopMeasuring();
	}

	WSACleanup();
    return 0;
}

