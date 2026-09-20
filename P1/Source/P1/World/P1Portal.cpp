// Fill out your copyright notice in the Description page of Project Settings.


#include "World/P1Portal.h"
#include "P1.h"

// Sets default values
AP1Portal::AP1Portal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AP1Portal::BeginPlay()
{
	Super::BeginPlay();	
}

void AP1Portal::SendEnterRoomPacket()
{
    if (PortalId == 0)
        return;

    Protocol::C_ENTER_ROOM EnterRoomPkt; 
    {
        EnterRoomPkt.set_enter_type(Protocol::ENTER_TYPE_SAME_MAP_TRANSFER);
        EnterRoomPkt.set_portal_id(PortalId);

        SEND_PACKET(EnterRoomPkt);
    }
}


