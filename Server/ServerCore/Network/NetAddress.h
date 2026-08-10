#pragma once

/*--------------
	NetAddress
---------------*/

class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(string ip, uint16 port);

	SOCKADDR_IN& GetSockAddr();
	string GetIPAddress();
	uint16 GetPort();

public:
	static IN_ADDR IPToAddress(const char* ip);

private:
	SOCKADDR_IN _sockAddr = {};
};

