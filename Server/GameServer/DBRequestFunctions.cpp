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
            WHERE user_id = (?)                                             \
            ORDER BY created_at                                             \
        ");

        Res res(dbBind, userId);

        if (dbBind.Execute() == false)
            throw runtime_error("fail to excute Query");

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        Protocol::S_LOGIN pkt;

        int32 records = 0;
        while(dbConn->Fetch())
        {
            records++;

            Protocol::CharacterOverview* overview = pkt.add_characters();

            wcout << "characterId: " << res.characterId << endl;
            wcout << "classId: " << res.classId << endl;
            wcout << "characterName: " << res.characterName << endl;
            wcout << "level: " << res.level << endl;

            overview->set_character_id(res.characterId);
            overview->set_class_(static_cast<Protocol::CharacterClass>(res.classId));
            overview->set_name(EncodingConverter::WCharToString(res.characterName));
            overview->set_level(res.level);
        }

        wcout << L"캐릭터 개수: " << records << endl;

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

    const int PARAMS = 3;
    const int COLS = 1;

    //struct Res
    //{
    //    Res(DBBind<PARAMS, COLS>& dbBind, const Protocol::CharacterOverview& character, int64 userId)
    //    {
    //        BindParam(dbBind, character, userId);
    //        BindCol(dbBind);
    //    }

    //    void BindParam(DBBind<PARAMS, COLS>& dbBind, const Protocol::CharacterOverview& character, int64 userId)
    //    {
    //        int32 classId = character.class_();
    //        //wstring name = EncodingConverter::StringToWString(character.name());
    //        WCHAR name[30] = L"루키스";

    //        dbBind.BindParam(0, name);
    //        dbBind.BindParam(1, userId);
    //        dbBind.BindParam(2, classId);
    //        dbBind.BindParam(3, name);
    //    }

    //    void BindCol(DBBind<PARAMS, COLS>& dbBind)
    //    {
    //        dbBind.BindCol(0, characterId);
    //    }

    //    int64 characterId;
    //};

    //DBConnection* dbConn = GDBConnectionPool->Pop();

    //try
    //{
    //    // 전달받은 이름이 중복인지 확인하고, 아니라면 INSERT한다.
    //    DBBind<PARAMS, COLS> dbBind(*dbConn, L"\
    //        BEGIN TRANSACTION;\
    //        \
    //        DECLARE @existing_character_id BIGINT;\
    //        SELECT @existing_character_id = character_id\
    //            FROM[dbo].[Characters]\
    //            WITH(UPDLOCK, HOLDLOCK)\
    //            WHERE character_name = (?);\
    //        IF @existing_character_id IS NULL\
    //            BEGIN\
    //                INSERT INTO[dbo].[Characters]([user_id], [class_id], [character_name])\
    //                OUTPUT INSERTED.character_id\
    //                VALUES(?, ?, ?)\
    //                COMMIT TRANSACTION;\
    //            END\
    //        ELSE\
    //            BEGIN\
    //                ROLLBACK TRANSACTION;\
    //            END\
    //    ");
    struct Res
    {
        Res(DBBind<PARAMS, COLS>& dbBind, const Protocol::CharacterOverview& character, int64 userId)
        {
            _userId = userId;
            _classId = character.class_();
            _name = EncodingConverter::StringToWString(character.name());

            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _userId);
            dbBind.BindParam(1, _classId);
            dbBind.BindParam(2, _name.c_str());
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, characterId);
        }

        /* param */
        int64 _userId;
        int32 _classId;
        wstring _name;

        /* col */
        int64 characterId;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 전달받은 이름이 중복인지 확인하고, 아니라면 INSERT한다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, L"\
        INSERT INTO[dbo].[Characters]([user_id], [class_id], [character_name])\
        OUTPUT INSERTED.character_id\
        VALUES(?, ?, ?)\
        ");

        Res res(dbBind, character, userId);

        if (dbBind.Execute() == false)
            throw wstring(L"캐릭터 생성 실패");

        dbConn->Fetch(); // character_id값 가져오기

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        Protocol::S_CREATE_CHARACTER pkt;
        pkt.set_success(true);
        pkt.set_characterid(res.characterId);
        SEND_PACKET(pkt);
    }
    catch (wstring& errMsg)
    {
        wstring cause = L"";
        if (dbConn->FindError(L"23000")/* 이름이 중복된 경우 */)
        {
            cause = L"이미 존재하는 캐릭터 이름입니다.";
        }
        else
        {
            cause = L"Internal Server Error";
        }

        wcout << L"오류 발생: " << errMsg << " " << cause << endl;

        Protocol::S_CREATE_CHARACTER pkt;
        pkt.set_success(false);
        pkt.set_cause(EncodingConverter::WCharToString(cause.c_str()));
        SEND_PACKET(pkt);
    }
    catch (exception& err)
    {
        wcout << L"표준 예외 발생: " << err.what() << endl;
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
            DELETE FROM [dbo].[Characters]                                      \
            WHERE [character_id] = (?)                                          \
        ");

        Res res(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw wstring(L"캐릭터 삭제 실패");

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        Protocol::S_DELETE_CHARACTER pkt;
        pkt.set_success(true);
        pkt.set_characterid(characterId);
        SEND_PACKET(pkt);

    }
    catch (wstring& errMsg)
    {

        wstring cause = L"";

        wcout << L"오류 발생: " << errMsg << " " << cause << endl;

        Protocol::S_DELETE_CHARACTER pkt;
        pkt.set_success(false);
        SEND_PACKET(pkt);
    }
    catch (exception& err)
    {
        wcout << L"표준 예외 발생: " << err.what() << endl;
    }

    GDBConnectionPool->Push(dbConn);
}
