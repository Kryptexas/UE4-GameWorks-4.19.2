// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AssetRegistryModule.h"
#include "GameplayCueInterface.h"
#include "GameplayCueManager.h"
#include "GameplayTagsModule.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif


UGameplayCueManager::UGameplayCueManager(const class FObjectInitializer& PCIP)
: Super(PCIP)
{
#if WITH_EDITOR
	bAccelerationMapOutdated = true;
	RegisteredEditorCallbacks = false;
#endif
}


void UGameplayCueManager::HandleGameplayCues(AActor* TargetActor, const FGameplayTagContainer& GameplayCueTags, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters)
{
	for (auto It = GameplayCueTags.CreateConstIterator(); It; ++It)
	{
		HandleGameplayCue(TargetActor, *It, EventType, Parameters);
	}
}

void UGameplayCueManager::HandleGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters)
{
	// GameplayCueTags could have been removed from the dictionary but not content. When the content is resaved the old tag will be cleaned up, but it could still come through here
	// at runtime. Since we only populate the map with dictionary gameplaycue tags, we may not find it here.
	int32* Ptr=GameplayCueDataMap.Find(GameplayCueTag);
	if (Ptr)
	{
		int32 DataIdx = *Ptr;
		HandleGameplayCueNotify_Internal(TargetActor, DataIdx, EventType, Parameters);

		IGameplayCueInterface* GameplayCueInterface = Cast<IGameplayCueInterface>(TargetActor);
		if (GameplayCueInterface)
		{
			GameplayCueInterface->HandleGameplayCue(TargetActor, GameplayCueTag, EventType, Parameters);
		}
	}
}

void UGameplayCueManager::HandleGameplayCueNotify_Internal(AActor* TargetActor, int32 DataIdx, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters)
{	
	if (DataIdx != INDEX_NONE)
	{
		check(GameplayCueData.IsValidIndex(DataIdx));

		FGameplayCueNotifyData& CueData = GameplayCueData[DataIdx];

		// If object is not loaded yet
		if (CueData.LoadedGameplayCueNotify == nullptr)
		{
			// Ignore removed events if this wasn't already loaded (only call Removed if we handled OnActive/WhileActive)
			if (EventType == EGameplayCueEvent::Removed)
			{
				return;
			}

			// See if the object is loaded but just not hooked up here
			UObject* FoundObject = FindObject<UObject>(nullptr, *CueData.GameplayCueNotifyObj.ToString());
			if (FoundObject == nullptr)
			{
				// Not loaded: start async loading and return
				StreamableManager.SimpleAsyncLoad(CueData.GameplayCueNotifyObj);

				ABILITY_LOG(Warning, TEXT("GameplayCueNotify %s was not loaded when GameplayCue was invoked. Starting async loading."), *CueData.GameplayCueNotifyObj.ToString());
				return;
			}
			else
			{
				// Found it - did we load a data asset (UGameplayCueNotify) or a Blueprint (UBlueprint)
				CueData.LoadedGameplayCueNotify = Cast<UGameplayCueNotify>(FoundObject);
				if (CueData.LoadedGameplayCueNotify == nullptr)
				{
					// Not a dataasset - maybe a blueprint
					
					UBlueprint* GameplayCueBlueprint = Cast<UBlueprint>(FoundObject);
					if (GameplayCueBlueprint && GameplayCueBlueprint->GeneratedClass)
					{
						CueData.LoadedGameplayCueNotify = Cast<UGameplayCueNotify>(GameplayCueBlueprint->GeneratedClass->ClassDefaultObject);
					}
					else
					{
						ABILITY_LOG(Warning, TEXT("GameplayCueNotify %s loaded object %s that is not a UGameplayCueNotify or UBlueprint"), *CueData.GameplayCueNotifyObj.ToString(), *FoundObject->GetName());
						return;
					}
					
				}
			}
		}

		// Handle the Notify if we found something
		if (CueData.LoadedGameplayCueNotify)
		{
			UGameplayCueNotify* SpawnedInstanced = nullptr;
			if (auto InnerMap = NotifyMap.Find(TargetActor))
			{
				if (auto WeakPtrPtr = InnerMap->Find(DataIdx))
				{
					SpawnedInstanced = WeakPtrPtr->Get();
				}
			}

			if (SpawnedInstanced == nullptr && CueData.LoadedGameplayCueNotify->NeedsInstanceForEvent(EventType))
			{
				// We don't have an instance for this, and we need one, so make one
				SpawnedInstanced = static_cast<UGameplayCueNotify*>(StaticDuplicateObject(CueData.LoadedGameplayCueNotify, this, TEXT("None"), ~RF_RootSet));

				auto& InnerMap = NotifyMap.FindOrAdd(TargetActor);

				InnerMap.Add(DataIdx) = SpawnedInstanced;

				// Fixme: this will grow unbounded
				InstantiatedObjects.Add(SpawnedInstanced);
			}
			
			UGameplayCueNotify* NotifyToExecute = SpawnedInstanced ? SpawnedInstanced : CueData.LoadedGameplayCueNotify;
			NotifyToExecute->HandleGameplayCue(TargetActor, EventType, Parameters);

			if (NotifyToExecute->IsOverride == false)
			{
				HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
			}
		}
	}
}

