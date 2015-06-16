// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CollectionManagerPrivatePCH.h"
#include "ISourceControlModule.h"

#define LOCTEXT_NAMESPACE "CollectionManager"


enum ECollectionFileVersion
{
	// The initial version for collection files
	COLLECTION_VER_INITIAL								= 1,

	// The automatic version count. This must always be at the end of the enum
	COLLECTION_VER_AUTOMATIC_VERSION_PLUS_ONE
};

struct FCollectionUtils
{
	static void AppendCollectionToArray(const TSet<FName>& InAssetSet, TArray<FName>& OutAssetArray)
	{
		OutAssetArray.Reserve(OutAssetArray.Num() + InAssetSet.Num());
		for (const FName& AssetName : InAssetSet)
		{
			OutAssetArray.Add(AssetName);
		}
	}

	static TArray<FName> CreateCollectionArray(const TSet<FName>& InAssetSet)
	{
		TArray<FName> AssetArray;
		AppendCollectionToArray(InAssetSet, AssetArray);
		return AssetArray;
	}
};

FCollection::FCollection()
{

}

FCollection::FCollection(FName InCollectionName, const FString& SourceFolder, const FString& CollectionExtension, bool InUseSCC)
{
	CollectionName = InCollectionName;
	bUseSCC = InUseSCC;

	// Initialize the file version to the most recent
	FileVersion = COLLECTION_VER_AUTOMATIC_VERSION_PLUS_ONE - 1;

	if ( ensure(SourceFolder.Len()) )
	{
		SourceFilename = SourceFolder / CollectionName.ToString() + TEXT(".") + CollectionExtension;
		SourceFilename.ReplaceInline(TEXT("//"), TEXT("/"));
	}
}

bool FCollection::LoadFromFile(const FString& InFilename, bool InUseSCC)
{
	Clear();

	FString FullFileContentsString;
	if ( !FFileHelper::LoadFileToString(FullFileContentsString, *InFilename) )
	{
		return false;
	}

	// Initialize the FCollection and set up the collection name
	bUseSCC = InUseSCC;
	SourceFilename = InFilename;
	CollectionName = FName(*FPaths::GetBaseFilename(InFilename));
		
	// Normalize line endings and parse into array
	TArray<FString> FileContents;
	FullFileContentsString.ReplaceInline(TEXT("\r"), TEXT(""));
	FullFileContentsString.ParseIntoArray(FileContents, TEXT("\n"), /*bCullEmpty=*/false);

	if ( FileContents.Num() == 0 )
	{
		// Empty file, assume static collection with no items
		return true;
	}

	// Load the header from the contents array
	TMap<FString,FString> HeaderPairs;
	while ( FileContents.Num() )
	{
		// Pop the 0th element from the contents array and read it
		const FString Line = FileContents[0].Trim().TrimTrailing();
		FileContents.RemoveAt(0);

		if (Line.Len() == 0)
		{
			// Empty line. Done reading headers.
			break;
		}

		FString Key;
		FString Value;
		if ( Line.Split(TEXT(":"), &Key, &Value) )
		{
			HeaderPairs.Add(Key, Value);
		}
	}

	// Now process the header pairs to prepare and validate this collection
	if ( !LoadHeaderPairs(HeaderPairs) )
	{
		// Bad header
		return false;
	}

	// Now load the content if the header load was successful
	if ( IsDynamic() )
	{
		// @todo collection Load dynamic collections
	}
	else
	{
		// Static collection, a flat list of asset paths
		for (FString Line : FileContents)
		{
			Line.Trim();
			Line.TrimTrailing();

			if ( Line.Len() )
			{
				AddAssetToCollection(FName(*Line));
			}
		}

		DiskAssetSet = AssetSet;
	}

	return true;
}

