// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EngineVersion.h"

// These are the project file versions. Add a new one if the file format changes.
enum EProjectFileVersion
{
	VER_PROJECT_FILE_INVALID = 0,
	VER_PROJECT_FILE_FIRST = 1,
	VER_PROJECT_FILE_NAMEHASH = 2,
	VER_PROJECT_FILE_PLUGIN_UNIFICATION = 3,
	// !!!!!!!!!! IMPORTANT: Remember to also update LatestPluginDescriptorFileVersion in Plugins.cs (and Plugin system documentation) when this changes!!!!!!!!!!!
	// -----<new versions can be added before this line>-------------------------------------------------
	// - this needs to be the last line (see note below)
	VER_PROJECT_FILE_AUTOMATIC_VERSION_PLUS_ONE,
	VER_LATEST_PROJECT_FILE = VER_PROJECT_FILE_AUTOMATIC_VERSION_PLUS_ONE - 1
};

/**
 * Base class to hold info about a single project or plugin
 */
class FProjectOrPluginInfo
{
public:
	struct EModuleType
	{
		enum Type
		{
			Runtime,
			RuntimeNoCommandlet,
			Developer,
			Editor,
			EditorNoCommandlet,
			// NOTE: If you add a new value, make sure to update the ToString() method below!

			Max
		};


		/**
		 * Converts an EModuleType::Type value to a string literal
		 *
		 * @param	The value to convert to a string
		 *
		 * @return	The string representation of this enum value
		 */
		inline static const TCHAR* ToString( const EModuleType::Type Value )
		{
			switch( Value )
			{
				case Runtime:
					return TEXT( "Runtime" );

				case RuntimeNoCommandlet:
					return TEXT( "RuntimeNoCommandlet" );

				case Developer:
					return TEXT( "Developer" );

				case Editor:
					return TEXT( "Editor" );

				case EditorNoCommandlet:
					return TEXT( "EditorNoCommandlet" );

				default:
					ensureMsgf( false, TEXT( "Unrecognized EModuleType value: %i" ), Value );
					return NULL;
			}
		}
	};


	struct FModuleInfo
	{
		/** Name of this module */
		FName Name;

		/** Type of module */
		EModuleType::Type Type;

		/** When should the module be loaded during the startup sequence?  This is sort of an advanced setting. */
		ELoadingPhase::Type LoadingPhase;

		/** List of allowed platforms */
		TArray<FString> WhitelistPlatforms;

		/** List of disallowed platforms */
		TArray<FString> BlacklistPlatforms;

		/** Module info constructor */
		FModuleInfo()
			: Name( NAME_None ),
			  Type( EModuleType::Runtime ),
			  LoadingPhase( ELoadingPhase::Default )
		{
		}
	};

	/** Name of the project or plugin, which is actually just the base name of the .uproject or .uplugin file.  Note that this is not guaranteed to
	    be globally unique!  Projects or plugins with the same name could exist in different directories. */
	FString Name;

	/** Path to the file that this project or plugin was loaded from */
	FString LoadedFromFile;

	/** Earliest phase of any module that this project or plugin contains */
	ELoadingPhase::Type EarliestModulePhase;


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Everything Below this line is serialized. If you add or remove properties, you must update
	// FProjectOrPlugin::SerializeToJSON and FProjectOrPlugin::DeserializeFromJSON
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	// @todo plugin: Versioning is kind of messy right now. Remove or consolidate some of these version numbers. We may also need a MinEngineVersion.

	/** The version of the file this project or plugin was loaded from */
	int32 FileVersion;

	/** Version number for the plugin.  The version number must increase with every version of the plugin, so that the system 
	    can determine whether one version of a plugin is newer than another, or to enforce other requirements.  This version
		number is not displayed in front-facing UI.  Use the VersionName for that. */
	int32 Version;

	/** Name of the version for this plugin.  This is the front-facing part of the version number.  It doesn't need to match
	    the version number numerically, but should be updated when the version number is increased accordingly. */
	FString VersionName;

	/** The engine version at the time this project was last updated */
	FEngineVersion EngineVersion;

	/** The package file version at the time this project was last updated */
	int32 PackageFileUE4Version;

	/** The licensee package file version at the time this project was last updated */
	int32 PackageFileLicenseeUE4Version;

	/** Friendly name of the project or plugin */
	FString FriendlyName;

	/** Description of the project or plugin */
	FString Description;

