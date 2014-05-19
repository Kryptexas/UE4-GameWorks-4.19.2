// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "AI/BehaviorTreeDelegates.h"
#include "../AI/Navigation/RecastNavMeshGenerator.h"
#include "../NavMeshRenderingHelpers.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

DEFINE_LOG_CATEGORY(LogGDT);

//----------------------------------------------------------------------//
// Composite Scene proxy
//----------------------------------------------------------------------//
class FDebugRenderSceneCompositeProxy : public FDebugRenderSceneProxy
{
public:
	FDebugRenderSceneCompositeProxy(const UPrimitiveComponent* InComponent) 
		: FDebugRenderSceneProxy(InComponent)
	{
	}

	virtual ~FDebugRenderSceneCompositeProxy()
	{
		for (int32 Index = 0; Index < ChildProxies.Num(); ++Index)
		{
			delete ChildProxies[Index];
		}
	}

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) OVERRIDE
	{
		for (int32 Index = 0; Index < ChildProxies.Num(); ++Index)
		{
			ChildProxies[Index]->DrawStaticElements(PDI);
		}
	}

	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) OVERRIDE
	{
		for (int32 Index = 0; Index < ChildProxies.Num(); ++Index)
		{
			ChildProxies[Index]->DrawDynamicElements(PDI, View);
		}
	}

	virtual void RegisterDebugDrawDelgate() OVERRIDE
	{
		for (int32 Index = 0; Index < ChildProxies.Num(); ++Index)
		{
			ChildProxies[Index]->RegisterDebugDrawDelgate();
		}
	}
	
	virtual void UnregisterDebugDrawDelgate() OVERRIDE
	{
		for (int32 Index = 0; Index < ChildProxies.Num(); ++Index)
		{
			ChildProxies[Index]->UnregisterDebugDrawDelgate();
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) OVERRIDE
	{
		FPrimitiveViewRelevance Result;
		for (int32 Index = 0; Index < ChildProxies.Num(); ++Index)
		{
			Result |= ChildProxies[Index]->GetViewRelevance(View);
		}
		return Result;
	}

	virtual uint32 GetMemoryFootprint( void ) const
	{
		return sizeof( *this ) + GetAllocatedSize(); 
	}

	uint32 GetAllocatedSize( void ) const
	{
		uint32 Size = 0;
		for (int32 Index = 0; Index < ChildProxies.Num(); ++Index)
		{
			ChildProxies[Index]->GetMemoryFootprint();
		}

		return Size + ChildProxies.GetAllocatedSize();
	}

	void AddChild(FDebugRenderSceneProxy* NewChild)
	{
		ChildProxies.AddUnique(NewChild);
	}
	
protected:
	TArray<FDebugRenderSceneProxy*> ChildProxies;
};

//----------------------------------------------------------------------//
// UGameplayDebuggingComponent
//----------------------------------------------------------------------//

FName UGameplayDebuggingComponent::DefaultComponentName = TEXT("GameplayDebuggingComponent");
FOnDebuggingTargetChanged UGameplayDebuggingComponent::OnDebuggingTargetChangedDelegate;
const uint32 UGameplayDebuggingComponent::ShowFlagIndex = uint32(FEngineShowFlags::FindIndexByName(TEXT("GameplayDebug")));

UGameplayDebuggingComponent::UGameplayDebuggingComponent(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, EQSTimestamp(0.f)
	, bDrawEQSLabels(true)
	, bDrawEQSFailedItems(true)
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	bAutoActivate = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bIsSelectedForDebugging = false;
	ActivationCounter = 0;
	ShowExtendedInformatiomCounter = 0;
#if WITH_EDITOR
	bWasSelectedInEditor = false;
#endif

	bHiddenInGame = false;
	bEnabledTargetSelection = false;

	bEnableClientEQSSceneProxy = false;
	NextTargrtSelectionTime = 0;
	ReplicateViewDataCounters.Init(0, EAIDebugDrawDataView::MAX);
}

void UGameplayDebuggingComponent::Activate(bool bReset)
{
	ActivationCounter++;
	if (ActivationCounter > 0)
	{
		if (IsActive() == false)
		{
			Super::Activate(bReset);
		}
		SetComponentTickEnabled(true);
	}
}

void UGameplayDebuggingComponent::Deactivate()
{
	ActivationCounter--;
	if (ActivationCounter <= 0)
	{
		ActivationCounter = 0;
		if (IsActive())
		{
			Super::Deactivate();
		}
		SetComponentTickEnabled(false);
	}
}

void UGameplayDebuggingComponent::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UGameplayDebuggingComponent, ActivationCounter );
	DOREPLIFETIME( UGameplayDebuggingComponent, ReplicateViewDataCounters );
	DOREPLIFETIME( UGameplayDebuggingComponent, ShowExtendedInformatiomCounter );
	DOREPLIFETIME( UGameplayDebuggingComponent, ControllerName )
	DOREPLIFETIME( UGameplayDebuggingComponent, PawnName );
	DOREPLIFETIME( UGameplayDebuggingComponent, DebugIcon );
	DOREPLIFETIME( UGameplayDebuggingComponent, BrainComponentName );
	DOREPLIFETIME( UGameplayDebuggingComponent, PawnClass );

	DOREPLIFETIME( UGameplayDebuggingComponent, PathPoints );
	DOREPLIFETIME( UGameplayDebuggingComponent, PathErrorString );

	DOREPLIFETIME( UGameplayDebuggingComponent, NavmeshRepData );

	DOREPLIFETIME( UGameplayDebuggingComponent, TargetActor );

	DOREPLIFETIME(UGameplayDebuggingComponent, EQSTimestamp);
	DOREPLIFETIME(UGameplayDebuggingComponent, EQSName);
	DOREPLIFETIME(UGameplayDebuggingComponent, EQSId);
	DOREPLIFETIME(UGameplayDebuggingComponent, EQSRepData);
}

