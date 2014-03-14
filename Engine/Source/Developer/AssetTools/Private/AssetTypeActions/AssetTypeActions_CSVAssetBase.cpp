// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_CSVAssetBase::ExecuteEdit(TArray< TWeakObjectPtr<UObject> > Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FAssetEditorManager::Get().OpenEditorForAsset(Object);
		}
	}
}

void FAssetTypeActions_CSVAssetBase::ExecuteReimport( TArray< TWeakObjectPtr<UObject> > Objects )
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FReimportManager::Instance()->Reimport(Object, /*bAskForNewFileIfMissing=*/true);
		}
	}
}

void FAssetTypeActions_CSVAssetBase::ExecuteFindInExplorer(TArray<FString> Filenames)
{
	for (TArray<FString>::TConstIterator FilenameIter(Filenames); FilenameIter; ++FilenameIter)
	{
		const FString FullImportPath = FPaths::GetPath(FPaths::ConvertRelativePathToFull(*FilenameIter));
		if (!FullImportPath.IsEmpty() && IFileManager::Get().DirectoryExists(*FullImportPath))
		{
			FPlatformProcess::ExploreFolder(*FullImportPath);
		}
	}
}

void FAssetTypeActions_CSVAssetBase::ExecuteOpenInExternalEditor(TArray<FString> Filenames, TArray<FString> OverrideExtensions)
{
	for (TArray<FString>::TConstIterator FilenameIter(Filenames); FilenameIter; ++FilenameIter)
	{
		const FString CSVFilename = FPaths::ConvertRelativePathToFull(*FilenameIter);
		if (OverrideExtensions.Num() > 0)
		{
			const FString RootPath = FPaths::GetPath(CSVFilename);
			const FString BaseFilename = FPaths::GetBaseFilename(CSVFilename, true);
			for (TArray<FString>::TConstIterator ExtensionItr(OverrideExtensions); ExtensionItr; ++ExtensionItr)
			{
				const FString FilenameWithExtension(FString::Printf(TEXT("%s/%s%s"), *RootPath, *BaseFilename, **ExtensionItr));
				if (VerifyFileExists(FilenameWithExtension))
				{
					FPlatformProcess::LaunchFileInDefaultExternalApplication(*FilenameWithExtension);
					break;
				}
			}
		}
		else if (VerifyFileExists(CSVFilename))
		{
			FPlatformProcess::LaunchFileInDefaultExternalApplication(*CSVFilename);
		}
	}
}

bool FAssetTypeActions_CSVAssetBase::CanExecuteLaunchExternalSourceCommands(TArray<FString> Filenames, TArray<FString> OverrideExtensions) const
{
	bool bCanExecute = false;

	for (TArray<FString>::TConstIterator FilenameIter(Filenames); FilenameIter; ++FilenameIter)
	{
		const FString CSVFilename = FPaths::ConvertRelativePathToFull(*FilenameIter);
		if (OverrideExtensions.Num() > 0)
		{
			const FString RootPath = FPaths::GetPath(CSVFilename);
			const FString BaseFilename = FPaths::GetBaseFilename(CSVFilename, true);
			for (TArray<FString>::TConstIterator ExtensionItr(OverrideExtensions); ExtensionItr; ++ExtensionItr)
			{
				const FString FilenameWithExtension(FString::Printf(TEXT("%s/%s%s"), *RootPath, *BaseFilename, **ExtensionItr));
				if (VerifyFileExists(FilenameWithExtension))
				{
					bCanExecute = true;
					break;
				}
			}
		}
		else if (VerifyFileExists(CSVFilename))
		{
			bCanExecute = true;
		}
	}

	return bCanExecute;
}

bool FAssetTypeActions_CSVAssetBase::CanExecuteFindInExplorerSourceCommand(TArray<FString> Filenames) const
{
	bool bCanExecute = false;
	for (TArray<FString>::TConstIterator FilenameItr(Filenames); FilenameItr; ++FilenameItr)
	{
		const FString FullImportPath = FPaths::GetPath(FPaths::ConvertRelativePathToFull(*FilenameItr));
		if (!FullImportPath.IsEmpty() && IFileManager::Get().DirectoryExists(*FullImportPath))
		{
			bCanExecute = true;
			break;
		}
	}

	return bCanExecute;
}

bool FAssetTypeActions_CSVAssetBase::VerifyFileExists(const FString& InFileName) const
{
	return (!InFileName.IsEmpty() && IFileManager::Get().FileSize(*InFileName) != INDEX_NONE);
}

#undef LOCTEXT_NAMESPACE