#include "Game/World/P1Portal.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "Network/P1PacketSender.h"
#include "Utils/LogCategory.h"

const FName AP1Portal::RangeComponentName(TEXT("PortalCollision"));

void AP1Portal::BeginPlay()
{
    Super::BeginPlay();

    TInlineComponentArray<UPrimitiveComponent*> Components(this);
    UPrimitiveComponent* const* Found = Components.FindByPredicate(
        [](const UPrimitiveComponent* Component) { return Component->GetFName() == RangeComponentName; });
    if (Found == nullptr)
    {
        UE_LOG(LogP1CharacterInteraction, Warning, TEXT("포털 %s에 %s 컴포넌트가 없다"), *GetName(), *RangeComponentName.ToString());
        return;
    }

    (*Found)->OnComponentBeginOverlap.AddUniqueDynamic(this, &AP1Portal::HandleBeginOverlap);
}

void AP1Portal::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 블루프린트는 BP_MyPlayer로 캐스트해 가렸다. 로컬 플레이어 컨트롤러가 조종하는 폰은 내 플레이어뿐이라 결과가 같다.
    // AI 컨트롤러도 단독 실행에서는 로컬로 치므로 플레이어 컨트롤러인지도 본다.
    const APawn* Pawn = Cast<APawn>(OtherActor);
    if (Pawn && Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled())
        RequestRoomTransfer();
}

void AP1Portal::RequestRoomTransfer()
{
    if (PortalId == 0)
        return;

    Protocol::C_ENTER_ROOM EnterRoomPkt;
    EnterRoomPkt.set_enter_type(Protocol::ENTER_TYPE_SAME_MAP_TRANSFER);
    EnterRoomPkt.set_portal_id(PortalId);
    FP1PacketSender::Send(this, EnterRoomPkt);
}
