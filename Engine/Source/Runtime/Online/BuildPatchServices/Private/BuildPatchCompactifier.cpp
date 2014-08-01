// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
BuildPatchCompactifier.cpp: Implements the classes that clean up chunk and file
data that are no longer referenced by the manifests in a given cloud directory.
=============================================================================*/

#include "BuildPatchServicesPrivatePCH.h"

#if WITH_BUILDPATCHGENERATION

/* Constructors
*****************************************************************************/

FBuildDataCompactifier::FBuildDataCompactifier()
{

}

FBuildDataCompactifier::FBuildDataCompactifier(const FString& CloudDir)
	: CloudDir(CloudDir)
{

}

/* Public static methods
*****************************************************************************/

bool FBuildDataCompactifier::CompactifyCloudDirectory(const FString& CloudDir, const bool bPreview)
{
	FBuildDataCompactifier Compactifier(CloudDir);
	return Compactifier.Compactify(bPreview);
}

bool FBuildDataCompactifier::CompactifyCloudDirectory(const bool bPreview)
{
	return CompactifyCloudDirectory(FBuildPatchServicesModule::GetCloudDirectory(), bPreview);
}

/* Private methods
*****************************************************************************/

bool FBuildDataCompactifier::Compactify(bool bPreview) const
{
	GLog->Logf(TEXT("Running Compactify on %s%s"), *CloudDir, bPreview ? TEXT(". Preview mode. NO action will be taken.") : TEXT(""));

	// We'll get ALL files first, so we can use the count to preallocate space within the data filenames array to save excessive reallocs
	TArray<FString> AllFiles;
	const bool bFindFiles = true;
	const bool bFindDirectories = false;
	IFileManager::Get().FindFilesRecursive(AllFiles, *CloudDir, TEXT("*.*"), bFindFiles, bFindDirectories);

	TSet<FGuid> ReferencedGuids; // The master list of *ALL* referenced chunk / file data Guids

	TArray<FString> ManifestFilenames;
	TArray<FGuid> DataGuids; // The Guids associated with the data files from a single manifest
	int32 NumDataFiles = 0;
	
	// Preallocate enough storage in DataGuids, to stop repeatedly expanding the allocation
	DataGuids.Reserve(AllFiles.Num());

	EnumerateManifests(ManifestFilenames);

	// If we don't have any manifest files, we'll treat that as an error condition
	if (ManifestFilenames.Num() == 0)
	{
		GLog->Log(ELogVerbosity::Warning, TEXT("Could not find any manifest files. Aborting operation."));
		return true;  // We're still going to return a success code, as this isn't a hard error
	}

	// Process each manifest, and build up a list of all referenced files
	for (const auto& ManifestFilename : ManifestFilenames)
	{
		const FString ManifestPath = CloudDir / ManifestFilename;
		GLog->Logf(TEXT("Extracting chunk filenames from %s"), *ManifestFilename);

		// Load the manifest data from the manifest
		FBuildPatchAppManifest Manifest;
		if (Manifest.LoadFromFile(ManifestPath))
		{
			// Work out all data Guids referenced in the manifest, and add them to our list of files to keep
			DataGuids.Empty();
			Manifest.GetDataList(DataGuids);

			GLog->Logf(TEXT("Extracted %d chunks from %s. Unioning with %d existing chunks"), DataGuids.Num(), *ManifestFilename, NumDataFiles);
			NumDataFiles += DataGuids.Num();

			// We're going to keep all the Guids so we know which files to keep later
			ReferencedGuids.Append(DataGuids);
		}
		else
		{
			// We failed to read from the manifest file.  This is an error which should halt progress and return a non-zero exit code
			FString ErrorMessage = FString::Printf(TEXT("Could not parse manifest file %s"), *ManifestFilename);
			GLog->Log(ELogVerbosity::Error, *ErrorMessage);
			return false;
		}
	}

	GLog->Logf(TEXT("Compactify walking %s to remove all unreferenced chunks and compute statistics."), *CloudDir);

	uint32 FilesProcessed = 0;
	uint32 NonPatchFilesProcessed = 0;
	uint32 FilesDeleted = 0;
	uint64 BytesProcessed = 0;
	uint64 NonPatchBytesProcessed = 0;
	uint64 BytesDeleted = 0;
	uint64 CurrentFileSize;
	FGuid FileGuid;

	for (const auto& File : AllFiles)
	{
		CurrentFileSize = IFileManager::Get().FileSize(*File);
		if (CurrentFileSize >= 0)
		{
			++FilesProcessed;
			BytesProcessed += CurrentFileSize;

			if (!GetPatchDataGuid(File, FileGuid))
			{
				if (!ManifestFilenames.Contains(FPaths::GetCleanFilename(File)))
				{
					++NonPatchFilesProcessed;
					NonPatchBytesProcessed += CurrentFileSize;
				}
				continue;
			}

			if (!ReferencedGuids.Contains(FileGuid))
			{
				DeleteFile(File, bPreview);
				++FilesDeleted;
				BytesDeleted += CurrentFileSize;
			}
		}
		else
		{
			GLog->Logf(TEXT("Warning. Could not determine size of %s. Perhaps it has been removed by another process."), *File);
		}
	}

	GLog->Logf(TEXT("Compactify of %s complete!"), *CloudDir);
	GLog->Logf(TEXT("Compactify found %u files totalling %s."), FilesProcessed, *HumanReadableSize(BytesProcessed));
	if (NonPatchFilesProcessed > 0)
	{
		GLog->Logf(TEXT("Of these, %d (totalling %s) were not chunk/manifest files."), NonPatchFilesProcessed, *HumanReadableSize(NonPatchBytesProcessed));
	}
	GLog->Logf(TEXT("Compactify deleted %u chunk/manifest files totalling %s."), FilesDeleted, *HumanReadableSize(BytesDeleted));
	return true;
}