bool UGameplayDebuggingComponent::ServerEnableTargetSelection_Validate(bool bEnable)
{
	return true;
}

void UGameplayDebuggingComponent::ServerEnableTargetSelection_Implementation(bool bEnable)
{
	bEnabledTargetSelection = bEnable;
	if (bEnabledTargetSelection && World && World->GetNetMode() < NM_Client)
	{
		NextTargrtSelectionTime = 0;
		SelectTargetToDebug();
	}
}

void UGameplayDebuggingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (World && World->GetNetMode() < NM_Client)
	{
		if (bEnabledTargetSelection)
		{
			SelectTargetToDebug();
		}
		CollectDataToReplicate();
	}
}

void UGameplayDebuggingComponent::SelectTargetToDebug()
{
	const APawn* MyPawn = Cast<APawn>(GetOwner());
	APlayerController* MyPC = Cast<APlayerController>(MyPawn->Controller);

	if (MyPC )
	{
		APawn* BestTarget = NULL;
		if (MyPC->GetViewTarget() != NULL && MyPC->GetViewTarget() != MyPC->GetPawn())
		{
			BestTarget = Cast<APawn>(MyPC->GetViewTarget());
			if ((BestTarget && BestTarget->PlayerState && !BestTarget->PlayerState->bIsABot) || BestTarget->GetActorEnableCollision() == false)
			{
				BestTarget = NULL;
			}
			else if (BestTarget)
			{
				// always update component for view target
				if (UGameplayDebuggingComponent *DebuggingComponent = BestTarget->GetDebugComponent(true))
				{
					DebuggingComponent->ServerReplicateData_Implementation(EDebugComponentMessage::ActivateReplication, EAIDebugDrawDataView::Empty);
				}
			}
		}

		float bestAim = 0.f;
		FVector CamLocation;
		FRotator CamRotation;
		check(MyPC->PlayerCameraManager != NULL);
		MyPC->PlayerCameraManager->GetCameraViewPoint(CamLocation, CamRotation);
		FVector FireDir = CamRotation.Vector();
		UWorld* World = MyPC->GetWorld();
		check( World );

		APawn* PossibleTarget = NULL;
		bool bUpdateComponentsToReplicate = GetWorld() && GetWorld()->TimeSeconds > NextTargrtSelectionTime;
		if (bUpdateComponentsToReplicate)
		{
			NextTargrtSelectionTime = GetWorld()->TimeSeconds + 1;
		}
		for (FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator )
		{
			APawn* NewTarget = *Iterator;
			//  NewTarget->GetActorEnableCollision() == false - HACK to get rid of pawns not relevant for us 
			if (!NewTarget || NewTarget == MyPC->GetPawn() || (NewTarget->PlayerState && !NewTarget->PlayerState->bIsABot) || NewTarget->GetActorEnableCollision() == false)
			{
				continue;
			}
			
			UGameplayDebuggingComponent *DebuggingComponent = NewTarget->GetDebugComponent(true);
			if (DebuggingComponent && bUpdateComponentsToReplicate)
			{
				DebuggingComponent->ServerReplicateData_Implementation(EDebugComponentMessage::ActivateReplication, EAIDebugDrawDataView::Empty);
			}

			if (BestTarget == NULL && NewTarget && (NewTarget != MyPC->GetPawn()))
			{
				// look for best controlled pawn target
				const FVector AimDir = NewTarget->GetActorLocation() - CamLocation;
				float FireDist = AimDir.SizeSquared();
				// only find targets which are < 25000 units away
				if (FireDist < 625000000.f)
				{
					FireDist = FMath::Sqrt(FireDist);
					float newAim = FireDir | AimDir;
					newAim = newAim/FireDist;
					if (newAim > bestAim)
					{
						PossibleTarget = NewTarget;
						bestAim = newAim;
					}
				}
			}
		}

		BestTarget = BestTarget == NULL ? PossibleTarget : BestTarget;
		if (BestTarget != NULL)
		{
			//always update component for best target
			if (UGameplayDebuggingComponent *DebuggingComponent = BestTarget->GetDebugComponent(true))
			{
				if (TargetActor != NULL)
				{
					if (UGameplayDebuggingComponent *OldDebuggingComponent = TargetActor->GetDebugComponent())
					{
						OldDebuggingComponent->SelectForDebugging(false);
					}
				}
				TargetActor = BestTarget;
				DebuggingComponent->SelectForDebugging(true);
				DebuggingComponent->ServerReplicateData_Implementation(EDebugComponentMessage::ActivateReplication, EAIDebugDrawDataView::Empty);
			}
		}
	}
}

void UGameplayDebuggingComponent::CollectDataToReplicate()
{
	if (ShouldReplicateData(EAIDebugDrawDataView::Basic) || ShouldReplicateData(EAIDebugDrawDataView::OverHead))
	{
		CollectBasicData();
	}

	if (bIsSelectedForDebugging || ShouldReplicateData(EAIDebugDrawDataView::EditorDebugAIFlag))
	{
		CollectPathData();
	}

	if (ShowExtendedInformatiomCounter > 0)
	{
		if (ShouldReplicateData(EAIDebugDrawDataView::BehaviorTree))
		{
			CollectBehaviorTreeData();
		}
		
		if (ShouldReplicateData(EAIDebugDrawDataView::EQS))
		{
			CollectEQSData();
		}
	}
}

void UGameplayDebuggingComponent::CollectBasicData()
{
	const APawn* MyPawn = Cast<APawn>(GetOwner());
	PawnName = MyPawn->GetHumanReadableName();
	PawnClass = MyPawn->GetClass()->GetName();
	const AAIController* MyController = Cast<const AAIController>(MyPawn->Controller);

	if (MyController != NULL)
	{
		if (MyController->IsPendingKill() == false)
		{
			ControllerName = MyController->GetName();
			DebugIcon = MyController->GetDebugIcon();
		}
		else
		{
			ControllerName = TEXT("Controller PENDING KILL");
		}
	}
	else
	{
		ControllerName = TEXT("No Controller");
	}
}

