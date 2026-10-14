#pragma once

#include "CoreMinimal.h"

/** 이동 동기화 컴포넌트가 맡는 크리처의 종류. */
enum class EP1MoveSyncMode : uint8
{
    /** 다른 플레이어. 서버 위치로 화면 위치를 끌어당기고, 회전은 서 있을 때만 서버 Yaw로 맞춘다. */
    RemotePlayer,

    /** 몬스터. 서버 위치로 화면 위치를 끌어당기고, 회전은 언제나 서버 Yaw로 맞춘다. */
    RemoteMonster,

    /** 내 플레이어. 입력으로 움직이고 이동 패킷을 보낸다. */
    MyPlayer,
};
