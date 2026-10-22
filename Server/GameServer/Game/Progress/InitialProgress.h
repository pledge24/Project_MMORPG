#pragma once

struct PlayerProgress;

/**
 * 새 캐릭터의 초기 진행.
 *
 * 아직 게임에 입장한 적이 없는 캐릭터가 첫 입장에서 갖는 진행을 기획 데이터로 정한다.
 */
namespace InitialProgress
{
    /**
     * progress의 직업으로 시작 맵, 위치, 방향, 골드와 1레벨의 가득 찬 HP와 MP를 채운다. 누적 경험치는 0이다.
     * 직업의 레벨 표가 없으면 아무것도 바꾸지 않고 false.
     */
    bool Fill(OUT PlayerProgress& progress);
}
