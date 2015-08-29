// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PluginBrowserPrivatePCH.h"
#include "PluginHelpers.h"
#include "GameProjectUtils.h"

#define LOCTEXT_NAMESPACE "PluginHelpers"

bool FPluginHelpers::ReadTemplateFile(const FString& TemplateFileName, FString& OutFileContents, FText& OutFailReason)
{
	const FString FullFileName = IPluginManager::Get().FindPlugin(TEXT("PluginBrowser"))->GetBaseDir() / TEXT("Templates") / TemplateFileName;
	if (FFileHelper::LoadFileToString(OutFileContents, *FullFileName))
	{
		return true;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("FullFileName"), FText::FromString(FullFileName));
	OutFailReason = FText::Format(LOCTEXT("FailedToReadTemplateFile", "Failed to read template file \"{FullFileName}\""), Args);
	return false;
}

bool FPluginHelpers::CreatePluginBuildFile(const FString& NewBuildFileName, const FString& PluginName, FText& OutFailReason, FString TemplateType, TArray<FString> InPrivateDependencyModuleNames)
{
	FString Template;

	FString TemplateFileName = TEXT("PluginModule.Build.cs.template");

	if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
	{
		return false;
	}

	TArray<FString> PublicIncludePathsNames;
	TArray<FString> PrivateIncludePathsNames;
	TArray<FString> PublicDependencyModuleNames;
	TArray<FString> PrivateDependencyModuleNames;
	TArray<FString> DynamicallyLoadedModuleNames;

	if (InPrivateDependencyModuleNames.Num() == 0)
	{
		PrivateDependencyModuleNames.Add("CoreUObject");
		PrivateDependencyModuleNames.Add("Engine");
		PrivateDependencyModuleNames.Add("Slate");
		PrivateDependencyModuleNames.Add("SlateCore");
	}
	else
	{
		PrivateDependencyModuleNames = InPrivateDependencyModuleNames;
	}
	
	FString FinalOutput = Template.Replace(TEXT("%PUBLIC_INCLUDE_PATHS_NAMES%"), *GameProjectUtils::MakeCommaDelimitedList(PublicIncludePathsNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PRIVATE_INCLUDE_PATHS_NAMES%"), *GameProjectUtils::MakeCommaDelimitedList(PrivateIncludePathsNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PUBLIC_DEPENDENCY_MODULE_NAMES%"), *GameProjectUtils::MakeCommaDelimitedList(PublicDependencyModuleNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PRIVATE_DEPENDENCY_MODULE_NAMES%"), *GameProjectUtils::MakeCommaDelimitedList(PrivateDependencyModuleNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%DYNAMICALLY_LOADED_MODULES_NAMES%"), *GameProjectUtils::MakeCommaDelimitedList(DynamicallyLoadedModuleNames), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *PluginName, ESearchCase::CaseSensitive);

	return GameProjectUtils::WriteOutputFile(NewBuildFileName, FinalOutput, OutFailReason);
}

bool FPluginHelpers::CreatePluginHeaderFile(const FString& FolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType)
{
	FString Template;

	FString TemplateFileName = TEXT("PluginModule.h.template");

	if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
	{
		return false;
	}

	TArray<FString> PublicHeaderIncludes;

	FString	FinalOutput = Template.Replace(TEXT("%PUBLIC_HEADER_INCLUDES%"), *GameProjectUtils::MakeIncludeList(PublicHeaderIncludes), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);

	return GameProjectUtils::WriteOutputFile(FolderPath / PluginName + TEXT(".h"), FinalOutput, OutFailReason);
}

bool FPluginHelpers::CreatePluginCPPFile(const FString& FolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType, bool bMakeEditorMode, bool bUsesToolkits)
{
	FString Template;

	FString TemplateFileName = TEXT("PluginModule.cpp.template");

	if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
	{
		return false;
	}

	TArray<FString> PublicHeaderIncludes;
	FString RegisterModeString;
	FString UnRegisterModeString;

	if (bMakeEditorMode)
	{
		FString EdModeName = PluginName + TEXT("EdMode");
		PublicHeaderIncludes.Add(FString::Printf(TEXT("%s.h"), *(EdModeName)));
		RegisterModeString = MakeRegisterEdModeString(EdModeName, bUsesToolkits);
		UnRegisterModeString = MakeUnRegisterEdModeString(EdModeName);
	}

	FString	FinalOutput = Template.Replace(TEXT("%PUBLIC_HEADER_INCLUDES%"), *GameProjectUtils::MakeIncludeList(PublicHeaderIncludes), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);

	FinalOutput = FinalOutput.Replace(TEXT("%REGISTER_EDMODE%"), *RegisterModeString, ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("%UNREGISTER_EDMODE%"), *UnRegisterModeString, ESearchCase::CaseSensitive);

	return GameProjectUtils::WriteOutputFile(FolderPath / PluginName + TEXT(".cpp"), FinalOutput, OutFailReason);
}

