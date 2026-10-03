#pragma once

#include "CoreMinimal.h"

/** 이동 동기화 컴포넌트가 맡는 쪽이다. 스포너가 컴포넌트를 붙일 때 정한다. */
enum class EP1MoveSyncMode : uint8
{
    /** 서버가 보낸 위치로 화면 위치를 끌어당긴다. 회전은 서 있을 때만 서버 Yaw로 맞춘다. 다른 플레이어다. */
    RemotePlayer,

    /** 서버가 보낸 위치로 화면 위치를 끌어당긴다. 회전은 언제나 서버 Yaw로 맞춘다. 몬스터다. */
    RemoteMonster,

    /** 입력으로 움직이고 이동 패킷을 보낸다. 내 플레이어다. */
    MyPlayer,
};
