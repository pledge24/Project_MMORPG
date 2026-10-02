#include "Network/P1PacketSender.h"
#include "Network/P1ConnectionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/LogCategory.h"

void FP1PacketSender::Send(const UObject* WorldContext, SendBufferRef SendBuffer)
{
    // 게임 인스턴스는 종료 중에 월드가 없을 수 있으므로 자신을 넘기면 그대로 쓴다.
    // 그 밖에는 호출한 객체의 월드에서 찾는다. PIE 창이 여럿이어도 제 창의 세션으로 간다.
    const UGameInstance* GameInstance = Cast<UGameInstance>(WorldContext);
    if (GameInstance == nullptr && WorldContext != nullptr)
        GameInstance = UGameplayStatics::GetGameInstance(WorldContext);

    UP1ConnectionSubsystem* Connection = GameInstance ? GameInstance->GetSubsystem<UP1ConnectionSubsystem>() : nullptr;
    if (Connection == nullptr)
    {
        UE_LOG(LogP1Network, Warning, TEXT("패킷을 보낼 연결을 찾지 못함: %s"), *GetNameSafe(WorldContext));
        return;
    }

    Connection->Send(SendBuffer);
}
