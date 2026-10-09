#pragma once
#include "Network/SaveGate.h"

struct PlayerSaveData;

/**
 * 접속 종료 저장과 입장 불러오기의 순서를 맞춘다. 「저장이 끝나기 전에는 같은 계정을 불러오지 않는다」를 이 클래스가 지킨다.
 * 접속 종료, 중복 로그인, 입장 요청은 함수 하나씩만 부른다. 저장 대기를 걸고 푸는 곳은 이 클래스 밖에 없다.
 * DB 큐, 저장 게이트, 진행 저장소, 만료 타이머를 주입받는다. 테스트는 DB 잡과 타이머를 쌓아 두고 원하는 순서로 돌린다.
 * 운영 코드는 전역 포인터 GProgressCoordinator가 가리키는 객체 하나만 쓴다.
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

    //~ 진입점
    /**
     * 세션의 접속 종료(GameSession::OnDisconnected)에서 IOCP 스레드가 부른다. player는 입장 전이면 nullptr.
     * 계정 등록(GameSessionManager::UnregisterUser)을 지우기 전에 불러야 한다. 반대로 하면 그 사이에 온 새 로그인이
     * 대기 없이 입장해 저장 전의 진행을 불러온다. 룸에 들어간 적이 있는 플레이어는 그 룸 큐에서 퇴장과 저장을 시작한다.
     * 룸에 들어간 적이 없으면 저장하지 않는다. 큐에 남은 첫 룸 입장도 그 플레이어를 저장하지 않고 뺀다.
     */
    void OnDisconnected(int64 userId, const PlayerRef& player);
    /**
     * 같은 계정의 새 로그인이 기존 세션을 밀어낼 때, 밀어내기 전에 부른다. 로그인 잡(DB 스레드)이 부른다.
     * replacedPlayer는 밀려나는 세션의 플레이어이고 입장 전이면 nullptr. 플레이어가 있으면 그 세션의 접속 종료까지 입장을 막는다.
     */
    void OnDuplicateLogin(int64 userId, const PlayerRef& replacedPlayer);
    /**
     * 입장 요청(C_ENTER_GAME)에서 IOCP 스레드가 부른다. load는 DB 잡 안에서 진행을 불러오고 응답까지 보낸다.
     * 저장 대기가 없으면 load를 곧바로 DB 큐에 넣는다. 대기 중이면 저장이 끝난 뒤에 돌리고, PARKED_LOAD_TIMEOUT_MS 안에
     * 끝나지 않거나 이미 기다리는 입장이 있으면 load 대신 reject를 부른다. 만료로 거절해도 대기는 저장이 끝날 때까지 남는다.
     */
    void RequestEnter(int64 userId, CallbackType load, CallbackType reject);

    //~ 룸 큐
    /**
     * 끊긴 플레이어를 room에서 빼고, 룸이 저장 사본을 내주면 DB 큐에 저장을 넣는다. room의 큐 위에서만 부른다.
     * 접속 종료와, 룸 이동 중에 끊긴 플레이어를 이어 받은 Room::EnterPlayer가 부른다.
     * 저장 잡은 끝나면 실패해도 저장 대기를 풀고, 기다리던 불러오기를 그 자리에서 돌린다.
     */
    void LeaveRoomAndSave(const RoomRef& room, const PlayerRef& player);

    /** 접속 종료 저장을 기다리는 입장 요청의 상한(ms). 넘기면 입장을 거절한다. */
    static constexpr uint64 PARKED_LOAD_TIMEOUT_MS = 5000;

private:
    /** 저장하지 않기로 한 계정의 대기를 풀고, 맡겨 둔 불러오기가 있으면 DB 큐에 넣는다. */
    void ReleaseHold(int64 userId);

    SaveGate& _saveGate;
    PushDBJob _pushDBJob;
    SaveProgress _saveProgress;
    ScheduleTimer _scheduleTimer;
};

/** 세션, 패킷 핸들러, 룸이 부르는 조율자. 테스트는 주입한 조율자로 바꿔 끼우고 끝나면 되돌린다. */
extern ProgressCoordinator* GProgressCoordinator;