void UGameplayCueManager::EndGameplayCuesFor(AActor* TargetActor)
{
	TMap<int32, TWeakObjectPtr<UGameplayCueNotify> > FoundMap;	
	if (NotifyMap.RemoveAndCopyValue(TargetActor, FoundMap))
	{
		for (auto It = FoundMap.CreateConstIterator(); It; ++It)
		{
			UGameplayCueNotify* InstancedCue =  It.Value().Get();
			if (InstancedCue)
			{
				InstancedCue->OnOwnerDestroyed();
			}
		}
	}
}

// ------------------------------------------------------------------------

void UGameplayCueManager::LoadObjectLibraryFromPaths(const TArray<FString>& InPaths, bool InFullyLoad)
{
	if (!GameplayCueNotifyObjectLibrary)
	{
		GameplayCueNotifyObjectLibrary = UObjectLibrary::CreateLibrary(UGameplayCueNotify::StaticClass(), true, GIsEditor);
	}

	LoadedPaths = InPaths;
	bFullyLoad = InFullyLoad;

	LoadObjectLibrary_Internal();
#if WITH_EDITOR
	bAccelerationMapOutdated = false;
	if (!RegisteredEditorCallbacks)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().OnInMemoryAssetCreated().AddUObject(this, &UGameplayCueManager::HandleAssetAdded);
		AssetRegistryModule.Get().OnInMemoryAssetDeleted().AddUObject(this, &UGameplayCueManager::HandleAssetDeleted);
		FWorldDelegates::OnPreWorldInitialization.AddUObject(this, &UGameplayCueManager::ReloadObjectLibrary);
		RegisteredEditorCallbacks = true;
	}
#endif
}

#if WITH_EDITOR
void UGameplayCueManager::ReloadObjectLibrary(UWorld* World, const UWorld::InitializationValues IVS)
{
	if (bAccelerationMapOutdated)
	{
		LoadObjectLibrary_Internal();
	}
}
#endif

void UGameplayCueManager::LoadObjectLibrary_Internal()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading Library"), STAT_ObjectLibrary, STATGROUP_LoadTime);

#if WITH_EDITOR
	bAccelerationMapOutdated = false;
	FFormatNamedArguments Args;
	FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("AbilitySystemEditor", "BeginLoadingGameplayCueNotify", "Loading GameplayCue Library"), Args));
	SlowTask.MakeDialog();
