#pragma once

/*--------------
	NetAddress
---------------*/

// SOCKADDR_IN을 감싼 값 타입. 복사해서 넘긴다.
class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(SOCKADDR_IN sockAddr);
	// 지금은 port 인자를 쓰지 않고 7777로 고정한다(TD-009).
	NetAddress(string ip, uint16 port);

	SOCKADDR_IN& GetSockAddr();
	string GetIPAddress();
	uint16 GetPort();

public:
	static IN_ADDR IPToAddress(const char* ip);

private:
	SOCKADDR_IN _sockAddr = {};
};

