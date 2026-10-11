#pragma once

/*------------------
	  DBManager
-------------------*/

/**
 * DB 큐 여러 개를 묶어 관리한다. 전역 객체 GDBManager 하나만 있다.
 * 큐마다 DB 스레드가 하나씩 붙는다(GameServer의 DBWorker::Run).
 */
class DBManager
{
public:
    DBManager();
    ~DBManager();

    /** 큐를 _dbQueueCount 만든다. 부르기 전에는 큐가 0개라 Get 함수를 쓰면 안 된다. */
    void                            Init(int32 dbQueueCount);
    void                            Clear();

    /** index는 0 이상 GetDBQueueCount() 미만이어야 한다. */
    DBQueueRef                      GetDBQueue(int32 index);
    /** 같은 id는 늘 같은 큐로 간다. 한 계정의 DB 작업을 순서대로 실행하려면 이 함수로 큐를 고른다. */
    DBQueueRef                      GetDBQueueFromId(int64 id);
    
    int32                           GetDBQueueCount() { return _dbQueueCount; }

private:
    vector<DBQueueRef>              _dbQueueList;
    int32                           _dbQueueCount = 0;
    std::hash<int64>                _hashGenerator;
};

