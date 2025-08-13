#pragma once

class DBManager
{
public:
    DBManager();
    ~DBManager();

    void                            Init(int32 dbQueueCount);
    void                            Clear();

    DBQueueRef                      GetDBQueue(int32 index);
    DBQueueRef                      GetDBQueueFromId(int64 id);
    
    int32                           GetDBQueueCount() { return _dbQueueCount; }

private:
    vector<DBQueueRef>              _dbQueueList;
    int32                           _dbQueueCount = 0;
    std::hash<int64>                _hashGenerator;
};

