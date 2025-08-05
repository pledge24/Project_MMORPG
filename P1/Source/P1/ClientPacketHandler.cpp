#include "ClientPacketHandler.h"
#include "LoginModeBase.h"
#include "LoginWidget.h"
#include "P1.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{

	return false;
}

bool Handle_S_PONG(PacketSessionRef& session, Protocol::S_PONG& pkt)
{
	return false;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	// 클래스 열거형 -> 직업 이름으로 바꾸기 위한 맵
	TMap<Protocol::CharacterClass, FString> ClassMap = {
		{Protocol::CharacterClass::CLASS_TYPE_KNIGHT, FString(TEXT("Warrir"))}
		//
	};

	// 언리얼 엔진에서 사용할 수 있는 형식으로 변경
	TArray<FCharacterOverview> Characters;
	for (auto& Character : pkt.characters())
	{
		FCharacterOverview character;
		character.CharacterId = Character.character_id();
		character.CharacterClass = ClassMap[Character.class_()];
		character.CharacterName = UTF8_TO_TCHAR(Character.name().c_str());
		character.CharacterLevel = Character.level();

		Characters.Add(character);
	}

	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		if (ALoginModeBase* Mode = Cast<ALoginModeBase>(UGameplayStatics::GetGameMode(GameInstance)))
		{
			if (ULoginManager* Manager = Mode->GetLoginManager())
			{
				if (ULoginWidget* LoginWidget = Manager->GetLoginWidget())
				{
					LoginWidget->OnRecvCharacterOverviews(Characters);
				}
			}
		}
	}
	return true;
}

bool Handle_S_CREATE_CHARACTER(PacketSessionRef& session, Protocol::S_CREATE_CHARACTER& pkt) {

	return true;
}

bool Handle_S_DELETE_CHARACTER(PacketSessionRef& session, Protocol::S_DELETE_CHARACTER& pkt) {

	return true;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleSpawn(pkt);
	}

	return true;
}

bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		// TODO: 게임 종료? 로비로?(연결을 곧바로 끊을지 선택해야함)

	}

	return true;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleSpawn(pkt);
	}

	return true;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleDespawn(pkt);
	}

	return true;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleMove(pkt);
	}

	return false;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	auto Msg = pkt.msg();


	return true;
}

