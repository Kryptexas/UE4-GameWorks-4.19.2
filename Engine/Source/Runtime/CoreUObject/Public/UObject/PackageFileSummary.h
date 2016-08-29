// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObjectBaseUtility.h"

/*----------------------------------------------------------------------------
	Items stored in Unrealfiles.
----------------------------------------------------------------------------*/

/**
 * Revision data for an Unreal package file.
 */
//@todo: shouldn't need ExportCount/NameCount with the linker free package map; if so, clean this stuff up
struct FGenerationInfo
{
	/**
	 * Number of exports in the linker's ExportMap for this generation.
	 */
	int32 ExportCount;

	/**
	 * Number of names in the linker's NameMap for this generation.
	 */
	int32 NameCount;

	/** Constructor */
	FGenerationInfo( int32 InExportCount, int32 InNameCount );

	/** I/O function
	 * we use a function instead of operator<< so we can pass in the package file summary for version tests, since archive version hasn't been set yet
	 */
	void Serialize(FArchive& Ar, const struct FPackageFileSummary& Summary);
};

#if WITH_ENGINE
/**
 * Information about the textures stored in the package.
 */
struct FTextureAllocations
{
	/**
	 * Stores an export index for each texture of a certain type (size, format, etc).
	 */
	struct FTextureType
	{
		FTextureType();
		COREUOBJECT_API FTextureType( int32 SizeX, int32 SizeY, int32 NumMips, uint32 Format, uint32 TexCreateFlags );

		/** Width of the largest mip-level stored in the package. */
		int32				SizeX;
		/** Height of the largest mip-level stored in the package. */
		int32				SizeY;
		/** Number of mips */
		int32				NumMips;
		/** Texture format */
		uint32			Format;
		/** ETextureCreateFlags bit flags */
		uint32			TexCreateFlags;
		/** Index into the package ExportMap. */
		TArray<int32>		ExportIndices;

		/** Not serialized. ResourceMems are constructed on load. */
		TArray<FTexture2DResourceMem*>	Allocations;
		/** Note serialized. Number of ExportIndices processed during load. */
		int32				NumExportIndicesProcessed;

		/**
		 * Checks whether all potential allocations for this TextureType have been considered yet (during load).
		 *
		 * @return true if all allocations have been started
		 */
		bool	HaveAllAllocationsBeenConsidered() const
		{
			return NumExportIndicesProcessed == ExportIndices.Num();
		}

		/**
		 * Serializes an FTextureType
		 */
		friend FArchive& operator<<( FArchive& Ar, FTextureAllocations::FTextureType& TextureType );
	};

	FTextureAllocations()
	:	PendingAllocationSize(0)
	,	NumTextureTypesConsidered(0)
	{
	}

	/** Array of texture types in the package. */
	TArray<FTextureType>	TextureTypes;
	/** Number of allocations that haven't been completed yet. */
	FThreadSafeCounter		PendingAllocationCount;
	/** Number of allocated bytes that has yet to be claimed by UTexture2D serialization. */
	int32						PendingAllocationSize;
	/** Number of TextureTypes that have been considered for potential allocations so far, during load. */
	int32						NumTextureTypesConsidered;

	/**
	 * Finds a suitable ResourceMem allocation, removes it from this container and returns it to the user.
	 *
	 * @param SizeX				Width of texture
	 * @param SizeY				Height of texture
	 * @param NumMips			Number of mips
	 * @param Format			Texture format (EPixelFormat)
	 * @param TexCreateFlags	ETextureCreateFlags bit flags
	 **/
	FTexture2DResourceMem*	FindAndRemove( int32 SizeX, int32 SizeY, int32 NumMips, uint32 Format, uint32 TexCreateFlags );

	/**
	 * Cancels any pending ResourceMem allocation that hasn't been claimed by a texture yet,
	 * just in case there are any mismatches at run-time.
	 *
	 * @param bCancelEverything		If true, cancels all allocations. If false, only cancels allocations that haven't been completed yet.
	 */
	void	CancelRemainingAllocations( bool bCancelEverything );

	/**
	 * Checks if all allocations that should be started have been started (during load).
	 *
	 * @return true if all allocations have been started
	 */
	bool	HaveAllAllocationsBeenConsidered() const
	{
		return NumTextureTypesConsidered == TextureTypes.Num();
	}

	/**
	 * Checks if all ResourceMem allocations has completed.
	 *
	 * @return true if all ResourceMem allocations has completed
	 */
	bool	HasCompleted() const
	{
		return PendingAllocationCount.GetValue() == 0;
	}

	/**
	 * Checks if all ResourceMem allocations have been claimed by a texture.
	 *
	 * @return true if there are no more pending ResourceMem allocations waiting for a texture to claim it
	 */
	bool	HasBeenFullyClaimed() const
	{
		return PendingAllocationSize == 0;
	}

	/**
	 * Serializes an FTextureType
	 */
	friend FArchive& operator<<( FArchive& Ar, FTextureAllocations::FTextureType& TextureAllocationType );

	/**
	 * Serializes an FTextureAllocations struct
	 */
	friend FArchive& operator<<( FArchive& Ar, FTextureAllocations& TextureAllocations );

	FTextureAllocations( const FTextureAllocations& Other );
	void operator=(const FTextureAllocations& Other);

private:
	/**
	 * Finds a texture type that matches the given specifications.
	 *
	 * @param SizeX				Width of the largest mip-level stored in the package
	 * @param SizeY				Height of the largest mip-level stored in the package
	 * @param NumMips			Number of mips
	 * @param Format			Texture format (EPixelFormat)
	 * @param TexCreateFlags	ETextureCreateFlags bit flags
	 * @return					Matching texture type, or NULL if none was found
	 */
	FTextureType*	FindTextureType( int32 SizeX, int32 SizeY, int32 NumMips, uint32 Format, uint32 TexCreateFlags );

