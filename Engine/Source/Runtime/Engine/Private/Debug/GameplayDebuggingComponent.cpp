// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"
#include "EngineClasses.h"
#include "Net/UnrealNetwork.h"
#include "AI/BehaviorTreeDelegates.h"
#include "../AI/Navigation/RecastNavMeshGenerator.h"
#include "../NavMeshRenderingHelpers.h"

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

	DOREPLIFETIME( UGameplayDebuggingComponent, NavMeshBoundingBox );
	DOREPLIFETIME( UGameplayDebuggingComponent, AllNavMeshAreas );
	DOREPLIFETIME( UGameplayDebuggingComponent, NavMeshEdgeVerts );
	DOREPLIFETIME( UGameplayDebuggingComponent, TileEdgeVerts );
	DOREPLIFETIME( UGameplayDebuggingComponent, OffMeshSegmentAreas );
	DOREPLIFETIME( UGameplayDebuggingComponent, OffMeshLinks );

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
	bool bCleared = !(ActiveViews & ( 1 << NewView));

	switch (NewView)
	{
	case EAIDebugDrawDataView::EQS:
		if (bCleared)
		{
			EQSQueryInstance = NULL;
			MarkRenderStateDirty();
		}
		break;
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

FString UGameplayDebuggingComponent::GetKeyboardDesc()
{
	return TEXT("/-NavMesh, 1-Basic, 2-BT, 3-EQS, 4-Perception");
}

void UGameplayDebuggingComponent::GatherDataForProxy()
{
	if (NavmeshData.IsValid())
	{
#if WITH_RECAST
		FScopeLock ScopeLock(&NavmeshData.Get()->CriticalSection);
		GatherData( NavmeshData.Get() );
		NavmeshData->bNeedsNewData = false;
#endif
	}
}

//----------------------------------------------------------------------//
// NavMesh rendering
//----------------------------------------------------------------------//
void UGameplayDebuggingComponent::OnRep_MarkRenderStateDirty()
{
	MarkRenderStateDirty();
}

bool UGameplayDebuggingComponent::ServerCollectNavmeshData_Validate()
{
	bool bIsValid = false;
#if WITH_RECAST
	bIsValid = GetWorld()->GetNavigationSystem() != NULL && GetWorld()->GetNavigationSystem()->GetMainNavData(NavigationSystem::DontCreate) != NULL;
#endif

	return bIsValid;
}

void UGameplayDebuggingComponent::ServerCollectNavmeshData_Implementation()
{
#if WITH_RECAST
	if (const UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem())
	{
		const ARecastNavMesh* const NavData = (ARecastNavMesh*)GetWorld()->GetNavigationSystem()->GetMainNavData(NavigationSystem::DontCreate);
		const FNavigationOctree* NavOctree = NavSys->GetNavOctree();
		const APawn* MyPawn = Cast<APawn>(GetOwner());	
		const FVector Center = MyPawn->GetActorLocation();
		const float Radius = 1024;
		const float RadiusSq = Radius*Radius;

		NavMeshBoundingBox = FBoxCenterAndExtent( Center, FVector(512) ).GetBox();
		FRecastDebugGeometry NavMeshGeometry;
		NavMeshGeometry.bGatherPolyEdges = true;
		NavMeshGeometry.bGatherNavMeshEdges = true;
		NavData->GetDebugGeometry(NavMeshGeometry);

		const FNavDataConfig& NavConfig = NavData->GetConfig();
		FColor NavMeshColors[RECAST_MAX_AREAS];
		NavMeshColors[RECAST_DEFAULT_AREA] = NavConfig.Color.DWColor() > 0 ? NavConfig.Color : NavMeshRenderColor_RecastMesh;
		for (uint8 i = 0; i < RECAST_DEFAULT_AREA; ++i)
		{
			NavMeshColors[i] = NavData->GetAreaIDColor(i);
		}

		for(int32 Index=0; Index < RECAST_MAX_AREAS; ++Index)
		{
			AllNavMeshAreas[Index].IndexBuffer.Empty();
			AllNavMeshAreas[Index].CoordBuffer.Empty();
			OffMeshLinks.Empty();
			AllNavMeshAreas[Index].NavMeshColor = NavMeshColors[Index];

			const TArray<int32>& CurrentIndices = NavMeshGeometry.AreaIndices[Index];
			for(int32 i=0; i<CurrentIndices.Num(); i+=3)
			{
				const FVector& V1 = NavMeshGeometry.MeshVerts[CurrentIndices[i+0]];
				const FVector& V2 = NavMeshGeometry.MeshVerts[CurrentIndices[i+1]];
				const FVector& V3 = NavMeshGeometry.MeshVerts[CurrentIndices[i+2]];

				if ( (V1-Center).SizeSquared() <= RadiusSq || (V2-Center).SizeSquared() <= RadiusSq || (V3-Center).SizeSquared() <= RadiusSq )
				{
					AllNavMeshAreas[Index].IndexBuffer.Add( AllNavMeshAreas[Index].CoordBuffer.Add( V1 ) );
					AllNavMeshAreas[Index].IndexBuffer.Add( AllNavMeshAreas[Index].CoordBuffer.Add( V2 ) );
					AllNavMeshAreas[Index].IndexBuffer.Add( AllNavMeshAreas[Index].CoordBuffer.Add( V3 ) );
				}
			}

			// cache segment links
			OffMeshSegmentAreas = NavMeshGeometry.OffMeshSegmentAreas[Index];

			for (int32 OffMeshLineIndex = 0; OffMeshLineIndex < NavMeshGeometry.OffMeshLinks.Num(); ++OffMeshLineIndex)
			{
				const FRecastDebugGeometry::FOffMeshLink &CurrentLink = NavMeshGeometry.OffMeshLinks[OffMeshLineIndex];
				if ( (CurrentLink.Left-Center).SizeSquared() <= RadiusSq || (CurrentLink.Right-Center).SizeSquared() <= RadiusSq )
				{
					FOffMeshLinkRenderData Link;
					Link.AreaID = CurrentLink.AreaID;
					Link.Left = CurrentLink.Left;
					Link.Right = CurrentLink.Right;
					Link.Direction = CurrentLink.Direction;
					Link.ValidEnds = CurrentLink.ValidEnds;
					Link.Radius = CurrentLink.Direction;
					Link.Color = CurrentLink.Color;
					OffMeshLinks.Add(Link);
				}
			}

		}

		NavMeshEdgeVerts.Empty();
		for(int32 Index=0; Index < NavMeshGeometry.NavMeshEdges.Num(); Index+=2)
		{
			const FVector& V1 = NavMeshGeometry.NavMeshEdges[Index+0];
			const FVector& V2 = NavMeshGeometry.NavMeshEdges[Index+1];
			if( (V1-Center).SizeSquared() <= RadiusSq || (V2-Center).SizeSquared() <= RadiusSq )
			{
				NavMeshEdgeVerts.Add( V1 );
				NavMeshEdgeVerts.Add( V2 );
			}
		}

		TileEdgeVerts.Empty();
		for(int32 Index=0; Index < NavMeshGeometry.PolyEdges.Num(); Index+=2)
		{
			const FVector& V1 = NavMeshGeometry.PolyEdges[Index+0];
			const FVector& V2 = NavMeshGeometry.PolyEdges[Index+1];
			if( (V1-Center).SizeSquared() <= RadiusSq || (V2-Center).SizeSquared() <= RadiusSq )
			{
				TileEdgeVerts.Add( V1 );
				TileEdgeVerts.Add( V2 );
			}
		}
	}
#endif
}

void UGameplayDebuggingComponent::GatherData(struct FNavMeshSceneProxyData* CurrentData) const
{
#if WITH_RECAST
	if (CurrentData)
	{
		CurrentData->Reset();
		CurrentData->bEnableDrawing = true;

		const TArray<FVector>& MeshVerts = CurrentData->NavMeshGeometry.MeshVerts;
		
		for(int32 Index=0; Index < RECAST_MAX_AREAS; ++Index)
		{
			FNavMeshSceneProxyData::FDebugMeshData DebugMeshData;
			for (int32 VertIdx=0; VertIdx < AllNavMeshAreas[Index].CoordBuffer.Num(); ++VertIdx)
			{
				AddVertexHelper(DebugMeshData, AllNavMeshAreas[Index].CoordBuffer[VertIdx] + CurrentData->NavMeshDrawOffset);
			}

			for (int32 TriIdx=0; TriIdx < AllNavMeshAreas[Index].IndexBuffer.Num(); TriIdx+=3)
			{
				AddTriangleHelper(DebugMeshData, AllNavMeshAreas[Index].IndexBuffer[TriIdx], AllNavMeshAreas[Index].IndexBuffer[TriIdx+1], AllNavMeshAreas[Index].IndexBuffer[TriIdx+2]);
			}

			DebugMeshData.ClusterColor = AllNavMeshAreas[Index].NavMeshColor;
			CurrentData->MeshBuilders.Add(DebugMeshData);

			/** off mesh links */
			for (int32 OffMeshLineIndex = 0; OffMeshLineIndex < OffMeshLinks.Num(); ++OffMeshLineIndex)
			{
				const FOffMeshLinkRenderData& Link = OffMeshLinks[OffMeshLineIndex];
				const bool bLinkValid = (Link.ValidEnds & FRecastDebugGeometry::OMLE_Left) && (Link.ValidEnds & FRecastDebugGeometry::OMLE_Right);

				if (true /*NavMesh->bDrawFailedNavLinks || (NavMesh->bDrawNavLinks && bLinkValid)*/)
				{
					const FVector V0 = Link.Left + CurrentData->NavMeshDrawOffset;
					const FVector V1 = Link.Right + CurrentData->NavMeshDrawOffset;
					const FColor LinkColor = Link.Color;//((Link.Direction && Link.ValidEnds) || (Link.ValidEnds & FRecastDebugGeometry::OMLE_Left)) ? DarkenColor(CurrentData->NavMeshColors[Link.AreaID]) : NavMeshRenderColor_OffMeshConnectionInvalid;

					CacheArc(CurrentData->NavLinkLines, V0, V1, 0.4f, 4, LinkColor, LinkLines_LineThickness);

					const FVector VOffset(0, 0, FVector::Dist(V0, V1) * 1.333f);
					CacheArrowHead(CurrentData->NavLinkLines, V1, V0+VOffset, 30.f, LinkColor, LinkLines_LineThickness);
					if (Link.Direction)
					{
						CacheArrowHead(CurrentData->NavLinkLines, V0, V1+VOffset, 30.f, LinkColor, LinkLines_LineThickness);
					}

					// if the connection as a whole is valid check if there are any of ends is invalid
					if (LinkColor != NavMeshRenderColor_OffMeshConnectionInvalid)
					{
						if (Link.Direction && (Link.ValidEnds & FRecastDebugGeometry::OMLE_Left) == 0)
						{
							// left end invalid - mark it
							DrawWireCylinder(CurrentData->NavLinkLines, V0, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), NavMeshRenderColor_OffMeshConnectionInvalid, Link.Radius, 30 /*NavMesh->AgentMaxStepHeight*/, 16, 0, DefaultEdges_LineThickness);
						}
						if ((Link.ValidEnds & FRecastDebugGeometry::OMLE_Right) == 0)
						{
							DrawWireCylinder(CurrentData->NavLinkLines, V1, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), NavMeshRenderColor_OffMeshConnectionInvalid, Link.Radius, 30 /*NavMesh->AgentMaxStepHeight*/, 16, 0, DefaultEdges_LineThickness);
						}
					}
				}					
			}

			/** off mesh segment areas */
			FNavMeshSceneProxyData::FDebugMeshData OffMeshDebugMeshData;
			int32 VertBase = 0;

			for (int32 i = 0; i < OffMeshSegmentAreas.Num(); i++)
			{
				int32 OffIndex = OffMeshSegmentAreas[i];
				FRecastDebugGeometry::FOffMeshSegment& SegInfo = CurrentData->NavMeshGeometry.OffMeshSegments[OffIndex];
				const FVector A0 = SegInfo.LeftStart + CurrentData->NavMeshDrawOffset;
				const FVector A1 = SegInfo.LeftEnd + CurrentData->NavMeshDrawOffset;
				const FVector B0 = SegInfo.RightStart + CurrentData->NavMeshDrawOffset;
				const FVector B1 = SegInfo.RightEnd + CurrentData->NavMeshDrawOffset;
				const FVector Edge0 = B0 - A0;
				const FVector Edge1 = B1 - A1;
				const float Len0 = Edge0.Size();
				const float Len1 = Edge1.Size();
				const FColor SegColor = DarkenColor(CurrentData->NavMeshColors[SegInfo.AreaID]);
				const FColor ColA = (SegInfo.ValidEnds & FRecastDebugGeometry::OMLE_Left) ? FColor::White : FColor::Black;
				const FColor ColB = (SegInfo.ValidEnds & FRecastDebugGeometry::OMLE_Right) ? FColor::White : FColor::Black;

				const int32 NumArcPoints = 8;
				const float ArcPtsScale = 1.0f / NumArcPoints;

				FVector Prev0 = EvalArc(A0, Edge0, Len0*0.25f, 0);
				FVector Prev1 = EvalArc(A1, Edge1, Len1*0.25f, 0);
				AddVertexHelper(OffMeshDebugMeshData, Prev0, ColA);
				AddVertexHelper(OffMeshDebugMeshData, Prev1, ColA);
				for (int32 j = 1; j <= NumArcPoints; j++)
				{
					const float u = j * ArcPtsScale;
					FVector Pt0 = EvalArc(A0, Edge0, Len0*0.25f, u);
					FVector Pt1 = EvalArc(A1, Edge1, Len1*0.25f, u);

					AddVertexHelper(OffMeshDebugMeshData, Pt0, (j == NumArcPoints) ? ColB : FColor::White);
					AddVertexHelper(OffMeshDebugMeshData, Pt1, (j == NumArcPoints) ? ColB : FColor::White);

					AddTriangleHelper(OffMeshDebugMeshData, VertBase+0, VertBase+2, VertBase+1);
					AddTriangleHelper(OffMeshDebugMeshData, VertBase+2, VertBase+3, VertBase+1);
					AddTriangleHelper(OffMeshDebugMeshData, VertBase+0, VertBase+1, VertBase+2);
					AddTriangleHelper(OffMeshDebugMeshData, VertBase+2, VertBase+1, VertBase+3);

					VertBase += 2;
					Prev0 = Pt0;
					Prev1 = Pt1;
				}
				VertBase += 2;

				OffMeshDebugMeshData.ClusterColor = SegColor;
				if (OffMeshDebugMeshData.Indices.Num())
				{
					CurrentData->MeshBuilders.Add(OffMeshDebugMeshData);
				}
			}
		}
		
		const FColor EdgesColor = DarkenColor(NavMeshRenderColor_Recast_NavMeshEdges);
		for (int32 Idx=0; Idx < NavMeshEdgeVerts.Num(); Idx += 2)
		{
			CurrentData->NavMeshEdgeLines.Add( FDebugRenderSceneProxy::FDebugLine(NavMeshEdgeVerts[Idx] + CurrentData->NavMeshDrawOffset, NavMeshEdgeVerts[Idx+1] + CurrentData->NavMeshDrawOffset, EdgesColor));
		}


		for (int32 Idx=0; Idx < TileEdgeVerts.Num(); Idx += 2)
		{
			CurrentData->TileEdgeLines.Add( FDebugRenderSceneProxy::FDebugLine(TileEdgeVerts[Idx] + CurrentData->NavMeshDrawOffset, TileEdgeVerts[Idx+1] + CurrentData->NavMeshDrawOffset, NavMeshRenderColor_Recast_TileEdges));
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
		if (!NavmeshData.IsValid())
		{
			NavmeshData = MakeShareable(new FNavMeshSceneProxyData());
		}
		NavmeshData->bNeedsNewData = true;
		CompositeProxy->AddChild(new FRecastRenderingSceneProxy(this, NavmeshData, FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &UGameplayDebuggingComponent::GatherDataForProxy), true));
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
	if (EQSQueryInstance.IsValid())
	{
		return FBoxSphereBounds(NavMeshBoundingBox + EQSQueryInstance->GetBoundingBox());
	}
	else
	{
		return FBoxSphereBounds(NavMeshBoundingBox);
	}
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
