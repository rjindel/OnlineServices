#include "..\stdafx.h"
#include "APIConnection.h"

bool APIConnection::GetQosServerNames(QosConnection*& qosServers, uint32_t& qosServerCount)
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
		int port = serverData.get<1>();

		if (!qosServers[i].CreateConnection(servername, std::to_string(port), false))
		{
			return false;
		}
		qosServers[i].SetNonBlockingMode();
	}
	
	return true;
}