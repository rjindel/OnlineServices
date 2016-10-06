//-------------------------------------------------------------------------
//
// File: SimpleConnection.cpp
//
// Connection class wraps socket calls and manages the lifetime of the socket
//
//--------------------------------------------------------------------------

#include "..\stdafx.h"
#include "SimpleConnection.h"

SimpleConnection::SimpleConnection() : connectedSocket(INVALID_SOCKET)
{
	memset(&addr, 0, sizeof(addr));
	addr.ai_family = AF_UNSPEC; //Allow IPv4 or IPv6
}

SimpleConnection::~SimpleConnection()
{
	if (connectedSocket != INVALID_SOCKET)
	{
		closesocket(connectedSocket);
	}
}

// Pass the port as string, so that getddrinfo can handle the conversion to network byte ordering
bool SimpleConnection::CreateConnection(const std::string& url, const std::string& connectionPort, bool tcpip)
{
	if (tcpip)
	{
		addr.ai_socktype = SOCK_STREAM;
		addr.ai_protocol = IPPROTO_TCP;
	}
	else
	{
		addr.ai_socktype = SOCK_DGRAM;
		addr.ai_protocol = IPPROTO_UDP;
	}

	addr.ai_flags = AI_CANONNAME;

	struct addrinfo *resultPtr;
	auto err = getaddrinfo(url.c_str(), connectionPort.c_str(), &addr, &resultPtr);
	std::shared_ptr<addrinfo> resultWrapper(resultPtr, freeaddrinfo);
	if (err != 0)
	{
		PrintError("Error getting address info");
		return false;
	}

	connectedSocket = socket(resultPtr->ai_family, resultPtr->ai_socktype, resultPtr->ai_protocol);
	if (connectedSocket == INVALID_SOCKET)
	{
		PrintError("Error connecting socket");
		return false;
	}
	
	err = connect(connectedSocket, resultPtr->ai_addr, resultPtr->ai_addrlen);
	if (err != 0)
	{
		PrintError("Error connecting");
		return false;
	}
	printf("Connected to %s\n", resultPtr->ai_canonname);

	ipAddress = url;
	port = connectionPort;

	return true;
}

//Send a packet of msgType with no payload
bool SimpleConnection::Send(uint16_t msgType) const
{
	SimpleHeader authHeader = { 0 };
	authHeader.msgType = msgType;

	auto err = send(connectedSocket, (char*)&authHeader, sizeof(SimpleHeader), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error sending header: ");
	}

	return true;
}

bool SimpleConnection::Send(uint16_t msgType, const msgpack::sbuffer& buffer) const
{
	SimpleHeader authHeader = { 0 };
	authHeader.msgType = msgType;
	authHeader.payloadSize = buffer.size();

	auto err = send(connectedSocket, (char*)&authHeader, sizeof(SimpleHeader), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error sending header: ");
		return false;
	}	

	err = send(connectedSocket, buffer.data(), buffer.size(), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error sending data: ");
		return false;
	}

	return true;
}

bool SimpleConnection::Send(uint16_t msgType, const std::vector<char>& buffer) const
{
	SimpleHeader authHeader = { 0 };
	authHeader.msgType = msgType;
	authHeader.payloadSize = buffer.size();

	auto err = send(connectedSocket, (char*)&authHeader, sizeof(SimpleHeader), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error sending header: ");
	}

	err = send(connectedSocket, buffer.data(), buffer.size(), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error sending data: ");
		return false;
	}

	return true;
}

bool SimpleConnection::Receive(uint16_t& msgType, std::vector<char>& buffer) const
{
	SimpleHeader header = { 0 };
	auto err = recv(connectedSocket, (char*)&header, sizeof(header), 0);
	if (err == SOCKET_ERROR)
	{
		PrintError("Error receiving header: ");
		return false;
	}
	msgType = header.msgType;

	uint32_t bufferSize = header.payloadSize;
	buffer.resize(bufferSize);
	bufferSize = recv(connectedSocket, buffer.data(), bufferSize, 0);
	if (bufferSize == SOCKET_ERROR)
	{
		PrintError("Error receiving data: ");
		return false;
	}
	
	if (bufferSize < 0)
	{
		printf("Error: no data received\n");
		return false;
	}

	if (msgType == 0)
	{
		msgpack::object_handle objectHandle = msgpack::unpack(buffer.data(), buffer.size());
		msgpack::object object = objectHandle.get();
		std::cout << object << std::endl;
		return false;
	}

	return true;
}

bool SimpleConnection::SetNonBlockingMode() const
{
	u_long mode = 1;
	if (ioctlsocket(connectedSocket, FIONBIO, &mode))
	{
		PrintError("Error setting non blocking mode: %s");
		return false;
	}

	return true;
}

void SimpleConnection::PrintError(const char * msg) const
{
	auto err = WSAGetLastError();
	if (err != 0)
	{
		const int MAX_STRING_SIZE = 256;
		char buffer[MAX_STRING_SIZE];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, buffer, MAX_STRING_SIZE, nullptr);
		printf("%s: %s", msg, buffer);
	}
}