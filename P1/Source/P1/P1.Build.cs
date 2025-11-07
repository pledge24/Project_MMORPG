// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class P1 : ModuleRules
{
	public P1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Sockets", "Networking", "EnhancedInput", "UMG", "HTTP", "Json"});

		PrivateDependencyModuleNames.AddRange(new string[] { "ProtobufCore", "Slate", "SlateCore" });

        PrivateIncludePaths.AddRange(new string[]
        {
            "P1/",
            "P1/Network/",
            "P1/Game/",
            "P1/Game/Objects",
            "P1/Game/Props",
            "P1/Game/Structs",
            "P1/Game/Widgets",
            "P1/Game/Components",
            "P1/Game/Subsystem",
            "P1/Game/Enums",
            "P1/Log",
            "P1/Login",
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