void UGameplayDebuggingComponent::CollectBehaviorTreeData()
{
	if (!ShouldReplicateData(EAIDebugDrawDataView::BehaviorTree))
	{
		return;
	}

	APawn* MyPawn = Cast<APawn>(GetOwner());
	if (AAIController *MyController = Cast<AAIController>(MyPawn->Controller))
	{
		BrainComponentName = MyController->BrainComponent != NULL ? MyController->BrainComponent->GetName() : TEXT(""); 
	}
}

void UGameplayDebuggingComponent::CollectPathData()
{
	APawn* MyPawn = Cast<APawn>(GetOwner());
	PathErrorString.Empty();
	if (AAIController *MyController = Cast<AAIController>(MyPawn->Controller))
	{
		if ( MyController->NavComponent && MyController->NavComponent->HasValidPath())
		{
			const FNavPathSharedPtr& NewPath = MyController->NavComponent->GetPath();
			if (!CurrentPath.HasSameObject(NewPath.Get()))
			{
				PathPoints.Reset();
				for (int32 Index=0; Index < NewPath->PathPoints.Num(); ++Index)
				{
					PathPoints.Add( NewPath->PathPoints[Index].Location );
				}
				CurrentPath = NewPath;
			}
		}
		else
		{
			CurrentPath = NULL;
			PathPoints.Reset();
		}
	}
}

void UGameplayDebuggingComponent::OnDebugAI(class UCanvas* Canvas, class APlayerController* PC)
{
#if WITH_EDITOR
	if (Canvas->SceneView->Family->EngineShowFlags.DebugAI && !IsPendingKill() && GetOwner() && !GetOwner()->IsPendingKill())
	{
		if (!IsComponentTickEnabled() && GetOwnerRole() == ROLE_Authority)
		{
			// collecting data manually here - it was activated by DebugAI flag and not by regular usage
			CollectDataToReplicate();
		}

		APawn* MyPawn = Cast<APawn>(GetOwner());
		if (MyPawn && MyPawn->IsSelected())
		{
			if (!bWasSelectedInEditor)
			{
				ServerReplicateData(EDebugComponentMessage::EnableExtendedView, EAIDebugDrawDataView::Basic);
				bWasSelectedInEditor = true;
			}
			ServerReplicateData(EDebugComponentMessage::ActivateDataView, EAIDebugDrawDataView::EditorDebugAIFlag);
			ServerReplicateData(EDebugComponentMessage::ActivateDataView, EAIDebugDrawDataView::Basic);
			SelectForDebugging(true);
		}
		else if (!MyPawn || MyPawn && !MyPawn->IsSelected())
		{
			if (bWasSelectedInEditor)
			{
				ServerReplicateData(EDebugComponentMessage::DisableExtendedView, EAIDebugDrawDataView::Basic);
				bWasSelectedInEditor = false;
			}
			ServerReplicateData(EDebugComponentMessage::ActivateDataView, EAIDebugDrawDataView::EditorDebugAIFlag);
			ServerReplicateData(EDebugComponentMessage::ActivateDataView, EAIDebugDrawDataView::Basic);
			SelectForDebugging(false);
		}
	}
#endif
}

void UGameplayDebuggingComponent::SelectForDebugging(bool bNewStatus)
{
	if (bIsSelectedForDebugging != bNewStatus)
	{
		bIsSelectedForDebugging = bNewStatus;
		
		OnDebuggingTargetChangedDelegate.Broadcast(GetOwner(), bNewStatus);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#if 0
		// temp: avoidance debug
		UAvoidanceManager* Avoidance = GetWorld()->GetAvoidanceManager();
		AController* ControllerOwner = Cast<AController>(GetOwner());
		APawn* PawnOwner = ControllerOwner ? ControllerOwner->GetPawn() : Cast<APawn>(GetOwner());
		UCharacterMovementComponent* MovementComp = PawnOwner ? PawnOwner->FindComponentByClass<UCharacterMovementComponent>() : NULL;
		if (MovementComp && Avoidance)
		{
			Avoidance->AvoidanceDebugForUID(MovementComp->AvoidanceUID, bNewStatus);
		}
#endif

		if (bNewStatus == false)
		{
			CachedQueryInstance.Reset();
			MarkRenderStateDirty();
		}
#endif
	}
}

void UGameplayDebuggingComponent::EnableDebugDraw(bool bEnable, bool InFocusedComponent)
{
	if (bEnable)
	{
		SelectForDebugging(InFocusedComponent);
	}
	else
	{
		SelectForDebugging(false);
		EnableClientEQSSceneProxy(false);
	}
}

bool UGameplayDebuggingComponent::ServerReplicateData_Validate( EDebugComponentMessage::Type InMessage, EAIDebugDrawDataView::Type DataView )
{
	switch ( InMessage )
	{
		case EDebugComponentMessage::EnableExtendedView:
		case EDebugComponentMessage::DisableExtendedView:
		case EDebugComponentMessage::ActivateReplication:
		case EDebugComponentMessage::DeactivateReplilcation:
		case EDebugComponentMessage::ActivateDataView:
		case EDebugComponentMessage::DeactivateDataView:
			break;

		default:
			return false;
	}

	return true;
}

