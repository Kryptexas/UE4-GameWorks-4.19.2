// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetRegistryPCH.h"

#define MAX_FILES_TO_PROCESS_BEFORE_FLUSH 250

FAssetDataGatherer::FAssetDataGatherer(const TArray<FString>& InPaths, bool bInIsSynchronous)
	: StopTaskCounter( 0 )
	, bIsSynchronous( bInIsSynchronous )
	, SearchStartTime( 0 )
{
	const FString AllIllegalCharacters = INVALID_LONGPACKAGE_CHARACTERS;
	for ( int32 CharIdx = 0; CharIdx < AllIllegalCharacters.Len() ; ++CharIdx )
	{
		InvalidAssetFileCharacters.Add(AllIllegalCharacters[CharIdx]);
	}

	PathsToSearch = InPaths;

	bGatherDependsData = GIsEditor && !FParse::Param( FCommandLine::Get(), TEXT("NoDependsGathering") );

	if ( bIsSynchronous )
	{
		Run();
	}
	else
	{
		Thread = FRunnableThread::Create( this, TEXT("FAssetDataGatherer"), false, false, 0, TPri_BelowNormal );
	}
}

bool FAssetDataGatherer::Init()
{
	return true;
}

uint32 FAssetDataGatherer::Run()
{
	TArray<FString> LocalFilesToSearch;
	TArray<FBackgroundAssetData*> LocalAssetResults;
	TArray<FPackageDependencyData> LocalDependencyResults;

	while ( StopTaskCounter.GetValue() == 0 )
	{
		// Check to see if there are any paths that need scanning for files.  On the first iteration, there will always
		// be work to do here.  Later, if new paths are added on the fly, we'll also process those.
		DiscoverFilesToSearch();

		{
			FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
			if ( LocalAssetResults.Num() )
			{
				AssetResults.Append(LocalAssetResults);
			}

			if ( LocalDependencyResults.Num() )
			{
				DependencyResults.Append(LocalDependencyResults);
			}

			if ( FilesToSearch.Num() )
			{
				if (SearchStartTime == 0)
				{
					SearchStartTime = FPlatformTime::Seconds();
				}

				const int32 NumFilesToProcess = FMath::Min<int32>(MAX_FILES_TO_PROCESS_BEFORE_FLUSH, FilesToSearch.Num());

				for (int32 FileIdx = 0; FileIdx < NumFilesToProcess; ++FileIdx)
				{
					LocalFilesToSearch.Add(FilesToSearch[FileIdx]);
				}

				FilesToSearch.RemoveAt(0, NumFilesToProcess);
			}
			else if (SearchStartTime != 0)
			{
				SearchTimes.Add(FPlatformTime::Seconds() - SearchStartTime);
				SearchStartTime = 0;
			}
		}

		if ( LocalAssetResults.Num() )
		{
			LocalAssetResults.Empty();
		}

		if ( LocalDependencyResults.Num() )
		{
			LocalDependencyResults.Empty();
		}

		if ( LocalFilesToSearch.Num() )
		{
			for (int32 FileIdx = 0; FileIdx < LocalFilesToSearch.Num(); ++FileIdx)
			{
				const FString& AssetFile = LocalFilesToSearch[FileIdx];

				if ( StopTaskCounter.GetValue() != 0 )
				{
					// We have been asked to stop, so don't read any more files
					break;
				}

				TArray<FBackgroundAssetData*> AssetDataFromFile;
				FPackageDependencyData DependencyData;

				if ( ReadAssetFile(AssetFile, AssetDataFromFile, DependencyData) )
				{
					LocalAssetResults.Append(AssetDataFromFile);
					LocalDependencyResults.Add(DependencyData);
				}
			}

			LocalFilesToSearch.Empty();
		}
		else
		{
			if (bIsSynchronous)
			{
				// This is synchronous. Since our work is done, we should safely exit
				Stop();
			}
			else
			{
				FPlatformProcess::Sleep(0.01);
			}
		}
	}

	return 0;
}

void FAssetDataGatherer::Stop()
{
	StopTaskCounter.Increment();
}

void FAssetDataGatherer::Exit()
{
    
}

void FAssetDataGatherer::EnsureCompletion()
{
	{
		FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
		FilesToSearch.Empty();
		PathsToSearch.Empty();
	}

	Stop();
	Thread->WaitForCompletion();
    delete Thread;
    Thread = NULL;
}

