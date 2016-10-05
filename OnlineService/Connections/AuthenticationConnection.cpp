#include "..\stdafx.h"
#include "AuthenticationConnection.h"
#include "..\Utils.h"

AuthenticationConnection::AuthenticationConnection()
{
}


AuthenticationConnection::~AuthenticationConnection()
{
}

//Get auth ticket from authentication server
//authsocket is a socket that is already connect to the authentication server
//clientID and clientSecret are null terminated string in hex form
//The returned authToken is a byte array of tokenLength. This is still "message packed", however there is no need to unpack this as it is needs to be 'packed' before sending to the API Server
bool AuthenticationConnection::GetAuthToken(const char* clientID, const char* clientSecret, std::vector<char>& authToken)
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

	if (!Send(static_cast<uint16_t>(AuthMessageType::AuthTicketRequest), streamBuffer))
	{
		printf("Error sending Authentication token");
		return false;
	}

	uint16_t messageType = 0;
	if(!Receive(messageType, authToken))
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
