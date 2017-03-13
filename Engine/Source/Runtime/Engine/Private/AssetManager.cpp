// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Engine/AssetManager.h"
#include "Engine/AssetManagerSettings.h"
#include "AssetRegistryModule.h"
#include "AssetData.h"
#include "ARFilter.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#endif

#define LOCTEXT_NAMESPACE "AssetManager"

DEFINE_LOG_CATEGORY_STATIC(LogAssetManager, Log, All);

/** Structure defining the current loading state of an asset */
struct FPrimaryAssetLoadState
{
	/** The handle to the streamable state for this asset, this keeps the objects in memory. If handle is invalid, not in memory at all */
	TSharedPtr<FStreamableHandle> Handle;

	/** The set of bundles to be loaded by the handle */
	TArray<FName> BundleNames;

	/** If this state is keeping things in memory */
	bool IsValid() { return Handle.IsValid() && Handle->IsActive(); }

	/** Reset this state */
	void Reset(bool bCancelHandle)
	{
		if (Handle.IsValid())
		{
			if (Handle->IsActive() && bCancelHandle)
			{
				// This will call the cancel callback if set
				Handle->CancelHandle();
			}
			
			Handle = nullptr;
		}
		BundleNames.Reset();
	}
};

/** Structure representing data about a specific asset */
struct FPrimaryAssetData
{
	/** Cached copy of the asset data from the asset registry, this is updated if things change */
	FAssetData AssetData;

	/** Path to this asset on disk */
	FAssetPtr AssetPtr;

	/** Current state of this asset */
	FPrimaryAssetLoadState CurrentState;

	/** Pending state of this asset, will be copied to CurrentState when load finishes */
	FPrimaryAssetLoadState PendingState;

	FPrimaryAssetData() {}

	/** Asset is considered loaded at all if there is an active handle for it */
	bool IsLoaded() { return CurrentState.IsValid(); }
};

/** Structure representing all items of a specific asset type */
struct FPrimaryAssetTypeData
{
	/** The public info struct */
	FPrimaryAssetTypeInfo Info;

	/** Map of scanned assets */
	TMap<FName, FPrimaryAssetData> AssetMap;

	/** In the editor, paths that we need to scan once asset registry is done loading */
	TArray<FString> DeferredAssetDataPaths;

	FPrimaryAssetTypeData() {}

	FPrimaryAssetTypeData(FName InPrimaryAssetType, UClass* InAssetBaseClass, bool bInHasBlueprintClasses, bool bInIsEditorOnly)
		: Info(InPrimaryAssetType, InAssetBaseClass, bInHasBlueprintClasses, bInIsEditorOnly)
		{}
};

UAssetManager::UAssetManager()
{
	bIsGlobalAsyncScanEnvironment = false;
	bShouldKeepHardRefs = !GIsEditor || IsRunningCommandlet();
	bShouldGuessTypeAndName = false;
	bShouldUseSynchronousLoad = IsRunningCommandlet();
	bIsBulkScanning = false;
}

void UAssetManager::StartInitialLoading()
{

}

void UAssetManager::FinishInitialLoading()
{
	
}

void UAssetManager::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{

#if WITH_EDITOR
		bIsGlobalAsyncScanEnvironment = GIsEditor && !IsRunningCommandlet();

		if (bIsGlobalAsyncScanEnvironment)
		{
			// Listen for when the asset registry has finished discovering files
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

			AssetRegistry.OnFilesLoaded().AddUObject(this, &UAssetManager::OnAssetRegistryFilesLoaded);
			AssetRegistry.OnInMemoryAssetCreated().AddUObject(this, &UAssetManager::OnInMemoryAssetCreated);
			AssetRegistry.OnInMemoryAssetDeleted().AddUObject(this, &UAssetManager::OnInMemoryAssetDeleted);
		}

		FEditorDelegates::PreBeginPIE.AddUObject(this, &UAssetManager::PreBeginPIE);
		FEditorDelegates::EndPIE.AddUObject(this, &UAssetManager::EndPIE);
#endif
		LoadRedirectorMaps();
	}
}

bool UAssetManager::IsValid()
{
	if (GEngine && GEngine->AssetManager)
	{
		return true;
	}

	return false;
}

UAssetManager& UAssetManager::Get()
{
	UAssetManager* Singleton = GEngine->AssetManager;

	if (Singleton)
	{
		return *Singleton;
	}
	else
	{
		UE_LOG(LogAssetManager, Fatal, TEXT("Cannot use AssetManager if no AssetManagerClassName is defined!"));
		return *NewObject<UAssetManager>(); // never calls this
	}
}

int32 UAssetManager::ScanPathsForPrimaryAssets(FName PrimaryAssetType, const TArray<FString>& Paths, UClass* BaseClass, bool bHasBlueprintClasses, bool bIsEditorOnly, bool bForceSynchronousScan)
{
	TArray<FString> Directories, PackageFilenames, PackageNames;
	TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetType);

#if !WITH_EDITORONLY_DATA
	if (bIsEditorOnly)
	{
		return 0;
	}
#endif

	check(BaseClass);

	if (!FoundType)
	{
		TSharedPtr<FPrimaryAssetTypeData> NewAsset = MakeShareable(new FPrimaryAssetTypeData(PrimaryAssetType, BaseClass, bHasBlueprintClasses, bIsEditorOnly));

		FoundType = &AssetTypeMap.Add(PrimaryAssetType, NewAsset.ToSharedRef());
	}

	// Should always be valid
	check(FoundType);

	FPrimaryAssetTypeData& TypeData = FoundType->Get();

	// Make sure types match
	if (!ensure(TypeData.Info.AssetBaseClass == BaseClass && TypeData.Info.bHasBlueprintClasses == bHasBlueprintClasses && TypeData.Info.bIsEditorOnly == bIsEditorOnly))
	{
		return 0;
	}

	// Add path info
	for (const FString& Path : Paths)
	{
		TypeData.Info.AssetDataPaths.AddUnique(Path);

		int32 DotIndex = INDEX_NONE;
		if (Path.FindChar('.', DotIndex))
		{
			FString PackageName = FPackageName::ObjectPathToPackageName(Path);

			PackageNames.AddUnique(PackageName);

			FString AssetFilename;
			if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, AssetFilename, FPackageName::GetAssetPackageExtension()))
			{
				// Search wants .uasset extension
				PackageFilenames.AddUnique(AssetFilename);
			}
		}
		else
		{
			Directories.AddUnique(Path);
		}
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

#if WITH_EDITOR
	// Cooked data has the asset data already set up
	const bool bShouldDoSynchronousScan = !bIsGlobalAsyncScanEnvironment || bForceSynchronousScan;
	if (bShouldDoSynchronousScan)
	{
		if (Directories.Num() > 0)
		{
			AssetRegistry.ScanPathsSynchronous(Directories);
		}
		if (PackageFilenames.Num() > 0)
		{
			AssetRegistry.ScanFilesSynchronous(PackageFilenames);
		}
	}
	else
	{
		if (AssetRegistry.IsLoadingAssets())
		{
			// Keep track of the paths we asked for so once assets are discovered we will refresh the list
			for (const FString& Path : Paths)
			{
				TypeData.DeferredAssetDataPaths.AddUnique(Path);
			}
		}
	}
