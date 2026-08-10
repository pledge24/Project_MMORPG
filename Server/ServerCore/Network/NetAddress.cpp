#include "pch.h"
#include "NetAddress.h"

/*--------------
	NetAddress
---------------*/

NetAddress::NetAddress(SOCKADDR_IN sockAddr) : _sockAddr(sockAddr)
{
}

NetAddress::NetAddress(string ip, uint16 port)
{
	::memset(&_sockAddr, 0, sizeof(_sockAddr));
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr = IPToAddress(ip.c_str());
	_sockAddr.sin_port = ::htons(7777);
}

SOCKADDR_IN& NetAddress::GetSockAddr()
{
	return _sockAddr;
}

string NetAddress::GetIPAddress()
{
	char buffer[100];
	int32 len = static_cast<int32>(sizeof(buffer) / sizeof(char));
	::inet_ntop(AF_INET, &_sockAddr.sin_addr, buffer, len);

	return string(buffer);
}

uint16 NetAddress::GetPort()
{
	return ::ntohs(_sockAddr.sin_port);
}

IN_ADDR NetAddress::IPToAddress(const char* ip)
{
	IN_ADDR address;
	::inet_pton(AF_INET, ip, &address);
	return address;
}

