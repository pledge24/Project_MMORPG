// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ClientPacketHandler.h"
#include "P1GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Types.h"
#include "InGamePlayerController.h"

#define SEND_PACKET(Pkt)															\
	SendBufferRef SendBuffer = ClientPacketHandler::MakeSerializedPacket(Pkt);		\
	Cast<UP1GameInstance>(GWorld->GetGameInstance())->SendPacket(SendBuffer);
