#pragma once

class DBManager
{
public:
    DBManager();
    ~DBManager();

    void                            Init(int32 dbQueueCount);
    void                            Clear();

    DBQueueRef                      GetDBQueue(int32 index);
    DBQueueRef                      GetDBQueueFromId(int32 id);

private:
    vector<DBQueueRef>              _dbQueueList;
    int32                           _dbQueueCount = 0;
    std::hash<int32>                _hashGenerator;
};

