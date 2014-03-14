// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Helper class for generating streaming install manifests
 */
class FChunkManifestGenerator
{
	/** Manifest type */
	typedef TSet<FString> FChunkManifest;
	/** Holds a reference to asset registry */
	IAssetRegistry& AssetRegistry;
	/** Platforms to generate the manifests for. */
	const TArray<ITargetPlatform*>& Platforms;
	/** Generated manifests for each platform. */
	TArray<FChunkManifest*> ChunkManifests;
	/** ChunkIDs associated with a package in the asset registry */
	TMap<FName, TArray<int32> > RegistryChunkIDsMap;
	/** List of all asset packages that were created while loading the last package in the cooker. */
	TSet<FName> AssetsLoadedWithLastPackage;
	/** Default engine content chunk index */
	int32 DefaultEngineChunkID;
	/** Default game content chunk index */
	int32 DefaultGameChunkID;
	/** The entire asset registry data */
	TArray<FAssetData> AssetRegistryData;
	/** Maps packages to assets from the asset registry */
	TMap<FName, TArray<int32> > PackageToRegistryDataMap;
	/** Should the chunks be generated or only asset registry */
	bool bGenerateChunks;

	/**
	 * Callback for FCoreDelegates::FOnAssetLoaded delegate.
	 * Collects all assets loaded with the last package.
	 */
	void OnAssetLoaded(UObject* Asset);

	/**
	 * Adds package to the specified chunk manifest. If the chunk does not exist, it will be created.
	 *
	 * @param PackageName Name of the package to add (this is the sandbox path)
	 * @param ChunkId Id of the chunk to add the package to.
	 * @param List of created chunks.
	 */
	void AddPackageToManifest(const FString& PackageName, int32 ChunkId);

	/**
	 * Finds a package in any of the created manifests and fills a list of all chunks this asset was found in.
	 *
	 * @param PackageName Sandbox path of the package to find
	 * @param Manifests List of manifests
	 * @param OutChunkIDs List to be populated with all chunk IDs the specified package has been added to
	 */
	void FindPackageInManifests(const FString& PackageName, TArray<int32>& OutChunkIDs) const;

	/**
	 * Returns the path of the temporary packaging directory for the specified platform.
	 */
	FString GetTempPackagingDirectoryForPlatform(const FString& Platform) const
	{
		return FPaths::GameSavedDir() / TEXT("TmpPackaging") / Platform;
	}

	/**
	 * Deletes the temporary packaging directory for the specified platform.
	 */
	bool CleanTempPackagingDirectory(const FString& Platform) const;

	/**
	 * Generates and saves streaming install chunk manifest.
	 *
	 * @param Platform Platform this manifest is going to be generated for.
	 * @param Chunks List of chunk manifests.
	 */
	bool GenerateStreamingInstallManifest(const FString& Platform);

public:

	/**
	 * Constructor
	 */
	FChunkManifestGenerator(const TArray<ITargetPlatform*>& InPlatforms);

	/**
	 * Destructor
	 */
	~FChunkManifestGenerator();

	/**
	 * Initializes manifest generator - creates manifest lists, hooks up delegates.
	 */
	void Initialize(bool InGenerateChunks);

	/**
	 * Adds a package to chunk manifest.
	 *
	 * @param Package Package to add to one of the manifests
	 * @param SandboxFilename Cooked sandbox path of the package to add to a manifest
	 * @param LastLoadedMapName Name of the last loaded map (can be empty)
	 */
	void AddPackageToChunkManifest(UPackage* Package, const FString& SandboxFilename, const FString& LastLoadedMapName);

	/**
	 * The cooker is about to load a new package from the list, reset AssetsLoadedWithLastPackage
	 */
	void PrepareToLoadNewPackage(const FString& Filename);

	/**
	 * Deletes temporary manifest directories.
	 */
	void CleanManifestDirectories();

	/**
	 * Saves all generated manifests for each target platform.
	 */
	bool SaveManifests();

	/**
	* Saves generated asset registry data for each platform.
	*/
	bool SaveAssetRegistry(const FString& SandboxPath);
};
