// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ClientPacketHandler.h"
#include "P1GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#define SEND_PACKET(Pkt)														\
	SendBufferRef SendBuffer = ClientPacketHandler::MakeSendBuffer(Pkt);		\
	Cast<UP1GameInstance>(GWorld->GetGameInstance())->SendPacket(SendBuffer);