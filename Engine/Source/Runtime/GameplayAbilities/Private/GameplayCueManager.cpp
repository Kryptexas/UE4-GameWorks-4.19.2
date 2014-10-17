// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AssetRegistryModule.h"
#include "GameplayCueInterface.h"
#include "GameplayCueManager.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

UGameplayCueManager::UGameplayCueManager(const class FObjectInitializer& PCIP)
: Super(PCIP)
{
	RegisteredEditorCallbacks = false;
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
	int32 DataIdx = GameplayCueDataMap.FindChecked(GameplayCueTag);	// We should always find data here. We prepopulate the entire map with every gameplaycue tag when we init.
	HandleGameplayCueNotify_Internal(TargetActor, DataIdx, EventType, Parameters);

	IGameplayCueInterface* GameplayCueInterface = Cast<IGameplayCueInterface>(TargetActor);
	if (GameplayCueInterface)
	{
		GameplayCueInterface->HandleGameplayCue(TargetActor, GameplayCueTag, EventType, Parameters);
	}
}

void UGameplayCueManager::HandleGameplayCueNotify_Internal(AActor* TargetActor, int32 DataIdx, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters)
{	
	if (DataIdx != INDEX_NONE)
	{
		check(GameplayCueData.IsValidIndex(DataIdx));

		FGameplayCueNotifyData& CueData = GameplayCueData[DataIdx];

		if (CueData.LoadedGameplayCueNotify == nullptr)
		{
			CueData.LoadedGameplayCueNotify = FindObject<UGameplayCueNotify>(nullptr, *CueData.GameplayCueNotifyObj.ToString());
			if (CueData.LoadedGameplayCueNotify == nullptr)
			{
				StreamableManager.SimpleAsyncLoad(CueData.GameplayCueNotifyObj);
				ABILITY_LOG(Warning, TEXT("GameplayCueNotify %s was not loaded when GameplayCue was invoked. Starting async loading."), *CueData.GameplayCueNotifyObj.ToString());
				return;
			}
		}

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

void UGameplayCueManager::LoadObjectLibraryFromPaths(const TArray<FString>& Paths, bool bFullyLoad)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading Library"), STAT_ObjectLibrary, STATGROUP_LoadTime);

#if WITH_EDITOR
	FFormatNamedArguments Args;
	FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("AbilitySystemEditor", "BeginLoadingGameplayCueNotify", "Loading GameplayCue Library"), Args));
	SlowTask.MakeDialog();
#endif

	if (!GameplayCueNotifyObjectLibrary)
	{
		GameplayCueNotifyObjectLibrary = UObjectLibrary::CreateLibrary(UGameplayCueNotify::StaticClass(), true, GIsEditor);
	}

	FScopeCycleCounterUObject PreloadScope(GameplayCueNotifyObjectLibrary);

	GameplayCueNotifyObjectLibrary->LoadBlueprintAssetDataFromPaths(Paths);
	GameplayCueNotifyObjectLibrary->LoadAssetDataFromPaths(Paths);	

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

	TSet<FGameplayTag>	FoundTags;

	for (FAssetData Data: AssetDatas)
	{
		const FString* FoundGameplayTag = Data.TagsAndValues.Find(GET_MEMBER_NAME_CHECKED(UGameplayCueNotify, GameplayCueName));
		if (FoundGameplayTag)
		{
			ABILITY_LOG(Warning, TEXT("Found: %s"), **FoundGameplayTag);

			FGameplayTag  GameplayCueTag = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTag(FName(**FoundGameplayTag), false);
			if (GameplayCueTag.IsValid())
			{
				FoundTags.Add(GameplayCueTag);

				// Add a new NotifyData entry to our flat list for this one
				FGameplayCueNotifyData NewData;
				FStringAssetReference StringRef;
				StringRef.AssetLongPathname = Data.ObjectPath.ToString();

				TAssetSubclassOf<UGameplayCueNotify> AssetRef;
				AssetRef = StringRef;
				
				NewData.GameplayCueNotifyObj = StringRef;
				int32 NewIdx = GameplayCueData.Add(NewData);

				// Add an entry into the acceleration map
				GameplayCueDataMap.FindOrAdd(GameplayCueTag) = NewIdx;
			}
			else
			{
				ABILITY_LOG(Warning, TEXT("Found GameplayCue tag %s in asset %s but there is no corresponding tag in the GameplayTagMAnager."), **FoundGameplayTag, *Data.PackageName.ToString());
			}
		}
	}

	// ---------------------------------------------------------
	//	Build up the rest of the acceleration map: every GameplayCue tag should have an entry in the map that points to the index into GameplayCueData to use when it is invoked.
	//	(or to -1 if no GameplayCueNotify is associated with that tag)
	// 
	// ---------------------------------------------------------
	
	FGameplayTagContainer AllGameplayCueTags = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTagChildren(BaseGameplayCueTag());

	GameplayCueDataMap.Add(BaseGameplayCueTag()) = INDEX_NONE;

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

#if WITH_EDITOR
	if (!RegisteredEditorCallbacks)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().OnInMemoryAssetCreated().AddUObject(this, &UGameplayCueManager::HandleAssetAdded);
		AssetRegistryModule.Get().OnInMemoryAssetDeleted().AddUObject(this, &UGameplayCueManager::HandleAssetDeleted);
		RegisteredEditorCallbacks = true;
	}
#endif

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

}

/** Handles cleaning up an object library if it matches the passed in object */
void UGameplayCueManager::HandleAssetDeleted(UObject *Object)
{

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