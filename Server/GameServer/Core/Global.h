#pragma once

/**
 * 게임 서버의 전역 변수.
 * 포인터 전역은 ServerContext가 살아 있는 동안만 유효하다.
 */

extern atomic<int64> GNextItemUID;
extern class RoomManager* GRoomManager;

/** 세션과 계정에 걸린 시간 제한을 돌리는 큐. 룸 상태와 무관한 타이머를 룸 큐에 섞지 않으려고 따로 둔다. */
extern JobQueueRef GSessionJobQueue;

/** 로그인한 세션을 계정별로 기록한다. */
extern class GameSessionManager* GSessionManager;

/** 테스트는 주입한 조율자로 바꿔 끼우고 끝나면 되돌린다. */
extern class ProgressCoordinator* GProgressCoordinator;
