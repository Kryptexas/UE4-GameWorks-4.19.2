// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AssetRegistryModule.h"
#include "GameplayCueInterface.h"
#include "GameplayCueManager.h"
#include "GameplayTagsModule.h"
#include "GameplayCueNotify_Static.h"
#include "AbilitySystemComponent.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif


UGameplayCueManager::UGameplayCueManager(const FObjectInitializer& PCIP)
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
	if (TargetActor == nullptr)
	{
		ABILITY_LOG(Warning, TEXT("UGameplayCueManager::HandleGameplayCue called on null TargetActor. GameplayCueTag: %s."), *GameplayCueTag.ToString());
		return;
	}


	// GameplayCueTags could have been removed from the dictionary but not content. When the content is resaved the old tag will be cleaned up, but it could still come through here
	// at runtime. Since we only populate the map with dictionary gameplaycue tags, we may not find it here.
	int32* Ptr=GameplayCueDataMap.Find(GameplayCueTag);
	if (Ptr)
	{
		int32 DataIdx = *Ptr;
		HandleGameplayCueNotify_Internal(TargetActor, DataIdx, EventType, Parameters);	
	}

	// Use the interface even if it's not in the map
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

		// If object is not loaded yet
		if (CueData.LoadedGameplayCueClass == nullptr)
		{
			// Ignore removed events if this wasn't already loaded (only call Removed if we handled OnActive/WhileActive)
			if (EventType == EGameplayCueEvent::Removed)
			{
				return;
			}

			// See if the object is loaded but just not hooked up here
			CueData.LoadedGameplayCueClass = FindObject<UClass>(nullptr, *CueData.GameplayCueNotifyObj.ToString());
			if (CueData.LoadedGameplayCueClass == nullptr)
			{
				// Not loaded: start async loading and return
				StreamableManager.SimpleAsyncLoad(CueData.GameplayCueNotifyObj);

				ABILITY_LOG(Warning, TEXT("GameplayCueNotify %s was not loaded when GameplayCue was invoked. Starting async loading."), *CueData.GameplayCueNotifyObj.ToString());
				return;
			}
		}

		// Handle the Notify if we found something
		if (UGameplayCueNotify_Static* NonInstancedCue = Cast<UGameplayCueNotify_Static>(CueData.LoadedGameplayCueClass->ClassDefaultObject))
		{
			if (NonInstancedCue->HandlesEvent(EventType))
			{
				NonInstancedCue->HandleGameplayCue(TargetActor, EventType, Parameters);
				if (!NonInstancedCue->IsOverride)
				{
					HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
				}
			}
			else
			{
				//Didn't even handle it, so IsOverride should not apply.
				HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
			}
		}
		else if (AGameplayCueNotify_Actor* InstancedCue = Cast<AGameplayCueNotify_Actor>(CueData.LoadedGameplayCueClass->ClassDefaultObject))
		{
			if (InstancedCue->HandlesEvent(EventType))
			{
				AGameplayCueNotify_Actor* SpawnedInstancedCue = nullptr;
				if (auto InnerMap = NotifyMapActor.Find(TargetActor))
				{
					if (auto WeakPtrPtr = InnerMap->Find(DataIdx))
					{
						SpawnedInstancedCue = WeakPtrPtr->Get();
					}
				}

				//Get our instance. We should probably have a flag or something to determine if we want to reuse or stack instances. That would mean changing our map to have a list of active instances.
				if (SpawnedInstancedCue == nullptr)
				{
					// We don't have an instance for this, and we need one, so make one
					//SpawnedInstancedCue = static_cast<AGameplayCueNotify_Actor*>(StaticDuplicateObject(InstancedCue, this, TEXT("None"), ~RF_RootSet));
					SpawnedInstancedCue = TargetActor->GetWorld()->SpawnActor<AGameplayCueNotify_Actor>(InstancedCue->GetClass(), TargetActor->GetActorLocation(), TargetActor->GetActorRotation());
					auto& InnerMap = NotifyMapActor.FindOrAdd(TargetActor);
					InnerMap.Add(DataIdx) = SpawnedInstancedCue;
				}
				check(SpawnedInstancedCue);
				SpawnedInstancedCue->HandleGameplayCue(TargetActor, EventType, Parameters);
				if (!SpawnedInstancedCue->IsOverride)
				{
					HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
				}
			}
			else
			{
				//Didn't even handle it, so IsOverride should not apply.
				HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
			}
		}
	}
}