bool FAssetDataGatherer::GetAndTrimSearchResults(TArray<FBackgroundAssetData*>& OutAssetResults, TArray<FString>& OutPathResults, TArray<FPackageDependencyData>& OutDependencyResults, TArray<double>& OutSearchTimes, int32& OutNumFilesToSearch)
{
	FScopeLock CritSectionLock(&WorkerThreadCriticalSection);

	OutAssetResults.Append(AssetResults);
	AssetResults.Empty();

	OutPathResults.Append(DiscoveredPaths);
	DiscoveredPaths.Empty();

	OutDependencyResults.Append(DependencyResults);
	DependencyResults.Empty();

	OutSearchTimes.Append(SearchTimes);
	SearchTimes.Empty();

	OutNumFilesToSearch = FilesToSearch.Num();

	return SearchStartTime > 0;
}

void FAssetDataGatherer::AddPathToSearch(const FString& Path)
{
	FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
	PathsToSearch.Add(Path);
}

void FAssetDataGatherer::AddFilesToSearch(const TArray<FString>& Files)
{
	TArray<FString> FilesToAdd;
	for (int32 FilenameIdx = 0; FilenameIdx < Files.Num(); FilenameIdx++)
	{
		FString Filename(Files[FilenameIdx]);
		if ( IsValidPackageFileToRead(Filename) )
		{
			// Add the path to this asset into the list of discovered paths
			FilesToAdd.Add(Filename);
		}
	}

	{
		FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
		FilesToSearch.Append(FilesToAdd);
	}
}

void FAssetDataGatherer::DiscoverFilesToSearch()
{
	if( PathsToSearch.Num() > 0 )
	{
		TArray<FString> DiscoveredFilesToSearch;
		TArray<FString> LocalDiscoveredPaths;

		TArray<FString> CopyOfPathsToSearch;
		{
			FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
			CopyOfPathsToSearch = PathsToSearch;

			// Remove all of the existing paths from the list, since we'll process them all below.  New paths may be
			// added to the original list on a different thread as we go along, but those new paths won't be processed
			// during this call of DisoverFilesToSearch().  But we'll get them on the next call!
			PathsToSearch.Empty();
		}

		// Iterate over any paths that we have remaining to scan
		for ( int32 PathIdx=0; PathIdx < CopyOfPathsToSearch.Num(); ++PathIdx )
		{
			const FString& Path = CopyOfPathsToSearch[PathIdx];

			// Convert the package path to a filename with no extension (directory)
			const FString FilePath = FPackageName::LongPackageNameToFilename(Path);

			// Gather the package files in that directory and subdirectories
			TArray<FString> Filenames;
			FPackageName::FindPackagesInDirectory(Filenames, FilePath);

			for (int32 FilenameIdx = 0; FilenameIdx < Filenames.Num(); FilenameIdx++)
			{
				FString Filename(Filenames[FilenameIdx]);
				if ( IsValidPackageFileToRead(Filename) )
				{
					// Add the path to this asset into the list of discovered paths
					const FString LongPackageName = FPackageName::FilenameToLongPackageName(Filename);
					LocalDiscoveredPaths.AddUnique( FPackageName::GetLongPackagePath(LongPackageName) );
					DiscoveredFilesToSearch.Add(Filename);
				}
			}
		}

		{
			// Place all the discovered files into the files to search list
			FScopeLock CritSectionLock(&WorkerThreadCriticalSection);
			FilesToSearch.Append(DiscoveredFilesToSearch);
			DiscoveredPaths.Append(LocalDiscoveredPaths);
		}
	}
}

bool FAssetDataGatherer::IsValidPackageFileToRead(const FString& Filename) const
{
	FString LongPackageName;
	if ( FPackageName::TryConvertFilenameToLongPackageName(Filename, LongPackageName) )
	{
		// Make sure the path does not contain invalid characters. These packages will not be successfully loaded or read later.
		for ( int32 CharIdx = 0; CharIdx < LongPackageName.Len() ; ++CharIdx )
		{
			if ( InvalidAssetFileCharacters.Contains(LongPackageName[CharIdx]) )
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

bool FAssetDataGatherer::ReadAssetFile(const FString& AssetFilename, TArray<FBackgroundAssetData*>& AssetDataList, FPackageDependencyData& DependencyData) const
{
	FPackageReader PackageReader;

	if ( !PackageReader.OpenPackageFile(AssetFilename) )
	{
		return false;
	}

	if ( !PackageReader.ReadAssetRegistryData(AssetDataList) )
	{
		if ( !PackageReader.ReadAssetDataFromThumbnailCache(AssetDataList) )
		{
			// It's ok to keep reading even if the asset registry data doesn't exist yet
			//return false;
		}
	}

	if ( bGatherDependsData )
	{
		if ( !PackageReader.ReadDependencyData(DependencyData) )
		{
			return false;
		}
	}

	return true;
}