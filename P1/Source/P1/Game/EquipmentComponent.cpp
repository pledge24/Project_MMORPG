// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/EquipmentComponent.h"
#include "Structs/ItemData.h"

// Sets default values for this component's properties
UEquipmentComponent::UEquipmentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UEquipmentComponent::Init(const Protocol::ObjectInfo& ObjectInfo)
{
    //const int EquipmentSubtypeN = 6;
    //Equipments.SetNum(EquipmentSubtypeN + 1);

    //for (const auto& item : ObjectInfo.player_info().equipment())
    //{
    //    // FItemData를 Equipments에 저장
    //    Equipments[item.slot_id()] = item.item_id();
    //}
}

void UEquipmentComponent::Equip(int32 SlotId)
{
    // 1. 장착되어 있는 아이템이 있으면 인벤으로 던져준다.

    // 2. 장착
    //Equipments[SlotId]
}

void UEquipmentComponent::UnEquip(int32 SlotId)
{
    // 검의 경우 강제 탈착 불가능
}


// Called when the game starts
void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

