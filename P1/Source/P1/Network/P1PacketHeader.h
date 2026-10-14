#pragma once

#include "CoreMinimal.h"

/** 수신 스레드가 스트림에서 읽는 패킷 헤더. */
struct P1_API FP1PacketHeader
{
	FP1PacketHeader() : PacketSize(0), PacketID(0)
	{
	}

	FP1PacketHeader(uint16 PacketSize, uint16 PacketID) : PacketSize(PacketSize), PacketID(PacketID)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FP1PacketHeader& Header)
	{
		Ar << Header.PacketSize;
		Ar << Header.PacketID;
		return Ar;
	}

	/** 헤더를 포함한 패킷 전체의 바이트 수. */
	uint16 PacketSize;
	uint16 PacketID;
};
