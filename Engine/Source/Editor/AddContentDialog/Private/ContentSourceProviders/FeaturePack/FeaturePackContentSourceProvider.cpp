// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AddContentDialogPCH.h"

#include "FeaturePackContentSourceProvider.h"
#include "FeaturePackContentSource.h"

class FFillArrayDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
{
public:
	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		if (bIsDirectory)
		{
			Directories.Add(FilenameOrDirectory);
		}
		else
		{
			Files.Add(FilenameOrDirectory);
		}
		return true;
	}

	TArray<FString> Directories;
	TArray<FString> Files;
};

FFeaturePackContentSourceProvider::FFeaturePackContentSourceProvider()
{
	//TODO: Add a directory watcher?
	FString FeaturePackPath = FPaths::Combine(*FPaths::RootDir(), TEXT("FeaturePacks"), TEXT("Packed"));
	IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FFillArrayDirectoryVisitor DirectoryVisitor;
	PlatformFile.IterateDirectory(*FeaturePackPath, DirectoryVisitor);
	for (auto FeaturePackFile : DirectoryVisitor.Files)
	{
		TUniquePtr<FFeaturePackContentSource> NewContentSource = MakeUnique<FFeaturePackContentSource>(FeaturePackFile);
		if(NewContentSource->IsDataValid())
		{
			ContentSources.Add(MakeShareable(NewContentSource.Release()));
		}
	}
}

const TArray<TSharedRef<IContentSource>> FFeaturePackContentSourceProvider::GetContentSources()
{
	return ContentSources;
}

void FFeaturePackContentSourceProvider::SetContentSourcesChanged(FOnContentSourcesChanged OnContentSourcesChangedIn)
{
	OnContentSourcesChanged = OnContentSourcesChangedIn;
}

FFeaturePackContentSourceProvider::~FFeaturePackContentSourceProvider()
{
}