#endif

	FARFilter ARFilter;

	TSet<FName> DerivedClassNames;
	TArray<FName> ClassNames;

	if (BaseClass)
	{
		if (!bShouldGuessTypeAndName)
		{
			// Primary type check
			ARFilter.TagsAndValues.Add(FPrimaryAssetId::PrimaryAssetTypeTag, PrimaryAssetType.ToString());
		}

		// Class check
		if (!bHasBlueprintClasses)
		{
			// For base classes, can do the filter before hand
			ARFilter.ClassNames.Add(BaseClass->GetFName());

#if WITH_EDITOR
			// Add any old names to the list in case things haven't been resaved
			TArray<FName> OldNames = FLinkerLoad::FindPreviousNamesForClass(BaseClass->GetPathName(), false);
			ARFilter.ClassNames.Append(OldNames);
#endif

			ARFilter.bRecursiveClasses = true;
		}
		else
		{
			ARFilter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());

			// Make sure this works, if it does remove post load check
			ClassNames.Add(BaseClass->GetFName());
			AssetRegistry.GetDerivedClassNames(ClassNames, TSet<FName>(), DerivedClassNames);

			/* DerivedClassNames are short names, GeneratedClass is a long name
			const FName GeneratedClassName = FName(TEXT("GeneratedClass"));
			for (FName DerivedClassName : DerivedClassNames)
			{
			ARFilter.TagsAndValues.Add(GeneratedClassName, DerivedClassName.ToString());

			// Add any old names to the list in case things haven't been resaved
			TArray<FName> OldNames = FLinkerLoad::FindPreviousNamesForClass(BaseClass->GetPathName(), false);

			for (FName OldClassName : OldNames)
			{
			ARFilter.TagsAndValues.Add(GeneratedClassName, OldClassName.ToString());
			}
			}
			*/
		}
	}

	for (const FString& Directory : Directories)
	{
		ARFilter.PackagePaths.Add(FName(*Directory));
	}

	for (const FString& PackageName : PackageNames)
	{
		ARFilter.PackageNames.Add(FName(*PackageName));
	}

	ARFilter.bRecursivePaths = true;
	ARFilter.bIncludeOnlyOnDiskAssets = !GIsEditor; // In editor check in memory, otherwise don't

	TArray<FAssetData> AssetDataList;

	AssetRegistry.GetAssets(ARFilter, AssetDataList);

	int32 NumAdded = 0;
	// Now add to map or update as needed
	for (FAssetData& Data : AssetDataList)
	{
		// Verify blueprint class
		if (bHasBlueprintClasses)
		{
			bool bShouldRemove = true;
			const FString ParentClassFromData = Data.GetTagValueRef<FString>("ParentClass");
			if (!ParentClassFromData.IsEmpty())
			{
				const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(ParentClassFromData);
				const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);

				TArray<FName> ValidNames;
#if WITH_EDITOR
				// Also check old names
				ValidNames = FLinkerLoad::FindPreviousNamesForClass(BaseClass->GetPathName(), false);
#endif
				ValidNames.Add(FName(*ClassName));		
				for (FName ValidName : ValidNames)
				{
					if (DerivedClassNames.Contains(ValidName))
					{
						// This asset is derived from ObjectBaseClass. Keep it.
						bShouldRemove = false;
						break;
					}
				}
			}

			if (bShouldRemove)
			{
				continue;
			}
		}

		FPrimaryAssetId PrimaryAssetId = GetPrimaryAssetIdFromData(Data, PrimaryAssetType);

		// Remove invalid or wrong type assets
		if (!PrimaryAssetId.IsValid() || PrimaryAssetId.PrimaryAssetType != PrimaryAssetType)
		{
			continue;
		}

		NumAdded++;

		UpdateCachedAssetData(PrimaryAssetId, Data, false);
	}

	if (!bIsBulkScanning)
	{
		RebuildObjectReferenceList();
	}

	return NumAdded;
}

void UAssetManager::StartBulkScanning()
{
	if (ensure(!bIsBulkScanning))
	{
		bIsBulkScanning = true;
	}
}

void UAssetManager::StopBulkScanning()
{
	if (ensure(bIsBulkScanning))
	{
		bIsBulkScanning = false;
	}
	RebuildObjectReferenceList();
}

void UAssetManager::UpdateCachedAssetData(const FPrimaryAssetId& PrimaryAssetId, const FAssetData& NewAssetData, bool bAllowDuplicates)
{
	const TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetId.PrimaryAssetType);

	if (ensure(FoundType))
	{
		FPrimaryAssetTypeData& TypeData = FoundType->Get();

		FPrimaryAssetData* OldData = TypeData.AssetMap.Find(PrimaryAssetId.PrimaryAssetName);

		FStringAssetReference NewStringReference = GetAssetPathForData(NewAssetData, TypeData.Info.bHasBlueprintClasses);

		if (OldData && OldData->AssetPtr.ToStringReference() != NewStringReference)
		{
			if (bAllowDuplicates)
			{
				UE_LOG(LogAssetManager, Warning, TEXT("Found Duplicate PrimaryAssetID %s during creation, this must be resolved before saving. Path %s is replacing path %s"), *PrimaryAssetId.ToString(), *OldData->AssetPtr.ToStringReference().ToString(), *NewStringReference.ToString());
			}
			else
			{
				ensureMsgf(!OldData, TEXT("Found Duplicate PrimaryAssetID %s! Path %s is replacing path %s"), *PrimaryAssetId.ToString(), *OldData->AssetPtr.ToStringReference().ToString(), *NewStringReference.ToString());
			}

#if WITH_EDITOR
			if (GIsEditor)
			{
				FNotificationInfo Info(FText::Format(LOCTEXT("DuplicateAssetId", "Duplicate Asset ID {0} used by {1} and {2}, you must delete or rename one!"), 
					FText::FromString(PrimaryAssetId.ToString()), FText::FromString(OldData->AssetPtr.ToStringReference().GetLongPackageName()), FText::FromString(NewStringReference.GetLongPackageName())));
				Info.ExpireDuration = 30.0f;
				
				TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
				if (Notification.IsValid())
				{
					Notification->SetCompletionState(SNotificationItem::CS_Fail);
				}
			}
#endif
		}

		FPrimaryAssetData& NameData = TypeData.AssetMap.FindOrAdd(PrimaryAssetId.PrimaryAssetName);

		// Update data and path, don't touch state or references
		NameData.AssetData = NewAssetData;
		NameData.AssetPtr = FAssetPtr(NewStringReference);

		if (bIsBulkScanning)
		{
			// Do a partial update, add to the path->asset map
			AssetPathMap.Add(NewStringReference, PrimaryAssetId);
		}

		if (OldData)
		{
			CachedAssetBundles.Remove(PrimaryAssetId);
		}
		
		FAssetBundleData BundleData;
		if (BundleData.SetFromAssetData(NewAssetData))
		{
			for (FAssetBundleEntry& Entry : BundleData.Bundles)
			{
				if (Entry.BundleScope.IsValid() && Entry.BundleScope == PrimaryAssetId)
				{
					TMap<FName, FAssetBundleEntry>& BundleMap = CachedAssetBundles.FindOrAdd(PrimaryAssetId);

					BundleMap.Emplace(Entry.BundleName, Entry);
				}
			}
		}
	}
}

int32 UAssetManager::ScanPathForPrimaryAssets(FName PrimaryAssetType, const FString& Path, UClass* BaseClass, bool bHasBlueprintClasses, bool bIsEditorOnly, bool bForceSynchronousScan)
{
	return ScanPathsForPrimaryAssets(PrimaryAssetType, TArray<FString>{Path}, BaseClass, bHasBlueprintClasses, bIsEditorOnly, bForceSynchronousScan);
}

