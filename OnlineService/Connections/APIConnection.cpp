//-------------------------------------------------------------------------
//
// File: APIConnection.cpp
//
// Wrap call to API server to get QosServers
//
//--------------------------------------------------------------------------

#include "..\stdafx.h"
#include "APIConnection.h"

bool APIConnection::GetQosServer(QosConnection*& qosServers, uint32_t& qosServerCount) const
{
	if (!Send(static_cast<uint16_t>(APIMessageType::GetServers)))
	{
		return false;
	}
	
	uint16_t messageType = 0;
	std::vector<char> payload;
	if (!Receive(messageType, payload))
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
		int qosPort = serverData.get<1>();

		if (!qosServers[i].CreateConnection(servername, std::to_string(qosPort), false))
		{
			return false;
		}
		
		if (!qosServers[i].SetNonBlockingMode())
		{
			printf("Error setting no blocking mode");
			return false;
		}
	}
	
	return true;
}