// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Array.h: Data structure containing the contents of an
	         UnrealHeaderTool.manifest file.
=============================================================================*/

#pragma once

struct FManifestModule
{
	/** The name of the module */
	FString Name;

	/** Long package name for this module's UObject class */
	FString LongPackageName;

	/** Base directory of this module on disk */
	FString BaseDirectory;

	/** Directory where generated include files should go */
	FString GeneratedIncludeDirectory;

	/** List of C++ public 'Classes' header files with UObjects in them (legacy) */
	TArray<FString> PublicUObjectClassesHeaders;

	/** List of C++ public header files with UObjects in them */
	TArray<FString> PublicUObjectHeaders;

	/** List of C++ private header files with UObjects in them */
	TArray<FString> PrivateUObjectHeaders;

	/** Absolute path to the module's PCH */
	FString PCH;

	/** Base (i.e. extensionless) path+filename of where to write out the module's .generated.* files */
	FString GeneratedCPPFilenameBase;

	/** Whether or not to write out headers that have changed */
	bool SaveExportedHeaders;
};

struct FManifest
{
	bool    UseRelativePaths;
	FString RootLocalPath;
	FString RootBuildPath;

	/** Ordered list of modules that define UObjects or UStructs, which we may need to generate
	    code for.  The list is in module dependency order, such that most dependent modules appear first. */
	TArray<FManifestModule> Modules;

	/**
	 * Loads an UnrealHeaderTool.manifest from the specified filename.
	 *
	 * @param Filename The filename of the manifest to load.
	 *
	 * @return The loaded module info.
	 */
	static FManifest LoadFromFile(const FString& Filename);
};
