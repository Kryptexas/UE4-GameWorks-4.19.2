// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetToolsModule.h"
#include "Toolkits/SimpleAssetEditor.h"

/** A base class for all AssetTypeActions. Provides helper functions useful for many types. Deriving from this class is optional. */
class FAssetTypeActions_Base : public IAssetTypeActions
{
public:
	// Begin IAssetTypeActions implementation
	virtual bool HasActions( const TArray<UObject*>& InObjects ) const OVERRIDE
	{
		return false;
	}

	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE
	{

	}

	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE
	{
		FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
	}
	
	virtual void AssetsActivated( const TArray<UObject*>& InObjects, EAssetTypeActivationMethod::Type ActivationType ) OVERRIDE
	{
		if ( ActivationType == EAssetTypeActivationMethod::DoubleClicked || ActivationType == EAssetTypeActivationMethod::EnterPressed )
		{
			if ( InObjects.Num() == 1 )
			{
				FAssetEditorManager::Get().OpenEditorForAsset(InObjects[0]);
			}
			else if ( InObjects.Num() > 1 )
			{
				FAssetEditorManager::Get().OpenEditorForAssets(InObjects);
			}
		}
	}

	virtual bool CanFilter() 
	{
		return true;
	}

	virtual bool ShouldForceWorldCentric()
	{
		return false;
	}

	virtual void PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const OVERRIDE
	{
		check(OldAsset != NULL);
		check(NewAsset != NULL);

		// Dump assets to temp text files
		FString OldTextFilename = DumpAssetToTempFile(OldAsset);
		FString NewTextFilename = DumpAssetToTempFile(NewAsset);
		FString DiffCommand = GetDefault<UEditorLoadingSavingSettings>()->TextDiffToolPath.FilePath;

		// args are just 2 temp filenames
		FString DiffArgs = FString::Printf(TEXT("%s %s"), *OldTextFilename, *NewTextFilename);

		CreateDiffProcess(DiffCommand, DiffArgs);
	}

	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const OVERRIDE
	{
		return NULL;
	}

	// End IAssetTypeActions implementation

protected:

	// Here are some convenience functions for common asset type actions logic

	/** Creates a unique package and asset name taking the form InBasePackageName+InSuffix */
	virtual void CreateUniqueAssetName(const FString& InBasePackageName, const FString& InSuffix, FString& OutPackageName, FString& OutAssetName) const
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(InBasePackageName, InSuffix, OutPackageName, OutAssetName);
	}

	/** Util for dumping an asset to a temporary text file. Returns absolute filename to temp file */
	virtual FString DumpAssetToTempFile(UObject* Asset) const
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		return AssetToolsModule.Get().DumpAssetToTempFile(Asset);
	}

	/* Attempt to spawn diff tool as external process */
	virtual bool CreateDiffProcess(const FString& DiffCommand, const FString& DiffArgs) const
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		return AssetToolsModule.Get().CreateDiffProcess(DiffCommand, DiffArgs);
	}

	/** Returns additional tooltip information for the specified asset, if it has any (otherwise return the null widget) */
	virtual FText GetAssetDescription(const FAssetData& AssetData) const
	{
		return FText::GetEmpty();
	}

	/** Helper function to convert the input for GetActions to a list that can be used for delegates */
	template <typename T>
	static TArray<TWeakObjectPtr<T>> GetTypedWeakObjectPtrs(const TArray<UObject*>& InObjects)
	{
		check(InObjects.Num() > 0);

		TArray<TWeakObjectPtr<T>> TypedObjects;
		for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			TypedObjects.Add( CastChecked<T>(*ObjIt) );
		}

		return TypedObjects;
	}
};