void FBuildDataCompactifier::DeleteFile(const FString& FilePath, const bool bPreview) const
{
	GLog->Logf(TEXT("Deleting file %s"), *FilePath);
	if (!bPreview)
	{
		IFileManager::Get().Delete(*FilePath);
	}
}

void FBuildDataCompactifier::EnumerateManifests(TArray<FString>& OutManifests) const
{
	// Get a list of all manifest filenames by using IFileManager
	FString FilePattern = CloudDir / TEXT("*.manifest");
	IFileManager::Get().FindFiles(OutManifests, *FilePattern, true, false);
}

bool FBuildDataCompactifier::GetPatchDataGuid(const FString& FilePath, FGuid& OutGuid) const
{
	FString Extension = FPaths::GetExtension(FilePath);
	if (Extension != TEXT("chunk") && Extension != TEXT("file"))
	{
		// Our filename doesn't have one of the allowed file extensions, so it's not patch data
		return false;
	}

	FString BaseFileName = FPaths::GetBaseFilename(FilePath);
	if (BaseFileName.Contains(TEXT("_")))
	{
		FString Right;
		BaseFileName.Split(TEXT("_"), NULL, &Right);
		if (Right.Len() != 32)
		{
			return false; // Valid data files which contain an _ in their filename must have 32 characters to the right of the underscore
		}
	}
	else
	{
		if (BaseFileName.Len() != 32)
		{
			return false; // Valid data files whose names do not contain underscores must be 32 characters long
		}
	}

	return FBuildPatchUtils::GetGUIDFromFilename(FilePath, OutGuid);
}

FString FBuildDataCompactifier::HumanReadableSize(uint64 NumBytes, uint8 DecimalPlaces, bool bUseBase10) const
{
	static const TCHAR* const Suffixes[2][7] = { { TEXT("Bytes"), TEXT("kB"), TEXT("MB"), TEXT("GB"), TEXT("TB"), TEXT("PB"), TEXT("EB") },
	                                             { TEXT("Bytes"), TEXT("KiB"), TEXT("MiB"), TEXT("GiB"), TEXT("TiB"), TEXT("PiB"), TEXT("EiB") }};

	float DataSize = (float)NumBytes;

	const int32 Base = bUseBase10 ? 1000 : 1024;
	int32 Index = FMath::FloorToInt(FMath::LogX(Base, NumBytes));
	Index = FMath::Clamp<int32>(Index, 0, ARRAY_COUNT(Suffixes[0]) - 1);

	DecimalPlaces = FMath::Clamp<int32>(DecimalPlaces, 0, Index*3);

	return FString::Printf(TEXT("%.*f %s"), DecimalPlaces, DataSize / (FMath::Pow(Base, Index)), Suffixes[bUseBase10 ? 0 : 1][Index]);
}
#endif
