//-------------------------------------------------------------------------
//
// File: SimpleConnection.h
//
// Connection class wraps socket calls and manages the lifetime of the socket
//
//--------------------------------------------------------------------------

#pragma once
#include "..\stdafx.h"

class SimpleConnection
{
protected:
	SOCKET				connectedSocket;
	struct addrinfo		addr;

	std::string			ipAddress;
	std::string			port;

#pragma pack(push, 1)
	struct SimpleHeader
	{
		uint32_t		payloadSize;
		uint16_t		msgType;
	};
#pragma pack(pop)

public:
	SimpleConnection();
	virtual ~SimpleConnection();

	bool CreateConnection(const std::string& url, const std::string& connectionPort, bool tcpip = true);

	bool Send(uint16_t msgType) const;
	bool Send(uint16_t msgType, const msgpack::sbuffer& buffer) const;
	bool Send(uint16_t msgType, const std::vector<char>& buffer) const;
	bool Receive(uint16_t& msgType, std::vector<char>& buffer) const;

	bool SetNonBlockingMode() const;

	void PrintError(const char * msg) const;

	const std::string& GetAddress() const
	{
		return ipAddress;
	}

	const std::string& GetPort() const
	{
		return port;
	}
};