bool UAssetManager::AddDynamicAsset(const FPrimaryAssetId& PrimaryAssetId, const FStringAssetReference& AssetPath, const FAssetBundleData& BundleData)
{
	if (!ensure(PrimaryAssetId.IsValid()))
	{
		return false;
	}

	FName PrimaryAssetType = PrimaryAssetId.PrimaryAssetType;
	TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetType);

	if (!FoundType)
	{
		TSharedPtr<FPrimaryAssetTypeData> NewAsset = MakeShareable(new FPrimaryAssetTypeData());
		NewAsset->Info.PrimaryAssetType = PrimaryAssetType;
		NewAsset->Info.bIsDynamicAsset = true;

		FoundType = &AssetTypeMap.Add(PrimaryAssetType, NewAsset.ToSharedRef());
	}

	// Should always be valid
	check(FoundType);

	FPrimaryAssetTypeData& TypeData = FoundType->Get();

	// This needs to be a dynamic type, types cannot be both dynamic and loaded off disk
	if (!ensure(TypeData.Info.bIsDynamicAsset))
	{
		return false;
	}

	FPrimaryAssetData* OldData = TypeData.AssetMap.Find(PrimaryAssetId.PrimaryAssetName);
	FPrimaryAssetData& NameData = TypeData.AssetMap.FindOrAdd(PrimaryAssetId.PrimaryAssetName);

	if (OldData && OldData->AssetPtr.ToStringReference() != AssetPath)
	{
		UE_LOG(LogAssetManager, Warning, TEXT("AddDynamicAsset on %s called with conflicting path. Path %s is replacing path %s"), *PrimaryAssetId.ToString(), *OldData->AssetPtr.ToStringReference().ToString(), *AssetPath.ToString());
	}

	NameData.AssetPtr = FAssetPtr(AssetPath);

	if (bIsBulkScanning && AssetPath.IsValid())
	{
		// Do a partial update, add to the path->asset map
		AssetPathMap.Add(AssetPath, PrimaryAssetId);
	}

	if (OldData)
	{
		CachedAssetBundles.Remove(PrimaryAssetId);
	}

	TMap<FName, FAssetBundleEntry>& BundleMap = CachedAssetBundles.FindOrAdd(PrimaryAssetId);

	for (const FAssetBundleEntry& Entry : BundleData.Bundles)
	{
		FAssetBundleEntry NewEntry(Entry);
		NewEntry.BundleScope = PrimaryAssetId;

		BundleMap.Emplace(Entry.BundleName, NewEntry);
	}
	return true;
}

void UAssetManager::RecursivelyExpandBundleData(FAssetBundleData& BundleData)
{
	TArray<FStringAssetReference> ReferencesToExpand;
	TSet<FName> FoundBundleNames;

	for (const FAssetBundleEntry& Entry : BundleData.Bundles)
	{
		FoundBundleNames.Add(Entry.BundleName);
		for (const FStringAssetReference& Reference : Entry.BundleAssets)
		{
			ReferencesToExpand.AddUnique(Reference);
		}
	}

	// Expandable references can increase recursively
	for (int32 i = 0; i < ReferencesToExpand.Num(); i++)
	{
		FPrimaryAssetId FoundId = GetPrimaryAssetIdForPath(ReferencesToExpand[i]);
		TArray<FAssetBundleEntry> FoundEntries;

		if (FoundId.IsValid() && GetAssetBundleEntries(FoundId, FoundEntries))
		{
			for (const FAssetBundleEntry& FoundEntry : FoundEntries)
			{
				// Make sure the bundle name matches
				if (FoundBundleNames.Contains(FoundEntry.BundleName))
				{
					BundleData.AddBundleAssets(FoundEntry.BundleName, FoundEntry.BundleAssets);

					for (const FStringAssetReference& FoundReference : FoundEntry.BundleAssets)
					{
						// Keep recursing
						ReferencesToExpand.AddUnique(FoundReference);
					}
				}
			}
		}
	}
}

bool UAssetManager::GetPrimaryAssetData(const FPrimaryAssetId& PrimaryAssetId, FAssetData& AssetData) const
{
	const FPrimaryAssetData* NameData = GetNameData(PrimaryAssetId);

	if (NameData)
	{
		AssetData = NameData->AssetData;
		return true;
	}
	return false;
}

bool UAssetManager::GetPrimaryAssetDataList(FName PrimaryAssetType, TArray<FAssetData>& AssetDataList) const
{
	bool bAdded = false;
	const TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetType);

	if (FoundType)
	{
		const FPrimaryAssetTypeData& TypeData = FoundType->Get();

		for (TPair<FName, FPrimaryAssetData> Pair : TypeData.AssetMap)
		{
			FAssetData& AssetData = Pair.Value.AssetData;
			if (AssetData.IsValid())
			{
				bAdded = true;
				AssetDataList.Add(AssetData);
			}
		}
	}

	return bAdded;
}

UObject* UAssetManager::GetPrimaryAssetObject(const FPrimaryAssetId& PrimaryAssetId) const
{
	const FPrimaryAssetData* NameData = GetNameData(PrimaryAssetId);

	if (NameData)
	{
		return NameData->AssetPtr.Get();
	}

	return nullptr;
}

bool UAssetManager::GetPrimaryAssetObjectList(FName PrimaryAssetType, TArray<UObject*>& ObjectList) const
{
	bool bAdded = false;
	const TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetType);

	if (FoundType)
	{
		const FPrimaryAssetTypeData& TypeData = FoundType->Get();

		for (TPair<FName, FPrimaryAssetData> Pair : TypeData.AssetMap)
		{
			UObject* FoundObject = Pair.Value.AssetPtr.Get();

			if (FoundObject)
			{
				ObjectList.Add(FoundObject);
				bAdded = true;
			}
		}
	}

	return bAdded;
}

FStringAssetReference UAssetManager::GetPrimaryAssetPath(const FPrimaryAssetId& PrimaryAssetId) const
{
	const FPrimaryAssetData* NameData = GetNameData(PrimaryAssetId);

	if (NameData)
	{
		return NameData->AssetPtr.ToStringReference();
	}
	return FStringAssetReference();
}

bool UAssetManager::GetPrimaryAssetPathList(FName PrimaryAssetType, TArray<FStringAssetReference>& AssetPathList) const
{
	const TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetType);

	if (FoundType)
	{
		const FPrimaryAssetTypeData& TypeData = FoundType->Get();

		for (TPair<FName, FPrimaryAssetData> Pair : TypeData.AssetMap)
		{
			AssetPathList.AddUnique(Pair.Value.AssetPtr.ToStringReference());
		}
	}

	return AssetPathList.Num() > 0;
}

FPrimaryAssetId UAssetManager::GetPrimaryAssetIdForPath(const FStringAssetReference& ObjectPath) const
{
	const FPrimaryAssetId* FoundIdentifier = AssetPathMap.Find(ObjectPath);

	// Check redirector list
	if (!FoundIdentifier)
	{
		FString RedirectedPath = GetRedirectedAssetPath(ObjectPath.ToString());

		if (!RedirectedPath.IsEmpty())
		{
			FoundIdentifier = AssetPathMap.Find(RedirectedPath);
		}
	}

	if (FoundIdentifier)
	{
		return *FoundIdentifier;
	}

	return FPrimaryAssetId();
}