bool FCollection::Save(FText& OutError)
{
	if ( !ensure(SourceFilename.Len()) )
	{
		OutError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	// Store the start time for profiling reasons
	double SaveStartTime = FPlatformTime::Seconds();

	// Keep track of save progress to update the slow task dialog
	const int32 SaveProgressDenominator = 3;
	int32 SaveProgressNumerator = 0;


	GWarn->BeginSlowTask( FText::Format( LOCTEXT("SavingCollection", "Saving Collection {0}"), FText::FromName( CollectionName ) ), true);
	GWarn->UpdateProgress(SaveProgressNumerator++, SaveProgressDenominator);

	if ( bUseSCC )
	{
		// Checkout the file
		if ( !CheckoutCollection(OutError) )
		{
			UE_LOG(LogCollectionManager, Error, TEXT("Failed to check out a collection file: %s"), *CollectionName.ToString());
			GWarn->EndSlowTask();
			return false;
		}
	}

	GWarn->UpdateProgress(SaveProgressNumerator++, SaveProgressDenominator);

	// Generate a string with the file contents
	FString FileOutput;

	// Start with the header
	TMap<FString,FString> HeaderPairs;
	GenerateHeaderPairs(HeaderPairs);
	for (const auto& HeaderPair : HeaderPairs)
	{
		FileOutput += HeaderPair.Key + TEXT(":") + HeaderPair.Value + LINE_TERMINATOR;
	}
	FileOutput += LINE_TERMINATOR;

	// Now for the content
	if ( IsDynamic() )
	{
		// @todo Dynamic collections
	}
	else
	{
		// Write out the set as a sorted array to keep things in a known order for diffing
		TArray<FName> AssetList = FCollectionUtils::CreateCollectionArray(AssetSet);
		AssetList.Sort();

		// Static collection. Save a flat list of all assets in the collection.
		for (const FName& AssetName : AssetList)
		{
			FileOutput += AssetName.ToString() + LINE_TERMINATOR;
		}
	}

	// Attempt to save the file
	bool bSaveSuccessful = false;
	if ( ensure(FileOutput.Len()) )
	{
		// We have some output, write it to file
		if ( FFileHelper::SaveStringToFile(FileOutput, *SourceFilename) )
		{
			bSaveSuccessful = true;
		}
		else
		{
			OutError = LOCTEXT("Error_WriteFailed", "Failed to write to collection file.");
			UE_LOG(LogCollectionManager, Error, TEXT("%s %s"), *OutError.ToString(), *CollectionName.ToString());
		}
	}
	else
	{
		OutError = LOCTEXT("Error_Internal", "There was an internal error.");
	}

	GWarn->UpdateProgress(SaveProgressNumerator++, SaveProgressDenominator);

	if ( bSaveSuccessful )
	{
		if ( bUseSCC )
		{
			// Check in the file if the save was successful
			if ( bSaveSuccessful )
			{
				if ( !CheckinCollection(OutError) )
				{
					UE_LOG(LogCollectionManager, Error, TEXT("Failed to check in a collection successfully saving: %s"), *CollectionName.ToString());
					bSaveSuccessful = false;
				}
			}
		
			// If the save was not successful or the checkin failed, revert
			if ( !bSaveSuccessful )
			{
				FText Unused;
				if ( !RevertCollection(Unused) )
				{
					// The revert failed... file will be left on disk as it was saved.
					// DiskAssetList will still hold the version of the file when this collection was last loaded or saved successfully so nothing will be out of sync.
					// If the user closes the editor before successfully saving, this file may not be exactly what was seen at the time the editor closed.
					UE_LOG(LogCollectionManager, Warning, TEXT("Failed to revert a checked out collection after failing to save or checkin: %s"), *CollectionName.ToString());
				}
			}
		}
	}

	GWarn->UpdateProgress(SaveProgressNumerator++, SaveProgressDenominator);

	if ( bSaveSuccessful )
	{
		DiskAssetSet = AssetSet;
	}

	GWarn->EndSlowTask();

	UE_LOG(LogCollectionManager, Verbose, TEXT("Saved collection %s in %0.6f seconds"), *CollectionName.ToString(), FPlatformTime::Seconds() - SaveStartTime);

	return bSaveSuccessful;
}

bool FCollection::DeleteSourceFile(FText& OutError)
{
	bool bSuccessfullyDeleted = false;

	if ( SourceFilename.Len() )
	{
		if ( bUseSCC )
		{
			bSuccessfullyDeleted = DeleteFromSourceControl(OutError);
		}
		else
		{
			bSuccessfullyDeleted = IFileManager::Get().Delete(*SourceFilename);
			if ( !bSuccessfullyDeleted )
			{
				OutError = LOCTEXT("Error_DiskDeleteFailed", "Failed to delete the collection file from disk.");
			}
		}
	}
	else
	{
		// No source file. Since it doesn't exist we will say it is deleted.
		bSuccessfullyDeleted = true;
	}

	if ( bSuccessfullyDeleted )
	{
		DiskAssetSet.Empty();
	}

	return bSuccessfullyDeleted;
}

bool FCollection::AddAssetToCollection(FName ObjectPath)
{
	// @todo collection Does this apply to dynamic collections?
	if ( !AssetSet.Contains(ObjectPath) )
	{
		AssetSet.Add(ObjectPath);

		return true;
	}

	return false;
}

bool FCollection::RemoveAssetFromCollection(FName ObjectPath)
{
	// @todo collection Does this apply to dynamic collections?
	return AssetSet.Remove(ObjectPath) > 0;
}

void FCollection::GetAssetsInCollection(TArray<FName>& Assets) const
{
	// @todo collection Does this apply to dynamic collections?
	for (const FName& AssetName : AssetSet)
	{
		if (!AssetName.ToString().StartsWith(TEXT("/Script/")))
		{
			Assets.Add(AssetName);
		}
	}
}

void FCollection::GetClassesInCollection(TArray<FName>& Classes) const
{
	// @todo collection Does this apply to dynamic collections?
	for (const FName& AssetName : AssetSet)
	{
		if (AssetName.ToString().StartsWith(TEXT("/Script/")))
		{
			Classes.Add(AssetName);
		}
	}
}

void FCollection::GetObjectsInCollection(TArray<FName>& Objects) const
{
	// @todo collection Does this apply to dynamic collections?
	FCollectionUtils::AppendCollectionToArray(AssetSet, Objects);
}

bool FCollection::IsObjectInCollection(FName ObjectPath) const
{
	// @todo collection Does this apply to dynamic collections?
	return AssetSet.Contains(ObjectPath);
}

void FCollection::Clear()
{
	CollectionName = NAME_None;
	bUseSCC = false;
	SourceFilename = TEXT("");
	AssetSet.Empty();
	DiskAssetSet.Empty();
}

bool FCollection::IsDynamic() const
{
	// @todo collection Dynamic collections
	return false;
}
	
bool FCollection::IsEmpty() const
{
	return AssetSet.Num() == 0;
}

void FCollection::PrintCollection() const
{
	if ( IsDynamic() )
	{
		// @todo collection Printing Dynamic collections
	}
	else
	{
		UE_LOG(LogCollectionManager, Log, TEXT("    Printing static elements of collection %s"), *CollectionName.ToString());
		UE_LOG(LogCollectionManager, Log, TEXT("    ============================="));

		// Print the set as a sorted array to keep things in a sane order
		TArray<FName> AssetList = FCollectionUtils::CreateCollectionArray(AssetSet);
		AssetList.Sort();

		for (const FName& AssetName : AssetList)
		{
			UE_LOG(LogCollectionManager, Log, TEXT("        %s"), *AssetName.ToString());
		}
	}
}

void FCollection::GenerateHeaderPairs(TMap<FString,FString>& OutHeaderPairs)
{
	// These pairs will appear at the top of the file followed by a newline
	ensure(FileVersion > 0);
	OutHeaderPairs.Add(TEXT("FileVersion"), FString::FromInt(FileVersion));
	OutHeaderPairs.Add(TEXT("Type"), IsDynamic() ? TEXT("Dynamic") : TEXT("Static"));
}

bool FCollection::LoadHeaderPairs(const TMap<FString,FString>& InHeaderPairs)
{
	// These pairs will appeared at the top of the file being loaded
	// First find all the known pairs
	const FString* Version = InHeaderPairs.Find(TEXT("FileVersion"));
	if ( !Version )
	{
		// FileVersion is required
		return false;
	}

	const FString* Type = InHeaderPairs.Find(TEXT("Type"));
	if ( !Type )
	{
		// Type is required
		return false;
	}

	FileVersion = FCString::Atoi(**Version);

	if ( *Type == TEXT("Dynamic") )
	{
		// @todo Set this file up to be dynamic
	}

	return FileVersion > 0;
}

void FCollection::MergeWithCollection(const FCollection& Other)
{
	if ( IsDynamic() )
	{
		// @todo collection Merging dynamic collections?
	}
	else
	{
		// Gather the differences from the file on disk
		TArray<FName> AssetsAdded;
		TArray<FName> AssetsRemoved;
		GetDifferencesFromDisk(AssetsAdded, AssetsRemoved);

		// Copy asset list from other collection
		DiskAssetSet = Other.DiskAssetSet;
		AssetSet = DiskAssetSet;

		// Add the assets that were added before the merge
		for (const FName& AddedAssetName : AssetsAdded)
		{
			AssetSet.Add(AddedAssetName);
		}

		// Remove the assets that were removed before the merge
		for (const FName& RemovedAssetName : AssetsRemoved)
		{
			AssetSet.Remove(RemovedAssetName);
		}
	}
}

void FCollection::GetDifferencesFromDisk(TArray<FName>& AssetsAdded, TArray<FName>& AssetsRemoved)
{
	// Find the assets that were removed since the disk list
	for (const FName& DiskAssetName : DiskAssetSet)
	{
		if (!AssetSet.Contains(DiskAssetName))
		{
			AssetsRemoved.Add(DiskAssetName);
		}
	}

	// Find the assets that were added since the disk list
	for (const FName& MemoryAssetName : AssetSet)
	{
		if (!DiskAssetSet.Contains(MemoryAssetName))
		{
			AssetsAdded.Add(MemoryAssetName);
		}
	}
}

bool FCollection::CheckoutCollection(FText& OutError)
{
	if ( !ensure(SourceFilename.Len()) )
	{
		OutError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( !ISourceControlModule::Get().IsEnabled() )
	{
		OutError = LOCTEXT("Error_SCCDisabled", "Source control is not enabled. Enable source control in the preferences menu.");
		return false;
	}

	if ( !SourceControlProvider.IsAvailable() )
	{
		OutError = LOCTEXT("Error_SCCNotAvailable", "Source control is currently not available. Check your connection and try again.");
		return false;
	}

	const FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(SourceFilename);
	FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);

	bool bSuccessfullyCheckedOut = false;

	if (SourceControlState.IsValid() && SourceControlState->IsDeleted())
	{
		// Revert our delete
		if ( !RevertCollection(OutError) )
		{
			return false;
		}

		// Make sure we get a fresh state from the server
		SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
	}

	// If not at the head revision, sync up
	if (SourceControlState.IsValid() && !SourceControlState->IsCurrent())
	{
		if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FSync>(), AbsoluteFilename) == ECommandResult::Failed )
		{
			// Could not sync up with the head revision
			OutError = LOCTEXT("Error_SCCSync", "Failed to sync collection to the head revision.");
			return false;
		}

		// Check to see if the file exists at the head revision
		if ( IFileManager::Get().FileSize(*SourceFilename) < 0 )
		{
			// File was deleted at head...
			// Just add our changes to an empty list so we can mark it for add
			FCollection NewCollection;
			MergeWithCollection(NewCollection);
		}
		else
		{
			// File found! Load it and merge with our local changes
			FCollection NewCollection;
			if ( !NewCollection.LoadFromFile(SourceFilename, false) )
			{
				// Failed to load the head revision file so it isn't safe to delete it
				OutError = LOCTEXT("Error_SCCBadHead", "Failed to load the collection at the head revision.");
				return false;
			}

			// Loaded the head revision, now merge up so the files are in a consistent state
			MergeWithCollection(NewCollection);
		}

		// Make sure we get a fresh state from the server
		SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
	}

	if(SourceControlState.IsValid())
	{
		if(!SourceControlState->IsSourceControlled())
		{
			// Not yet in the depot. We'll add it when we call CheckinCollection
			bSuccessfullyCheckedOut = true;
		}
		else if(SourceControlState->IsAdded() || SourceControlState->IsCheckedOut())
		{
			// Already checked out or opened for add
			bSuccessfullyCheckedOut = true;
		}
		else if(SourceControlState->CanCheckout())
		{
			// In depot and needs to be checked out
			bSuccessfullyCheckedOut = (SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), AbsoluteFilename) == ECommandResult::Succeeded);
			if (!bSuccessfullyCheckedOut)
			{
				OutError = LOCTEXT("Error_SCCCheckout", "Failed to check out collection.");
			}
		}
		else if(!SourceControlState->IsCurrent())
		{
			OutError = LOCTEXT("Error_SCCNotCurrent", "Collection is not at head revision after sync.");
		}
		else if(SourceControlState->IsCheckedOutOther())
		{
			OutError = LOCTEXT("Error_SCCCheckedOutOther", "Collection is checked out by another user.");
		}
		else
		{
			OutError = LOCTEXT("Error_SCCUnknown", "Could not determine source control state.");
		}
	}
	else
	{
		OutError = LOCTEXT("Error_SCCInvalid", "Source control state is invalid.");
	}

	return bSuccessfullyCheckedOut;
}