void UGameplayDebuggingComponent::ServerReplicateData_Implementation( EDebugComponentMessage::Type InMessage, EAIDebugDrawDataView::Type DataView )
{
	switch (InMessage)
	{
	case EDebugComponentMessage::EnableExtendedView:
		ShowExtendedInformatiomCounter++;
		break;

	case EDebugComponentMessage::DisableExtendedView:
		ShowExtendedInformatiomCounter = FMath::Max(0, ShowExtendedInformatiomCounter - 1);
		break;

	case EDebugComponentMessage::ActivateReplication:
		Activate();
		break;

	case EDebugComponentMessage::DeactivateReplilcation:
		{
			Deactivate();
			APawn* MyPawn = Cast<APawn>(GetOwner());
			if (MyPawn != NULL && IsActive() == false)
			{
				//MyPawn->RemoveDebugComponent();
			}
		}
		break;

	case EDebugComponentMessage::ActivateDataView:
		if (ReplicateViewDataCounters.IsValidIndex(DataView))
		{
			ReplicateViewDataCounters[DataView] += 1;
		}
		break;

	case EDebugComponentMessage::DeactivateDataView:
		if (ReplicateViewDataCounters.IsValidIndex(DataView))
		{
			ReplicateViewDataCounters[DataView] = FMath::Max(0, ReplicateViewDataCounters[DataView] - 1);
		}
		break;

	default:
		break;
	}
}

void UGameplayDebuggingComponent::GetKeyboardDesc(TArray<FDebugCategoryView>& Categories)
{
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::NavMesh, "NavMesh"));			// Num0
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::Basic, "Basic"));				// Num1
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::BehaviorTree, "Behavior"));		// Num2
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::EQS, "EQS"));					// Num3
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::Perception, "Perception"));		// Num4
}

//////////////////////////////////////////////////////////////////////////
// EQS Data
//////////////////////////////////////////////////////////////////////////
const FEnvQueryResult* UGameplayDebuggingComponent::GetQueryResult() const
{
	return CachedQueryInstance.Get();
}

const FEnvQueryInstance* UGameplayDebuggingComponent::GetQueryInstance() const
{
	return CachedQueryInstance.Get();
}

FArchive& operator<<(FArchive& Ar, FDebugRenderSceneProxy::FSphere& Data)
{
	Ar << Data.Radius;
	Ar << Data.Location;
	Ar << Data.Color;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FDebugRenderSceneProxy::FText3d& Data)
{
	Ar << Data.Text;
	Ar << Data.Location;
	Ar << Data.Color;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, EQSDebug::FItemData& Data)
{
	Ar << Data.Desc;
	Ar << Data.ItemIdx;
	Ar << Data.TotalScore;
	Ar << Data.TestValues;
	Ar << Data.TestScores;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, EQSDebug::FTestData& Data)
{
	Ar << Data.ShortName;
	Ar << Data.Detailed;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, EQSDebug::FQueryData& Data)
{
	Ar << Data.Items;
	Ar << Data.Tests;
	Ar << Data.SolidSpheres;
	Ar << Data.Texts;
	Ar << Data.NumValidItems;
	return Ar;
}

void UGameplayDebuggingComponent::OnRep_UpdateEQS()
{
	// decode scoring data
	if (World && World->GetNetMode() == NM_Client)
	{
		TArray<uint8> UncompressedBuffer;
		int32 UncompressedSize = 0;
		const int32 HeaderSize = sizeof(int32);
		uint8* SrcBuffer = (uint8*)EQSRepData.GetTypedData();
		FMemory::Memcpy(&UncompressedSize, SrcBuffer, HeaderSize);
		SrcBuffer += HeaderSize;
		const int32 CompressedSize = EQSRepData.Num() - HeaderSize;

		UncompressedBuffer.AddZeroed(UncompressedSize);

		FCompression::UncompressMemory((ECompressionFlags)(COMPRESS_ZLIB), (void*)UncompressedBuffer.GetTypedData(), UncompressedSize, SrcBuffer, CompressedSize);
		FMemoryReader ArReader(UncompressedBuffer);

		ArReader << EQSLocalData;
	}	

	UpdateBounds();
	MarkRenderStateDirty();
}

