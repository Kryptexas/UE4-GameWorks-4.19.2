// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "UProjectInfo.h"
#include "AssertionMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUProjectInfo, Verbose, All);
DEFINE_LOG_CATEGORY(LogUProjectInfo);

FUProjectDictionary::FUProjectDictionary(const FString& InRootDir)
{
	FString RootDir = FPaths::ConvertRelativePathToFull(InRootDir);

	TArray<FString> DirectoriesToSearch;
	
	// Find all the .uprojectdirs files contained in the root folder and add their entries to the search array
	TArray<FString> UProjectDirsFiles;
	FString SearchPath = RootDir / FString(TEXT("*.uprojectdirs"));
	IFileManager::Get().FindFiles(UProjectDirsFiles, *SearchPath, true, false);

	for (int32 FileIndex = 0; FileIndex < UProjectDirsFiles.Num(); ++FileIndex)
	{
		FString ProjDirsFile = RootDir / UProjectDirsFiles[FileIndex];

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

	for (int32 DirIdx = 0; DirIdx < DirectoriesToSearch.Num(); DirIdx++)
	{
		FString DirToSearch = DirectoriesToSearch[DirIdx];

		// To specific the <UE4> folder itself, './' is used... strip it out if present
		DirToSearch = DirToSearch.Replace(TEXT("/./"), TEXT("/"));

		// Get each sub-directory of this folder and check it for uproject files
		TArray<FString> SubDirectories;
		SearchPath = DirToSearch / FString(TEXT("*"));
		IFileManager::Get().FindFiles(SubDirectories, *SearchPath, false, true);
		
		for (int32 SubIdx = 0; SubIdx < SubDirectories.Num(); SubIdx++)
		{
			FString SubDir = DirToSearch / SubDirectories[SubIdx];

			TArray<FString> SubDirFiles;
			SearchPath = SubDir / TEXT("*.uproject");
			IFileManager::Get().FindFiles(SubDirFiles, *SearchPath, true, false);

			for (int32 UProjIdx = 0; UProjIdx < SubDirFiles.Num(); UProjIdx++)
			{
				FString ProjectFileName = SubDir / SubDirFiles[UProjIdx];
				FPaths::NormalizeFilename(ProjectFileName);

				FString ShortName = FPaths::GetBaseFilename(ProjectFileName);
				ShortProjectNameDictionary.Add(ShortName, ProjectFileName);
			}
		}
	}
}

bool FUProjectDictionary::IsForeignProject(const FString& InProjectFileName) const
{
	FString ProjectFileName = FPaths::ConvertRelativePathToFull(InProjectFileName);

	for(TMap<FString, FString>::TConstIterator Iter(ShortProjectNameDictionary); Iter; ++Iter)
	{
		if(Iter.Value() == ProjectFileName)
		{
			return false;
		}
	}

	return true;
}

FString FUProjectDictionary::GetRelativeProjectPathForGame(const TCHAR* InGameName, const FString& BaseDir) const
{
	const FString* ProjectFile = ShortProjectNameDictionary.Find(*(FString(InGameName).ToLower()));
	if (ProjectFile != NULL)
	{
		FString RelativePath = *ProjectFile;
		FPaths::MakePathRelativeTo(RelativePath, *BaseDir);
		return RelativePath;
	}
	return TEXT("");
}

TArray<FString> FUProjectDictionary::GetProjectPaths() const
{
	TArray<FString> Paths;
	ShortProjectNameDictionary.GenerateValueArray(Paths);
	return Paths;
}

FUProjectDictionary& FUProjectDictionary::GetDefault()
{
	static FUProjectDictionary DefaultDictionary(FPaths::RootDir());

#if !NO_LOGGING
	static bool bHaveLoggedProjects = false;
	if(!bHaveLoggedProjects)
	{
		UE_LOG(LogUProjectInfo, Log, TEXT("Found projects:"));
		for(TMap<FString, FString>::TConstIterator Iter(DefaultDictionary.ShortProjectNameDictionary); Iter; ++Iter)
		{
			UE_LOG(LogUProjectInfo, Log, TEXT("    %s: \"%s\""), *Iter.Key(), *Iter.Value());
		}
		bHaveLoggedProjects = true;
	}
#endif

	return DefaultDictionary;
}
