//#pragma once
//
//#include "CoreMinimal.h"
//#include "Structs/ItemData.h"
//#include "SlotData.generated.h"
//
//UENUM(BlueprintType)
//enum class ESlotType : uint8
//{
//    SLOT_TYPE_NONE = 0,
//    SLOT_TYPE_INVENTORY_GEAR = 1,
//    SLOT_TYPE_INVENTORY_CONSUMABLE = 2,
//    SLOT_TYPE_INVENTORY_MISC = 3
//};
//
///*-----------------
//      SlotData
//-----------------*/
//
//USTRUCT(BlueprintType)
//struct FSlotData
//{
//    GENERATED_BODY()
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite)
//    ESlotType SlotType;
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite)
//    int32 SlotId;
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite)
//    FItemData Item;
//
//    UPROPERTY(EditAnywhere, BlueprintReadWrite)
//    int32 Count;
//};