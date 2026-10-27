#include "Core/pch.h"
#include "Lobby/LobbyRequests.h"
#include "Lobby/AccountDAO.h"
#include "Lobby/CharacterListDAO.h"
#include "Lobby/Data/LobbyData.h"

namespace
{
    // 새 캐릭터 행의 레벨. Character.Level 열의 기본값(DF_Character_Level)과 같아야 한다.
    constexpr int32 NEW_CHARACTER_LEVEL = 1;

    Protocol::LobbyResult ToLobbyResult(CharacterRejection rejection)
    {
        switch (rejection)
        {
        case CharacterRejection::DUPLICATE_NAME: return Protocol::LOBBY_RESULT_NAME_TAKEN;
        case CharacterRejection::NO_EMPTY_SLOT: return Protocol::LOBBY_RESULT_NO_EMPTY_SLOT;
        }

        return Protocol::LOBBY_RESULT_INTERNAL_ERROR;
    }

    // 세션이 로비에 들어갈 때마다 보내는 목록. characterSlotCount는 계정 행의 값이다.
    Protocol::S_CHARACTER_LIST MakeCharacterList(DBConnection& conn, int64 accountId, int32 characterSlotCount)
    {
        vector<CharacterListRow> characters;
        CharacterListDAO::LoadCharacterList(conn, accountId, OUT characters);

        Protocol::S_CHARACTER_LIST listPkt;
        for (CharacterListRow& character : characters)
        {
            Protocol::LobbyCharacter* lobbyCharacter = listPkt.add_characters();
            lobbyCharacter->set_character_id(character.characterId);
            lobbyCharacter->set_name(std::move(character.name));
            lobbyCharacter->set_character_class(static_cast<Protocol::CharacterClass>(character.classId));
            lobbyCharacter->set_level(character.level);
        }

        // 클라이언트는 이 값으로 슬롯을 보여 주고 생성 버튼을 막는다. 생성 쿼리가 막는 한도와 같은 계정 행의 값이다.
        listPkt.set_character_slot_count(characterSlotCount);

        // 캐릭터 생성 화면은 이 직업만 보여 준다. 클라이언트에 따로 목록을 두면 로비 표를 바꿀 때 클라이언트도 패치해야 한다.
        for (Protocol::CharacterClass characterClass : LobbyData::GetCreatableClasses())
            listPkt.add_creatable_classes(characterClass);

        return listPkt;
    }
}

Protocol::S_CHARACTER_LIST LobbyRequests::ConnectLobby(DBConnection& conn, int64 accountId)
{
    // 이 월드에 처음 들어온 계정이면 계정 행을 만든다. 캐릭터 생성은 이 행이 있어야 한다.
    const int32 characterSlotCount = AccountDAO::RecordLogin(conn, accountId);

    return MakeCharacterList(conn, accountId, characterSlotCount);
}

Protocol::S_CHARACTER_LIST LobbyRequests::ReturnToLobby(DBConnection& conn, int64 accountId)
{
    // 로비 복귀는 로그인이 아니므로 마지막 로그인 시각을 바꾸지 않는다.
    return MakeCharacterList(conn, accountId, AccountDAO::LoadCharacterSlotCount(conn, accountId));
}

Protocol::S_CREATE_CHARACTER LobbyRequests::CreateCharacter(DBConnection& conn, int64 accountId, int64 characterId, const Protocol::C_CREATE_CHARACTER& pkt)
{
    Protocol::S_CREATE_CHARACTER createPkt;
    const CreateCharacterResult result = CharacterListDAO::CreateCharacter(conn, accountId, characterId, pkt.name(), pkt.character_class());
    if (result.rejection.has_value())
    {
        createPkt.set_result(ToLobbyResult(result.rejection.value()));
        return createPkt;
    }

    // 클라이언트는 응답의 캐릭터를 목록에 그대로 더한다. 요청할 때 이름과 직업을 적어 둘 필요가 없다.
    Protocol::LobbyCharacter* character = createPkt.mutable_character();
    character->set_character_id(result.characterId);
    character->set_name(pkt.name());
    character->set_character_class(pkt.character_class());
    character->set_level(NEW_CHARACTER_LEVEL);
    createPkt.set_result(Protocol::LOBBY_RESULT_SUCCESS);
    return createPkt;
}

Protocol::S_DELETE_CHARACTER LobbyRequests::DeleteCharacter(DBConnection& conn, int64 accountId, int64 characterId)
{
    Protocol::S_DELETE_CHARACTER deletePkt;
    deletePkt.set_character_id(characterId);

    // 지운 행이 없으면 이 계정의 캐릭터가 아니거나 이미 지운 캐릭터다.
    const bool deleted = CharacterListDAO::DeleteCharacter(conn, accountId, characterId);
    deletePkt.set_result(deleted ? Protocol::LOBBY_RESULT_SUCCESS : Protocol::LOBBY_RESULT_CHARACTER_NOT_FOUND);
    return deletePkt;
}

Protocol::LobbyResult LobbyRequests::SelectCharacter(DBConnection& conn, int64 accountId, int64 characterId)
{
    if (CharacterListDAO::OwnsCharacter(conn, accountId, characterId) == false)
        return Protocol::LOBBY_RESULT_CHARACTER_NOT_FOUND;

    return Protocol::LOBBY_RESULT_SUCCESS;
}