void UGameplayDebuggingComponent::CollectEQSData()
{
#if USE_EQS_DEBUGGER
	if (!ShouldReplicateData(EAIDebugDrawDataView::EQS))
	{
		return;
	}

	UWorld* World = GetWorld();
	UEnvQueryManager* QueryManager = World ? UEnvQueryManager::GetCurrent(World) : NULL;
	const AActor* Owner = GetOwner();

	if (QueryManager == NULL || Owner == NULL)
	{
		return;
	}

	TSharedPtr<FEnvQueryInstance> CurrentInstance = QueryManager->GetDebugger().GetQueryForOwner(Owner);
	if (!CurrentInstance.IsValid() || (CachedQueryInstance.IsValid() && CachedQueryInstance->QueryID == CurrentInstance->QueryID))
	{
		return;
	}

	const double Timer1 = FPlatformTime::Seconds();

	CachedQueryInstance = CurrentInstance;
	EQSTimestamp = World->GetTimeSeconds();
	EQSName = CachedQueryInstance->QueryName;
	EQSId = CachedQueryInstance->QueryID;

	TArray<uint8> UncompressedBuffer;
	FMemoryWriter ArWriter(UncompressedBuffer);

	// step 1: data for rendering component
	EQSLocalData.SolidSpheres.Reset();
	EQSLocalData.Texts.Reset();
	FEQSSceneProxy::CollectEQSData(this, NULL, EQSLocalData.SolidSpheres, EQSLocalData.Texts);

	// step 2: detailed scoring data for HUD
	const int32 MaxDetailedItems = 10;
	const int32 FirstItemIndex = 0;

	const int32 NumTests = CachedQueryInstance->ItemDetails.IsValidIndex(0) ? CachedQueryInstance->ItemDetails[0].TestResults.Num() : 0;
	const int32 NumItems = FMath::Min(MaxDetailedItems, CachedQueryInstance->NumValidItems);

	EQSLocalData.Items.Reset(NumItems);
	EQSLocalData.Tests.Reset(NumTests);
	EQSLocalData.NumValidItems = CachedQueryInstance->NumValidItems;

	UEnvQueryItemType* ItemCDO = CachedQueryInstance->ItemType.GetDefaultObject();
	for (int32 ItemIdx = 0; ItemIdx < NumItems; ItemIdx++)
	{
		EQSDebug::FItemData ItemInfo;
		ItemInfo.ItemIdx = ItemIdx + FirstItemIndex;
		ItemInfo.TotalScore = CachedQueryInstance->Items[ItemInfo.ItemIdx].Score;

		const uint8* ItemData = CachedQueryInstance->RawData.GetTypedData() + CachedQueryInstance->Items[ItemInfo.ItemIdx].DataOffset;
		ItemInfo.Desc = FString::Printf(TEXT("[%d] %s"), ItemInfo.ItemIdx, *ItemCDO->GetDescription(ItemData));

		ItemInfo.TestValues.Reserve(NumTests);
		ItemInfo.TestScores.Reserve(NumTests);
		for (int32 TestIdx = 0; TestIdx < NumTests; TestIdx++)
		{
			const float ScoreN = CachedQueryInstance->ItemDetails[ItemInfo.ItemIdx].TestResults[TestIdx];
			const float ScoreW = CachedQueryInstance->ItemDetails[ItemInfo.ItemIdx].TestWeightedScores[TestIdx];

			ItemInfo.TestValues.Add(ScoreN);
			ItemInfo.TestScores.Add(ScoreW);
		}

		EQSLocalData.Items.Add(ItemInfo);
	}

	{
		for (int32 TestIdx = 0; TestIdx < NumTests; TestIdx++)
		{
			EQSDebug::FTestData TestInfo;

			UEnvQueryTest* TestOb = Cast<UEnvQueryTest>(CachedQueryInstance->Options[CachedQueryInstance->OptionIndex].TestDelegates[TestIdx].GetUObject());
			TestInfo.ShortName = TestOb->GetDescriptionTitle();
			TestInfo.Detailed = TestOb->GetDescriptionDetails().ToString().Replace(TEXT("\n"), TEXT(", "));
			
			EQSLocalData.Tests.Add(TestInfo);
		}
	}
	ArWriter << EQSLocalData;

	const double Timer2 = FPlatformTime::Seconds();

	const int32 UncompressedSize = UncompressedBuffer.Num();
	const int32 HeaderSize = sizeof(int32);
	EQSRepData.Init(0, HeaderSize + FMath::TruncToInt(1.1f * UncompressedSize));

	int32 CompressedSize = EQSRepData.Num() - HeaderSize;
	uint8* DestBuffer = EQSRepData.GetTypedData();
	FMemory::Memcpy(DestBuffer, &UncompressedSize, HeaderSize);
	DestBuffer += HeaderSize;

	FCompression::CompressMemory((ECompressionFlags)(COMPRESS_ZLIB | COMPRESS_BiasMemory), (void*)DestBuffer, CompressedSize, (void*)UncompressedBuffer.GetData(), UncompressedSize);

	EQSRepData.SetNum(CompressedSize + HeaderSize, false);
	const double Timer3 = FPlatformTime::Seconds();

	UE_LOG(LogEQS, Log, TEXT("Preparing EQS data: %.1fkB took %.3fms (collect: %.3fms + compress %d%%: %.3fms)"),
		EQSRepData.Num() / 1024.0f, 1000.0f * (Timer3 - Timer1),
		1000.0f * (Timer2 - Timer1),
		FMath::TruncToInt(100.0f * EQSRepData.Num() / UncompressedBuffer.Num()), 1000.0f * (Timer3 - Timer2));

	if (World && World->GetNetMode() != NM_DedicatedServer)
	{
		OnRep_UpdateEQS();
	}
#endif
}

//----------------------------------------------------------------------//
// NavMesh rendering
//----------------------------------------------------------------------//
void UGameplayDebuggingComponent::OnRep_UpdateNavmesh()
{
	NavMeshBounds = FBox(FVector(-HALF_WORLD_MAX, -HALF_WORLD_MAX, -HALF_WORLD_MAX), FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX));
	UpdateBounds();
	MarkRenderStateDirty();
}

bool UGameplayDebuggingComponent::ServerCollectNavmeshData_Validate(FVector_NetQuantize10 TargetLocation)
{
	bool bIsValid = false;
#if WITH_RECAST
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	bIsValid = NavSys && NavSys->GetMainNavData(NavigationSystem::DontCreate);
#endif

	return bIsValid;
}

bool UGameplayDebuggingComponent::ServerDiscardNavmeshData_Validate()
{
	return true;
}

namespace NavMeshDebug
{
	struct FShortVector
	{
		int16 X;
		int16 Y;
		int16 Z;

		FShortVector()
			:X(0), Y(0), Z(0)
		{

		}

		FShortVector(const FVector& Vec)
			: X(Vec.X)
			, Y(Vec.Y)
			, Z(Vec.Z)
		{

		}
		FShortVector& operator=(const FVector& R)
		{
			X = R.X;
			Y = R.Y;
			Z = R.Z;
			return *this;
		}

		FVector ToVector() const
		{
			return FVector(X, Y, Z);
		}
	};

	struct FOffMeshLinkFlags
	{
		uint8 Radius : 6;
		uint8 Direction : 1;
		uint8 ValidEnds : 1;
	};
	struct FOffMeshLink
	{
		FShortVector Left;
		FShortVector Right;
		FColor Color;
		union
		{
			FOffMeshLinkFlags PackedFlags;
			uint8 ByteFlags;
		};
	};

	struct FAreaPolys
	{
		TArray<int16> Indices;
		FColor Color;
	};

	struct FTileData
	{
		FVector Location;
		TArray<FAreaPolys> Areas;
		TArray<FShortVector> Verts;
		TArray<FOffMeshLink> Links;
	};
}

