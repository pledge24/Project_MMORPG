#include "Core/P1GameInstance.h"

#include "Game/Data/Generated/P1DesignDataVersion.h"
#include "Utils/LogCategory.h"

UP1GameInstance::UP1GameInstance()
{
}

void UP1GameInstance::Init()
{
    Super::Init();

    // 빌드에 들어간 기획 데이터의 버전을 남긴다. 서버가 기동할 때 남기는 버전과 대조할 수 있다.
    UE_LOG(LogP1System, Log, TEXT("기획 데이터 버전 %s"), P1DesignData::Version);
}
