// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FunnyorDie : ModuleRules
{
	public FunnyorDie(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			
			// EOS
			"OnlineSubsystem",        
			"OnlineSubsystemEOS",     
			"OnlineSubsystemUtils"    
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });
		
		PublicIncludePaths.AddRange(new string[] { "FunnyorDie" });
	}
}
