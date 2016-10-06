#pragma once
#include "..\stdafx.h"

class SimpleConnection
{
protected:
	SOCKET			connectedSocket;
	struct addrinfo addr;

	std::string		ipAddress;
	std::string		port;

#pragma pack(push, 1)
	struct SimpleHeader
	{
		uint32_t	payloadSize;
		uint16_t	msgType;
	};
#pragma pack(pop)

public:
	SimpleConnection();
	virtual ~SimpleConnection();

	bool CreateConnection(const std::string& url, const std::string& port, bool tcpip = true);

	bool Send(uint16_t msgType);
	bool Send(uint16_t msgType, const msgpack::sbuffer& buffer);
	bool Send(uint16_t msgType, const std::vector<char>& buffer);
	bool Receive(uint16_t& msgType, std::vector<char>& buffer);

	bool SetNonBlockingMode();

	void PrintError(const char * msg);

	const std::string& GetAddress()
	{
		return ipAddress;
	}

	const std::string& GetPort()
	{
		return port;
	}
};