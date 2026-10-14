#pragma once

/**
 * DB 스레드의 작업 루프.
 *
 * DB 큐 하나에서 잡을 꺼내 차례로 실행하고, 잡 하나의 실패가 DB 스레드를 멈추지 않게 한다.
 */
namespace DBWorker
{
    /** DB 스레드 하나의 본체다. 큐가 멈추면 남은 잡을 다 돌린 뒤 돌아온다. */
    void Run(DBQueueRef dbQueue);

    /**
     * 잡 하나를 실행하고, 끝까지 돌면 true. 잡이 던진 예외는 종류를 가리지 않고 받아 로그만 남기고 false.
     * 예외로 끝난 잡의 응답은 여기서 보내지 않으므로, 클라이언트 응답은 잡이 스스로 보낸다.
     */
    bool RunJob(const JobRef& job);
}
