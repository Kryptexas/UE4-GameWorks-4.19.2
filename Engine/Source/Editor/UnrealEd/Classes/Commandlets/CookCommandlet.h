// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CookCommandlet.cpp: Commandlet for cooking content
=============================================================================*/

#pragma once

#include "CookCommandlet.generated.h"

UCLASS()
class UCookCommandlet
	: public UCommandlet
{
	GENERATED_UCLASS_BODY()

	/** If true, iterative cooking is being done */
	bool bIterativeCooking;
	/** If true, packages are cooked compressed */
	bool bCompressed;
	/** Prototype cook-on-the-fly server */
	bool bCookOnTheFly; 
	/** Cook everything */
	bool bCookAll;
	/** Skip saving any packages in Engine/COntent/Editor* UNLESS TARGET HAS EDITORONLY DATA (in which case it will save those anyway) */
	bool bSkipEditorContent;
	/** Test for UObject leaks */
	bool bLeakTest;  
	/** Save all cooked packages without versions. These are then assumed to be current version on load. This is dangerous but results in smaller patch sizes. */
	bool bUnversioned;
	/** Generate manifests for building streaming install packages */
	bool bGenerateStreamingInstallManifests;
	/** All commandline tokens */
	TArray<FString> Tokens;
	/** All commandline switches */
	TArray<FString> Switches;
	/** All commandline params */
	FString Params;

	/**
	 * Cook on the fly routing for the commandlet
	 *
	 * @param  BindAnyPort					Whether to bind on any port or the default port.
	 * @param  Timeout						Length of time to wait for connections before attempting to close
	 * @param  bForceClose					Whether or not the server should always shutdown after a timeout or after a user disconnects
	 *
	 * @return true on success, false otherwise.
	 */
	bool CookOnTheFly( bool BindAnyPort, int32 Timeout = 180, bool bForceClose = false );

	/**
	 * Returns cooker output directory.
	 *
	 * @param PlatformName Target platform name.
	 * @return Cooker output directory for the specified platform.
	 */
	FString GetOutputDirectory( const FString& PlatformName ) const;

	/**
	 *	Get the given packages 'cooked' timestamp (i.e. account for dependencies)
	 *
	 *	@param	InFilename			The filename of the package
	 *	@param	OutDateTime			The timestamp the cooked file should have
	 *
	 *	@return	bool				true if the package timestamp was found, false if not
	 */
	bool GetPackageTimestamp( const FString& InFilename, FDateTime& OutDateTime );

	/**
	 *	Cook (save) the given package
	 *
	 *	@param	Package				The package to cook/save
	 *	@param	SaveFlags			The flags to pass to the SavePackage function
	 *	@param	bOutWasUpToDate		Upon return, if true then the cooked package was cached (up to date)
	 *
	 *	@return	bool			true if packages was cooked
	 */
	bool SaveCookedPackage( UPackage* Package, uint32 SaveFlags, bool& bOutWasUpToDate );

	bool ShouldCook(const FString& InFilename);

public:

	// Begin UCommandlet Interface

	virtual int32 Main(const FString& CmdLineParams) OVERRIDE;
	
	// End UCommandlet Interface

private:

	// Callback for handling a network file request.
	void HandleNetworkFileServerFileRequest( const FString& Filename, TArray<FString>& UnsolicitedFiles );

	// Callback for recompiling shaders
	void HandleNetworkFileServerRecompileShaders(const struct FShaderRecompileData& RecompileData);


private:

	/** Holds the instance identifier. */
	FGuid InstanceId;

	/** Holds the sandbox file wrapper to handle sandbox path conversion. */
	TAutoPtr<class FSandboxPlatformFile> SandboxFile;

	/** We hook this up to a delegate to avoid reloading textures and whatnot */
	TSet<FString> PackagesToNotReload;

	/** Leak test: last gc items */
	TSet<FWeakObjectPtr> LastGCItems;

	void MaybeMarkPackageAsAlreadyLoaded(UPackage *Package);

	/** Gets the output directory respecting any command line overrides */
	FString GetOutputDirectoryOverride() const;

	/** Cleans sandbox folders for all target platforms */
	void CleanSandbox(const TArray<ITargetPlatform*>& Platforms);

	/** Generates asset registry */
	void GenerateAssetRegistry(const TArray<ITargetPlatform*>& Platforms);

	/** Saves global shader map files */
	void SaveGlobalShaderMapFiles(const TArray<ITargetPlatform*>& Platforms);

	/** Collects all files to be cooked. This includes all commandline specified maps */
	void CollectFilesToCook(TArray<FString>& FilesInPath);

	/** Generates long package names for all files to be cooked */
	void GenerateLongPackageNames(TArray<FString>& FilesInPath);

	/** Cooks all files */
	bool Cook(const TArray<ITargetPlatform*>& Platforms, TArray<FString>& FilesInPath);


};