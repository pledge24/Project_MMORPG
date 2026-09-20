// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Network/ClientPacketHandler.h"
#include "Core/P1GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Utils/Types.h"
#include "Core/P1InGamePlayerController.h"

#include "Kismet/KismetMathLibrary.h"

#include "Protocol.pb.h"

#define SEND_PACKET(Pkt)															\
	SendBufferRef SendBuffer = ClientPacketHandler::MakeSerializedPacket(Pkt);		\
	Cast<UP1GameInstance>(GWorld->GetGameInstance())->SendPacket(SendBuffer);
