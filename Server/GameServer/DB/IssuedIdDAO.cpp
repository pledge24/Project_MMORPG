#include "Core/pch.h"
#include "DB/IssuedIdDAO.h"
#include "DB/DAOCommon.h"
#include "Utils/SnowflakeIdGenerator.h"

int64 IssuedIdDAO::GetLastIssuedId(DBConnection& conn, int32 nodeId)
{
    const int PARAMS = 3;
    const int COLS = 1;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int32 nodeId) : _nodeId(nodeId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _nodeUnit);
            dbBind.BindParam(1, _nodeCount);
            dbBind.BindParam(2, _nodeId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _lastIssuedId);
        }

        //~ Params
        // 노드 번호를 꺼내는 데 쓰는 비트 배치. SQL에 숫자로 적으면 발급기의 배치가 바뀔 때 함께 고칠 곳을 놓친다.
        int64 _nodeUnit = int64{ 1 } << SnowflakeIdGenerator::SEQUENCE_BITS;
        int64 _nodeCount = int64{ SnowflakeIdGenerator::MAX_NODE_ID } + 1;
        int64 _nodeId;

        //~ Cols
        int64 _lastIssuedId = 0;
    };


    // 다른 노드의 ID는 세지 않는다. 다른 노드의 시각을 따라가면 이 노드의 ID가 쓸데없이 앞선다.
    // 두 테이블을 끝까지 훑지만 부팅할 때 한 번만 돈다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SET NOCOUNT ON;

        DECLARE @node_unit BIGINT = (?);
        DECLARE @node_count BIGINT = (?);
        DECLARE @node_id BIGINT = (?);

        SELECT ISNULL(MAX(IssuedId), 0) AS LastIssuedId
        FROM (
            SELECT MAX(CharacterId) AS IssuedId
            FROM dbo.[Character]
            WHERE (CharacterId / @node_unit) % @node_count = @node_id

            UNION ALL

            SELECT MAX(ItemUid)
            FROM dbo.Item
            WHERE (ItemUid / @node_unit) % @node_count = @node_id
        ) AS Issued;
    )SQL");

    BindObject bindObject(dbBind, nodeId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    if (conn.Fetch() == false)
        throw DBError(__func__, "결과 행이 없다");

    return bindObject._lastIssuedId;
}
