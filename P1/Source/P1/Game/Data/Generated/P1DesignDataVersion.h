// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"

/** DataBuilder가 생성한 기획 데이터의 버전. */
namespace P1DesignData
{
    /** 원본 해시로 만든 데이터 버전. 같은 원본이면 서버가 기동할 때 남기는 버전과 같다. */
    inline constexpr const TCHAR* Version = TEXT("49704b9922faeb67");
}
