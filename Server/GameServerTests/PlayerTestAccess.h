#pragma once
#include "Game/Entities/Player.h"

/**
 * 테스트가 준비 단계에서 플레이어의 상태(레벨, 직업, 골드 등)를 직접 채운다. Player가 friend로 연다.
 * 운영 코드에는 이 상태를 바로 쓰는 경로가 없다. 레벨은 OnLevelUp이, 직업과 골드는 불러오기와 거래가 쓴다.
 */
struct PlayerTestAccess
{
    static Protocol::PlayerInfo& PlayerInfo(Player& player) { return *player._playerInfo; }
    static Protocol::Possession& Possession(Player& player) { return *player._possession; }
    /** 세션 없이 만든 플레이어에 계정 번호를 준다. 운영 코드는 Init이 세션에서 읽는다. */
    static void SetUserId(Player& player, int64 userId) { player._userId = userId; }
};
