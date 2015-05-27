// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetImportData.generated.h"

/** Struct that is used to store an array of asset import data as an asset registry tag */
struct ENGINE_API FAssetImportInfo
{
#if WITH_EDITORONLY_DATA
	struct FSourceFile
	{
		FSourceFile(FString InRelativeFilename, const FDateTime& InTimestamp = 0, const FMD5Hash& InFileHash = FMD5Hash())
			: RelativeFilename(MoveTemp(InRelativeFilename))
			, Timestamp(0)
			, FileHash(InFileHash)
		{}

		/** The path to the file that this asset was imported from. Relative to either the asset's package, BaseDir(), or absolute */
		FString RelativeFilename;

		/** The timestamp of the file when it was imported (as UTC). 0 when unknown. */
		FDateTime Timestamp;

		/** The MD5 hash of the file when it was imported. Invalid when unknown. */
		FMD5Hash FileHash;
	};

	/** Convert this import information to JSON */
	FString ToJson() const;

	/** Attempt to parse an asset import structure from the specified json string. */
	static TOptional<FAssetImportInfo> FromJson(FString InJsonString);

	/** Insert information pertaining to a new source file to this structure */
	void Insert(const FSourceFile& InSourceFile) { SourceFiles.Add(InSourceFile); }

	/** Copy the information from another asset import info structure into this one. */
	void CopyFrom(const FAssetImportInfo& Other) { SourceFiles = Other.SourceFiles; }

	/** Const public access to the source file data */
	const TArray<FSourceFile>& GetSourceFileData() const { return SourceFiles; }

protected:

	/** Array of information pertaining to the source files that this asset was imported from */
	TArray<FSourceFile> SourceFiles;
#endif // WITH_EDITORONLY_DATA
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnImportDataChanged, const FAssetImportInfo& /*OldData*/, const class UAssetImportData* /* NewData */);

UCLASS()
class ENGINE_API UAssetImportData : public UObject, public FAssetImportInfo
{
public:
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	/** If true, the settings have been modified but not reimported yet */
	UPROPERTY()
	uint32 bDirty : 1;

public:

	/** Static event that is broadcast whenever any asset has updated its import data */
	static FOnImportDataChanged OnImportDataChanged;

	/** Update this import data using the specified file. Called when an asset has been imported from a file. */
	void Update(const FString& AbsoluteFilename);

	/** Update this import data using the specified filename. Will not update the imported timestamp or MD5. */
	void UpdateFilenameOnly(const FString& InPath);

	/** Helper function to return the first filename stored in this data. The resulting filename will be absolute (ie, not relative to the asset).  */
	FString GetFirstFilename() const;

	/** Extract all the (resolved) filenames from this data  */
	void ExtractFilenames(TArray<FString>& AbsoluteFilenames) const;

	/** Extract all the (resolved) filenames from this data  */
	TArray<FString> ExtractFilenames() const;

	/** Resolve a filename that is relative to either the specified package, BaseDir() or absolute */
	static FString ResolveImportFilename(const FString& InRelativePath, const UPackage* Outermost);

protected:

	/** Convert an absolute import path so that it's relative to either this object's package, BaseDir() or leave it absolute */
	FString SanitizeImportFilename(const FString& InPath) const;

	/** Resolve a filename that is relative to either this object's package, BaseDir() or absolute */
	FString ResolveImportFilename(const FString& InRelativePath) const;

	/** Overridden serialize function to write out the underlying data as json */
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
#endif		// WITH_EDITORONLY_DATA
};


