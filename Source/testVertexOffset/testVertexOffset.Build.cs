// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class testVertexOffset : ModuleRules
{
	public testVertexOffset(ReadOnlyTargetRules Target) : base(Target)  
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "RenderCore", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