#endif

	FScopeCycleCounterUObject PreloadScope(GameplayCueNotifyObjectLibrary);
	
	GameplayCueNotifyObjectLibrary->LoadBlueprintAssetDataFromPaths(LoadedPaths);

	if (bFullyLoad)
	{
#if STATS
		FString PerfMessage = FString::Printf(TEXT("Fully Loaded GameplayCueNotify object library"));
		SCOPE_LOG_TIME_IN_SECONDS(*PerfMessage, nullptr)
#endif
		GameplayCueNotifyObjectLibrary->LoadAssetsFromAssetData();
	}

	// ---------------------------------------------------------
	// Look for GameplayCueNotifies that handle events
	// ---------------------------------------------------------
	
	TArray<FAssetData> AssetDatas;
	GameplayCueNotifyObjectLibrary->GetAssetDataList(AssetDatas);

	IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();

	GameplayCueData.Empty();
	GameplayCueDataMap.Empty();

	for (FAssetData Data: AssetDatas)
	{
		const FString* FoundGameplayTag = Data.TagsAndValues.Find(GET_MEMBER_NAME_CHECKED(UGameplayCueNotify, GameplayCueName));
		if (FoundGameplayTag && FoundGameplayTag->Equals(TEXT("None")) == false)
		{
			ABILITY_LOG(Warning, TEXT("Found: %s"), **FoundGameplayTag);

			FGameplayTag  GameplayCueTag = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTag(FName(**FoundGameplayTag), false);
			if (GameplayCueTag.IsValid())
			{
				// Add a new NotifyData entry to our flat list for this one
				FStringAssetReference StringRef;
				StringRef.AssetLongPathname = Data.ObjectPath.ToString();

				AddGameplayCueData_Internal(GameplayCueTag, StringRef);
				
			}
			else
			{
				ABILITY_LOG(Warning, TEXT("Found GameplayCue tag %s in asset %s but there is no corresponding tag in the GameplayTagMAnager."), **FoundGameplayTag, *Data.PackageName.ToString());
			}
		}
	}

	BuildAccelerationMap_Internal();
}

void UGameplayCueManager::AddGameplayCueData_Internal(FGameplayTag  GameplayCueTag, FStringAssetReference StringRef)
{
	// Check for duplicates: we may want to remove this eventually.
	for (FGameplayCueNotifyData Data : GameplayCueData)
	{
		if (Data.GameplayCueTag == GameplayCueTag)
		{
			ABILITY_LOG(Warning, TEXT("AddGameplayCueData_Internal called for [%s,%s] when it already existed [%s,%s]. Skipping."), *GameplayCueTag.ToString(), *StringRef.AssetLongPathname, *Data.GameplayCueTag.ToString(), *Data.GameplayCueNotifyObj.AssetLongPathname);
			return;
		}
	}

	FGameplayCueNotifyData NewData;
	NewData.GameplayCueNotifyObj = StringRef;
	NewData.GameplayCueTag = GameplayCueTag;

	GameplayCueData.Add(NewData);
}

void UGameplayCueManager::BuildAccelerationMap_Internal()
{
	// ---------------------------------------------------------
	//	Build up the rest of the acceleration map: every GameplayCue tag should have an entry in the map that points to the index into GameplayCueData to use when it is invoked.
	//	(or to -1 if no GameplayCueNotify is associated with that tag)
	// 
	// ---------------------------------------------------------

	GameplayCueDataMap.Empty();
	GameplayCueDataMap.Add(BaseGameplayCueTag()) = INDEX_NONE;

	for (int32 idx = 0; idx < GameplayCueData.Num(); ++idx)
	{
		GameplayCueDataMap.FindOrAdd(GameplayCueData[idx].GameplayCueTag) = idx;
	}

	FGameplayTagContainer AllGameplayCueTags = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTagChildren(BaseGameplayCueTag());

	for (FGameplayTag ThisGameplayCueTag : AllGameplayCueTags)
	{
		if (GameplayCueDataMap.Contains(ThisGameplayCueTag))
		{
			continue;
		}

		FGameplayTag Parent = ThisGameplayCueTag.RequestDirectParent();

		GameplayCueDataMap.Add(ThisGameplayCueTag) = GameplayCueDataMap.FindChecked(Parent);
	}

	// PrintGameplayCueNotifyMap();
}

