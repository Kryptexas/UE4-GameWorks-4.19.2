// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Object.h"
#include "PluginDescriptor.h"
#include "PluginDescriptorObject.generated.h"

/* Struct that holds basic Editor mode parameters*/
USTRUCT()
struct FEdModeSettings
{
	GENERATED_USTRUCT_BODY()

	FEdModeSettings(){}
	~FEdModeSettings(){}

	// Name of your editor mode
	// It will be filled automatically if you specify plugin name
	UPROPERTY(EditAnywhere, Category = "EdModeSettings")
	FString Name;

	// Should this mode use toolkit?
	// Toolkits can specify UI that will appear in "Modes" tab (next to Foliage, Landscape etc)
	UPROPERTY(EditAnywhere, Category = "EdModeSettings")
	bool bUsesToolkits;

	// Do you want this mode to include very basic UI that demonstrates editor interaction and undo/redo functions usage?
	UPROPERTY(EditAnywhere, Category = "EdModeSettings", meta = (editcondition = "bUsesToolkits"))
	bool bIncludeSampleUI;

	/*
	*	Make string that will register mode.
	*	@return - string which will be used to register mode in PluginModule.cpp (template) or empty string if there is no mode
	*/
	FString MakeRegisterModeString()
	{
		if (Name.IsEmpty())
		{
			return FString("");
		}

		return FString::Printf(TEXT("FEditorModeRegistry::Get().RegisterMode<F%s>(F%s::EM_%sId, LOCTEXT(\"%sName\", \"%s\"), FSlateIcon(), %s);"), *Name, *Name, *Name, *Name, *Name, bUsesToolkits ? TEXT("true") : TEXT("false"));
	}

	/*
	*	Make string that will register mode.
	*	@return - string which will be used to unregister mode in PluginModule.cpp (template) or empty string if there is no mode
	*/
	FString MakeUnRegisterModeString()
	{
		if (Name.IsEmpty())
		{
			return FString("");
		}

		return FString::Printf(TEXT("FEditorModeRegistry::Get().UnregisterMode(F%s::EM_%sId);"), *Name, *Name);
	}
};

/**
*	We use this object to display plugin properties using details view.
*/
UCLASS()
class UPluginDescriptorObject : public UObject
{
	GENERATED_BODY()

	UPluginDescriptorObject(const FObjectInitializer& ObjectInitializer);

public:	

	/** Should new plugin be considered as 'Engine' plugin?
	If so, it will be put under Engine/Plugins directory. */
	UPROPERTY(EditAnywhere, Category = "CreationOptions")
	bool bIsEnginePlugin;

	// Should the project files be regenerated when the plugin is created?
	UPROPERTY(EditAnywhere, Category = "CreationOptions")
	bool bRegenerateProjectFiles;

	// Should the plugin folder be shown in the file system when the plugin is created?
	UPROPERTY(EditAnywhere, Category = "CreationOptions")
	bool bShowPluginFolder;

	// Do you want to create Editor mode within this plugin?
	UPROPERTY(EditAnywhere, Category = "CreationOptions", AdvancedDisplay)
	bool bMakeEditorMode;

	UPROPERTY(EditAnywhere, Category = "CreationOptions", AdvancedDisplay, meta = (editcondition = "bMakeEditorMode"))
	FEdModeSettings EditorModeSettings;

	// Do you want to create blueprint function library within this plugin?
	// Blueprint function library lets you create your own, static blueprint nodes
	UPROPERTY(EditAnywhere, Category = "CreationOptions", AdvancedDisplay)
	bool bAddBPLibrary;

	// Modify this array to set Private Dependencies for this plugin
	// Do it only if you know what you're doing! Missing dependencies can cause build failure!
	UPROPERTY(EditAnywhere, Category = "CreationOptions", AdvancedDisplay)
	TArray<FString> PrivateDependencyModuleNames;

	/** Descriptor version number */
	UPROPERTY(VisibleAnywhere, Category = "PluginDescription")
	int32 FileVersion;

	/** Version number for the plugin.  The version number must increase with every version of the plugin, so that the system
	can determine whether one version of a plugin is newer than another, or to enforce other requirements.  This version
	number is not displayed in front-facing UI.  Use the VersionName for that. */
	UPROPERTY(VisibleAnywhere, Category = "PluginDescription")
	int32 Version;

	/** Name of the version for this plugin.  This is the front-facing part of the version number.  It doesn't need to match
	the version number numerically, but should be updated when the version number is increased accordingly. */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	FString VersionName;

	/** Friendly name of the plugin */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	FString FriendlyName;

	/** Description of the plugin */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	FString Description;

	/** The category that this plugin belongs to */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	FString Category;

	/** The company or individual who created this plugin.  This is an optional field that may be displayed in the user interface. */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	FString CreatedBy;

	/** Hyperlink URL string for the company or individual who created this plugin.  This is optional. */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	FString CreatedByURL;

	/** Documentation URL string. */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	FString DocsURL;

	/** Support URL/email for this plugin. Email addresses must be prefixed with 'mailto:' */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	FString SupportURL;

	/** List of all modules associated with this plugin */
	TArray<FModuleDescriptor> Modules;

	/** Can this plugin contain content? */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	bool bCanContainContent;

	/** Marks the plugin as beta in the UI */
	UPROPERTY(EditAnywhere, Category = "PluginDescription")
	bool bIsBetaVersion;

public:
	void FillDescriptor(FPluginDescriptor& OutDescriptor);

};
