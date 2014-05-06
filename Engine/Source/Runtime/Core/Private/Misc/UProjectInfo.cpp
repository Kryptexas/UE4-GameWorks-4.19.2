// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "UProjectInfo.h"
#include "AssertionMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUProjectInfo, Verbose, All);
DEFINE_LOG_CATEGORY(LogUProjectInfo);

FUProjectDictionary::FUProjectDictionary(const FString& InRootDir)
{
	double StartTime = FPlatformTime::Seconds();

	FString RootDir = InRootDir;
	FPaths::NormalizeDirectoryName(RootDir);

	TArray<FString> DirectoriesToSearch;
	
	// Find all the .uprojectdirs files contained in the root folder and add their entries to the search array
	TArray<FString> UProjectDirsFiles;
	FString SearchPath = RootDir / FString(TEXT("*.uprojectdirs"));
	IFileManager::Get().FindFiles(UProjectDirsFiles, *SearchPath, true, false);

	for (int32 FileIndex = 0; FileIndex < UProjectDirsFiles.Num(); ++FileIndex)
	{
		FString ProjDirsFile = RootDir / UProjectDirsFiles[FileIndex];
		UE_LOG(LogUProjectInfo, Log, TEXT("Found uprojectdirs file %s"), *ProjDirsFile);

		TArray<FString> FileStrings;
		if (FFileHelper::LoadANSITextFileToStrings(*ProjDirsFile, &IFileManager::Get(), FileStrings))
		{
			for (int32 EntryIdx = 0; EntryIdx < FileStrings.Num(); EntryIdx++)
			{
				FString ProjDirEntry = FileStrings[EntryIdx];
				ProjDirEntry.Trim();
				if (ProjDirEntry.IsEmpty() == false)
				{
					if (ProjDirEntry.StartsWith(";"))
					{
						// Commented out line... skip it
						continue;
					}
					else
					{
						DirectoriesToSearch.Add(RootDir / ProjDirEntry);
					}
				}
			}
		}
	}

	UE_LOG(LogUProjectInfo, Log, TEXT("Found %d directories to search"), DirectoriesToSearch.Num());

	for (int32 DirIdx = 0; DirIdx < DirectoriesToSearch.Num(); DirIdx++)
	{
		FString DirToSearch = DirectoriesToSearch[DirIdx];
		UE_LOG(LogUProjectInfo, Log, TEXT("\tSearching %s"), *DirToSearch);

		// To specific the <UE4> folder itself, './' is used... strip it out if present
		DirToSearch = DirToSearch.Replace(TEXT("/./"), TEXT("/"));

		// Get each sub-directory of this folder and check it for uproject files
		TArray<FString> SubDirectories;
		SearchPath = DirToSearch / FString(TEXT("*"));
		IFileManager::Get().FindFiles(SubDirectories, *SearchPath, false, true);
		UE_LOG(LogUProjectInfo, Log, TEXT("\t\tFound %2d subdirectories"), SubDirectories.Num());
		
		for (int32 SubIdx = 0; SubIdx < SubDirectories.Num(); SubIdx++)
		{
			FString SubDir = DirToSearch / SubDirectories[SubIdx];
			UE_LOG(LogUProjectInfo, Log, TEXT("\t\t\tSubdir %s"), *SubDir);

			TArray<FString> SubDirFiles;
			SearchPath = SubDir / TEXT("*.uproject");
			IFileManager::Get().FindFiles(SubDirFiles, *SearchPath, true, false);
			UE_LOG(LogUProjectInfo, Log, TEXT("\t\t\t\tFound %2d uproject files"), SubDirFiles.Num());

			for (int32 UProjIdx = 0; UProjIdx < SubDirFiles.Num(); UProjIdx++)
			{
				FString ProjectFileName = SubDir / SubDirFiles[UProjIdx];
				FPaths::NormalizeFilename(ProjectFileName);

				FString ShortName = FPaths::GetBaseFilename(ProjectFileName);
				ShortProjectNameDictionary.Add(ShortName, ProjectFileName);
			}
		}
	}

	double TotalProjectInfoTime = FPlatformTime::Seconds() - StartTime;
	UE_LOG(LogUProjectInfo, Log, TEXT("FillProjectInfo took %5.4f seconds"), TotalProjectInfoTime);
}

bool FUProjectDictionary::IsForeignProject(const FString& InProjectFileName)
{
	FString ProjectFileName = InProjectFileName;
	FPaths::NormalizeFilename(ProjectFileName);

	for(TMap<FString, FString>::TConstIterator Iter(ShortProjectNameDictionary); Iter; ++Iter)
	{
		if(Iter.Value() == ProjectFileName)
		{
			return false;
		}
	}

	return true;
}

FString FUProjectDictionary::GetRelativeProjectPathForGame(const TCHAR* InGameName, const FString& BaseDir)
{
	FString* ProjectFile = ShortProjectNameDictionary.Find(*(FString(InGameName).ToLower()));
	if (ProjectFile != NULL)
	{
		FString RelativePath = *ProjectFile;
		FPaths::MakePathRelativeTo(RelativePath, *BaseDir);
		return RelativePath;
	}
	return TEXT("");
}

FUProjectDictionary& FUProjectDictionary::GetDefault()
{
	static FUProjectDictionary DefaultDictionary(FPaths::RootDir());
	return DefaultDictionary;
}