void UGameplayCueManager::EndGameplayCuesFor(AActor* TargetActor)
{
	TMap<int32, TWeakObjectPtr<AGameplayCueNotify_Actor>> FoundMapActor;
	if (NotifyMapActor.RemoveAndCopyValue(TargetActor, FoundMapActor))
	{
		for (auto It = FoundMapActor.CreateConstIterator(); It; ++It)
		{
			AGameplayCueNotify_Actor* InstancedCue =  It.Value().Get();
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
	if (!GameplayCueNotifyActorObjectLibrary)
	{
		GameplayCueNotifyActorObjectLibrary = UObjectLibrary::CreateLibrary(AGameplayCueNotify_Actor::StaticClass(), true, GIsEditor && !IsRunningCommandlet());
	}
	if (!GameplayCueNotifyStaticObjectLibrary)
	{
		GameplayCueNotifyStaticObjectLibrary = UObjectLibrary::CreateLibrary(UGameplayCueNotify_Static::StaticClass(), true, GIsEditor && !IsRunningCommandlet());
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

	FScopeCycleCounterUObject PreloadScopeActor(GameplayCueNotifyActorObjectLibrary);
	GameplayCueNotifyActorObjectLibrary->LoadBlueprintAssetDataFromPaths(LoadedPaths);
	GameplayCueNotifyStaticObjectLibrary->LoadBlueprintAssetDataFromPaths(LoadedPaths);		//No separate cycle counter for this.

	if (bFullyLoad)
	{
#if STATS
		FString PerfMessage = FString::Printf(TEXT("Fully Loaded GameplayCueNotify object library"));
		SCOPE_LOG_TIME_IN_SECONDS(*PerfMessage, nullptr)
#endif
		GameplayCueNotifyActorObjectLibrary->LoadAssetsFromAssetData();
		GameplayCueNotifyStaticObjectLibrary->LoadAssetsFromAssetData();
	}

	// ---------------------------------------------------------
	// Look for GameplayCueNotifies that handle events
	// ---------------------------------------------------------
	
	TArray<FAssetData> ActorAssetDatas;
	GameplayCueNotifyActorObjectLibrary->GetAssetDataList(ActorAssetDatas);

	TArray<FAssetData> StaticAssetDatas;
	GameplayCueNotifyStaticObjectLibrary->GetAssetDataList(StaticAssetDatas);

	GameplayCueData.Empty();
	GameplayCueDataMap.Empty();

	AddAssetDataList_Internal(ActorAssetDatas, GET_MEMBER_NAME_CHECKED(AGameplayCueNotify_Actor, GameplayCueName));
	AddAssetDataList_Internal(StaticAssetDatas, GET_MEMBER_NAME_CHECKED(UGameplayCueNotify_Static, GameplayCueName));

	BuildAccelerationMap_Internal();
}

void UGameplayCueManager::AddAssetDataList_Internal(const TArray<FAssetData>& AssetDataList, FName TagPropertyName)
{
	IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();

	for (FAssetData Data: AssetDataList)
	{
		const FString* FoundGameplayTag = Data.TagsAndValues.Find(TagPropertyName);
		if (FoundGameplayTag && FoundGameplayTag->Equals(TEXT("None")) == false)
		{
			const FString* GeneratedClassTag = Data.TagsAndValues.Find(TEXT("GeneratedClass"));
			if (GeneratedClassTag == nullptr)
			{
				ABILITY_LOG(Warning, TEXT("Unable to find GeneratedClass value for AssetData %s"), *Data.ObjectPath.ToString());
				continue;
			}

			ABILITY_LOG(Log, TEXT("GameplayCueManager Found: %s / %s"), **FoundGameplayTag, **GeneratedClassTag);

			FGameplayTag  GameplayCueTag = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTag(FName(**FoundGameplayTag), false);
			if (GameplayCueTag.IsValid())
			{
				// Add a new NotifyData entry to our flat list for this one
				FStringAssetReference StringRef;
				StringRef.AssetLongPathname = *GeneratedClassTag;
				AddGameplayCueData_Internal(GameplayCueTag, StringRef);				
			}
			else
			{
				ABILITY_LOG(Warning, TEXT("Found GameplayCue tag %s in asset %s but there is no corresponding tag in the GameplayTagMAnager."), **FoundGameplayTag, *Data.PackageName.ToString());
			}
		}
	}
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

	UObject* FoundObject = FindObject<UObject>(nullptr, *NewData.GameplayCueNotifyObj.ToString());

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


	// Create entries for children.
	// E.g., if "a.b" notify exists but "a.b.c" does not, point "a.b.c" entry to "a.b"'s notify.
	for (FGameplayTag ThisGameplayCueTag : AllGameplayCueTags)
	{
		if (GameplayCueDataMap.Contains(ThisGameplayCueTag))
		{
			continue;
		}

		FGameplayTag Parent = ThisGameplayCueTag.RequestDirectParent();

		GameplayCueDataMap.Add(ThisGameplayCueTag) = GameplayCueDataMap.FindChecked(Parent);
	}


	// Build up parentIdx on each item in GameplayCUeData
	for (FGameplayCueNotifyData& Data : GameplayCueData)
	{
		FGameplayTag Parent = Data.GameplayCueTag.RequestDirectParent();
		while(Parent != BaseGameplayCueTag())
		{
			int32* idxPtr = GameplayCueDataMap.Find(Parent);
			if (idxPtr)
			{
				Data.ParentDataIdx = *idxPtr;
				break;
			}
			Parent = Parent.RequestDirectParent();
		}
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
		UGameplayCueNotify_Static* StaticCDO = Cast<UGameplayCueNotify_Static>(Blueprint->GeneratedClass->ClassDefaultObject);
		AGameplayCueNotify_Actor* ActorCDO = Cast<AGameplayCueNotify_Actor>(Blueprint->GeneratedClass->ClassDefaultObject);
		
		FStringAssetReference StringRef;
		StringRef.AssetLongPathname = Blueprint->GeneratedClass->GetPathName();
		
		if (StaticCDO)
		{
			AddGameplayCueData_Internal(StaticCDO->GameplayCueTag, StringRef);
			BuildAccelerationMap_Internal();
		}
		else if (ActorCDO)
		{
			AddGameplayCueData_Internal(ActorCDO->GameplayCueTag, StringRef);
			BuildAccelerationMap_Internal();
		}
	}
}

/** Handles cleaning up an object library if it matches the passed in object */
void UGameplayCueManager::HandleAssetDeleted(UObject *Object)
{
	FStringAssetReference StringRefToRemove;
	UBlueprint* Blueprint = Cast<UBlueprint>(Object);
	if (Blueprint && Blueprint->GeneratedClass)
	{
		UGameplayCueNotify_Static* StaticCDO = Cast<UGameplayCueNotify_Static>(Blueprint->GeneratedClass->ClassDefaultObject);
		AGameplayCueNotify_Actor* ActorCDO = Cast<AGameplayCueNotify_Actor>(Blueprint->GeneratedClass->ClassDefaultObject);
		
		if (StaticCDO || ActorCDO)
		{
			StringRefToRemove.AssetLongPathname = Blueprint->GeneratedClass->GetPathName();
		}
	}

	if (StringRefToRemove.IsValid())
	{
		for (int32 idx = 0; idx < GameplayCueData.Num(); ++idx)
		{
			if (GameplayCueData[idx].GameplayCueNotifyObj == StringRefToRemove)
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

FScopedGameplayCueSendContext::FScopedGameplayCueSendContext()
{
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->StartGameplayCueSendContext();
}
FScopedGameplayCueSendContext::~FScopedGameplayCueSendContext()
{
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->EndGameplayCueSendContext();
}

void UGameplayCueManager::InvokeGameplayCueExecuted_FromSpec(UAbilitySystemComponent* OwningComponent, const FGameplayEffectSpecForRPC Spec, FPredictionKey PredictionKey)
{
	FGameplayCuePendingExecute PendingCue;
	PendingCue.PayloadType = EGameplayCuePayloadType::FromSpec;
	PendingCue.OwningComponent = OwningComponent;
	PendingCue.FromSpec = Spec;
	PendingCue.PredictionKey = PredictionKey;

	if (ProcessPendingCueExecute(PendingCue))
	{
		PendingExecuteCues.Add(PendingCue);
	}

	if (GameplayCueSendContextCount == 0)
	{
		// Not in a context, flush now
		FlushPendingCues();
	}
}

void UGameplayCueManager::InvokeGameplayCueExecuted(UAbilitySystemComponent* OwningComponent, const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayEffectContextHandle EffectContext)
{
	FGameplayCuePendingExecute PendingCue;
	PendingCue.PayloadType = EGameplayCuePayloadType::EffectContext;
	PendingCue.GameplayCueTag = GameplayCueTag;
	PendingCue.OwningComponent = OwningComponent;
	PendingCue.CueParameters.EffectContext = EffectContext;
	PendingCue.PredictionKey = PredictionKey;

	if (ProcessPendingCueExecute(PendingCue))
	{
		PendingExecuteCues.Add(PendingCue);
	}

	if (GameplayCueSendContextCount == 0)
	{
		// Not in a context, flush now
		FlushPendingCues();
	}
}

void UGameplayCueManager::InvokeGameplayCueExecuted_WithParams(UAbilitySystemComponent* OwningComponent, const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters)
{
	FGameplayCuePendingExecute PendingCue;
	PendingCue.PayloadType = EGameplayCuePayloadType::CueParameters;
	PendingCue.GameplayCueTag = GameplayCueTag;
	PendingCue.OwningComponent = OwningComponent;
	PendingCue.CueParameters = GameplayCueParameters;
	PendingCue.PredictionKey = PredictionKey;

	if (ProcessPendingCueExecute(PendingCue))
	{
		PendingExecuteCues.Add(PendingCue);
	}

	if (GameplayCueSendContextCount == 0)
	{
		// Not in a context, flush now
		FlushPendingCues();
	}
}

void UGameplayCueManager::StartGameplayCueSendContext()
{
	GameplayCueSendContextCount++;
}

void UGameplayCueManager::EndGameplayCueSendContext()
{
	GameplayCueSendContextCount--;

	if (GameplayCueSendContextCount == 0)
	{
		FlushPendingCues();
	}
	else if (GameplayCueSendContextCount < 0)
	{
		ABILITY_LOG(Warning, TEXT("UGameplayCueManager::EndGameplayCueSendContext called too many times! Negative context count"));
	}
}

void UGameplayCueManager::FlushPendingCues()
{
	for (int32 i = 0; i < PendingExecuteCues.Num(); i++)
	{
		FGameplayCuePendingExecute& PendingCue = PendingExecuteCues[i];

		// Our component may have gone away
		if (PendingCue.OwningComponent)
		{
			bool bHasAuthority = PendingCue.OwningComponent->IsOwnerActorAuthoritative();
			bool bValidPredictionKey = PendingCue.PredictionKey.IsValidKey();

			// TODO: Could implement non-rpc method for replicating if desired
			switch (PendingCue.PayloadType)
			{
			case EGameplayCuePayloadType::CueParameters:
				if (bHasAuthority)
				{
					PendingCue.OwningComponent->ForceReplication();
					PendingCue.OwningComponent->NetMulticast_InvokeGameplayCueExecuted_WithParams(PendingCue.GameplayCueTag, PendingCue.PredictionKey, PendingCue.CueParameters);
				}
				else if (bValidPredictionKey)
				{
					PendingCue.OwningComponent->InvokeGameplayCueEvent(PendingCue.GameplayCueTag, EGameplayCueEvent::Executed, PendingCue.CueParameters);
				}
				break;
			case EGameplayCuePayloadType::EffectContext:
				if (bHasAuthority)
				{
					PendingCue.OwningComponent->ForceReplication();
					PendingCue.OwningComponent->NetMulticast_InvokeGameplayCueExecuted(PendingCue.GameplayCueTag, PendingCue.PredictionKey, PendingCue.CueParameters.EffectContext);
				}
				else if (bValidPredictionKey)
				{
					PendingCue.OwningComponent->InvokeGameplayCueEvent(PendingCue.GameplayCueTag, EGameplayCueEvent::Executed, PendingCue.CueParameters.EffectContext);
				}
				break;
			case EGameplayCuePayloadType::FromSpec:
				if (bHasAuthority)
				{
					PendingCue.OwningComponent->ForceReplication();
					PendingCue.OwningComponent->NetMulticast_InvokeGameplayCueExecuted_FromSpec(PendingCue.FromSpec, PendingCue.PredictionKey);
				}
				else if (bValidPredictionKey)
				{
					PendingCue.OwningComponent->InvokeGameplayCueEvent(PendingCue.FromSpec, EGameplayCueEvent::Executed);
				}
				break;
			}
		}
	}

	PendingExecuteCues.Empty();
}

bool UGameplayCueManager::ProcessPendingCueExecute(FGameplayCuePendingExecute& PendingCue)
{
	// Subclasses can do something here
	return true;
}

bool UGameplayCueManager::DoesPendingCueExecuteMatch(FGameplayCuePendingExecute& PendingCue, FGameplayCuePendingExecute& ExistingCue)
{
	const FHitResult* PendingHitResult = NULL;
	const FHitResult* ExistingHitResult = NULL;

	if (PendingCue.PayloadType != ExistingCue.PayloadType)
	{
		return false;
	}

	if (PendingCue.OwningComponent != ExistingCue.OwningComponent)
	{
		return false;
	}

	if (PendingCue.PredictionKey.PredictiveConnection != ExistingCue.PredictionKey.PredictiveConnection)
	{
		// They can both by null, but if they were predicted by different people exclude it
		return false;
	}

	if (PendingCue.PayloadType == EGameplayCuePayloadType::FromSpec)
	{
		if (PendingCue.FromSpec.Def != ExistingCue.FromSpec.Def)
		{
			return false;
		}

		if (PendingCue.FromSpec.Level != ExistingCue.FromSpec.Level)
		{
			return false;
		}
	}
	else
	{
		if (PendingCue.GameplayCueTag != ExistingCue.GameplayCueTag)
		{
			return false;
		}
	}

	return true;
}

