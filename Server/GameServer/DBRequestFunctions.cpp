#include "pch.h"
#include "DBRequestFunctions.h"
#include "EncodingConverter.h"

void DBRequestFunctions::GetCharacterData(SessionRef session, int64 userId)
{
    cout << "GetCharacterData!" << endl;

    struct Res
    {
        Res(DBBind<1, 4>& dbBind, int64 userId)
        {
            BindParam(dbBind, userId);
            BindCol(dbBind);
        }

        void BindParam(DBBind<1, 4>& dbBind, int64 userId)
        {
            dbBind.BindParam(0, userId);
        }

        void BindCol(DBBind<1, 4>& dbBind)
        {
            dbBind.BindCol(0, characterId);
            dbBind.BindCol(1, classId);
            dbBind.BindCol(2, characterName);
            dbBind.BindCol(3, level);
        }

        int64 characterId;
        int32 classId;
        WCHAR characterName[100];
        int16 level;
    };
    
    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 유저의 캐릭터 기본 정보들을 가져온다.
        DBBind<1, 4> dbBind(*dbConn, L"                                     \
            SELECT character_id, class_id, character_name, level            \
            FROM [dbo].[Characters]                                         \
            WHERE deleted_at = NULL AND user_id = (?)                       \
            ORDER BY created_at                                             \
        ");

        Res res(dbBind, userId);

        if (dbBind.Execute() == false)
            throw runtime_error("fail to excute Query");

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        Protocol::S_LOGIN pkt;

        int records = dbConn->GetRowCount();
        wcout << L"캐릭터 개수: " << records << endl;
        for (int i = 0; i < records; i++)
        {
            Protocol::CharacterOverview* overview = pkt.add_characters();

            dbConn->Fetch();
            wcout << "characterId: " << res.characterId << endl;
            wcout << "classId: " << res.classId << endl;
            wcout << "characterName: " << res.characterName << endl;
            wcout << "level: " << res.level << endl;

            overview->set_character_id(res.characterId);
            overview->set_class_(static_cast<Protocol::CharacterClass>(res.classId));
            overview->set_name(EncodingConverter::WCharToString(res.characterName));
            overview->set_level(res.level);
        }

        pkt.set_success(true);
        SEND_PACKET(pkt);
    }
    catch (exception& err){
        wcout << L"캐릭터 정보 가져오기 실패: " << err.what() << endl;

        Protocol::S_LOGIN pkt;
        pkt.set_success(false);
        SEND_PACKET(pkt);
    }
    
    GDBConnectionPool->Push(dbConn);   
}

void DBRequestFunctions::CreateCharacter(SessionRef session, const Protocol::CharacterOverview& character, int64 userId)
{
    cout << "CreateCharacter!" << endl;

    struct Res
    {
        Res(DBBind<3, 1>& dbBind, const Protocol::CharacterOverview& character, int64 userId)
        {
            BindParam(dbBind, character, userId);
        }

        void BindParam(DBBind<3, 1>& dbBind, const Protocol::CharacterOverview& character, int64 userId)
        {
            int32 classId = character.class_();
            string name = character.name();

            dbBind.BindParam(0, userId);
            dbBind.BindParam(1, classId);
            dbBind.BindParam(2, EncodingConverter::StringToWString(name).c_str());
        }

        void BindCol(DBBind<3, 1>& dbBind)
        {
            dbBind.BindCol(0, characterId);
        }

        int64 characterId;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 유저의 캐릭터 기본 정보들을 가져온다.
        DBBind<3, 1> dbBind(*dbConn, L"                                                 \
            INSERT INTO [dbo].[Characters] ([user_id], [class_id], [character_name])    \
            OUTPUT INSERTED.character_id                                                \
            VALUES(?, ?, ?)                                                             \
        ");

        Res res(dbBind, character, userId);

        if (dbBind.Execute() == false)
            throw runtime_error("fail to excute Query");

        dbConn->Fetch(); // character_id값 가져오기

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        Protocol::S_CREATE_CHARACTER pkt;
        pkt.set_success(true);
        pkt.set_characterid(res.characterId);
        SEND_PACKET(pkt);
    }
    catch (exception& err)
    {
        wcout << "캐릭터 생성 실패: " << err.what() << endl;

        Protocol::S_CREATE_CHARACTER pkt;
        pkt.set_success(false);
        SEND_PACKET(pkt);
    }

    GDBConnectionPool->Push(dbConn);
}

void DBRequestFunctions::DeleteCharacter(SessionRef session, int64 characterId)
{
    cout << "DeleteCharacter!" << endl;

    struct Res
    {
        Res(DBBind<1, 0>& dbBind, int64 characterId)
        {
            BindParam(dbBind, characterId);
        }

        void BindParam(DBBind<1, 0>& dbBind, int64 characterId)
        {
            dbBind.BindParam(0, characterId);
        }
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 삭제할 캐릭터의 deleted_at을 일주일 뒤로 설정.
        DBBind<1, 0> dbBind(*dbConn, L"                                         \
            UPDATE [dbo].[Characters]                                           \
            SET [deleted_at] = DATEADD(DAY, 7, GETDATE())                       \
            WHERE [character_id] = (?)                                          \
        ");

        Res res(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw runtime_error("fail to excute Query");

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        Protocol::S_DELETE_CHARACTER pkt;
        pkt.set_success(true);
        SEND_PACKET(pkt);

    }
    catch (exception& err)
    {
        wcout << "캐릭터 삭제 실패: " << err.what() << endl;

        Protocol::S_DELETE_CHARACTER pkt;
        pkt.set_success(false);
        SEND_PACKET(pkt);
    }

    GDBConnectionPool->Push(dbConn);
}
