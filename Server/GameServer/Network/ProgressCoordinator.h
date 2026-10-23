#pragma once
#include "Network/SaveGate.h"

struct PlayerSaveData;

/**
 * 진행 저장과 입장 불러오기의 순서를 맞추는 조율자.
 *
 * 같은 계정의 저장이 끝나기 전에는 그 계정의 진행을 불러오지 않도록 지킨다.
 * 접속 종료, 로비 복귀, 같은 계정의 새 로비 접속, 입장 요청이 저장과 불러오기에 닿을 때는 모두 이 클래스를 거친다.
 */
class ProgressCoordinator
{
public:
    /** 계정 번호로 고른 DB 큐에 잡을 넣는다. 같은 계정의 잡은 넣은 순서대로 돌아야 한다. */
    using PushDBJob = function<void(int64 userId, CallbackType job)>;
    /** 저장 사본 하나를 DB에 쓴다. DB 잡 안에서 부른다. 실패는 예외로 알려도 된다. */
    using SaveProgress = function<void(const PlayerSaveData& data)>;
    /** delayMs(ms) 뒤에 job을 돌리도록 예약한다. 룸 큐가 아닌 곳에서 돌아야 한다. */
    using ScheduleTimer = function<void(uint64 delayMs, CallbackType job)>;

    ProgressCoordinator(SaveGate& saveGate, PushDBJob pushDBJob, SaveProgress saveProgress, ScheduleTimer scheduleTimer);
    /** 운영 코드의 DB 큐, 진행 저장소, 타이머 큐를 쓰는 조율자를 만든다. saveGate보다 먼저 지운다. */
    static unique_ptr<ProgressCoordinator> CreateForServer(SaveGate& saveGate);

    //~ 진입점
public:
    /**
     * 세션이 끊길 때 IOCP 스레드가 부른다. 입장 전이면 player는 nullptr.
     * 계정 등록(UnregisterUser)을 지우기 전에 불러야 한다.
     */
    void OnDisconnected(int64 userId, const PlayerRef& player);
    /**
     * 로비 복귀를 받은 핸들러(IOCP 스레드)가 부른다. 접속 종료와 같은 절차로 player를 룸에서 빼고 저장한 뒤,
     * 이 계정의 DB 잡 안에서 afterSave를 부른다. 저장할 것이 없으면 저장 없이 부른다.
     */
    void OnReturnToLobby(int64 userId, const PlayerRef& player, CallbackType afterSave);
    /**
     * 같은 계정의 새 로비 접속이 앞 세션을 밀어내기 직전에 로비 접속 잡(DB 스레드)이 부른다.
     * 입장 전의 세션이면 replacedPlayer는 nullptr.
     */
    void OnDuplicateConnection(int64 userId, const PlayerRef& replacedPlayer);
    /**
     * 게임 입장 창구가 부른다. 어느 스레드에서 불러도 된다. 이 계정의 저장이 끝난 뒤 DB 잡 안에서 load.run을 부른다.
     * PARKED_LOAD_TIMEOUT_MS 안에 저장이 끝나지 않거나 이미 기다리는 입장이 있으면 load.reject를 부른다.
     */
    void RequestEnter(int64 userId, SaveGate::ParkedLoad load);

    /** 접속 종료 저장을 기다리는 입장 요청의 상한(ms). 넘기면 입장을 거절한다. */
    static constexpr uint64 PARKED_LOAD_TIMEOUT_MS = 5000;

    //~ 룸 큐
public:
    /** 끊긴 플레이어를 room에서 빼고 저장한다. 그사이 소속 룸이 바뀌었으면 그 룸의 큐로 넘긴다. room의 큐 위에서만 부른다. */
    void LeaveRoomAndSave(const RoomRef& room, const PlayerRef& player);
    /**
     * 어느 룸에도 없는 끊긴 플레이어를 저장한다.
     * 룸 이동 중에 끊긴 플레이어가 들어갈 룸에 들어가지 못했을 때 쓴다.
     */
    void SaveLeftPlayer(const PlayerRef& player, PlayerSaveData saveData);

    //~ 공통
private:
    /** 접속 종료와 로비 복귀가 함께 쓰는 퇴장과 저장. 저장 대기를 걸고, 저장할 것이 없으면 곧바로 푼다. */
    void LeaveGameAndSave(int64 userId, const PlayerRef& player);
    /** 저장하지 않기로 한 계정의 대기를 풀고, 맡겨 둔 불러오기가 있으면 DB 큐에 넣는다. */
    void ReleaseHold(int64 userId);

    SaveGate& _saveGate;
    PushDBJob _pushDBJob;
    SaveProgress _saveProgress;
    ScheduleTimer _scheduleTimer;
};
