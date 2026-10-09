#pragma once

/**
 * 플레이어의 진행(용어집의 「진행」). 접속이 끊겨도 남는 상태이고, 입장할 때 불러오고 접속이 끊길 때 저장한다.
 * 불러오기와 저장이 이 구조체 하나를 쓴다. 한쪽에만 있는 필드를 두지 않는다(ADR-0012).
 * 살아 있는 Player가 아니라 사본이므로 DB 스레드가 읽고 써도 된다.
 */
struct PlayerProgress
{
    /** 캐릭터 번호, 직업, 이름, 레벨, 마지막 맵과 룸. */
    Protocol::PlayerInfo playerInfo;
    /** 마지막 위치. */
    Protocol::PosInfo posInfo;
    /** 경험치, 현재 HP와 MP, 공격력. */
    Protocol::StatInfo statInfo;
    /** 골드, 인벤토리, 착용 장비. */
    Protocol::Possession possession;
};