FPrimaryAssetId UAssetManager::GetPrimaryAssetIdFromData(const FAssetData& AssetData, FName SuggestedType) const
{
	FPrimaryAssetId FoundId = AssetData.GetPrimaryAssetId();

	if (!FoundId.IsValid() && bShouldGuessTypeAndName && SuggestedType != NAME_None)
	{
		const TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(SuggestedType);

		if (ensure(FoundType))
		{
			// If asset at this path is already known about return that
			FPrimaryAssetId OldID = GetPrimaryAssetIdForPath(GetAssetPathForData(AssetData, (*FoundType)->Info.bHasBlueprintClasses));

			if (OldID.IsValid())
			{
				return OldID;
			}

			return FPrimaryAssetId(SuggestedType, AssetData.AssetName);
		}
	}

	return FoundId;
}

bool UAssetManager::GetPrimaryAssetIdList(FName PrimaryAssetType, TArray<FPrimaryAssetId>& PrimaryAssetIdList) const
{
	const TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetType);

	if (FoundType)
	{
		const FPrimaryAssetTypeData& TypeData = FoundType->Get();

		for (TPair<FName, FPrimaryAssetData> Pair : TypeData.AssetMap)
		{
			PrimaryAssetIdList.Add(FPrimaryAssetId(PrimaryAssetType, Pair.Key));
		}
	}

	return PrimaryAssetIdList.Num() > 0;
}

bool UAssetManager::GetPrimaryAssetTypeInfo(FName PrimaryAssetType, FPrimaryAssetTypeInfo& AssetTypeInfo) const
{
	const TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetType);

	if (FoundType)
	{
		AssetTypeInfo = (*FoundType)->Info;

		return true;
	}

	return false;
}

void UAssetManager::GetPrimaryAssetTypeInfoList(TArray<FPrimaryAssetTypeInfo>& AssetTypeInfoList) const
{
	for (const TPair<FName, TSharedRef<FPrimaryAssetTypeData>> TypePair : AssetTypeMap)
	{
		const FPrimaryAssetTypeData& TypeData = TypePair.Value.Get();

		AssetTypeInfoList.Add(TypeData.Info);
	}
}

TSharedPtr<FStreamableHandle> UAssetManager::ChangeBundleStateForPrimaryAssets(const TArray<FPrimaryAssetId>& AssetsToChange, const TArray<FName>& AddBundles, const TArray<FName>& RemoveBundles, bool bRemoveAllBundles, FStreamableDelegate DelegateToCall, TAsyncLoadPriority Priority)
{
	TArray<TSharedPtr<FStreamableHandle> > NewHandles;
	TArray<FPrimaryAssetId> NewAssets;
	TSharedPtr<FStreamableHandle> ReturnHandle;

	for (const FPrimaryAssetId& PrimaryAssetId : AssetsToChange)
	{
		FPrimaryAssetData* NameData = GetNameData(PrimaryAssetId);

		if (NameData)
		{
			// Iterate list of changes, compute new bundle set
			bool bLoadIfNeeded = false;
			
			// Use pending state if valid
			TArray<FName> CurrentBundleState = NameData->PendingState.IsValid() ? NameData->PendingState.BundleNames : NameData->CurrentState.BundleNames;
			TArray<FName> NewBundleState;

			if (!bRemoveAllBundles)
			{
				NewBundleState = CurrentBundleState;

				for (FName RemoveBundle : RemoveBundles)
				{
					NewBundleState.Remove(RemoveBundle);
				}
			}

			for (FName AddBundle : AddBundles)
			{
				NewBundleState.AddUnique(AddBundle);
			}

			NewBundleState.Sort();

			// If the pending state is valid, check if it is different
			if (NameData->PendingState.IsValid())
			{
				if (NameData->PendingState.BundleNames == NewBundleState)
				{
					continue;
				}
				// Clear pending state
				NameData->PendingState.Reset(true);
			}
			else if (NameData->CurrentState.IsValid() && NameData->CurrentState.BundleNames == NewBundleState)
			{
				// If no pending, compare with current
				continue;
			}

			TSet<FStringAssetReference> PathsToLoad;

			// Gather asset refs
			const FStringAssetReference& AssetPath = NameData->AssetPtr.ToStringReference();

			if (!AssetPath.IsNull())
			{
				// Dynamic types can have no base asset path
				PathsToLoad.Add(AssetPath);
			}
			
			for (FName BundleName : NewBundleState)
			{
				FAssetBundleEntry Entry = GetAssetBundleEntry(PrimaryAssetId, BundleName);

				if (Entry.IsValid())
				{
					PathsToLoad.Append(Entry.BundleAssets);
				}
				UE_LOG(LogAssetManager, Verbose, TEXT("ChangeBundleStateForPrimaryAssets: No assets for bundle %s::%s"), *PrimaryAssetId.ToString(), *BundleName.ToString());
			}

			TSharedPtr<FStreamableHandle> NewHandle;

			FString DebugName = PrimaryAssetId.ToString();

			if (NewBundleState.Num() > 0)
			{
				DebugName += TEXT(" (");

				for (int32 i = 0; i < NewBundleState.Num(); i++)
				{
					if (i != 0)
					{
						DebugName += TEXT(", ");
					}
					DebugName += NewBundleState[i].ToString();
				}

				DebugName += TEXT(")");
			}

			// Create the per asset handle
			if (bShouldUseSynchronousLoad)
			{
				NewHandle = StreamableManager.RequestSyncLoad(PathsToLoad.Array(), false, DebugName);
			}
			else
			{
				NewHandle = StreamableManager.RequestAsyncLoad(PathsToLoad.Array(), FStreamableDelegate(), Priority, false, DebugName);
			}

			if (!ensure(NewHandle.IsValid()))
			{
				return nullptr;
			}

			if (NewHandle->HasLoadCompleted())
			{
				// Copy right into active
				NameData->CurrentState.BundleNames = NewBundleState;
				NameData->CurrentState.Handle = NewHandle;
			}
			else
			{
				// Copy into pending and set delegate
				NameData->PendingState.BundleNames = NewBundleState;
				NameData->PendingState.Handle = NewHandle;

				NewHandle->BindCompleteDelegate(FStreamableDelegate::CreateUObject(this, &UAssetManager::OnAssetStateChangeCompleted, PrimaryAssetId, NewHandle, FStreamableDelegate()));
			}

			NewHandles.Add(NewHandle);
			NewAssets.Add(PrimaryAssetId);
		}
	}

	if (NewHandles.Num() > 1)
	{
		// If multiple handles, need to make wrapper handle
		ReturnHandle = StreamableManager.CreateCombinedHandle(NewHandles, FString::Printf(TEXT("%s CreateCombinedHandle"), *GetName()));

		// Call delegate or bind to meta handle
		if (ReturnHandle->HasLoadCompleted())
		{
			FStreamableHandle::ExecuteDelegate(DelegateToCall);
		}
		else
		{
			// Call external callback when completed
			ReturnHandle->BindCompleteDelegate(DelegateToCall);
		}
	}
	else if (NewHandles.Num() == 1)
	{
		ReturnHandle = NewHandles[0];
		ensure(NewAssets.Num() == 1);

		// If only one handle, return it and add callback
		if (ReturnHandle->HasLoadCompleted())
		{
			FStreamableHandle::ExecuteDelegate(DelegateToCall);
		}
		else
		{
			// Call internal callback and external callback when it finishes
			ReturnHandle->BindCompleteDelegate(FStreamableDelegate::CreateUObject(this, &UAssetManager::OnAssetStateChangeCompleted, NewAssets[0], ReturnHandle, DelegateToCall));
		}
	}
	else
	{
		// Call completion callback, nothing to do
		FStreamableHandle::ExecuteDelegate(DelegateToCall);
	}

	return ReturnHandle;
}