FArchive& operator<<(FArchive& Ar, NavMeshDebug::FShortVector& ShortVector)
{
	Ar << ShortVector.X;
	Ar << ShortVector.Y;
	Ar << ShortVector.Z;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, NavMeshDebug::FOffMeshLink& Data)
{
	Ar << Data.Left;
	Ar << Data.Right;
	Ar << Data.Color.R;
	Ar << Data.Color.G;
	Ar << Data.Color.B;
	Ar << Data.ByteFlags;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, NavMeshDebug::FAreaPolys& Data)
{
	Ar << Data.Indices;
	Ar << Data.Color.R;
	Ar << Data.Color.G;
	Ar << Data.Color.B;
	return Ar;
}


FArchive& operator<<(FArchive& Ar, NavMeshDebug::FTileData& Data)
{
	Ar << Data.Areas;
	Ar << Data.Location;
	Ar << Data.Verts;
	Ar << Data.Links;
	return Ar;
}

void UGameplayDebuggingComponent::ServerDiscardNavmeshData_Implementation()
{
	NavmeshRepData.Empty();
}

FORCEINLINE bool LineInCorrectDistance(const FVector& PlayerLoc, const FVector& Start, const FVector& End, float CorrectDistance = -1)
{
	const float MaxDistance = CorrectDistance > 0 ? (CorrectDistance*CorrectDistance) : ARecastNavMesh::GetDrawDistanceSq();

	if ((FVector::DistSquared(Start, PlayerLoc) > MaxDistance || FVector::DistSquared(End, PlayerLoc) > MaxDistance))
	{
		return false;
	}
	return true;
}

void UGameplayDebuggingComponent::ServerCollectNavmeshData_Implementation(FVector_NetQuantize10 TargetLocation)
{
#if WITH_RECAST
	UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
	ARecastNavMesh* NavData = NavSys ? Cast<ARecastNavMesh>(NavSys->GetMainNavData(NavigationSystem::DontCreate)) : NULL;
	const APawn* MyPawn = Cast<APawn>(GetOwner());	
	if (NavData == NULL || MyPawn == NULL)
	{
		NavmeshRepData.Empty();
		return;
	}

	const double Timer1 = FPlatformTime::Seconds();

	TArray<int32> Indices;
	int32 TileX = 0;
	int32 TileY = 0;
	const int32 DeltaX[] = { 0, 1, 1, 0, -1, -1, -1,  0,  1 };
	const int32 DeltaY[] = { 0, 0, 1, 1,  1,  0, -1, -1, -1 };

	NavData->BeginBatchQuery();

	// gather 3x3 neighborhood of player
	//if (TargetActor == NULL || TargetActor->IsPendingKill())
	//{
	//	NavData->GetNavMeshTileXY(MyPawn->GetActorLocation(), TileX, TileY);
	//	for (int32 i = 0; i < ARRAY_COUNT(DeltaX); i++)
	//	{
	//		NavData->GetNavMeshTilesAt(TileX + DeltaX[i], TileY + DeltaY[i], Indices);
	//	}
	//}

	// add 3x3 neighborhood of target
	int32 TargetTileX = 0;
	int32 TargetTileY = 0;
	NavData->GetNavMeshTileXY(TargetLocation, TargetTileX, TargetTileY);
	for (int32 i = 0; i < ARRAY_COUNT(DeltaX); i++)
	{
		const int32 NeiX = TargetTileX + DeltaX[i];
		const int32 NeiY = TargetTileY + DeltaY[i];
		if (FMath::Abs(NeiX - TileX) > 1 || FMath::Abs(NeiY - TileY) > 1)
		{
			NavData->GetNavMeshTilesAt(NeiX, NeiY, Indices);
		}
	}

	const FNavDataConfig& NavConfig = NavData->GetConfig();
	FColor NavMeshColors[RECAST_MAX_AREAS];
	NavMeshColors[RECAST_DEFAULT_AREA] = NavConfig.Color.DWColor() > 0 ? NavConfig.Color : NavMeshRenderColor_RecastMesh;
	for (uint8 i = 0; i < RECAST_DEFAULT_AREA; ++i)
	{
		NavMeshColors[i] = NavData->GetAreaIDColor(i);
	}

	TArray<uint8> UncompressedBuffer;
	FMemoryWriter ArWriter(UncompressedBuffer);

	int32 NumIndices = Indices.Num();
	ArWriter << NumIndices;

	for (int32 i = 0; i < NumIndices; i++)
	{
		FRecastDebugGeometry NavMeshGeometry;
		NavMeshGeometry.bGatherPolyEdges = false;
		NavMeshGeometry.bGatherNavMeshEdges = false;
		NavData->GetDebugGeometry(NavMeshGeometry, Indices[i]);

		NavMeshDebug::FTileData TileData;
		const FBox TileBoundingBox = NavData->GetNavMeshTileBounds(Indices[i]);
		TileData.Location = TileBoundingBox.GetCenter();
		for (int32 VertIndex = 0; VertIndex < NavMeshGeometry.MeshVerts.Num(); ++VertIndex)
		{
			const NavMeshDebug::FShortVector SV = NavMeshGeometry.MeshVerts[VertIndex] - TileData.Location;
			TileData.Verts.Add(SV);
		}

		for (int32 iArea = 0; iArea < RECAST_MAX_AREAS; iArea++)
		{
			const int32 NumTris = NavMeshGeometry.AreaIndices[iArea].Num();
			if (NumTris)
			{
				NavMeshDebug::FAreaPolys AreaPolys;
				for (int32 AreaIndicesIndex = 0; AreaIndicesIndex < NavMeshGeometry.AreaIndices[iArea].Num(); ++AreaIndicesIndex)
				{
					AreaPolys.Indices.Add(NavMeshGeometry.AreaIndices[iArea][AreaIndicesIndex]);
				}
				AreaPolys.Color = NavMeshColors[iArea];
				TileData.Areas.Add(AreaPolys);
			}
		}

		TileData.Links.Reserve(NavMeshGeometry.OffMeshLinks.Num());
		for (int32 iLink = 0; iLink < NavMeshGeometry.OffMeshLinks.Num(); iLink++)
		{
			const FRecastDebugGeometry::FOffMeshLink& SrcLink = NavMeshGeometry.OffMeshLinks[iLink];
			NavMeshDebug::FOffMeshLink Link;
			Link.Left = SrcLink.Left - TileData.Location;
			Link.Right = SrcLink.Right - TileData.Location;
			Link.Color = NavMeshColors[SrcLink.AreaID];
			Link.PackedFlags.Radius = (int8)SrcLink.Radius;
			Link.PackedFlags.Direction = SrcLink.Direction;
			Link.PackedFlags.ValidEnds = SrcLink.ValidEnds;
			TileData.Links.Add(Link);
		}

		ArWriter << TileData;
	}
	NavData->FinishBatchQuery();

	const double Timer2 = FPlatformTime::Seconds();

	const int32 HeaderSize = sizeof(int32);
	NavmeshRepData.Init(0, HeaderSize + FMath::TruncToInt(1.1f * UncompressedBuffer.Num()));

	const int32 UncompressedSize = UncompressedBuffer.Num();
	int32 CompressedSize = NavmeshRepData.Num() - HeaderSize;
	uint8* DestBuffer = NavmeshRepData.GetTypedData();
	FMemory::Memcpy(DestBuffer, &UncompressedSize, HeaderSize);
	DestBuffer += HeaderSize;

	FCompression::CompressMemory((ECompressionFlags)(COMPRESS_ZLIB | COMPRESS_BiasMemory),
		(void*)DestBuffer, CompressedSize, (void*)UncompressedBuffer.GetData(), UncompressedSize);

	NavmeshRepData.SetNum(CompressedSize + HeaderSize, false);

	const double Timer3 = FPlatformTime::Seconds();
	UE_LOG(LogNavigation, Log, TEXT("Preparing navmesh data: %.1fkB took %.3fms (collect: %.3fms + compress %d%%: %.3fms)"),
		NavmeshRepData.Num() / 1024.0f, 1000.0f * (Timer3 - Timer1),
		1000.0f * (Timer2 - Timer1),
		FMath::TruncToInt(100.0f * NavmeshRepData.Num() / UncompressedBuffer.Num()), 1000.0f * (Timer3 - Timer2));
#endif

	if (World && World->GetNetMode() != NM_DedicatedServer)
	{
		OnRep_UpdateNavmesh();
	}
}

