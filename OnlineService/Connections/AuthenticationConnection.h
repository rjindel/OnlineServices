#pragma once
#include "SimpleConnection.h"

//Message types for Authentication server
enum class AuthMessageType : uint16_t
{
	Error = 0,
	AuthTicketRequest = 1, 
	AuthTicketResponse = 2
};

class AuthenticationConnection :
	public SimpleConnection
{
public:
	AuthenticationConnection();
	virtual ~AuthenticationConnection();

	//Get auth ticket from authentication server
	bool GetAuthToken(const char* clientID, const char* clientSecret, std::vector<char>& authToken);
};

