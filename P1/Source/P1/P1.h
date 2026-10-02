// Copyright Epic Games, Inc. All Rights Reserved.

// 이 헤더는 쓰지 않는다. 새 코드에서 부르지 않는다.
// 게임 인스턴스, 컨트롤러, 패킷 핸들러처럼 서로 다른 도메인의 헤더를 한꺼번에 끌어오던 모듈 헤더다.
// 부르는 쪽이 무엇에 기대는지 include 줄에 드러나지 않아 docs/folder-structure.md 3.3의 의존 방향을
// 어기는 참조를 숨겼다. 그래서 #123에서 부르는 곳을 모두 없애고 필요한 헤더를 각자 직접 부르게 했다.
// 모듈 구현(P1.cpp)도 이 헤더 없이 돈다. 파일은 이력 확인용으로 남기고 include는 주석으로만 둔다.

#pragma once

// #include "CoreMinimal.h"

// #include "Network/ClientPacketHandler.h"
// #include "Core/P1GameInstance.h"
// #include "Kismet/GameplayStatics.h"
// #include "Engine/World.h"
// #include "GameFramework/Actor.h"
// #include "Utils/Types.h"
// #include "Core/P1InGamePlayerController.h"

// #include "Kismet/KismetMathLibrary.h"

// #include "Protocol.pb.h"

// 패킷 전송은 Network/P1PacketSender.h의 FP1PacketSender::Send를 쓴다.