void UGameplayDebuggingComponent::PrepareNavMeshData(struct FNavMeshSceneProxyData* CurrentData) const
{
#if WITH_RECAST
	if (CurrentData)
	{
		CurrentData->Reset();
		CurrentData->bNeedsNewData = false;

		// uncompress data
		TArray<uint8> UncompressedBuffer;
		const int32 HeaderSize = sizeof(int32);
		if (NavmeshRepData.Num() > HeaderSize)
		{
			int32 UncompressedSize = 0;
			uint8* SrcBuffer = (uint8*)NavmeshRepData.GetTypedData();
			FMemory::Memcpy(&UncompressedSize, SrcBuffer, HeaderSize);
			SrcBuffer += HeaderSize;
			const int32 CompressedSize = NavmeshRepData.Num() - HeaderSize;

			UncompressedBuffer.AddZeroed(UncompressedSize);

			FCompression::UncompressMemory((ECompressionFlags)(COMPRESS_ZLIB),
				(void*)UncompressedBuffer.GetTypedData(), UncompressedSize, SrcBuffer, CompressedSize);
		}

		// read serialized values
		CurrentData->bEnableDrawing = (UncompressedBuffer.Num() > 0);
		if (!CurrentData->bEnableDrawing)
		{
			return;
		}

		FMemoryReader ArReader(UncompressedBuffer);
		int32 NumTiles = 0;
		ArReader << NumTiles;

		for (int32 iTile = 0; iTile < NumTiles; iTile++)
		{
			NavMeshDebug::FTileData TileData;
			ArReader << TileData;

			FVector OffsetLocation = TileData.Location;
			TArray<FVector> Verts; 
			Verts.Reserve(TileData.Verts.Num());
			for (int32 VertIndex = 0; VertIndex < TileData.Verts.Num(); ++VertIndex)
			{
				const FVector Loc = TileData.Verts[VertIndex].ToVector() + OffsetLocation;
				Verts.Add(Loc);
			}
			CurrentData->Bounds += FBox(Verts);

			for (int32 iArea = 0; iArea < TileData.Areas.Num(); iArea++)
			{
				const NavMeshDebug::FAreaPolys& SrcArea = TileData.Areas[iArea];
				FNavMeshSceneProxyData::FDebugMeshData DebugMeshData;
				DebugMeshData.ClusterColor = SrcArea.Color;

				for (int32 iVert = 0; iVert < Verts.Num(); iVert++)
				{
					AddVertexHelper(DebugMeshData, Verts[iVert] + CurrentData->NavMeshDrawOffset);
				}
				
				for (int32 iTri = 0; iTri < SrcArea.Indices.Num(); iTri += 3)
				{
					AddTriangleHelper(DebugMeshData, SrcArea.Indices[iTri], SrcArea.Indices[iTri + 1], SrcArea.Indices[iTri + 2]);

					FVector V0 = Verts[SrcArea.Indices[iTri+0]];
					FVector V1 = Verts[SrcArea.Indices[iTri+1]];
					FVector V2 = Verts[SrcArea.Indices[iTri+2]];
					CurrentData->TileEdgeLines.Add(FDebugRenderSceneProxy::FDebugLine(V0 + CurrentData->NavMeshDrawOffset, V1 + CurrentData->NavMeshDrawOffset, NavMeshRenderColor_Recast_TileEdges));
					CurrentData->TileEdgeLines.Add(FDebugRenderSceneProxy::FDebugLine(V1 + CurrentData->NavMeshDrawOffset, V2 + CurrentData->NavMeshDrawOffset, NavMeshRenderColor_Recast_TileEdges));
					CurrentData->TileEdgeLines.Add(FDebugRenderSceneProxy::FDebugLine(V2 + CurrentData->NavMeshDrawOffset, V0 + CurrentData->NavMeshDrawOffset, NavMeshRenderColor_Recast_TileEdges));
				}

				CurrentData->MeshBuilders.Add(DebugMeshData);
			}

			for (int32 i = 0; i < TileData.Links.Num(); i++)
			{
				const NavMeshDebug::FOffMeshLink& SrcLink = TileData.Links[i];

				const FVector V0 = SrcLink.Left.ToVector() + OffsetLocation + CurrentData->NavMeshDrawOffset;
				const FVector V1 = SrcLink.Right.ToVector() + OffsetLocation + CurrentData->NavMeshDrawOffset;
				const FColor LinkColor = SrcLink.Color;

				CacheArc(CurrentData->NavLinkLines, V0, V1, 0.4f, 4, LinkColor, LinkLines_LineThickness);

				const FVector VOffset(0, 0, FVector::Dist(V0, V1) * 1.333f);
				CacheArrowHead(CurrentData->NavLinkLines, V1, V0+VOffset, 30.f, LinkColor, LinkLines_LineThickness);
				if (SrcLink.PackedFlags.Direction)
				{
					CacheArrowHead(CurrentData->NavLinkLines, V0, V1+VOffset, 30.f, LinkColor, LinkLines_LineThickness);
				}

				// if the connection as a whole is valid check if there are any of ends is invalid
				if (LinkColor != NavMeshRenderColor_OffMeshConnectionInvalid)
				{
					if (SrcLink.PackedFlags.Direction && (SrcLink.PackedFlags.ValidEnds & FRecastDebugGeometry::OMLE_Left) == 0)
					{
						// left end invalid - mark it
						DrawWireCylinder(CurrentData->NavLinkLines, V0, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), NavMeshRenderColor_OffMeshConnectionInvalid, SrcLink.PackedFlags.Radius, 30 /*NavMesh->AgentMaxStepHeight*/, 16, 0, DefaultEdges_LineThickness);
					}

					if ((SrcLink.PackedFlags.ValidEnds & FRecastDebugGeometry::OMLE_Right) == 0)
					{
						DrawWireCylinder(CurrentData->NavLinkLines, V1, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), NavMeshRenderColor_OffMeshConnectionInvalid, SrcLink.PackedFlags.Radius, 30 /*NavMesh->AgentMaxStepHeight*/, 16, 0, DefaultEdges_LineThickness);
					}
				}
			}
		}
	}
