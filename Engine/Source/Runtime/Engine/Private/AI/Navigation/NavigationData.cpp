// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "../../Public/AI/NavDataGenerator.h"

//----------------------------------------------------------------------//
// FNavDataGenerator
//----------------------------------------------------------------------//
void FNavDataGenerator::TiggerGeneration()
{
}

//----------------------------------------------------------------------//
// ANavigationData                                                                
//----------------------------------------------------------------------//
ANavigationData::ANavigationData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bEnableDrawing(true)
	, bShowOnlyDefaultAgent(true)
	, bRebuildAtRuntime(true)
	, DataVersion(NAVMESHVER_LATEST)
	, FindPathImplementation(NULL)
	, FindHierarchicalPathImplementation(NULL)
	, bRegistered(false)
	, NavDataUniqueID(GetNextUniqueID())
{
	PrimaryActorTick.bCanEverTick = true;
	bNetLoadOnClient = false;
	bCanBeDamaged = false;
	DefaultQueryFilter = MakeShareable(new FNavigationQueryFilter());
}

ANavigationData::~ANavigationData()
{
	CleanUp();
}

uint16 ANavigationData::GetNextUniqueID()
{
	check(IsInGameThread());
	static uint16 StaticID = INVALID_NAVDATA;
	return ++StaticID;
}

void ANavigationData::PostInitProperties()
{
	Super::PostInitProperties();

	if (IsPendingKill() == true)
	{
		return;
	}
	
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		UWorld* WorldOuter = GetWorld();
		
		if (WorldOuter != NULL && WorldOuter->GetNavigationSystem() != NULL)
		{
			WorldOuter->GetNavigationSystem()->RequestRegistration(this);
		}

		RenderingComp = ConstructRenderingComponent();
	}
}

void ANavigationData::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (GetWorld() == NULL || GetWorld()->GetNavigationSystem() == NULL)
	{
		CleanUpAndMarkPendingKill();
		return;
	}

	if (IsPendingKill() == true)
	{
		return;
	}

#if WITH_NAVIGATION_GENERATOR
	if (bRebuildAtRuntime == true && NavDataGenerator.IsValid() == false)
	{
		GetGenerator(NavigationSystem::Create);
	}
#endif
}

void ANavigationData::PostLoad() 
{
	Super::PostLoad();

	InstantiateAndRegisterRenderingComponent();
}

void ANavigationData::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	PurgeUnusedPaths();
}

void ANavigationData::RerunConstructionScripts()
{
	Super::RerunConstructionScripts();

	InstantiateAndRegisterRenderingComponent();
}

void ANavigationData::OnRegistered() 
{ 
	InstantiateAndRegisterRenderingComponent();

	bRegistered = true; 
}

void ANavigationData::InstantiateAndRegisterRenderingComponent()
{
#if !UE_BUILD_SHIPPING
	if (!IsPendingKill() && (RenderingComp == NULL || RenderingComp->IsPendingKill()))
	{
		if (RenderingComp)
		{
			// rename the old rendering component out of the way
			RenderingComp->Rename(NULL, NULL, REN_DontCreateRedirectors | REN_ForceGlobalUnique | REN_DoNotDirty | REN_NonTransactional);
		}

		RenderingComp = ConstructRenderingComponent();
		if (RenderingComp != NULL)
		{
			RenderingComp->RegisterComponent();
		}
	}
#endif // !UE_BUILD_SHIPPING
}

void ANavigationData::DestroyGenerator()
{
#if WITH_NAVIGATION_GENERATOR
	if (NavDataGenerator.IsValid() == true)
	{
		NavDataGenerator->OnNavigationDataDestroyed(this);
		NavDataGenerator.Reset();
	}
#endif // WITH_NAVIGATION_GENERATOR
};

void ANavigationData::PurgeUnusedPaths()
{
	check(IsInGameThread());

	const int32 Count = ActivePaths.Num();
	FNavPathWeakPtr* WeakPathPtr = (ActivePaths.GetTypedData() + Count - 1);
	for (int32 i = Count - 1; i >= 0; --i, --WeakPathPtr)
	{
		if (WeakPathPtr->IsValid() == false)
		{
			ActivePaths.RemoveAtSwap(i, 1, /*bAllowShrinking=*/false);
		}
	}
}