bool FCollection::CheckinCollection(FText& OutError)
{
	if ( !ensure(SourceFilename.Len()) )
	{
		OutError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( !ISourceControlModule::Get().IsEnabled() )
	{
		OutError = LOCTEXT("Error_SCCDisabled", "Source control is not enabled. Enable source control in the preferences menu.");
		return false;
	}

	if ( !SourceControlProvider.IsAvailable() )
	{
		OutError = LOCTEXT("Error_SCCNotAvailable", "Source control is currently not available. Check your connection and try again.");
		return false;
	}

	const FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(SourceFilename);
	FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);

	if (SourceControlState.IsValid() && !SourceControlState->IsSourceControlled())
	{
		// Not yet in the depot. Add it.
		const bool bWasAdded = (SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), AbsoluteFilename) == ECommandResult::Succeeded);
		if (!bWasAdded)
		{
			OutError = LOCTEXT("Error_SCCAdd", "Failed to add new collection to source control.");
			return false;
		}
		SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
	}

	if ( SourceControlState.IsValid() && !(SourceControlState->IsCheckedOut() || SourceControlState->IsAdded()) )
	{
		OutError = LOCTEXT("Error_SCCNotCheckedOut", "Collection not checked out or open for add.");
		return false;
	}

	// Form an appropriate summary for the changelist
	const FText CollectionNameText = FText::FromName( CollectionName );
	FText ChangelistDesc = FText::Format( LOCTEXT("CollectionGenericModifiedDesc", "Modified collection: {0}"), CollectionNameText );
	if ( SourceControlState.IsValid() && SourceControlState->IsAdded() )
	{
		ChangelistDesc = FText::Format( LOCTEXT("CollectionAddedNewDesc", "Added collection"), CollectionNameText);
	}
	else if ( IsDynamic() )
	{
		// @todo collection Change description for dynamic collections
	}
	else
	{
		// Gather differences from disk
		TArray<FName> AssetsAdded;
		TArray<FName> AssetsRemoved;
		GetDifferencesFromDisk(AssetsAdded, AssetsRemoved);

		AssetsAdded.Sort();
		AssetsRemoved.Sort();

		// Clear description
		ChangelistDesc = FText::GetEmpty();

		// Report added files
		FFormatNamedArguments Args;
		Args.Add( TEXT("AssetAdded"), AssetsAdded.Num() > 0 ? FText::FromName( AssetsAdded[0] ) : NSLOCTEXT( "Core", "None", "None" ) );
		Args.Add( TEXT("NumberAdded"), FText::AsNumber( FMath::Max( AssetsAdded.Num() - 1, 0 ) ));
		Args.Add( TEXT("CollectionName"), CollectionNameText );

		if ( AssetsRemoved.Num() == 0 )
		{
			if (AssetsAdded.Num() == 1)
			{
				ChangelistDesc = FText::Format( LOCTEXT("CollectionAddedSingleDesc", "Added {AssetAdded} to collection: {CollectionName}"), Args );
			}
			else if (AssetsAdded.Num() > 1)
			{
				ChangelistDesc = FText::Format( LOCTEXT("CollectionAddedMultipleDesc", "Added {AssetAdded} and {NumberAdded} other(s) to collection: {CollectionName}"), Args );
			}
		}
		else
		{
			Args.Add( TEXT("AssetRemoved"), FText::FromName( AssetsRemoved[0] ) );
			Args.Add( TEXT("NumberRemoved"), FText::AsNumber( AssetsRemoved.Num() - 1 ) );

			if ( AssetsAdded.Num() == 1 )
			{
				if ( AssetsRemoved.Num() == 1 )
				{
					ChangelistDesc = FText::Format( LOCTEXT("CollectionRemovedSingle_AddedSingleDesc", "Added {AssetAdded} to collection: {CollectionName} : Removed {AssetRemoved} from collection: {CollectionName}"), Args );
				}
				else if (AssetsRemoved.Num() > 1)
				{
					ChangelistDesc = FText::Format( LOCTEXT("CollectionRemovedMultiple_AddedSingleDesc", "Added {AssetAdded} to collection: {CollectionName} : Removed {AssetRemoved} and {NumberRemoved} other(s) from collection: {CollectionName}"), Args );
				}
			}
			else if (AssetsAdded.Num() > 1)
			{
				if ( AssetsRemoved.Num() == 1 )
				{
					ChangelistDesc = FText::Format( LOCTEXT("CollectionRemovedSingle_AddedMultpleDesc", "Added {AssetAdded} and {NumberAdded} other(s) to collection: {CollectionName} : Removed {AssetRemoved} from collection: {CollectionName}"), Args );
				}
				else if (AssetsRemoved.Num() > 1)
				{
					ChangelistDesc = FText::Format( LOCTEXT("CollectionRemovedMultiple_AddedMultpleDesc", "Added {AssetAdded} and {NumberAdded} other(s) to collection: {CollectionName} : Removed {AssetRemoved} and {NumberRemoved} other(s) from collection: {CollectionName}"), Args );
				}
			}
			else
			{
				if ( AssetsRemoved.Num() == 1 )
				{
					ChangelistDesc = FText::Format( LOCTEXT("CollectionRemovedSingleDesc", "Removed {AssetRemoved} from collection: {CollectionName}"), Args );
				}
				else if (AssetsRemoved.Num() > 1)
				{
					ChangelistDesc = FText::Format( LOCTEXT("CollectionRemovedMultipleDesc", "Removed {AssetRemoved} and {NumberRemoved} other(s) from collection: {CollectionName}"), Args );
				}
			}
		}
	}

	if ( ChangelistDesc.IsEmpty() )
	{
		// No files were added or removed
		FFormatNamedArguments Args;
		Args.Add( TEXT("CollectionName"), CollectionNameText );
		ChangelistDesc = FText::Format( LOCTEXT("CollectionNotModifiedDesc", "Collection not modified: {CollectionName}"), Args );
	}

	// Finally check in the file
	TSharedRef<FCheckIn, ESPMode::ThreadSafe> CheckInOperation = ISourceControlOperation::Create<FCheckIn>();
	CheckInOperation->SetDescription( ChangelistDesc );
	if ( SourceControlProvider.Execute( CheckInOperation, AbsoluteFilename ) )
	{
		return true;
	}
	else 
	{
		OutError = LOCTEXT("Error_SCCCheckIn", "Failed to check in collection.");
		return false;
	}
}

