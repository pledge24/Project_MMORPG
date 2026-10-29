// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"

/** Interaction 설정(Config.xlsx 시트 'Interaction')에서 클라이언트로 가는 값. */
namespace P1DesignData::Interaction
{
    /** NPC를 조준해 상호작용할 수 있는 평면 거리(cm). 서버는 여기에 여유를 더해 판정한다. 0보다 크다 */
    inline constexpr float InteractionRange = 300.0f;
}