void UAssetManager::OnAssetStateChangeCompleted(FPrimaryAssetId PrimaryAssetId, TSharedPtr<FStreamableHandle> BoundHandle, FStreamableDelegate WrappedDelegate)
{
	FPrimaryAssetData* NameData = GetNameData(PrimaryAssetId);

	if (NameData)
	{
		if (NameData->PendingState.Handle == BoundHandle)
		{
			NameData->CurrentState.Handle = NameData->PendingState.Handle;
			NameData->CurrentState.BundleNames = NameData->PendingState.BundleNames;

			WrappedDelegate.ExecuteIfBound();

			// Clear old state, but don't cancel handle as we just copied it into current
			NameData->PendingState.Reset(false);
		}
		else
		{
			UE_LOG(LogAssetManager, Verbose, TEXT("OnAssetStateChangeCompleted: Received after pending data changed, ignoring (%s)"), *PrimaryAssetId.ToString());
		}
	}
	else
	{
		UE_LOG(LogAssetManager, Error, TEXT("OnAssetStateChangeCompleted: Received for invalid asset! (%s)"), *PrimaryAssetId.ToString());
	}
}

TSharedPtr<FStreamableHandle> UAssetManager::LoadPrimaryAssets(const TArray<FPrimaryAssetId>& AssetsToLoad, const TArray<FName>& LoadBundles, FStreamableDelegate DelegateToCall, TAsyncLoadPriority Priority)
{
	return ChangeBundleStateForPrimaryAssets(AssetsToLoad, LoadBundles, TArray<FName>(), true, DelegateToCall, Priority);
}

TSharedPtr<FStreamableHandle> UAssetManager::LoadPrimaryAsset(const FPrimaryAssetId& AssetToLoad, const TArray<FName>& LoadBundles, FStreamableDelegate DelegateToCall, TAsyncLoadPriority Priority)
{
	return LoadPrimaryAssets(TArray<FPrimaryAssetId>{AssetToLoad}, LoadBundles, DelegateToCall, Priority);
}

TSharedPtr<FStreamableHandle> UAssetManager::LoadPrimaryAssetsWithType(FName PrimaryAssetType, const TArray<FName>& LoadBundles, FStreamableDelegate DelegateToCall, TAsyncLoadPriority Priority)
{
	TArray<FPrimaryAssetId> Assets;
	GetPrimaryAssetIdList(PrimaryAssetType, Assets);
	return LoadPrimaryAssets(Assets, LoadBundles, DelegateToCall, Priority);
}

TSharedPtr<FStreamableHandle> UAssetManager::GetPrimaryAssetHandle(const FPrimaryAssetId& PrimaryAssetId, bool bForceCurrent, TArray<FName>* Bundles)
{
	FPrimaryAssetData* NameData = GetNameData(PrimaryAssetId);

	if (!NameData)
	{
		return nullptr;
	}

	FPrimaryAssetLoadState& LoadState = (bForceCurrent || !NameData->PendingState.IsValid()) ? NameData->CurrentState : NameData->PendingState;

	if (Bundles)
	{
		*Bundles = LoadState.BundleNames;
	}
	return LoadState.Handle;
}

bool UAssetManager::GetPrimaryAssetsWithBundleState(TArray<FPrimaryAssetId>& PrimaryAssetList, const TArray<FName>& ValidTypes, const TArray<FName>& RequiredBundles, const TArray<FName>& ExcludedBundles, bool bForceCurrent)
{
	bool bFoundAny = false;

	for (TPair<FName, TSharedRef<FPrimaryAssetTypeData>> TypePair : AssetTypeMap)
	{
		if (ValidTypes.Num() > 0 && !ValidTypes.Contains(TypePair.Key))
		{
			// Skip this type
			continue;
		}

		FPrimaryAssetTypeData& TypeData = TypePair.Value.Get();

		for (TPair<FName, FPrimaryAssetData> NamePair : TypeData.AssetMap)
		{
			FPrimaryAssetData& NameData = NamePair.Value;

			FPrimaryAssetLoadState& LoadState = (bForceCurrent || !NameData.PendingState.IsValid()) ? NameData.CurrentState : NameData.PendingState;

			if (!LoadState.IsValid())
			{
				// Only allow loaded assets
				continue;
			}

			bool bFailedTest = false;

			// Check bundle requirements
			for (FName RequiredName : RequiredBundles)
			{
				if (!LoadState.BundleNames.Contains(RequiredName))
				{
					bFailedTest = true;
					break;
				}
			}

			for (FName ExcludedName : ExcludedBundles)
			{
				if (LoadState.BundleNames.Contains(ExcludedName))
				{
					bFailedTest = true;
					break;
				}
			}

			if (!bFailedTest)
			{
				PrimaryAssetList.Add(FPrimaryAssetId(TypePair.Key, NamePair.Key));
				bFoundAny = true;
			}
		}
	}

	return bFoundAny;
}

void UAssetManager::GetPrimaryAssetBundleStateMap(TMap<FPrimaryAssetId, TArray<FName>>& BundleStateMap, bool bForceCurrent)
{
	BundleStateMap.Reset();

	for (TPair<FName, TSharedRef<FPrimaryAssetTypeData>> TypePair : AssetTypeMap)
	{
		FPrimaryAssetTypeData& TypeData = TypePair.Value.Get();

		for (TPair<FName, FPrimaryAssetData> NamePair : TypeData.AssetMap)
		{
			FPrimaryAssetData& NameData = NamePair.Value;

			FPrimaryAssetLoadState& LoadState = (bForceCurrent || !NameData.PendingState.IsValid()) ? NameData.CurrentState : NameData.PendingState;

			if (!LoadState.IsValid())
			{
				continue;
			}

			FPrimaryAssetId AssetID(TypePair.Key, NamePair.Key);

			BundleStateMap.Add(AssetID, LoadState.BundleNames);
		}
	}
}

int32 UAssetManager::UnloadPrimaryAssets(const TArray<FPrimaryAssetId>& AssetsToUnload)
{
	int32 NumUnloaded = 0;

	for (const FPrimaryAssetId& PrimaryAssetId : AssetsToUnload)
	{
		FPrimaryAssetData* NameData = GetNameData(PrimaryAssetId);

		if (NameData)
		{
			// Undo current and pending
			if (NameData->CurrentState.IsValid() || NameData->PendingState.IsValid())
			{
				NumUnloaded++;
				NameData->CurrentState.Reset(true);
				NameData->PendingState.Reset(true);
			}
		}
	}

	return NumUnloaded;
}

int32 UAssetManager::UnloadPrimaryAsset(const FPrimaryAssetId& AssetToUnload)
{
	return UnloadPrimaryAssets(TArray<FPrimaryAssetId>{AssetToUnload});
}

int32 UAssetManager::UnloadPrimaryAssetsWithType(FName PrimaryAssetType)
{
	TArray<FPrimaryAssetId> Assets;
	GetPrimaryAssetIdList(PrimaryAssetType, Assets);
	return UnloadPrimaryAssets(Assets);
}

TSharedPtr<FStreamableHandle> UAssetManager::LoadAssetList(const TArray<FStringAssetReference>& AssetList)
{
	return GetStreamableManager().RequestAsyncLoad(AssetList, FStreamableDelegate(), FStreamableManager::DefaultAsyncLoadPriority, false);
}

