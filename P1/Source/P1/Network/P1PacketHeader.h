#pragma once

#include "CoreMinimal.h"

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

	uint16 PacketSize;
	uint16 PacketID;
};
