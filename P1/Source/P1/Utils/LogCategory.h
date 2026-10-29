#pragma once
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogP1CharacterComp, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1CharacterInteraction, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1CharacterInventory, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1QuickSlots, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1Entity, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1UI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1Protobuf, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1Network, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1System, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1Lobby, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogP1Chat, Log, All);
// 에디터 모듈의 배치 추출 커맨드렛이 쓴다. 다른 모듈에서 쓰려면 모듈 밖으로 내보내야 한다.
P1_API DECLARE_LOG_CATEGORY_EXTERN(LogP1Placement, Log, All);
// 에디터 모듈의 크리처 추출 커맨드렛이 쓴다.
P1_API DECLARE_LOG_CATEGORY_EXTERN(LogP1CreatureCombat, Log, All);


