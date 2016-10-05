#pragma once
#include "..\stdafx.h"

class SimpleConnection
{
protected:
	SOCKET connectedSocket;
	struct addrinfo addr;

	std::string ipAddress;
	std::string port;

#pragma pack(push, 1)
	struct SimpleHeader
	{
		uint32_t payloadSize;
		uint16_t msgType;
	};
#pragma pack(pop)

public:
	SimpleConnection() : connectedSocket(INVALID_SOCKET) 
	{
		memset(&addr, 0, sizeof(addr));
		addr.ai_family = AF_UNSPEC; //Allow IPv4 or IPv6
	};
	virtual ~SimpleConnection();

	bool CreateConnection(std::string url, std::string port, bool tcpip = true);

	bool Send(uint16_t msgType);
	bool Send(uint16_t msgType, msgpack::sbuffer& buffer);
	bool Send(uint16_t msgType, const std::vector<char> buffer);
	bool Receive(uint16_t& msgType, std::vector<char>& buffer);

	void SetNonBlockingMode();

	void PrintError(const char * msg);

	std::string GetAddress()
	{
		return ipAddress;
	}

	std::string GetPort()
	{
		return port;
	}
};