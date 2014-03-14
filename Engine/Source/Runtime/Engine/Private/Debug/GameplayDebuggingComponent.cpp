// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineClasses.h"
#include "Net/UnrealNetwork.h"
#include "AI/BehaviorTreeDelegates.h"
#include "../AI/Navigation/RecastNavMeshGenerator.h"
#include "../NavMeshRenderingHelpers.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

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
uint32 UGameplayDebuggingComponent::StaticActiveViews = 0;
const uint32 UGameplayDebuggingComponent::ShowFlagIndex = uint32(FEngineShowFlags::FindIndexByName(TEXT("GameplayDebug")));

UGameplayDebuggingComponent::UGameplayDebuggingComponent(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, ActiveViews(EAIDebugDrawDataView::Empty)
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
	NavmeshRenderData = MakeShareable(new FNavMeshSceneProxyData());
#if WITH_RECAST
	NavmeshRenderData->bNeedsNewData = false;
	NavmeshRenderData->bEnableDrawing = false;
#endif
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
	}
}

void UGameplayDebuggingComponent::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UGameplayDebuggingComponent, ActivationCounter );
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

}

bool UGameplayDebuggingComponent::ServerEnableTargetSelection_Validate(bool bEnable)
{
	return true;
}

void UGameplayDebuggingComponent::ServerEnableTargetSelection_Implementation(bool bEnable)
{
	bEnabledTargetSelection = bEnable;
	if (bEnabledTargetSelection && GetOwnerRole() == ROLE_Authority)
	{
		SelectTargetToDebug();
	}
}

void UGameplayDebuggingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwnerRole() == ROLE_Authority)
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

	if (MyPC)
	{
		APawn* BestTarget = NULL;
		if (MyPC->GetViewTarget() != NULL && MyPC->GetViewTarget() != MyPC->GetPawn())
		{
			BestTarget = Cast<APawn>(MyPC->GetViewTarget());
			if (BestTarget && BestTarget->PlayerState && !BestTarget->PlayerState->bIsABot)
			{
				BestTarget = NULL;
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
		for (FConstPawnIterator Iterator = World->GetPawnIterator(); Iterator; ++Iterator )
		{
			APawn* NewTarget = *Iterator;
			if ( NewTarget == MyPC->GetPawn() || (NewTarget->PlayerState && !NewTarget->PlayerState->bIsABot))
			{
				continue;
			}
			
			if (BestTarget == NULL && NewTarget && (NewTarget != MyPC->GetPawn()))
			{
				// look for best controlled pawn target
				const FVector AimDir = NewTarget->GetActorLocation() - CamLocation;
				float FireDist = AimDir.SizeSquared();
				// only find targets which are < 5000 units away
				if (FireDist < 25000000.f)
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
			if (UGameplayDebuggingComponent *DebuggingComponent = BestTarget->GetDebugComponent(true))
			{
				TargetActor = BestTarget;
				MyPC->ServerReplicateMessageToAIDebugView(TargetActor, EDebugComponentMessage::ActivateReplication, 0);

				if (DebuggingComponent->GetActiveViews() == 0 )
				{
					DebuggingComponent->EnableDebugDraw(true, false);
				}
				DebuggingComponent->SetActiveViews(DebuggingComponent->GetActiveViews() | (1 << EAIDebugDrawDataView::OverHead));
			}
		}
	}
}

void UGameplayDebuggingComponent::CollectDataToReplicate()
{
	CollectBasicData();
	if (bIsSelectedForDebugging || IsViewActive(EAIDebugDrawDataView::EditorDebugAIFlag))
	{
		CollectPathData();
	}
	if (ShowExtendedInformatiomCounter > 0)
	{
		CollectBehaviorTreeData();
		if (IsViewActive(EAIDebugDrawDataView::EQS))
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
	APawn* MyPawn = Cast<APawn>(GetOwner());
	if (AAIController *MyController = Cast<AAIController>(MyPawn->Controller))
	{
		BrainComponentName = MyController->BrainComponent != NULL ? MyController->BrainComponent->GetName() : TEXT(""); 
	}
}

void UGameplayDebuggingComponent::CollectEQSData()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (World != NULL && World->GetEnvironmentQueryManager() != NULL)
	{
		const AActor* Owner = GetOwner();
		if (Owner)
		{
			TSharedPtr<FEnvQueryInstance> QueryInstance = World->GetEnvironmentQueryManager()->GetDebugger().GetQueryForOwner(Owner);
			if (QueryInstance.IsValid() && (EQSQueryInstance.IsValid() == false || EQSQueryInstance->QueryID != QueryInstance->QueryID))
			{
				EQSQueryInstance = QueryInstance;
				EQSTimestamp = GetWorld()->GetTimeSeconds();
				MarkRenderStateDirty();
			}
		}
	}
#endif // WITH_EDITOR
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
	if (Canvas->SceneView->Family->EngineShowFlags.DebugAI && !IsActive() && !IsPendingKill() && GetOwner() && !GetOwner()->IsPendingKill())
	{
		if (!IsActive() && GetOwnerRole() == ROLE_Authority) 
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
			SetActiveView(EAIDebugDrawDataView::EditorDebugAIFlag);
			EnableActiveView(EAIDebugDrawDataView::Basic, true);
			SelectForDebugging(true);
		}
		else if (!MyPawn || MyPawn && !MyPawn->IsSelected())
		{
			if (bWasSelectedInEditor)
			{
				ServerReplicateData(EDebugComponentMessage::DisableExtendedView, EAIDebugDrawDataView::Basic);
				bWasSelectedInEditor = false;
			}
			SetActiveView(EAIDebugDrawDataView::EditorDebugAIFlag);
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
		// temp: avoidance debug
		UAvoidanceManager* Avoidance = GetWorld()->GetAvoidanceManager();
		AController* ControllerOwner = Cast<AController>(GetOwner());
		APawn* PawnOwner = ControllerOwner ? ControllerOwner->GetPawn() : Cast<APawn>(GetOwner());
		UCharacterMovementComponent* MovementComp = PawnOwner ? PawnOwner->FindComponentByClass<UCharacterMovementComponent>() : NULL;
		if (MovementComp && Avoidance)
		{
			Avoidance->AvoidanceDebugForUID(MovementComp->AvoidanceUID, bNewStatus);
		}

#if WITH_EDITOR
		if (bNewStatus == false)
		{
			EQSQueryInstance = NULL;
			MarkRenderStateDirty();
		}
#endif // WITH_EDITOR
#endif
	}
}

bool UGameplayDebuggingComponent::ToggleStaticView(EAIDebugDrawDataView::Type View) 
{ 
	StaticActiveViews ^= 1 << View; 
	const bool bIsActive = (StaticActiveViews & (1 << View)) != 0;

#if WITH_EDITOR
	if (View == EAIDebugDrawDataView::EQS && GCurrentLevelEditingViewportClient)
	{
		GCurrentLevelEditingViewportClient->EngineShowFlags.SetSingleFlag(UGameplayDebuggingComponent::ShowFlagIndex, bIsActive);
	}
#endif

	return bIsActive; 
}

uint32 UGameplayDebuggingComponent::GetActiveViews()
{
	return ActiveViews;
}

void UGameplayDebuggingComponent::SetActiveViews( uint32 InActiveViews )
{
	ActiveViews = InActiveViews;
}

void UGameplayDebuggingComponent::SetActiveView( EAIDebugDrawDataView::Type NewView )
{
	ActiveViews = 1 << NewView;
}

void UGameplayDebuggingComponent::EnableActiveView( EAIDebugDrawDataView::Type View, bool bEnable )
{
	if (bEnable)
	{
		ActiveViews |= (1 << View);
	}
	else
	{
		ActiveViews &= ~(1 << View);
	}
}

void UGameplayDebuggingComponent::ToggleActiveView(EAIDebugDrawDataView::Type NewView)
{
	ActiveViews ^= 1 << NewView;

#if WITH_EDITOR
	const bool bCleared = !(ActiveViews & ( 1 << NewView));
	if (bCleared && NewView == EAIDebugDrawDataView::EQS)
	{
		EQSQueryInstance = NULL;
		MarkRenderStateDirty();
	}
#endif // WITH_EDITOR
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
		case EDebugComponentMessage::CycleReplicationView:
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
		if (--ShowExtendedInformatiomCounter <= 0) ShowExtendedInformatiomCounter = 0;
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

	case EDebugComponentMessage::CycleReplicationView:
		CycleActiveView();
		break;

	default:
		break;
	}
}

void UGameplayDebuggingComponent::CycleActiveView()
{
	int32 Index = 0;
	for (Index = 0; Index < 32; ++Index)
	{
		if (ActiveViews & (1 << Index))
			break;
	}
	if (++Index >= EAIDebugDrawDataView::EditorDebugAIFlag)
		Index = EAIDebugDrawDataView::Basic;

	ActiveViews = (1 << EAIDebugDrawDataView::OverHead) | (1 << Index);
}

void UGameplayDebuggingComponent::GetKeyboardDesc(TArray<FDebugCategoryView>& Categories)
{
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::NavMesh, "NavMesh"));			// Num0
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::Basic, "Basic"));				// Num1
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::BehaviorTree, "Behavior"));		// Num2
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::EQS, "EQS"));					// Num3
	Categories.Add(FDebugCategoryView(EAIDebugDrawDataView::Perception, "Perception"));		// Num4
}

