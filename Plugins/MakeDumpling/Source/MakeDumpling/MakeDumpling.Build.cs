// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class MakeDumpling : ModuleRules
{
	public MakeDumpling(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bEnableUndefinedIdentifierWarnings = false; // Ϊ���޸��������
        bEnableExceptions = true; // ����std��exception��û���õĻ��������ַ�����ת����ʱ�򣬷�Windowsƽ̨���б������

        string LibDir = Path.Combine(ModuleDirectory, "Zlib/lib");

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				"MakeDumpling/Public/",
				"MakeDumpling/Private/",
				"MakeDumpling/Zlib/"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"MakeDumpling/Public/",
                //"MakeDumpling/Private/",
                "MakeDumpling/Zlib/"
            }
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "zlib",
                "DesktopPlatform" 
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
