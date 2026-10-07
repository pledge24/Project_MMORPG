#pragma once

/*--------------
	NetAddress
---------------*/

/**
 * 소켓 주소(SOCKADDR_IN) Wrapper 클래스(사실상 구조체).
 * 소켓 주소를 넘기면 복사해서 들고있는다.
 */
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