bool FPluginHelpers::CreatePrivatePCHFile(const FString& FolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType)
{
	FString Template;
	FString TemplateFileName = TEXT("PluginModulePCH.h.template");

	if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
	{
		return false;
	}

	TArray<FString> PublicHeaderIncludes;

	FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);

	return GameProjectUtils::WriteOutputFile(FolderPath / PluginName + TEXT("PrivatePCH.h"), FinalOutput, OutFailReason);
}

bool FPluginHelpers::CreateBlueprintFunctionLibraryFiles(const FString& PrivateFolderPath, const FString& PublicFolderPath, const FString& PluginName, FText& OutFailReason)
{
	{
		FString Template;
		FString TemplateFileName = TEXT("BlueprintFunctionLibrary.h.template");

		if (!ReadTemplateFile(TemplateFileName, Template, OutFailReason))
		{
			return false;
		}

		FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);

		if (!GameProjectUtils::WriteOutputFile(PublicFolderPath / PluginName + TEXT("BPLibrary.h"), FinalOutput, OutFailReason))
		{
			return false;
		}
	}

	{
		FString Template;
		FString TemplateFileName = TEXT("BlueprintFunctionLibrary.cpp.template");

		if (!ReadTemplateFile(TemplateFileName, Template, OutFailReason))
		{
			return false;
		}

		FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);

		if (!GameProjectUtils::WriteOutputFile(PrivateFolderPath / PluginName + TEXT("BPLibrary.cpp"), FinalOutput, OutFailReason))
		{
			return false;
		}
	}
	

	return true;
}

bool FPluginHelpers::CreatePluginStyleFiles(const FString& PrivateFolderPath, const FString& PublicFolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType)
{
	{
		// Make Style.h file first

		FString Template;
		FString TemplateFileName = TEXT("PluginStyle.h.template");

		if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
		{
			return false;
		}

		TArray<FString> PublicHeaderIncludes;

		FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);

		if (!GameProjectUtils::WriteOutputFile(PublicFolderPath / PluginName + TEXT("Style.h"), FinalOutput, OutFailReason))
		{
			return false;
		}
	}

	{
		// Now .cpp file

		FString Template;

		FString TemplateFileName = TEXT("PluginStyle.cpp.template");

		if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
		{
			return false;
		}

		TArray<FString> PublicHeaderIncludes;

		FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);

		if (!GameProjectUtils::WriteOutputFile(PrivateFolderPath / PluginName + TEXT("Style.cpp"), FinalOutput, OutFailReason))
		{
			return false;
		}
	}

	return true;
	
}

