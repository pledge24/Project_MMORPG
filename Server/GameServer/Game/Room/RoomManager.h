#pragma once

USING_SHARED_PTR(Room);

/**
 * 게임 서버의 모든 룸을 가진 관리자.
 *
 * 서버가 뜰 때 맵마다 채널 수만큼 룸을 만들어 두고, 룸을 번호나 맵과 채널로 찾아 주는 창구가 된다.
 * 룸 목록은 부팅 뒤로 바뀌지 않으므로 어느 스레드에서 조회해도 된다.
 */
class RoomManager
{
public:
    RoomManager() = default;
    ~RoomManager() = default;

    /**
     * 맵 표의 맵마다 channelCount개의 룸을 만들어 시작하고 등록한다. 워커 스레드를 띄우기 전에 한 번만 부른다.
     * 하나라도 실패하면 false이고, 그때는 서버를 띄우지 않는다.
     */
    bool CreateAllRooms(int32 channelCount);
    /**
     * 맵 표의 맵마다 channelCount개의 룸을 만들기만 하고 시작하지 않는다. 맵 번호 순서, 그 안에서 채널 순서다.
     * 배치 데이터가 없는 맵이 있으면 빈 목록이다.
     */
    static vector<RoomRef> MakeRooms(int32 channelCount);

    /** 등록되지 않은 번호면 nullptr. */
    RoomRef FindRoom(int32 roomId) const;
    /** 그 맵의 그 채널(1부터) 룸. 없으면 nullptr. */
    RoomRef FindRoom(int32 mapId, int32 channel) const;
    /** 그 맵의 룸을 채널 순서로 돌려준다. 맵이 없으면 빈 목록. */
    vector<RoomRef> GetChannelRooms(int32 mapId) const;
    /**
     * 그 맵에서 인원이 가장 적은 채널의 룸. 입장과 맵 간 이동이 들어갈 룸을 고를 때 쓴다. 맵이 없으면 nullptr.
     * 인원은 각 룸 큐가 바꾸는 값을 읽은 순간의 것이라, 동시에 고른 입장끼리는 같은 룸에 갈 수 있다.
     */
    RoomRef SelectRoom(int32 mapId) const;
    /** 등록된 룸 전부. 서버 종료 절차가 룸 큐를 비울 때 쓴다. */
    vector<RoomRef> GetAllRooms() const;

private:
    /** 테스트가 시작하지 않은 룸을 등록한다. */
    friend struct RoomManagerTestAccess;

    /** 룸을 번호와 맵의 채널 목록에 함께 등록한다. 채널은 등록 순서대로 1부터다. */
    void AddRoom(const RoomRef& room);

    unordered_map<int32, RoomRef> _rooms; // <RoomId, RoomRef>
    /** 맵 번호 → 채널 순서의 룸. 인덱스 0이 1채널이다. */
    map<int32, vector<RoomRef>> _channelRooms;
};