//----------------------------------------------------------------------//
// NavMesh rendering
//----------------------------------------------------------------------//
void UGameplayDebuggingComponent::OnRep_UpdateNavmesh()
{
	PrepareNavMeshData(NavmeshRenderData.Get());

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
	struct FOffMeshLink
	{
		FVector Left;
		FVector Right;
		FColor Color;
		float Radius;
		uint8 Direction;
		uint8 ValidEnds;
	};

	struct FAreaPolys
	{
		TArray<int32> Indices;
		FColor Color;
	};

	struct FTileData
	{
		TArray<FAreaPolys> Areas;
		TArray<FVector> Verts;
		TArray<FVector> EdgesInner;
		TArray<FVector> EdgesOuter;
		TArray<FOffMeshLink> Links;
	};
}

FArchive& operator<<(FArchive& Ar, NavMeshDebug::FOffMeshLink& Data)
{
	Ar << Data.Left;
	Ar << Data.Right;
	Ar << Data.Color;
	Ar << Data.Radius;
	Ar << Data.Direction;
	Ar << Data.ValidEnds;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, NavMeshDebug::FAreaPolys& Data)
{
	Ar << Data.Indices;
	Ar << Data.Color;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, NavMeshDebug::FTileData& Data)
{
	Ar << Data.Areas;
	Ar << Data.Verts;
	Ar << Data.EdgesInner;
	Ar << Data.EdgesOuter;
	Ar << Data.Links;
	return Ar;
}

void UGameplayDebuggingComponent::ServerDiscardNavmeshData_Implementation()
{
	NavmeshRepData.Empty();
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
	NavData->GetNavMeshTileXY(MyPawn->GetActorLocation(), TileX, TileY);
	for (int32 i = 0; i < ARRAY_COUNT(DeltaX); i++)
	{
		NavData->GetNavMeshTilesAt(TileX + DeltaX[i], TileY + DeltaY[i], Indices);
	}

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
		NavMeshGeometry.bGatherPolyEdges = true;
		NavMeshGeometry.bGatherNavMeshEdges = true;
		NavData->GetDebugGeometry(NavMeshGeometry, Indices[i]);

		NavMeshDebug::FTileData TileData;
		TileData.Verts = NavMeshGeometry.MeshVerts;
		TileData.EdgesInner = NavMeshGeometry.PolyEdges;
		TileData.EdgesOuter = NavMeshGeometry.NavMeshEdges;

		for (int32 iArea = 0; iArea < RECAST_MAX_AREAS; iArea++)
		{
			const int32 NumTris = NavMeshGeometry.AreaIndices[iArea].Num();
			if (NumTris)
			{
				NavMeshDebug::FAreaPolys AreaPolys;
				AreaPolys.Indices = NavMeshGeometry.AreaIndices[iArea];
				AreaPolys.Color = NavMeshColors[iArea];
				TileData.Areas.Add(AreaPolys);
			}
		}

		TileData.Links.Reserve(NavMeshGeometry.OffMeshLinks.Num());
		for (int32 iLink = 0; iLink < NavMeshGeometry.OffMeshLinks.Num(); iLink++)
		{
			const FRecastDebugGeometry::FOffMeshLink& SrcLink = NavMeshGeometry.OffMeshLinks[iLink];
			NavMeshDebug::FOffMeshLink Link;
			Link.Left = SrcLink.Left;
			Link.Right = SrcLink.Right;
			Link.Radius = SrcLink.Radius;
			Link.Color = NavMeshColors[SrcLink.AreaID];
			Link.Direction = SrcLink.Direction;
			Link.ValidEnds = SrcLink.ValidEnds;
			TileData.Links.Add(Link);
		}

		ArWriter << TileData;
	}
	NavData->FinishBatchQuery();

	const double Timer2 = FPlatformTime::Seconds();

	const int32 HeaderSize = sizeof(int32);
	NavmeshRepData.Init(0, HeaderSize + FMath::Trunc(1.1f * UncompressedBuffer.Num()));

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
		FMath::Trunc(100.0f * NavmeshRepData.Num() / UncompressedBuffer.Num()), 1000.0f * (Timer3 - Timer2));
