// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class KeyboardGuitarEditorTarget : TargetRules
{
	public KeyboardGuitarEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("KeyboardGuitar");
	}
}
