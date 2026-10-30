// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class KeyboardGuitar : ModuleRules
{
	public KeyboardGuitar(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UMG" });
		PrivateDependencyModuleNames.AddRange(new string[] { "InputCore", "EnhancedInput", "Slate", "SlateCore" });

		// 模块内部使用 "Instrument/GuitarDefinition.h" 这样的路径。
		PrivateIncludePaths.Add(ModuleDirectory);
	}
}
