#pragma once
#include "Game/Room/RoomManager.h"
#include "Game/Room/Room.h"
#include "TestAreaTables.h"

/**
 * 테스트가 시작하지 않은 룸으로 룸 관리자를 채운다. RoomManager가 friend로 연다.
 * 운영 코드는 CreateAllRooms로만 룸을 만들고, 만들 때 룸을 시작해 룸 틱과 몬스터 스폰을 건다.
 */
struct RoomManagerTestAccess
{
    /** 같은 맵의 룸은 넣은 순서대로 1채널부터다. */
    static void AddRoom(RoomManager& manager, const RoomRef& room) { manager.AddRoom(room); }
};

/**
 * 시작하지 않은 룸 하나. 그 맵의 지역은 설치된 기획 데이터에서 찾고, 경계는 원점을 중심으로 반경 halfExtent다.
 * 셀 크기를 따로 주지 않으면 룸의 모든 플레이어가 서로를 본다.
 */
inline RoomRef CreateTestRoom(int32 roomId, int32 mapId, float halfExtent, float cellSize = TestAreaTables::WIDE_CELL_SIZE)
{
    return Room::Create(roomId, 1, TestAreaTables::MakeMap(mapId, PlaneBounds::FromCenter(0.f, 0.f, halfExtent, halfExtent), cellSize));
}

/**
 * 지금 설치된 맵 표의 맵마다 채널 수만큼 시작하지 않은 룸을 만들어 등록한 룸 관리자.
 *
 * 이 객체가 사는 동안 GRoomManager가 된다. 룸을 찾는 운영 코드(게임 입장, 포털 이동, 리스폰, 채널 변경)를 룸 틱 없이 돌린다.
 * Gamedata::Install 뒤에 만든다. 맵마다 배치 데이터가 있어야 한다(TestAreaTables::AddArea가 넣는다).
 */
class ScopedTestRoomManager
{
public:
    explicit ScopedTestRoomManager(int32 channelCount = 1)
    {
        for (const RoomRef& room : RoomManager::MakeRooms(channelCount))
            RoomManagerTestAccess::AddRoom(_manager, room);

        GRoomManager = &_manager;
    }

    ~ScopedTestRoomManager()
    {
        GRoomManager = nullptr;
    }

    /** 그 맵의 그 채널 룸이 없으면 nullptr. */
    RoomRef FindRoom(int32 mapId, int32 channel = 1) const { return _manager.FindRoom(mapId, channel); }

private:
    RoomManager _manager;
};