	/**
	 * Adds a dummy export index (-1) for a specified texture type.
	 * Creates the texture type entry if needed.
	 *
	 * @param SizeX				Width of the largest mip-level stored in the package
	 * @param SizeY				Height of the largest mip-level stored in the package
	 * @param NumMips			Number of mips
	 * @param Format			Texture format (EPixelFormat)
	 * @param TexCreateFlags	ETextureCreateFlags bit flags
	 */
	void			AddResourceMemInfo( int32 SizeX, int32 SizeY, int32 NumMips, uint32 Format, uint32 TexCreateFlags );
};
#endif		// WITH_ENGINE

/**
 * A "table of contents" for an Unreal package file.  Stored at the top of the file.
 */
struct FPackageFileSummary
{
	/**
	 * Magic tag compared against PACKAGE_FILE_TAG to ensure that package is an Unreal package.
	 */
	int32		Tag;

private:
	/* UE4 file version */
	int32		FileVersionUE4;
	/* Licensee file version */
	int32		FileVersionLicenseeUE4;
	/* Custom version numbers. Keyed off a unique tag for each custom component. */
	FCustomVersionContainer CustomVersionContainer;

public:
	/**
	 * Total size of all information that needs to be read in to create a FLinkerLoad. This includes
	 * the package file summary, name table and import & export maps.
	 */
	int32		TotalHeaderSize;

	/**
	 * The flags for the package
	 */
	uint32	PackageFlags;

	/**
	 * The Generic Browser folder name that this package lives in
	 */
	FString	FolderName;

	/**
	 * Number of names used in this package
	 */
	int32		NameCount;

	/**
	 * Location into the file on disk for the name data
	 */
	int32 	NameOffset;

	/**
	 * Number of gatherable text data items in this package
	 */
	int32	GatherableTextDataCount;

	/**
	 * Location into the file on disk for the gatherable text data items
	 */
	int32 	GatherableTextDataOffset;
	
	/**
	 * Number of exports contained in this package
	 */
	int32		ExportCount;

	/**
	 * Location into the file on disk for the ExportMap data
	 */
	int32		ExportOffset;

	/**
	 * Number of imports contained in this package
	 */
	int32		ImportCount;

	/**
	 * Location into the file on disk for the ImportMap data
	 */
	int32		ImportOffset;

	/**
	* Location into the file on disk for the DependsMap data
	*/
	int32		DependsOffset;

	/**
	 * Number of references contained in this package
	 */
	int32		StringAssetReferencesCount;

	/**
	 * Location into the file on disk for the string asset references map data
	 */
	int32		StringAssetReferencesOffset;

	/**
	 * Thumbnail table offset
	 */
	int32		ThumbnailTableOffset;

	/**
	 * Current id for this package
	 */
	FGuid	Guid;

	/**
	 * Data about previous versions of this package
	 */
	TArray<FGenerationInfo> Generations;

	/**
	 * Engine version this package was saved with. For hotfix releases and engine versions which maintain strict binary compatibility with another version, this may differ from CompatibleWithEngineVersion.
	 */
	FEngineVersion SavedByEngineVersion;

	/**
	 * Engine version this package is compatible with. See SavedByEngineVersion.
	 */
	FEngineVersion CompatibleWithEngineVersion;

	/**
	 * Flags used to compress the file on save and uncompress on load.
	 */
	uint32	CompressionFlags;

	/**
	 * Value that is used to determine if the package was saved by Epic (or licensee) or by a modder, etc
	 */
	uint32	PackageSource;

	/**
	 * Array of compressed chunks in case this package was stored compressed.
	 */
	TArray<FCompressedChunk> CompressedChunks;

	/**
	 * List of additional packages that are needed to be cooked for this package (ie streaming levels)
	 */
	TArray<FString>	AdditionalPackagesToCook;

	/** 
	 * If true, this file will not be saved with version numbers or was saved without version numbers. In this case they are assumed to be the current version. 
	 * This is only used for full cooks for distribution because it is hard to guarantee correctness 
	 **/
	bool bUnversioned;

#if WITH_ENGINE
	/**
	 * Information about the textures stored in the package.
	 */
	FTextureAllocations	TextureAllocations;
#endif		// WITH_ENGINE

	/**
	 * Location into the file on disk for the asset registry tag data
	 */
	int32 	AssetRegistryDataOffset;

	/** Offset to the location in the file where the bulkdata starts */
	int64	BulkDataStartOffset;
	/**
	 * Offset to the location in the file where the FWorldTileInfo data starts
	 */
	int32 	WorldTileInfoDataOffset;

	/**
	 * Streaming install ChunkIDs
	 */
	TArray<int32>	ChunkIDs;


	/** Constructor */
	COREUOBJECT_API FPackageFileSummary();

	int32 GetFileVersionUE4() const
	{
		return FileVersionUE4;
	}

	int32 GetFileVersionLicenseeUE4() const
	{
		return FileVersionLicenseeUE4;
	}

	const FCustomVersionContainer& GetCustomVersionContainer() const
	{
		return CustomVersionContainer;
	}

	void SetCustomVersionContainer(const FCustomVersionContainer& InContainer)
	{
		CustomVersionContainer = InContainer;
	}

	void SetFileVersions(const int32 EpicUE4, const int32 LicenseeUE4, const bool bInSaveUnversioned = false)
	{
		FileVersionUE4 = EpicUE4;
		FileVersionLicenseeUE4 = LicenseeUE4;
		bUnversioned = bInSaveUnversioned;
	}

	/** I/O function */
	friend COREUOBJECT_API FArchive& operator<<( FArchive& Ar, FPackageFileSummary& Sum );
};