int32 UGameplayCueManager::FinishLoadingGameplayCueNotifies()
{
	int32 NumLoadeded = 0;
	return NumLoadeded;
}

void UGameplayCueManager::BeginLoadingGameplayCueNotify(FGameplayTag GameplayCueTag)
{

}

#if WITH_EDITOR

void UGameplayCueManager::HandleAssetAdded(UObject *Object)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(Object);
	if (Blueprint && Blueprint->GeneratedClass)
	{
		UGameplayCueNotify* CDO = Cast<UGameplayCueNotify>(Blueprint->GeneratedClass->ClassDefaultObject);
		if (CDO && CDO->GameplayCueTag.IsValid())
		{
			FStringAssetReference StringRef;
			StringRef.AssetLongPathname = Blueprint->GetPathName();

			AddGameplayCueData_Internal(CDO->GameplayCueTag, StringRef);
			BuildAccelerationMap_Internal();
		}
	}
	
	
	UGameplayCueNotify* Notify = Cast<UGameplayCueNotify>(Object);
	if (Notify && Notify->GameplayCueTag.IsValid())
	{
		FStringAssetReference StringRef;
		StringRef.AssetLongPathname = Notify->GetPathName();

		AddGameplayCueData_Internal(Notify->GameplayCueTag, StringRef);
		BuildAccelerationMap_Internal();
	}
}

/** Handles cleaning up an object library if it matches the passed in object */
void UGameplayCueManager::HandleAssetDeleted(UObject *Object)
{
	FGameplayTag TagtoRemove;

	UBlueprint* Blueprint = Cast<UBlueprint>(Object);
	if (Blueprint && Blueprint->GeneratedClass)
	{
		UGameplayCueNotify* CDO = Cast<UGameplayCueNotify>(Blueprint->GeneratedClass->ClassDefaultObject);
		if (CDO && CDO->GameplayCueTag.IsValid())
		{
			TagtoRemove = CDO->GameplayCueTag;
		}
	}

	UGameplayCueNotify* Notify = Cast<UGameplayCueNotify>(Object);
	if (Notify && Notify->GameplayCueTag.IsValid())
	{
		TagtoRemove = Notify->GameplayCueTag;
	}

	if (TagtoRemove.IsValid())
	{
		for (int32 idx = 0; idx < GameplayCueData.Num(); ++idx)
		{
			if (GameplayCueData[idx].GameplayCueTag == TagtoRemove)
			{
				GameplayCueData.RemoveAt(idx);
				BuildAccelerationMap_Internal();
				break;
			}
		}
	}
}

#endif

void UGameplayCueManager::PrintGameplayCueNotifyMap()
{
	FGameplayTagContainer AllGameplayCueTags = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTagChildren(BaseGameplayCueTag());

	for (FGameplayTag ThisGameplayCueTag : AllGameplayCueTags)
	{
		int32 idx = GameplayCueDataMap.FindChecked(ThisGameplayCueTag);
		if (idx != INDEX_NONE)
		{
			ABILITY_LOG(Warning, TEXT("   %s -> %d"), *ThisGameplayCueTag.ToString(), idx);
		}
		else
		{
			ABILITY_LOG(Warning, TEXT("   %s -> unmapped"), *ThisGameplayCueTag.ToString());
		}
	}
}

FGameplayTag UGameplayCueManager::BaseGameplayCueTag()
{
	static FGameplayTag  BaseGameplayCueTag = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTag(TEXT("GameplayCue"), false);
	return BaseGameplayCueTag;
}

static void	PrintGameplayCueNotifyMapConsoleCommandFunc(UWorld* InWorld)
{
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->PrintGameplayCueNotifyMap();
}

FAutoConsoleCommandWithWorld PrintGameplayCueNotifyMapConsoleCommand(
	TEXT("GameplayCue.PrintGameplayCueNotifyMap"),
	TEXT("Displays GameplayCue notify map"),
	FConsoleCommandWithWorldDelegate::CreateStatic(PrintGameplayCueNotifyMapConsoleCommandFunc)
	);