FAssetBundleEntry UAssetManager::GetAssetBundleEntry(const FPrimaryAssetId& BundleScope, FName BundleName) const
{
	FAssetBundleEntry InvalidEntry;

	const TMap<FName, FAssetBundleEntry>* FoundMap = CachedAssetBundles.Find(BundleScope);

	if (FoundMap)
	{
		const FAssetBundleEntry* FoundEntry = FoundMap->Find(BundleName);
		if (FoundEntry)
		{
			return *FoundEntry;
		}
	}
	return InvalidEntry;
}

bool UAssetManager::GetAssetBundleEntries(const FPrimaryAssetId& BundleScope, TArray<FAssetBundleEntry>& OutEntries) const
{
	bool bFoundAny = false;

	const TMap<FName, FAssetBundleEntry>* FoundMap = CachedAssetBundles.Find(BundleScope);

	if (FoundMap)
	{
		for (const TPair<FName, FAssetBundleEntry>& BundlePair : *FoundMap)
		{
			bFoundAny = true;

			OutEntries.Add(BundlePair.Value);
		}
	}
	return bFoundAny;
}

FPrimaryAssetData* UAssetManager::GetNameData(const FPrimaryAssetId& PrimaryAssetId, bool bCheckRedirector)
{
	return const_cast<FPrimaryAssetData*>(const_cast<const UAssetManager*>(this)->GetNameData(PrimaryAssetId));
}

const FPrimaryAssetData* UAssetManager::GetNameData(const FPrimaryAssetId& PrimaryAssetId, bool bCheckRedirector) const
{
	const TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetId.PrimaryAssetType);

	// Try redirected name

	if (FoundType)
	{
		const FPrimaryAssetData* FoundName = (*FoundType)->AssetMap.Find(PrimaryAssetId.PrimaryAssetName);
		
		if (FoundName)
		{
			return FoundName;
		}
	}

	if (bCheckRedirector)
	{
		FPrimaryAssetId RedirectedId = GetRedirectedPrimaryAssetId(PrimaryAssetId);

		if (RedirectedId.IsValid())
		{
			// Recursively call self, but turn off recursion flag
			return GetNameData(RedirectedId, false);
		}
	}

	return nullptr;
}

void UAssetManager::RebuildObjectReferenceList()
{
	AssetPathMap.Reset();
	ObjectReferenceList.Reset();

	// Iterate primary asset map
	for (TPair<FName, TSharedRef<FPrimaryAssetTypeData>> TypePair : AssetTypeMap)
	{
		FPrimaryAssetTypeData& TypeData = TypePair.Value.Get();

		// Add base class in case it's a blueprint
		if (bShouldKeepHardRefs && !TypeData.Info.bIsDynamicAsset)
		{
			ObjectReferenceList.AddUnique(TypeData.Info.AssetBaseClass);
		}

		TypeData.Info.NumberOfAssets = TypeData.AssetMap.Num();

		for (TPair<FName, FPrimaryAssetData> NamePair : TypeData.AssetMap)
		{
			FPrimaryAssetData& NameData = NamePair.Value;
			
			const FStringAssetReference& AssetRef = NameData.AssetPtr.ToStringReference();

			// Dynamic types can have null asset refs
			if (!AssetRef.IsNull())
			{
				AssetPathMap.Add(AssetRef, FPrimaryAssetId(TypePair.Key, NamePair.Key));
			}
		}
	}
}

void UAssetManager::LoadRedirectorMaps()
{
	AssetPathRedirects.Reset();
	PrimaryAssetIdRedirects.Reset();
	PrimaryAssetTypeRedirects.Reset();

	const UAssetManagerSettings* Settings = GetDefault<UAssetManagerSettings>();

	for (const FAssetManagerRedirect& Redirect : Settings->PrimaryAssetTypeRedirects)
	{
		PrimaryAssetTypeRedirects.Add(FName(*Redirect.Old), FName(*Redirect.New));
	}

	for (const FAssetManagerRedirect& Redirect : Settings->PrimaryAssetIdRedirects)
	{
		PrimaryAssetIdRedirects.Add(Redirect.Old, Redirect.New);
	}

	for (const FAssetManagerRedirect& Redirect : Settings->AssetPathRedirects)
	{
		AssetPathRedirects.Add(Redirect.Old, Redirect.New);
	}
}

FPrimaryAssetId UAssetManager::GetRedirectedPrimaryAssetId(const FPrimaryAssetId& OldId) const
{
	FString OldIdString = OldId.ToString();

	const FString* FoundId = PrimaryAssetIdRedirects.Find(OldIdString);

	if (FoundId)
	{
		return FPrimaryAssetId(*FoundId);
	}

	// Now look for type redirect
	const FName* FoundType = PrimaryAssetTypeRedirects.Find(OldId.PrimaryAssetType);

	if (FoundType)
	{
		return FPrimaryAssetId(*FoundType, OldId.PrimaryAssetName);
	}

	return FPrimaryAssetId();
}

void UAssetManager::GetPreviousPrimaryAssetIds(const FPrimaryAssetId& NewId, TArray<FPrimaryAssetId>& OutOldIds) const
{
	FString NewIdString = NewId.ToString();

	for (const TPair<FString, FString>& Redirect : PrimaryAssetIdRedirects)
	{
		if (Redirect.Value == NewIdString)
		{
			OutOldIds.AddUnique(FPrimaryAssetId(Redirect.Key));
		}
	}

	// Also look for type redirects
	for (const TPair<FName, FName>& Redirect : PrimaryAssetTypeRedirects)
	{
		if (Redirect.Value == NewId.PrimaryAssetType)
		{
			OutOldIds.AddUnique(FPrimaryAssetId(Redirect.Key, NewId.PrimaryAssetName));
		}
	}
}

FString UAssetManager::GetRedirectedAssetPath(const FString& OldPath) const
{
	const FString* FoundPath = AssetPathRedirects.Find(OldPath);

	if (FoundPath)
	{
		return *FoundPath;
	}

	return FString();
}

void UAssetManager::ExtractStringAssetReferences(const UStruct* Struct, const void* StructValue, TArray<FStringAssetReference>& FoundAssetReferences, const TArray<FName>& PropertiesToSkip) const
{
	if (!ensure(Struct && StructValue))
	{
		return;
	}

	for (TPropertyValueIterator<const UProperty> It(Struct, StructValue); It; ++It)
	{
		const UProperty* Property = It.Key();
		const void* PropertyValue = It.Value();
		
		if (PropertiesToSkip.Contains(Property->GetFName()))
		{
			It.SkipRecursiveProperty();
			continue;
		}

		FStringAssetReference FoundRef;
		if (const UAssetClassProperty* AssetClassProp = Cast<UAssetClassProperty>(Property))
		{
			const TAssetSubclassOf<UObject>* AssetClassPtr = reinterpret_cast<const TAssetSubclassOf<UObject>*>(PropertyValue);
			if (AssetClassPtr)
			{
				FoundRef = AssetClassPtr->ToStringReference();
			}
		}
		else if (const UAssetObjectProperty* AssetProp = Cast<UAssetObjectProperty>(Property))
		{
			const TAssetPtr<UObject>* AssetPtr = reinterpret_cast<const TAssetPtr<UObject>*>(PropertyValue);
			if (AssetPtr)
			{
				FoundRef = AssetPtr->ToStringReference();
			}
		}
		else if (const UStructProperty* StructProperty = Cast<UStructProperty>(Property))
		{
			// String Class Reference is binary identical with StringAssetReference
			if (StructProperty->Struct == TBaseStructure<FStringAssetReference>::Get() || StructProperty->Struct == TBaseStructure<FStringClassReference>::Get())
			{
				const FStringAssetReference* AssetRefPtr = reinterpret_cast<const FStringAssetReference*>(PropertyValue);
				if (AssetRefPtr)
				{
					FoundRef = *AssetRefPtr;
				}

				// Skip recursion, we don't care about the raw string property
				It.SkipRecursiveProperty();
			}
		}
		if (!FoundRef.IsNull())
		{
			FoundAssetReferences.AddUnique(FoundRef);
		}
	}
}