bool FPluginHelpers::CreatePluginEdModeFiles(const FString& PrivateFolderPath, const FString& PublicFolderPath, const FString& PluginName, bool bUsesToolkits, bool bIncludeSampleUI, FText& OutFailReason, FString TemplateType /*= FString("")*/)
{
	FString EdModeName = PluginName +  + TEXT("EdMode");

	{
		// Make EdMode.h file first

		FString Template;
		FString TemplateFileName = TEXT("EdMode.h.template");

		if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
		{
			return false;
		}

		FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);
		FinalOutput = FinalOutput.Replace(TEXT("%MODE_NAME%"), *EdModeName, ESearchCase::CaseSensitive);

		if (!GameProjectUtils::WriteOutputFile(PublicFolderPath / EdModeName + TEXT(".h"), FinalOutput, OutFailReason))
		{
			return false;
		}
	}

	{
		// Now .cpp file

		FString Template;

		FString TemplateFileName = TEXT("EdMode.cpp.template");

		if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
		{
			return false;
		}

		FString UsesToolkit = bUsesToolkits ? TEXT("true") : TEXT("false");
		FString BeginComment = bUsesToolkits ? TEXT("") : TEXT("/*");
		FString EndComment = bUsesToolkits ? TEXT("") : TEXT("*/");

		FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);
		FinalOutput = FinalOutput.Replace(TEXT("%MODE_NAME%"), *EdModeName, ESearchCase::CaseSensitive);
		FinalOutput = FinalOutput.Replace(TEXT("%BEGIN_BLOCK_COMMENT%"), *BeginComment, ESearchCase::CaseSensitive);
		FinalOutput = FinalOutput.Replace(TEXT("%END_BLOCK_COMMENT%"), *EndComment, ESearchCase::CaseSensitive);
		FinalOutput = FinalOutput.Replace(TEXT("%USES_TOOLKIT%"), *UsesToolkit, ESearchCase::CaseSensitive);


		if (!GameProjectUtils::WriteOutputFile(PrivateFolderPath / EdModeName + TEXT(".cpp"), FinalOutput, OutFailReason))
		{
			return false;
		}
	}

	// Make toolkit files if necessary
	if (bUsesToolkits)
	{
		{
			FString Template;
			FString TemplateFileName = TEXT("Toolkit.h.template");

			if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
			{
				return false;
			}

			FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);
			FinalOutput = FinalOutput.Replace(TEXT("%MODE_NAME%"), *EdModeName, ESearchCase::CaseSensitive);

			if (!GameProjectUtils::WriteOutputFile(PublicFolderPath / EdModeName + TEXT("Toolkit.h"), FinalOutput, OutFailReason))
			{
				return false;
			}
		}

		{
			FString Template;
			FString TemplateFileName = TEXT("Toolkit.cpp.template");

			if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
			{
				return false;
			}

			FString SampleUITemplate = TEXT("");

			if (bIncludeSampleUI)
			{
				TemplateFileName = TEXT("ToolkitUI.template");

				if (!ReadTemplateFile(TemplateFileName, SampleUITemplate, OutFailReason))
				{
					return false;
				}
			}

			FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);
			FinalOutput = FinalOutput.Replace(TEXT("%MODE_NAME%"), *EdModeName, ESearchCase::CaseSensitive);
			FinalOutput = FinalOutput.Replace(TEXT("%TOOLKIT_SAMPLE_UI%"), *SampleUITemplate, ESearchCase::CaseSensitive);

			if (!GameProjectUtils::WriteOutputFile(PrivateFolderPath / EdModeName + TEXT("Toolkit.cpp"), FinalOutput, OutFailReason))
			{
				return false;
			}
		}
	}

	return true;
}

bool FPluginHelpers::CreateCommandsFiles(const FString& FolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType)
{
	{
		// Make Commands.h file first
		FString Template;

		FString TemplateFileName = TEXT("PluginCommands.h.template");

		if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
		{
			return false;
		}

		TArray<FString> PublicHeaderIncludes;
		FString ButtonLabel = TEXT("BUTTON LABEL");

		FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);
		FinalOutput = FinalOutput.Replace(TEXT("%PLUGIN_BUTTON_LABEL%"), *PluginName, ESearchCase::CaseSensitive);

		if (!GameProjectUtils::WriteOutputFile(FolderPath / PluginName + TEXT("Commands.h"), FinalOutput, OutFailReason))
		{
			return false;
		}
	}

	{
		// Now .cpp file
		FString Template;

		FString TemplateFileName = TEXT("PluginCommands.cpp.template");

		if (!ReadTemplateFile(TemplateType.IsEmpty() ? TemplateFileName : TemplateType / TemplateFileName, Template, OutFailReason))
		{
			return false;
		}

		TArray<FString> PublicHeaderIncludes;
		FString ButtonLabel = TEXT("BUTTON LABEL");

		FString	FinalOutput = Template.Replace(TEXT("%PLUGIN_NAME%"), *PluginName, ESearchCase::CaseSensitive);
		FinalOutput = FinalOutput.Replace(TEXT("%PLUGIN_BUTTON_LABEL%"), *PluginName, ESearchCase::CaseSensitive);

		if (!GameProjectUtils::WriteOutputFile(FolderPath / PluginName + TEXT("Commands.cpp"), FinalOutput, OutFailReason))
		{
			return false;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE