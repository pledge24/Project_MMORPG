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
            "P1/Characters",
            "P1/Combat",
            "P1/Core",
            "P1/Data",
            "P1/Entities",
            "P1/Equipment",
            "P1/Inventory",
            "P1/Network",
            "P1/Online",
            "P1/UI",
            "P1/Utils",
            "P1/World",
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