bool UAssetManager::GetAssetDataForPath(const FStringAssetReference& ObjectPath, FAssetData& AssetData) const
{
	auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FString AssetPath = ObjectPath.ToString();

	// First check local redirector
	FString RedirectedPath = GetRedirectedAssetPath(AssetPath);

	if (!RedirectedPath.IsEmpty())
	{
		AssetPath = RedirectedPath;
	}

	GetAssetDataForPathInternal(AssetRegistry, AssetPath, AssetData);

#if WITH_EDITOR
	// Cooked data has the asset data already set up. Uncooked builds may need to manually scan for this file
	if (!AssetData.IsValid())
	{
		ScanForAssetPath(AssetRegistry, AssetPath);
		GetAssetDataForPathInternal(AssetRegistry, AssetPath, AssetData);
	}

	// Handle redirector chains
	const FString* DestinationObjectStrPtr = AssetData.TagsAndValues.Find("DestinationObject");

	while (DestinationObjectStrPtr)
	{
		FString DestinationObjectPath = *DestinationObjectStrPtr;
		ConstructorHelpers::StripObjectClass(DestinationObjectPath);
		AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*DestinationObjectPath);
		DestinationObjectStrPtr = AssetData.TagsAndValues.Find("DestinationObject");
	}
#endif

	return AssetData.IsValid();
}

FStringAssetReference UAssetManager::GetAssetPathForData(const FAssetData& AssetData, bool bIsBlueprint) const
{
	FString AssetPath = AssetData.ObjectPath.ToString();

	if (bIsBlueprint)
	{
		AssetPath += TEXT("_C");
	}

	return FStringAssetReference(AssetPath);
}

void UAssetManager::GetAssetDataForPathInternal(IAssetRegistry& AssetRegistry, const FString& AssetPath, OUT FAssetData& OutAssetData) const
{
	// We're a class if our path is foo.foo_C
	bool bIsClass = AssetPath.EndsWith(TEXT("_C"), ESearchCase::CaseSensitive) && !AssetPath.Contains(TEXT("_C."), ESearchCase::CaseSensitive);

	// If we're a class, first look for the asset data without the trailing _C
	// We do this first because in cooked builds you have to search the asset registery for the Blueprint, not the class itself
	if (bIsClass)
	{
		// We need to strip the class suffix because the asset registry has it listed by blueprint name
		const bool bIncludeOnlyOnDiskAssets = !GIsEditor;
		OutAssetData = AssetRegistry.GetAssetByObjectPath(FName(*AssetPath.LeftChop(2)), bIncludeOnlyOnDiskAssets);

		if (OutAssetData.IsValid())
		{
			return;
		}
	}

	OutAssetData = AssetRegistry.GetAssetByObjectPath(FName(*AssetPath));
}

static FAutoConsoleCommand CVarDumpAssetSummary(
	TEXT("AssetManager.DumpSummary"),
	TEXT("Shows a summary of things loaded into the asset manager."),
	FConsoleCommandDelegate::CreateStatic(UAssetManager::DumpAssetSummary),
	ECVF_Cheat);

void UAssetManager::DumpAssetSummary()
{
	TArray<FPrimaryAssetTypeInfo> TypeInfos;

	Get().GetPrimaryAssetTypeInfoList(TypeInfos);

	UE_LOG(LogAssetManager, Log, TEXT("=========== Dumping Asset Manager Info ==========="));

	for (const FPrimaryAssetTypeInfo& Info : TypeInfos)
	{
		UE_LOG(LogAssetManager, Log, TEXT("  %s: Class %s, Count %d, Paths %s"), *Info.PrimaryAssetType.ToString(), *Info.AssetBaseClass->GetName(), Info.NumberOfAssets, *FString::Join(Info.AssetDataPaths, TEXT(", ")));
	}
}

#if WITH_EDITOR
void UAssetManager::ScanForAssetPath(IAssetRegistry& AssetRegistry, const FString& AssetPath) const
{
	const FString PackageName = FPackageName::ObjectPathToPackageName(AssetPath);

	FString AssetFilename;
	if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, AssetFilename, FPackageName::GetAssetPackageExtension()))
	{
		AssetRegistry.ScanFilesSynchronous(TArray<FString>{AssetFilename});
	}
	else
	{
		UE_LOG(LogAssetManager, Error, TEXT("GetAssetDataForPath: Invalid ObjectPath (%s)"), *AssetPath);
	}
}

void UAssetManager::OnAssetRegistryFilesLoaded()
{
	StartBulkScanning();

	for (TPair<FName, TSharedRef<FPrimaryAssetTypeData>> TypePair : AssetTypeMap)
	{
		FPrimaryAssetTypeData& TypeData = TypePair.Value.Get();

		if (TypeData.DeferredAssetDataPaths.Num())
		{
			// File scan finished, now scan for assets. Maps are sorted so this will be in the order of original scan requests
			ScanPathsForPrimaryAssets(TypePair.Key, TypeData.DeferredAssetDataPaths, TypeData.Info.AssetBaseClass, TypeData.Info.bHasBlueprintClasses, TypeData.Info.bIsEditorOnly, true);

			TypeData.DeferredAssetDataPaths.Empty();
		}
	}

	StopBulkScanning();
}

void UAssetManager::ModifyCook(TArray<FString>& PackageNames)
{

}

void UAssetManager::AssignStreamingChunk(const FString& PackageToAdd, const FString& LastLoadedMapName, const TArray<int32>& AssetRegistryChunkIDs, const TArray<int32>& ExistingChunkIds, TArray<int32>& OutChunkIndexList)
{
	// Take asset registry assignments and existing assignments
	OutChunkIndexList.Append(AssetRegistryChunkIDs);
	OutChunkIndexList.Append(ExistingChunkIds);
}

bool UAssetManager::GetPackageDependenciesForManifestGenerator(FName PackageName, TArray<FName>& DependentPackageNames, EAssetRegistryDependencyType::Type DependencyType)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	return AssetRegistry.GetDependencies(PackageName, DependentPackageNames, DependencyType);
}

void UAssetManager::PreBeginPIE(bool bStartSimulate)
{
	RefreshPrimaryAssetDirectory();

	// Cache asset state
	GetPrimaryAssetBundleStateMap(PrimaryAssetStateBeforePIE, false);
}

void UAssetManager::EndPIE(bool bStartSimulate)
{
	// Reset asset load state
	for (TPair<FName, TSharedRef<FPrimaryAssetTypeData>> TypePair : AssetTypeMap)
	{
		FPrimaryAssetTypeData& TypeData = TypePair.Value.Get();

		for (TPair<FName, FPrimaryAssetData> NamePair : TypeData.AssetMap)
		{
			FPrimaryAssetData& NameData = NamePair.Value;

			FPrimaryAssetLoadState& LoadState = (!NameData.PendingState.IsValid()) ? NameData.CurrentState : NameData.PendingState;

			if (!LoadState.IsValid())
			{
				// Don't worry about things that aren't loaded
				continue;
			}

			FPrimaryAssetId AssetID(TypePair.Key, NamePair.Key);

			TArray<FName>* BundleState = PrimaryAssetStateBeforePIE.Find(AssetID);

			if (BundleState)
			{
				// This will reset state to what it was before
				LoadPrimaryAsset(AssetID, *BundleState);
			}
			else
			{
				// Not in map, unload us
				UnloadPrimaryAsset(AssetID);
			}
		}
	}
}

