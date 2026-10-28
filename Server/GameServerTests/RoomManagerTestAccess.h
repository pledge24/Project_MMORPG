#pragma once
#include "Game/Room/RoomManager.h"
#include "Game/Room/Room.h"

/**
 * 테스트가 시작하지 않은 룸으로 룸 관리자를 채운다. RoomManager가 friend로 연다.
 * 운영 코드는 CreateAllRooms로만 룸을 만들고, 만들 때 룸을 시작해 룸 틱과 몬스터 스폰을 건다.
 */
struct RoomManagerTestAccess
{
    static void AddRoom(RoomManager& manager, const RoomRef& room) { manager._rooms.emplace(room->GetRoomId(), room); }
};

/**
 * 지금 설치된 지역 표의 행마다 시작하지 않은 룸을 만들어 등록한 룸 관리자.
 *
 * 이 객체가 사는 동안 GRoomManager가 된다. 룸을 찾는 운영 코드(게임 입장, 포털 이동, 리스폰)를 룸 틱 없이 돌린다.
 * Gamedata::Install 뒤에 만든다.
 */
class ScopedTestRoomManager
{
public:
    ScopedTestRoomManager()
    {
        for (const auto& [areaId, area] : Gamedata::GetAreas())
        {
            const AreaPlacement* placement = Gamedata::FindAreaPlacement(areaId);
            RoomManagerTestAccess::AddRoom(_manager, Room::Create(area, placement != nullptr ? *placement : AreaPlacement{}));
        }

        GRoomManager = &_manager;
    }

    ~ScopedTestRoomManager()
    {
        GRoomManager = nullptr;
    }

    /** 그 번호의 룸이 없으면 nullptr. */
    RoomRef FindRoom(int32 roomId) const { return _manager.FindRoom(roomId); }

private:
    RoomManager _manager;
};