#endif

	if (GetWorld()->GetNetMode() != NM_DedicatedServer)
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

			CurrentData->Bounds += FBox(TileData.Verts);

			for (int32 iArea = 0; iArea < TileData.Areas.Num(); iArea++)
			{
				const NavMeshDebug::FAreaPolys& SrcArea = TileData.Areas[iArea];
				FNavMeshSceneProxyData::FDebugMeshData DebugMeshData;
				DebugMeshData.ClusterColor = SrcArea.Color;

				for (int32 iVert = 0; iVert < TileData.Verts.Num(); iVert++)
				{
					AddVertexHelper(DebugMeshData, TileData.Verts[iVert] + CurrentData->NavMeshDrawOffset);
				}
				
				for (int32 iTri = 0; iTri < SrcArea.Indices.Num(); iTri += 3)
				{
					AddTriangleHelper(DebugMeshData, SrcArea.Indices[iTri], SrcArea.Indices[iTri + 1], SrcArea.Indices[iTri + 2]);
				}

				CurrentData->MeshBuilders.Add(DebugMeshData);
			}

			for (int32 i = 0; i < TileData.EdgesInner.Num(); i += 2)
			{
				CurrentData->TileEdgeLines.Add( FDebugRenderSceneProxy::FDebugLine(TileData.EdgesInner[i] + CurrentData->NavMeshDrawOffset, TileData.EdgesInner[i+1] + CurrentData->NavMeshDrawOffset, NavMeshRenderColor_Recast_TileEdges));
			}

			const FColor EdgesColor = DarkenColor(NavMeshRenderColor_Recast_NavMeshEdges);
			for (int32 i = 0; i < TileData.EdgesOuter.Num(); i += 2)
			{
				CurrentData->NavMeshEdgeLines.Add( FDebugRenderSceneProxy::FDebugLine(TileData.EdgesOuter[i] + CurrentData->NavMeshDrawOffset, TileData.EdgesOuter[i+1] + CurrentData->NavMeshDrawOffset, EdgesColor));
			}

			for (int32 i = 0; i < TileData.Links.Num(); i++)
			{
				const NavMeshDebug::FOffMeshLink& SrcLink = TileData.Links[i];

				const FVector V0 = SrcLink.Left + CurrentData->NavMeshDrawOffset;
				const FVector V1 = SrcLink.Right + CurrentData->NavMeshDrawOffset;
				const FColor LinkColor = SrcLink.Color;

				CacheArc(CurrentData->NavLinkLines, V0, V1, 0.4f, 4, LinkColor, LinkLines_LineThickness);

				const FVector VOffset(0, 0, FVector::Dist(V0, V1) * 1.333f);
				CacheArrowHead(CurrentData->NavLinkLines, V1, V0+VOffset, 30.f, LinkColor, LinkLines_LineThickness);
				if (SrcLink.Direction)
				{
					CacheArrowHead(CurrentData->NavLinkLines, V0, V1+VOffset, 30.f, LinkColor, LinkLines_LineThickness);
				}

				// if the connection as a whole is valid check if there are any of ends is invalid
				if (LinkColor != NavMeshRenderColor_OffMeshConnectionInvalid)
				{
					if (SrcLink.Direction && (SrcLink.ValidEnds & FRecastDebugGeometry::OMLE_Left) == 0)
					{
						// left end invalid - mark it
						DrawWireCylinder(CurrentData->NavLinkLines, V0, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), NavMeshRenderColor_OffMeshConnectionInvalid, SrcLink.Radius, 30 /*NavMesh->AgentMaxStepHeight*/, 16, 0, DefaultEdges_LineThickness);
					}

					if ((SrcLink.ValidEnds & FRecastDebugGeometry::OMLE_Right) == 0)
					{
						DrawWireCylinder(CurrentData->NavLinkLines, V1, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), NavMeshRenderColor_OffMeshConnectionInvalid, SrcLink.Radius, 30 /*NavMesh->AgentMaxStepHeight*/, 16, 0, DefaultEdges_LineThickness);
					}
				}
			}
		}
	}
#endif
}

const FEnvQueryResult* UGameplayDebuggingComponent::GetQueryResult() const
{
	return EQSQueryInstance.Get();
}

const FEnvQueryInstance* UGameplayDebuggingComponent::GetQueryInstance() const
{
	return EQSQueryInstance.Get();
}

//----------------------------------------------------------------------//
// rendering
//----------------------------------------------------------------------//
FPrimitiveSceneProxy* UGameplayDebuggingComponent::CreateSceneProxy()
{
	FDebugRenderSceneCompositeProxy* CompositeProxy = new FDebugRenderSceneCompositeProxy(this);
#if WITH_RECAST	
	if (IsViewActive(EAIDebugDrawDataView::NavMesh))
	{
		CompositeProxy->AddChild(new FRecastRenderingSceneProxy(this, NavmeshRenderData, FSimpleDelegateGraphTask::FDelegate(), true));
	}
#endif
	if (EQSQueryInstance.IsValid())
	{
		CompositeProxy->AddChild(new FEQSSceneProxy(this, TEXT("GameplayDebug"), /*bDrawOnlyWhenSelected=*/false));
	}

	return CompositeProxy;
}

FBoxSphereBounds UGameplayDebuggingComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBox MyBounds;
	MyBounds.Init();

#if WITH_RECAST
	if (NavmeshRenderData.IsValid())
	{
		MyBounds += NavmeshRenderData->Bounds;
	}
#endif

	if (EQSQueryInstance.IsValid())
	{
		MyBounds += EQSQueryInstance->GetBoundingBox();
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