void UAssetManager::RefreshPrimaryAssetDirectory()
{
	StartBulkScanning();

	for (TPair<FName, TSharedRef<FPrimaryAssetTypeData>> TypePair : AssetTypeMap)
	{
		FPrimaryAssetTypeData& TypeData = TypePair.Value.Get();

		if (TypeData.Info.AssetDataPaths.Num())
		{
			// Clear old data
			TypeData.AssetMap.Reset();

			// Rescan all assets
			ScanPathsForPrimaryAssets(TypePair.Key, TypeData.Info.AssetDataPaths, TypeData.Info.AssetBaseClass, TypeData.Info.bHasBlueprintClasses, TypeData.Info.bIsEditorOnly, true);
		}
	}

	StopBulkScanning();
}

void UAssetManager::OnInMemoryAssetCreated(UObject *Object)
{
	// Ignore PIE changes
	if (GIsPlayInEditorWorld || !Object)
	{
		return;
	}

	FPrimaryAssetId PrimaryAssetId = Object->GetPrimaryAssetId();

	if (PrimaryAssetId.IsValid())
	{
		TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetId.PrimaryAssetType);

		if (FoundType)
		{
			auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

			FPrimaryAssetTypeData& TypeData = FoundType->Get();

			FAssetData NewAssetData;

			GetAssetDataForPathInternal(AssetRegistry, Object->GetPathName(), NewAssetData);

			if (ensure(NewAssetData.IsValid()))
			{
				// Make sure it's in a valid path
				bool bFoundPath = false;
				for (const FString& Path : TypeData.Info.AssetDataPaths)
				{
					if (NewAssetData.PackagePath.ToString().Contains(Path))
					{
						bFoundPath = true;
						break;
					}
				}

				if (bFoundPath)
				{
					// Add or update asset data
					UpdateCachedAssetData(PrimaryAssetId, NewAssetData, true);

					RebuildObjectReferenceList();
				}
			}
		}
	}
}

void UAssetManager::OnInMemoryAssetDeleted(UObject *Object)
{
	// Ignore PIE changes
	if (GIsPlayInEditorWorld || !Object)
	{
		return;
	}

	FPrimaryAssetId PrimaryAssetId = Object->GetPrimaryAssetId();

	RemovePrimaryAssetId(PrimaryAssetId);
}

void UAssetManager::OnAssetRenamed(const FAssetData& NewData, const FString& OldPath)
{
	// Ignore PIE changes
	if (GIsPlayInEditorWorld || !NewData.IsValid())
	{
		return;
	}

	FPrimaryAssetId OldPrimaryAssetId = GetPrimaryAssetIdForPath(OldPath);

	RemovePrimaryAssetId(OldPrimaryAssetId);

	// This will always be in memory
	UObject *NewObject = NewData.GetAsset();

	OnInMemoryAssetCreated(NewObject);
}

void UAssetManager::RemovePrimaryAssetId(const FPrimaryAssetId& PrimaryAssetId)
{
	if (PrimaryAssetId.IsValid() && GetNameData(PrimaryAssetId))
	{
		// It's in our dictionary, remove it

		TSharedRef<FPrimaryAssetTypeData>* FoundType = AssetTypeMap.Find(PrimaryAssetId.PrimaryAssetType);
		check(FoundType);
		FPrimaryAssetTypeData& TypeData = FoundType->Get();

		TypeData.AssetMap.Remove(PrimaryAssetId.PrimaryAssetName);

		RebuildObjectReferenceList();
	}
}

void UAssetManager::RefreshAssetData(UObject* ChangedObject)
{
	// Only update things it knows about
	FPrimaryAssetId PrimaryAssetId = ChangedObject->GetPrimaryAssetId();
	FStringAssetReference ChangedObjectPath(ChangedObject);

	if (PrimaryAssetId.IsValid() && GetPrimaryAssetPath(PrimaryAssetId) == ChangedObjectPath)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		// This will load it out of the in memory object
		FAssetData NewData;
		GetAssetDataForPathInternal(AssetRegistry, ChangedObjectPath.ToString(), NewData);

		if (ensure(NewData.IsValid()))
		{
			UpdateCachedAssetData(PrimaryAssetId, NewData, false);
		}	
	}
}

void UAssetManager::InitializeAssetBundlesFromMetadata(const UStruct* Struct, const void* StructValue, FAssetBundleData& AssetBundle) const
{
	static FName AssetBundlesName = TEXT("AssetBundles");

	if (!ensure(Struct && StructValue))
	{
		return;
	}

	for (TPropertyValueIterator<const UProperty> It(Struct, StructValue); It; ++It)
	{
		const UProperty* Property = It.Key();
		const void* PropertyValue = It.Value();

		FStringAssetReference FoundRef;
		if (const UAssetClassProperty* AssetClassProp = Cast<UAssetClassProperty>(Property))
		{
			const TAssetSubclassOf<UObject>* AssetClassPtr = reinterpret_cast<const TAssetSubclassOf<UObject>*>(PropertyValue);
			if (AssetClassPtr)
			{
				FoundRef = AssetClassPtr->ToStringReference();
			}
		}
		else if (const UAssetObjectProperty* AssetProp = Cast<UAssetObjectProperty>(Property))
		{
			const TAssetPtr<UObject>* AssetPtr = reinterpret_cast<const TAssetPtr<UObject>*>(PropertyValue);
			if (AssetPtr)
			{
				FoundRef = AssetPtr->ToStringReference();
			}
		}
		else if (const UStructProperty* StructProperty = Cast<UStructProperty>(Property))
		{
			// String Class Reference is binary identical with StringAssetReference
			if (StructProperty->Struct == TBaseStructure<FStringAssetReference>::Get() || StructProperty->Struct == TBaseStructure<FStringClassReference>::Get())
			{
				const FStringAssetReference* AssetRefPtr = reinterpret_cast<const FStringAssetReference*>(PropertyValue);
				if (AssetRefPtr)
				{
					FoundRef = *AssetRefPtr;
				}
				// Skip recursion, we don't care about the raw string property
				It.SkipRecursiveProperty();
			}
		}

		if (!FoundRef.IsNull())
		{
			// Compute the intersection of all specified bundle sets in this property and parent properties
			TSet<FName> BundleSet;

			TArray<const UProperty*> PropertyChain;
			It.GetPropertyChain(PropertyChain);

			for (const UProperty* PropertyToSearch : PropertyChain)
			{
				if (PropertyToSearch->HasMetaData(AssetBundlesName))
				{
					TSet<FName> LocalBundleSet;
					TArray<FString> BundleList;
					FString BundleString = PropertyToSearch->GetMetaData(AssetBundlesName);
					BundleString.ParseIntoArrayWS(BundleList, TEXT(","));

					for (const FString& BundleNameString : BundleList)
					{
						LocalBundleSet.Add(FName(*BundleNameString));
					}

					// If Set is empty, initialize. Otherwise intersect
					if (BundleSet.Num() == 0)
					{
						BundleSet = LocalBundleSet;
					}
					else
					{
						BundleSet = BundleSet.Intersect(LocalBundleSet);
					}
				}
			}

			for (const FName& BundleName : BundleSet)
			{
				AssetBundle.AddBundleAsset(BundleName, FoundRef);
			}
		}
	}
}

#endif // #if WITH_EDITOR

#undef LOCTEXT_NAMESPACE