#if WITH_EDITOR
void ANavigationData::PostEditUndo()
{
	Super::PostEditUndo();
#if WITH_NAVIGATION_GENERATOR
	GetGenerator(NavigationSystem::Create);
#endif // WITH_NAVIGATION_GENERATOR

	UWorld* WorldOuter = GetWorld();
	if (WorldOuter != NULL && WorldOuter->GetNavigationSystem() != NULL)
	{
		WorldOuter->GetNavigationSystem()->RequestRegistration(this);
	}
}
#endif // WITH_EDITOR

void ANavigationData::Destroyed()
{
	UWorld* WorldOuter = GetWorld();

	bRegistered = false;
	if (WorldOuter != NULL && WorldOuter->GetNavigationSystem() != NULL)
	{
		WorldOuter->GetNavigationSystem()->UnregisterNavData(this);
	}
	
	CleanUp();

	Super::Destroyed();
}

void ANavigationData::CleanUp()
{
	bRegistered = false;
	DestroyGenerator();
}

void ANavigationData::CleanUpAndMarkPendingKill()
{
	CleanUp();
	SetActorHiddenInGame(true);

	// do NOT destroy here! it can be called from PostLoad and will crash in DestroyActor()
	MarkPendingKill();
	MarkComponentsAsPendingKill();
}

#if WITH_NAVIGATION_GENERATOR

FNavDataGenerator* ANavigationData::GetGenerator(NavigationSystem::ECreateIfEmpty CreateIfNone)
{
	if ((NavDataGenerator.IsValid() == false || NavDataGenerator.GetSharedReferenceCount() <= 0)
		&& CreateIfNone == NavigationSystem::Create
		&& (bRebuildAtRuntime == true || GetWorld()->IsGameWorld() == false))
	{
		// @todo this is not-that-clear design and is subject to change
		if (NavDataConfig.IsValid() == false 
			&& GetWorld()->GetNavigationSystem() != NULL
			&& GetWorld()->GetNavigationSystem()->SupportedAgents.Num() < 2
			)
		{
			// fill in AgentProps with whatever is the instance's setup
			FillConfig(NavDataConfig);
		}

		FNavDataGenerator* Generator = ConstructGenerator(NavDataConfig);

		if (Generator != NULL)
		{
			// make sure no one has made this sharable yet
			// it's a "check" not an "ensure" since it's really crucial
			check(Generator->HasBeenAlreadyMadeSharable() == false);
			NavDataGenerator = MakeShareable(Generator);
		}
	}

	return NavDataGenerator.IsValid() && NavDataGenerator.GetSharedReferenceCount() > 0 ? NavDataGenerator.Get() : NULL;
}
#endif // WITH_NAVIGATION_GENERATOR

void ANavigationData::DrawDebugPath(FNavigationPath* Path, FColor PathColor, UCanvas* Canvas, bool bPersistent, const uint32 NextPathPointIndex) const
{
	Path->DebugDraw(this, PathColor, Canvas, bPersistent, NextPathPointIndex);
}

