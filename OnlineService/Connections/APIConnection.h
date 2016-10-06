//-------------------------------------------------------------------------
//
// File: APIConnection.h
//
// Wrap call to API server to get QosServers
//
//--------------------------------------------------------------------------

#pragma once
#include "SimpleConnection.h"
#include "QosConnection.h"

//Message types for Api Server 
enum class APIMessageType : UINT16
{
	Error = 0,
	Authorize = 1, 
	GetServers = 2,
	GetServerResponse = 3,
};

class APIConnection : public SimpleConnection
{
public:
	bool GetQosServer(QosConnection*& qosServers, uint32_t& qosServerCount) const;
};

