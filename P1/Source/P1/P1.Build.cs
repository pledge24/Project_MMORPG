// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class P1 : ModuleRules
{
	public P1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Sockets", "Networking", "EnhancedInput", "UMG", "HTTP", "Json"});

		PrivateDependencyModuleNames.AddRange(new string[] { "ProtobufCore", "Slate", "SlateCore" });

        // 도메인 폴더는 등록하지 않는다. `#include`를 경로 한정으로 쓰게 해서 경계를 넘는
        // 참조가 include 줄에 드러나게 한다. 근거는 ADR-0005.
        PrivateIncludePaths.AddRange(new string[]
        {
            "P1/",
            // 생성물 폴더는 예외로 남긴다. protobuf 생성 코드가 서로를 `#include "Enum.pb.h"`
            // 형태로 부르는데 생성물은 손으로 고치지 않는다.
            "P1/Network",
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
