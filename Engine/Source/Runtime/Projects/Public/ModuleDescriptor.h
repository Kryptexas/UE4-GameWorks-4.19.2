// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json.h"

/**
 * Phase at which this module should be loaded during startup.
 */
namespace ELoadingPhase
{
	enum Type
	{
		/** Loaded at the default loading point during startup (during engine init, after game modules are loaded.) */
		Default,

		/** Right after the default phase */
		PostDefault,

		/** Right before the default phase */
		PreDefault,

		/** Loaded before the engine is fully initialized, immediately after the config system has been initialized.  Necessary only for very low-level hooks */
		PostConfigInit,

		/** Loaded before the engine is fully initialized for modules that need to hook into the loading screen before it triggers */
		PreLoadingScreen,
		// NOTE: If you add a new value, make sure to update the ToString() method below!

		Max
	};

	/**
	 * Returns the earliest of two load phases.
	 *
	 * @param	The value to convert to a string
	 * @return	The string representation of this enum value
	 */
	ELoadingPhase::Type Earliest( ELoadingPhase::Type First, ELoadingPhase::Type Second );
	
	/**
	 * Converts a string to a ELoadingPhase::Type value
	 *
	 * @param	The string to convert to a value
	 * @return	The corresponding value, or 'Max' if the string is not valid.
	 */
	ELoadingPhase::Type FromString( const TCHAR *Text );

	/**
	 * Returns the name of a module load phase.
	 *
	 * @param	The value to convert to a string
	 * @return	The string representation of this enum value
	 */
	const TCHAR* ToString( const ELoadingPhase::Type Value );
};

/**
 * Environment that can load a module.
 */
namespace EHostType
{
	enum Type
	{
		Runtime,
		RuntimeNoCommandlet,
		Developer,
		Editor,
		EditorNoCommandlet,
		Program,		//!< Program-only plugin type

		// NOTE: If you add a new value, make sure to update the ToString() method below!

		Max
	};

	/**
	 * Converts a string to a EHostType::Type value
	 *
	 * @param	The string to convert to a value
	 * @return	The corresponding value, or 'Max' if the string is not valid.
	 */
	EHostType::Type FromString( const TCHAR *Text );

	/**
	 * Converts an EHostType::Type value to a string literal
	 *
	 * @param	The value to convert to a string
	 * @return	The string representation of this enum value
	 */
	const TCHAR* ToString( const EHostType::Type Value );
};

/**
 * Description of a loadable module.
 */
struct FModuleDescriptor
{
	/** Name of this module */
	FName Name;

	/** Usage type of module */
	EHostType::Type Type;

	/** When should the module be loaded during the startup sequence?  This is sort of an advanced setting. */
	ELoadingPhase::Type LoadingPhase;

	/** List of allowed platforms */
	TArray<FString> WhitelistPlatforms;

	/** List of disallowed platforms */
	TArray<FString> BlacklistPlatforms;

	/** Module info constructor */
	FModuleDescriptor();

	/** Reads a descriptor from the given JSON object */
	bool Read(const FJsonObject& Object, FText& OutFailReason);

	/** Reads an array of modules from the given JSON object */
	static bool ReadArray(const FJsonObject& Object, const TCHAR* Name, TArray<FModuleDescriptor>& OutModules, FText& OutFailReason);

	/** Writes a descriptor to JSON */
	void Write(TJsonWriter<>& Writer) const;

	/** Writes an array of modules to JSON */
	static void WriteArray(TJsonWriter<>& Writer, const TCHAR* Name, const TArray<FModuleDescriptor>& Modules);

	/** Tests whether the module should be built for the current engine configuration */
	bool IsCompiledInCurrentConfiguration() const;

	/** Tests whether the module should be loaded for the current engine configuration */
	bool IsLoadedInCurrentConfiguration() const;
};
