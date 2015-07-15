// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FPluginHelpers
{
public:
	/**
	 * Returns the contents of the specified template file.
	 * @param TemplateFileName - name of the file with full path
	 * @param OutFileContents - contents of given file
	 * @param OutFailReason - fail reason text if something goes wrong
	 * @return true if the file was loaded, false otherwise
	 */
	static bool ReadTemplateFile(const FString& TemplateFileName, FString& OutFileContents, FText& OutFailReason);

	/**
	 * Create a .build.cs file for a plugin.
	 * @param NewBuildFileName - filename with full path
	 * @param PluginName - name of the plugin this file belongs to
	 * @param OutFailReason - fail reason text if something goes wrong
	 * @param TemplateType - empty by default; can be Blank, Basic or Advanced
	 * @param InPrivateDependencyModuleNames - Array of module names that should be included in this plugin. If it's empty, basic modules will be included (see definition of this function).
	 * @return true if file was created, false otherwise
	 */
	static bool CreatePluginBuildFile(const FString& NewBuildFileName, const FString& PluginName, FText& OutFailReason, FString TemplateType = FString(""), TArray<FString> InPrivateDependencyModuleNames = TArray<FString>());

	/**
	 * Create plugin header file.
	 * @param FolderPath - filename with full path
	 * @param PluginName - name of the plugin this file belongs to
	 * @param OutFailReason - fail reason text if something goes wrong
	 * @param TemplateType - empty by default; can be Blank, Basic or Advanced
	 * @return true if file was created, false otherwise
	 */
	static bool CreatePluginHeaderFile(const FString& FolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType = FString(""));

	/**
	 * Create plugin style files (source and header).
	 * @param PrivateFolderPath - Private folder path for source file
	 * @param PublicFolderPath - Public folder path for header file
	 * @param PluginName - name of the plugin this file belongs to
	 * @param OutFailReason - fail reason text if something goes wrong
	 * @param TemplateType - empty by default; can be Blank, Basic or Advanced
	 * @return true if file was created, false otherwise
	 */
	static bool CreatePluginStyleFiles(const FString& PrivateFolderPath, const FString& PublicFolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType = FString(""));

	/**
	 * Create plugin editor mode files (source and header).
	 * @param PrivateFolderPath - Private folder path for source file
	 * @param PublicFolderPath - Public folder path for header file
	 * @param PluginName - name of the plugin this file belongs to
	 * @param bUsesToolkits - whether the editor mode uses toolkits
	 * @param bIncludeSampleUI - whether the toolkit includes sample UI if enabled
	 * @param OutFailReason - fail reason text if something goes wrong
	 * @param TemplateType - empty by default; can be Blank, Basic or Advanced
	 * @return true if file was created, false otherwise
	 */
	static bool CreatePluginEdModeFiles(const FString& PrivateFolderPath, const FString& PublicFolderPath, const FString& PluginName, bool bUsesToolkits, bool bIncludeSampleUI, FText& OutFailReason, FString TemplateType = FString(""));

	/**
	 * Create plugin source file.
	 * @param FolderPath - filename with full path
	 * @param PluginName - name of the plugin this file belongs to
	 * @param OutFailReason - fail reason text if something goes wrong
	 * @param TemplateType - empty by default; can be Blank, Basic or Advanced
	 * @param bMakeEditorMode - Whether we are making cpp file for plugin with Editor Mode
	 * @param bUsesToolkits - Whether the Editor Mode is using toolkits
	 * @return true if file was created, false otherwise
	 */
	static bool CreatePluginCPPFile(const FString& FolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType = FString(""), bool bMakeEditorMode = false, bool bUsesToolkits = false);

	/**
	 * Create plugin UI commands file.
	 * @param FolderPath - filename with full path
	 * @param PluginName - name of the plugin this file belongs to
	 * @param OutFailReason - fail reason text if something goes wrong
	 * @param TemplateType - empty by default; can be Blank, Basic or Advanced
	 * @return true if file was created, false otherwise
	 */
	static bool CreateCommandsFiles(const FString& FolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType = FString(""));

	/**
	 * Create plugin private precompiled header file.
	 * @param FolderPath - filename with full path
	 * @param PluginName - name of the plugin this file belongs to
	 * @param OutFailReason - fail reason text if something goes wrong
	 * @param TemplateType - empty by default; can be Blank, Basic or Advanced
	 * @return true if file was created, false otherwise
	 */
	static bool CreatePrivatePCHFile(const FString& FolderPath, const FString& PluginName, FText& OutFailReason, FString TemplateType = FString(""));

	/**
	 * Create Blueprint Function library class file.
	 * @param PrivateFolderPath - Private folder path for source file
	 * @param PublicFolderPath - Public folder path for header file
	 * @param PluginName - name of the plugin this file belongs to
	 * @param OutFailReason - fail reason text if something goes wrong
	 * @return true if file was created, false otherwise
	 */
	static bool CreateBlueprintFunctionLibraryFiles(const FString& PrivateFolderPath, const FString& PublicFolderPath, const FString& PluginName, FText& OutFailReason);

	/**
	 * Make string that will register editor mode.
	 * @param EdModeName The name of the editor mode
	 * @param bUsesToolkits Whether the editor mode uses toolkits
	 * @return - string which will be used to register editor mode in PluginModule.cpp (template) or empty string if there is no mode
	 */
	static FString MakeRegisterEdModeString(const FString& EdModeName, bool bUsesToolkits)
	{
		if (EdModeName.IsEmpty())
		{
			return FString("");
		}

		return FString::Printf(TEXT("FEditorModeRegistry::Get().RegisterMode<F%s>(F%s::EM_%sId, LOCTEXT(\"%sName\", \"%s\"), FSlateIcon(), %s);"), *EdModeName, *EdModeName, *EdModeName, *EdModeName, *EdModeName, bUsesToolkits ? TEXT("true") : TEXT("false"));
	}

	/**
	 * Make string that will unregister editor mode.
	 * @param EdModeName The name of the editor mode
	 * @return - string which will be used to unregister editor mode in PluginModule.cpp (template) or empty string if there is no mode
	 */
	static FString MakeUnRegisterEdModeString(const FString& EdModeName)
	{
		if (EdModeName.IsEmpty())
		{
			return FString("");
		}

		return FString::Printf(TEXT("FEditorModeRegistry::Get().UnregisterMode(F%s::EM_%sId);"), *EdModeName, *EdModeName);
	}
};