#endif
}

//----------------------------------------------------------------------//
// rendering
//----------------------------------------------------------------------//
FPrimitiveSceneProxy* UGameplayDebuggingComponent::CreateSceneProxy()
{
	FDebugRenderSceneCompositeProxy* CompositeProxy = NULL;

#if WITH_RECAST	
	const APawn* MyPawn = Cast<APawn>(GetOwner());
	const APlayerController* MyPC = MyPawn ? Cast<APlayerController>(MyPawn->Controller) : NULL;
	if (ShouldReplicateData(EAIDebugDrawDataView::NavMesh) && MyPC && MyPC->IsLocalController())
	{
		TSharedPtr<struct FNavMeshSceneProxyData, ESPMode::ThreadSafe> NewNavmeshRenderData = MakeShareable(new FNavMeshSceneProxyData());
		NewNavmeshRenderData->bNeedsNewData = false;
		NewNavmeshRenderData->bEnableDrawing = false;
		PrepareNavMeshData(NewNavmeshRenderData.Get());
		if (NewNavmeshRenderData.IsValid())
		{
			NavMeshBounds = NewNavmeshRenderData->Bounds;
			CompositeProxy = CompositeProxy ? CompositeProxy : (new FDebugRenderSceneCompositeProxy(this));
			CompositeProxy->AddChild(new FRecastRenderingSceneProxy(this, NewNavmeshRenderData, FSimpleDelegateGraphTask::FDelegate(), true));
		}
	}
#endif

	if (ShouldReplicateData(EAIDebugDrawDataView::EQS) && IsClientEQSSceneProxyEnabled() )
	{
		CompositeProxy = CompositeProxy ? CompositeProxy : (new FDebugRenderSceneCompositeProxy(this));
		CompositeProxy->AddChild(new FEQSSceneProxy(this, TEXT("GameplayDebug"), /*bDrawOnlyWhenSelected=*/false, EQSLocalData.SolidSpheres, EQSLocalData.Texts));
	}

	return CompositeProxy;
}

FBoxSphereBounds UGameplayDebuggingComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBox MyBounds;
	MyBounds.Init();

#if WITH_RECAST
	if (ShouldReplicateData(EAIDebugDrawDataView::NavMesh))
	{
		MyBounds = NavMeshBounds;
	}
#endif

	if (EQSRepData.Num())
	{
		MyBounds = FBox(FVector(-HALF_WORLD_MAX, -HALF_WORLD_MAX, -HALF_WORLD_MAX), FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX));
	}

	return MyBounds;
}

void UGameplayDebuggingComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

#if WITH_EDITOR
	if (SceneProxy)
	{
		static_cast<FDebugRenderSceneCompositeProxy*>(SceneProxy)->RegisterDebugDrawDelgate();
	}
#endif
}

void UGameplayDebuggingComponent::DestroyRenderState_Concurrent()
{
#if WITH_EDITOR
	if (SceneProxy)
	{
		static_cast<FDebugRenderSceneCompositeProxy*>(SceneProxy)->UnregisterDebugDrawDelgate();
	}
#endif

	Super::DestroyRenderState_Concurrent();
}