void ANavigationData::OnNavAreaAdded(const UClass* NavAreaClass, int32 AgentIndex)
{
	// check if area can be added
	const UNavArea* DefArea = NavAreaClass ? ((UClass*)NavAreaClass)->GetDefaultObject<UNavArea>() : NULL;
	const bool bIsMetaArea = NavAreaClass && NavAreaClass->IsChildOf(UNavAreaMeta::StaticClass());
	if (!DefArea || bIsMetaArea || !DefArea->IsSupportingAgent(AgentIndex))
	{
		UE_LOG(LogNavigation, Verbose, TEXT("%s discarded area %s (valid:%s meta:%s validAgent[%d]:%s)"),
			*GetName(), *GetNameSafe(NavAreaClass),
			DefArea ? TEXT("yes") : TEXT("NO"),
			bIsMetaArea ? TEXT("YES") : TEXT("no"),
			AgentIndex, DefArea->IsSupportingAgent(AgentIndex) ? TEXT("yes") : TEXT("NO"));
		return;
	}

	// check if area is already on supported list
	FString AreaClassName = NavAreaClass->GetName();
	for (int32 i = 0; i < SupportedAreas.Num(); i++)
	{
		if (SupportedAreas[i].AreaClassName == AreaClassName)
		{
			SupportedAreas[i].AreaClass = NavAreaClass;
			UE_LOG(LogNavigation, Verbose, TEXT("%s updated area %s with ID %d"), *GetName(), *AreaClassName, SupportedAreas[i].AreaID);
			return;
		}
	}

	// try adding new one
	const int32 MaxSupported = GetMaxSupportedAreas();
	if (SupportedAreas.Num() >= MaxSupported)
	{
		UE_LOG(LogNavigation, Error, TEXT("%s can't support area %s - limit reached! (%d)"), *GetName(), *AreaClassName, MaxSupported);
		return;
	}

	FSupportedAreaData NewAgentData;
	NewAgentData.AreaClass = NavAreaClass;
	NewAgentData.AreaClassName = AreaClassName;
	NewAgentData.AreaID = GetNewAreaID(NavAreaClass);
	SupportedAreas.Add(NewAgentData);
	
	UE_LOG(LogNavigation, Verbose, TEXT("%s registered area %s with ID %d"), *GetName(), *AreaClassName, NewAgentData.AreaID);
}

void ANavigationData::OnNavAreaAddedNotify(const UClass* NavAreaClass)
{
	const UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
	const int32 AgentIndex = NavSys->GetSupportedAgentIndex(this);

	OnNavAreaAdded(NavAreaClass, AgentIndex);
}

void ANavigationData::OnNavAreaRemoved(const UClass* NavAreaClass)
{
	for (int32 i = 0; i < SupportedAreas.Num(); i++)
	{
		if (SupportedAreas[i].AreaClass == NavAreaClass)
		{
			SupportedAreas.RemoveAt(i);
			break;
		}
	}
}

void ANavigationData::OnNavAreaRemovedNotify(const UClass* NavAreaClass)
{
	OnNavAreaRemoved(NavAreaClass);
}

void ANavigationData::ProcessNavAreas(const TArray<const UClass*>& AreaClasses, int32 AgentIndex)
{
	for (int32 i = 0; i < AreaClasses.Num(); i++)
	{
		OnNavAreaAdded(AreaClasses[i], AgentIndex);
	}
}

int32 ANavigationData::GetNewAreaID(const UClass* AreaClass) const
{
	int TestId = 0;
	while (TestId < SupportedAreas.Num())
	{
		const bool bIsTaken = IsAreaAssigned(TestId);
		if (bIsTaken)
		{
			TestId++;
		}
		else
		{
			break;
		}
	}

	return TestId;
}

const UClass* ANavigationData::GetAreaClass(int32 AreaID) const
{
	for (int32 i = 0; i < SupportedAreas.Num(); i++)
	{
		if (SupportedAreas[i].AreaID == AreaID)
		{
			return SupportedAreas[i].AreaClass;
		}
	}

	return NULL;
}

bool ANavigationData::IsAreaAssigned(int32 AreaID) const
{
	for (int32 i = 0; i < SupportedAreas.Num(); i++)
	{
		if (SupportedAreas[i].AreaID == AreaID)
		{
			return true;
		}
	}

	return false;
}

int32 ANavigationData::GetAreaID(const UClass* AreaClass) const
{
	for (int32 i = 0; i < SupportedAreas.Num(); i++)
	{
		if (SupportedAreas[i].AreaClass == AreaClass)
		{
			return SupportedAreas[i].AreaID;
		}
	}

	return -1;
}

void ANavigationData::UpdateSmartLink(class USmartNavLinkComponent* LinkComp)
{
	// no implementation for abstract class
}

uint32 ANavigationData::LogMemUsed() const
{
	const uint32 MemUsed = ActivePaths.GetAllocatedSize() + SupportedAreas.GetAllocatedSize();

	UE_LOG(LogNavigation, Display, TEXT("%s: ANavigationData: %u\n    self: %d"), *GetName(), MemUsed, sizeof(ANavigationData));	

#if WITH_NAVIGATION_GENERATOR
	if (NavDataGenerator.IsValid())
	{
		NavDataGenerator->LogMemUsed();
	}
#endif // WITH_NAVIGATION_GENERATOR

	return MemUsed;
}
