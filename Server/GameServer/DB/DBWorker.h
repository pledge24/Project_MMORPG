#pragma once

/**
 * DB 스레드 하나가 도는 루프. 큐마다 DB 스레드가 하나씩 붙는다(GameServer.cpp의 main이 띄운다).
 */
namespace DBWorker
{
    /** 잡을 꺼내 RunJob으로 실행한다. 큐가 멈추면 남은 잡을 다 돌린 뒤 끝난다. */
    void Run(DBQueueRef dbQueue);

    /**
     * 잡 하나를 실행한다. 잡이 끝까지 돌면 true.
     * 잡이 던진 예외는 종류를 가리지 않고 받아 로그를 남기고 false를 돌려준다. DB 스레드는 다음 잡을 계속 돌린다.
     * 잡이 클라이언트에 보낼 응답은 잡이 스스로 보내야 한다. 여기서 받은 예외에는 응답을 보내지 않는다.
     */
    bool RunJob(const JobRef& job);
}