bool FCollection::RevertCollection(FText& OutError)
{
	if ( !ensure(SourceFilename.Len()) )
	{
		OutError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( !ISourceControlModule::Get().IsEnabled() )
	{
		OutError = LOCTEXT("Error_SCCDisabled", "Source control is not enabled. Enable source control in the preferences menu.");
		return false;
	}

	if ( !SourceControlProvider.IsAvailable() )
	{
		OutError = LOCTEXT("Error_SCCNotAvailable", "Source control is currently not available. Check your connection and try again.");
		return false;
	}

	FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(SourceFilename);
	FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);

	if ( SourceControlState.IsValid() && !(SourceControlState->IsCheckedOut() || SourceControlState->IsAdded()) )
	{
		OutError = LOCTEXT("Error_SCCNotCheckedOut", "Collection not checked out or open for add.");
		return false;
	}

	if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FRevert>(), AbsoluteFilename) == ECommandResult::Succeeded)
	{
		return true;
	}
	else
	{
		OutError = LOCTEXT("Error_SCCRevert", "Could not revert Collection.");
		return false;
	}
}

bool FCollection::DeleteFromSourceControl(FText& OutError)
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( !ISourceControlModule::Get().IsEnabled() )
	{
		OutError = LOCTEXT("Error_SCCDisabled", "Source control is not enabled. Enable source control in the preferences menu.");
		return false;
	}

	if ( !SourceControlProvider.IsAvailable() )
	{
		OutError = LOCTEXT("Error_SCCNotAvailable", "Source control is currently not available. Check your connection and try again.");
		return false;
	}

	bool bDeletedSuccessfully = false;

	const int32 DeleteProgressDenominator = 2;
	int32 DeleteProgressNumerator = 0;

	const FText CollectionNameText = FText::FromName( CollectionName );

	FFormatNamedArguments Args;
	Args.Add( TEXT("CollectionName"), CollectionNameText );
	const FText StatusUpdate = FText::Format( LOCTEXT("DeletingCollection", "Deleting Collection {CollectionName}"), Args );

	GWarn->BeginSlowTask( StatusUpdate, true );
	GWarn->UpdateProgress(DeleteProgressNumerator++, DeleteProgressDenominator);

	FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(SourceFilename);
	FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
	
	GWarn->UpdateProgress(DeleteProgressNumerator++, DeleteProgressDenominator);

	// If checked out locally for some reason, revert
	if (SourceControlState.IsValid() && (SourceControlState->IsAdded() || SourceControlState->IsCheckedOut() || SourceControlState->IsDeleted()))
	{
		if ( !RevertCollection(OutError) )
		{
			// Failed to revert, just bail out
			GWarn->EndSlowTask();
			return false;
		}

		// Make sure we get a fresh state from the server
		SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
	}

	// If not at the head revision, sync up
	if (SourceControlState.IsValid() && !SourceControlState->IsCurrent())
	{
		if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FSync>(), AbsoluteFilename) == ECommandResult::Failed)
		{
			// Could not sync up with the head revision
			GWarn->EndSlowTask();
			OutError = LOCTEXT("Error_SCCSync", "Failed to sync collection to the head revision.");
			return false;
		}

		// Check to see if the file exists at the head revision
		if ( IFileManager::Get().FileSize(*SourceFilename) < 0 )
		{
			// File was already deleted, consider this a success
			GWarn->EndSlowTask();
			return true;
		}
			
		FCollection NewCollection;
				
		if ( !NewCollection.LoadFromFile(SourceFilename, false) )
		{
			// Failed to load the head revision file so it isn't safe to delete it
			GWarn->EndSlowTask();
			OutError = LOCTEXT("Error_SCCBadHead", "Failed to load the collection at the head revision.");
			return false;
		}

		// Loaded the head revision, now merge up so the files are in a consistent state
		MergeWithCollection(NewCollection);

		// Make sure we get a fresh state from the server
		SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
	}

	GWarn->UpdateProgress(DeleteProgressNumerator++, DeleteProgressDenominator);

	if(SourceControlState.IsValid())
	{
		if(SourceControlState->IsAdded() || SourceControlState->IsCheckedOut())
		{
			OutError = LOCTEXT("Error_SCCDeleteWhileCheckedOut", "Failed to delete collection in source control because it is checked out or open for add.");
		}
		else if(SourceControlState->CanCheckout())
		{
			if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FDelete>(), AbsoluteFilename) == ECommandResult::Succeeded )
			{
				// Now check in the delete
				const FText ChangelistDesc = FText::Format( LOCTEXT("CollectionDeletedDesc", "Deleted collection: {CollectionName}"), CollectionNameText );
				TSharedRef<FCheckIn, ESPMode::ThreadSafe> CheckInOperation = ISourceControlOperation::Create<FCheckIn>();
				CheckInOperation->SetDescription(ChangelistDesc);
				if ( SourceControlProvider.Execute( CheckInOperation, AbsoluteFilename ) )
				{
					// Deleted successfully!
					bDeletedSuccessfully = true;
				}
				else
				{
					FText Unused;
					if ( !RevertCollection(Unused) )
					{
						UE_LOG(LogCollectionManager, Warning, TEXT("Failed to revert collection '%s' after failing to check in the file that was marked for delete."), *CollectionName.ToString());
					}

					OutError = LOCTEXT("Error_SCCCheckIn", "Failed to check in collection.");
				}
			}
			else
			{
				OutError = LOCTEXT("Error_SCCDeleteFailed", "Failed to delete collection in source control.");
			}
		}
		else if(!SourceControlState->IsSourceControlled())
		{
			// Not yet in the depot or deleted. We can just delete it from disk.
			bDeletedSuccessfully = IFileManager::Get().Delete(*AbsoluteFilename);
			if ( !bDeletedSuccessfully )
			{
				OutError = LOCTEXT("Error_DiskDeleteFailed", "Failed to delete the collection file from disk.");
			}
		}
		else if (!SourceControlState->IsCurrent())
		{
			OutError = LOCTEXT("Error_SCCNotCurrent", "Collection is not at head revision after sync.");
		}
		else if(SourceControlState->IsCheckedOutOther())
		{
			OutError = LOCTEXT("Error_SCCCheckedOutOther", "Collection is checked out by another user.");
		}
		else
		{
			OutError = LOCTEXT("Error_SCCUnknown", "Could not determine source control state.");
		}
	}
	else
	{
		OutError = LOCTEXT("Error_SCCInvalid", "Source control state is invalid.");
	}

	GWarn->UpdateProgress(DeleteProgressNumerator++, DeleteProgressDenominator);

	GWarn->EndSlowTask();

	return bDeletedSuccessfully;
}


#undef LOCTEXT_NAMESPACE