	/** The name of the category this project or plugin */
	FString Category;

	/** The company or individual who created this project or plugin.  This is an optional field that may be displayed in the user interface. */
	FString CreatedBy;

	/** Hyperlink URL string for the company or individual who created this project or plugin.  This is optional. */
	FString CreatedByURL;

	/** List of all modules associated with this project or plugin */
	TArray<FModuleInfo> Modules;


	/** ProjectOrPlugin info constructor */
	FProjectOrPluginInfo()
		: EarliestModulePhase( ELoadingPhase::Default )
		, FileVersion(VER_PROJECT_FILE_INVALID)
		, Version(0)
		, VersionName( TEXT( "0.0" ) )
		, EngineVersion()
		, PackageFileUE4Version(INDEX_NONE)
		, PackageFileLicenseeUE4Version(INDEX_NONE)
	{
	}
};

/**
 * Instance of a plugin or project in memory
 */
class FProjectOrPlugin
{
public:

	/** Virtual destructor */
	virtual ~FProjectOrPlugin() {}

	/**
	 * Loads the project or plugin
	 *
	 * @param	LoadingPhase		Which loading phase we're loading modules from.  Only modules that are configured to be
	 *								loaded at the specified loading phase will be loaded during this call.
	 * @param	ModuleLoadErrors	If there were load errors, this map has an entry for every failed module.
	 */
	void LoadModules( const ELoadingPhase::Type LoadingPhase, TMap<FName, ELoadModuleFailureReason::Type>& ModuleLoadErrors );

	/** Populates ProjectInfo from disk */
	bool LoadFromFile( const FString& FileToLoad, FText& OutFailureReason );

	/**
	 * Populates this project file with the supplied JSON string.
	 *
	 * @param JSONInput - The JSON input string.
	 * @param OutFailReason - Will contain the reason for failure, if any.
	 *
	 * @return true if successful, false otherwise.
	 *
	 * @see SerializeToJSON
	 */
	bool DeserializeFromJSON( const FString& JSONInput, FText& OutFailReason );

	/**
	 * Converts this project file to a JSON string.
	 *
	 * @return The JSON string.
	 *
	 * @see DeserializeFromJSON
	 */
	FString SerializeToJSON( ) const;

	/**
	 * Checks whether this project file is up to date with the engine version.
	 *
	 * @return true if the project file is up to date, false otherwise.
	 */
	bool IsUpToDate( ) const;

	/**
	 * Updates all version info to match the currently running executable.
	 */
	void UpdateVersionToCurrent( );

	/**
	 * Updates the modules in the project file
	 *
	 * @param StartupModuleNames	if specified, replaces the existing module names with this version
	 */
	void ReplaceModulesInProject(const TArray<FString>* StartupModuleNames);

protected:
	/** @return Exposes access to the project or plugin's descriptor */
	virtual FProjectOrPluginInfo& GetProjectOrPluginInfo() = 0;
	virtual const FProjectOrPluginInfo& GetProjectOrPluginInfo() const = 0;

	virtual bool PerformAdditionalDeserialization(const TSharedRef< FJsonObject >& FileObject) = 0;
	virtual void PerformAdditionalSerialization(const TSharedRef< TJsonWriter<> >& Writer) const = 0;

	/** Helper functions to read values from the JsonObject */
	bool ReadNumberFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, int32& OutNumber ) const;
	bool ReadNumberFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, uint32& OutNumber ) const;
	bool ReadNumberFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, int64& OutNumber ) const;
	bool ReadStringFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, FString& OutString ) const;
	bool ReadBoolFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, bool& OutBool ) const;

private:
	/** Helper functions to read specific special values from the JsonObject */
	bool ReadFileVersionFromJSON(const TSharedRef< FJsonObject >& FileObject, int32& OutVersion ) const;
	bool ReadModulesFromJSON(const TSharedRef< FJsonObject >& FileObject, TArray<FProjectOrPluginInfo::FModuleInfo>& OutModules, FText& OutFailReason ) const;
};

/**
 * ProjectAndPluginManager is a base class that manages available code and content extensions (both loaded and not loaded.)
 */
class FProjectAndPluginManager
{
public:
	/** Returns the earliest phase in the specified list of modules */
	static ELoadingPhase::Type GetEarliestPhaseFromModules(const TArray<FProjectOrPluginInfo::FModuleInfo>& Modules);
};


