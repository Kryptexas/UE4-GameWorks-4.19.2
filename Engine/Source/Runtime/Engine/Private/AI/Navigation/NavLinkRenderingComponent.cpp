// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "../../Public/AI/NavLinkRenderingProxy.h"

//----------------------------------------------------------------------//
// UNavLinkRenderingComponent
//----------------------------------------------------------------------//
UNavLinkRenderingComponent::UNavLinkRenderingComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// properties

	// Allows updating in game, while optimizing rendering for the case that it is not modified
	Mobility = EComponentMobility::Stationary;

	BodyInstance.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	AlwaysLoadOnClient = false;
	AlwaysLoadOnServer = false;

	bGenerateOverlapEvents = false;
}

FBoxSphereBounds UNavLinkRenderingComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return FBoxSphereBounds( FVector::ZeroVector, FVector(512), 512 ).TransformBy(LocalToWorld);
}

FPrimitiveSceneProxy* UNavLinkRenderingComponent::CreateSceneProxy()
{
	return new FNavLinkRenderingProxy(this);
}

//----------------------------------------------------------------------//
// FNavLinkRenderingProxy
//----------------------------------------------------------------------//
FNavLinkRenderingProxy::FNavLinkRenderingProxy(const UPrimitiveComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
	LinkOwnerActor = Cast<AActor>(InComponent->GetOwner());
	LinkOwnerHost = InterfaceCast<INavLinkHostInterface>(InComponent->GetOwner());

	if (LinkOwnerActor != NULL && LinkOwnerHost != NULL)
	{
		const FTransform LocalToWorld = LinkOwnerActor->ActorToWorld();
		TArray<TSubclassOf<class UNavLinkDefinition> > NavLinkClasses;
		LinkOwnerHost->GetNavigationLinksClasses(NavLinkClasses);

		for (int32 NavLinkClassIdx = 0; NavLinkClassIdx < NavLinkClasses.Num(); ++NavLinkClassIdx)
		{
			if (NavLinkClasses[NavLinkClassIdx] != NULL)
			{
				StorePointLinks(LocalToWorld, UNavLinkDefinition::GetLinksDefinition(NavLinkClasses[NavLinkClassIdx]));
				StoreSegmentLinks(LocalToWorld, UNavLinkDefinition::GetSegmentLinksDefinition(NavLinkClasses[NavLinkClassIdx]));
			}
		}

		TArray<FNavigationLink> PointLinks;
		TArray<FNavigationSegmentLink> SegmentLinks;
		if (LinkOwnerHost->GetNavigationLinksArray(PointLinks, SegmentLinks))
		{
			StorePointLinks(LocalToWorld, PointLinks);
			StoreSegmentLinks(LocalToWorld, SegmentLinks);
		}
	}
}

void FNavLinkRenderingProxy::StorePointLinks(const FTransform& LocalToWorld, const TArray<FNavigationLink>& LinksArray)
{
	const FNavigationLink* Link = LinksArray.GetTypedData();
	for (int32 LinkIndex = 0; LinkIndex < LinksArray.Num(); ++LinkIndex, ++Link)
	{	
		FNavLinkDrawing LinkDrawing;
		LinkDrawing.Left = LocalToWorld.TransformPosition(Link->Left);
		LinkDrawing.Right = LocalToWorld.TransformPosition(Link->Right);
		LinkDrawing.Direction = Link->Direction;
		LinkDrawing.Color = UNavArea::GetColor(Link->AreaClass);
		LinkDrawing.SnapRadius = Link->SnapRadius;
		OffMeshPointLinks.Add(LinkDrawing);
	}
}

void FNavLinkRenderingProxy::StoreSegmentLinks(const FTransform& LocalToWorld, const TArray<FNavigationSegmentLink>& LinksArray)
{
	const FNavigationSegmentLink* Link = LinksArray.GetTypedData();
	for (int32 LinkIndex = 0; LinkIndex < LinksArray.Num(); ++LinkIndex, ++Link)
	{	
		FNavLinkSegmentDrawing LinkDrawing;
		LinkDrawing.LeftStart = LocalToWorld.TransformPosition(Link->LeftStart);
		LinkDrawing.LeftEnd = LocalToWorld.TransformPosition(Link->LeftEnd);
		LinkDrawing.RightStart = LocalToWorld.TransformPosition(Link->RightStart);
		LinkDrawing.RightEnd = LocalToWorld.TransformPosition(Link->RightEnd);
		LinkDrawing.Direction = Link->Direction;
		LinkDrawing.Color = UNavArea::GetColor(Link->AreaClass);
		LinkDrawing.SnapRadius = Link->SnapRadius;
		OffMeshSegmentLinks.Add(LinkDrawing);
	}
}

void FNavLinkRenderingProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_NavLinkRenderingProxy_DrawDynamicElements );

	if (LinkOwnerActor && LinkOwnerActor->GetWorld())
	{
		const UNavigationSystem* NavSys = LinkOwnerActor->GetWorld()->GetNavigationSystem();
		TArray<float> StepHeights;
		if (NavSys != NULL)
		{
			StepHeights.Reserve(NavSys->NavDataSet.Num());
			for(int32 DataIndex = 0; DataIndex < NavSys->NavDataSet.Num(); ++DataIndex)
			{
				const ARecastNavMesh* NavMesh = Cast<const ARecastNavMesh>(NavSys->NavDataSet[DataIndex]);
				if (NavMesh != NULL && NavMesh->AgentMaxStepHeight > 0 && NavMesh->bEnableDrawing)
				{
					StepHeights.Add(NavMesh->AgentMaxStepHeight);
				}
			}
		}

		static const FColor RadiusColor(150, 160, 150, 48);
		FMaterialRenderProxy* const MeshColorInstance = new(FMemStack::Get()) FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(false), RadiusColor);
		FNavLinkRenderingProxy::DrawLinks(PDI, OffMeshPointLinks, OffMeshSegmentLinks, StepHeights, MeshColorInstance);
	}
}

void FNavLinkRenderingProxy::DrawLinks(FPrimitiveDrawInterface* PDI, TArray<FNavLinkDrawing>& OffMeshPointLinks, TArray<FNavLinkSegmentDrawing>& OffMeshSegmentLinks, TArray<float>& StepHeights, FMaterialRenderProxy* const MeshColorInstance)
{
	static const FColor LinkColor(0,0,166);
	static const float LinkArcThickness = 3.f;
	static const float LinkArcHeight = 0.4f;
	
	if (StepHeights.Num() == 0)
	{
		StepHeights.Add(NavigationSystem::FallbackAgentHeight / 2);
	}

	for (int32 LinkIndex = 0; LinkIndex < OffMeshPointLinks.Num(); ++LinkIndex)
	{
		const FNavLinkDrawing& Link = OffMeshPointLinks[LinkIndex];
		const uint32 Segments = FPlatformMath::Max<uint32>(LinkArcHeight*(Link.Right-Link.Left).Size()/10, 8);
		DrawArc(PDI, Link.Left, Link.Right, LinkArcHeight, Segments, Link.Color, SDPG_World, 3.5f);
		const FVector VOffset(0,0,FVector::Dist(Link.Left, Link.Right)*1.333f);

		switch (Link.Direction)
		{
		case ENavLinkDirection::LeftToRight:
			DrawArrowHead(PDI, Link.Right, Link.Left+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			break;
		case ENavLinkDirection::RightToLeft:
			DrawArrowHead(PDI, Link.Left, Link.Right+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			break;
		case ENavLinkDirection::BothWays:
		default:
			DrawArrowHead(PDI, Link.Right, Link.Left+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			DrawArrowHead(PDI, Link.Left, Link.Right+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			break;
		}

		// draw snap-spheres on both ends
		for (int32 StepHeightIndex = 0; StepHeightIndex < StepHeights.Num(); ++StepHeightIndex)
		{
			DrawCylinder(PDI, Link.Right, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), Link.SnapRadius, StepHeights[StepHeightIndex], 10, MeshColorInstance, SDPG_World);
			DrawCylinder(PDI, Link.Left, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), Link.SnapRadius, StepHeights[StepHeightIndex], 10, MeshColorInstance, SDPG_World);
		}
	}

	static const float SegmentArcHeight = 0.25f;
	for (int32 LinkIndex = 0; LinkIndex < OffMeshSegmentLinks.Num(); ++LinkIndex)
	{
		const FNavLinkSegmentDrawing& Link = OffMeshSegmentLinks[LinkIndex];
		const uint32 SegmentsStart = FPlatformMath::Max<uint32>(SegmentArcHeight*(Link.RightStart-Link.LeftStart).Size()/10, 8);
		const uint32 SegmentsEnd = FPlatformMath::Max<uint32>(SegmentArcHeight*(Link.RightEnd-Link.LeftEnd).Size()/10, 8);
		DrawArc(PDI, Link.LeftStart, Link.RightStart, SegmentArcHeight, SegmentsStart, Link.Color, SDPG_World, 3.5f);
		DrawArc(PDI, Link.LeftEnd, Link.RightEnd, SegmentArcHeight, SegmentsEnd, Link.Color, SDPG_World, 3.5f);
		const FVector VOffset(0,0,FVector::Dist(Link.LeftStart, Link.RightStart)*1.333f);

		switch (Link.Direction)
		{
		case ENavLinkDirection::LeftToRight:
			DrawArrowHead(PDI, Link.RightStart, Link.LeftStart+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			DrawArrowHead(PDI, Link.RightEnd, Link.LeftEnd+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			break;
		case ENavLinkDirection::RightToLeft:
			DrawArrowHead(PDI, Link.LeftStart, Link.RightStart+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			DrawArrowHead(PDI, Link.LeftEnd, Link.RightEnd+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			break;
		case ENavLinkDirection::BothWays:
		default:
			DrawArrowHead(PDI, Link.RightStart, Link.LeftStart+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			DrawArrowHead(PDI, Link.RightEnd, Link.LeftEnd+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			DrawArrowHead(PDI, Link.LeftStart, Link.RightStart+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			DrawArrowHead(PDI, Link.LeftEnd, Link.RightEnd+VOffset, 30.f, Link.Color, SDPG_World, 3.5f);
			break;
		}

		// draw snap-spheres on both ends
		for (int32 StepHeightIndex = 0; StepHeightIndex < StepHeights.Num(); ++StepHeightIndex)
		{
			DrawCylinder(PDI, Link.RightStart, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), Link.SnapRadius, StepHeights[StepHeightIndex], 10, MeshColorInstance, SDPG_World);
			DrawCylinder(PDI, Link.RightEnd, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), Link.SnapRadius, StepHeights[StepHeightIndex], 10, MeshColorInstance, SDPG_World);
			DrawCylinder(PDI, Link.LeftStart, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), Link.SnapRadius, StepHeights[StepHeightIndex], 10, MeshColorInstance, SDPG_World);
			DrawCylinder(PDI, Link.LeftEnd, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), Link.SnapRadius, StepHeights[StepHeightIndex], 10, MeshColorInstance, SDPG_World);
		}
	}
}

FPrimitiveViewRelevance FNavLinkRenderingProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View) && IsSelected() && (View && View->Family && View->Family->EngineShowFlags.Navigation);
	Result.bDynamicRelevance = true;
	Result.bNormalTranslucencyRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
	return Result;
}

uint32 FNavLinkRenderingProxy::GetMemoryFootprint( void ) const 
{ 
	return( sizeof( *this ) + GetAllocatedSize() ); 
}

uint32 FNavLinkRenderingProxy::GetAllocatedSize( void ) const 
{ 
	return( FPrimitiveSceneProxy::GetAllocatedSize() + OffMeshPointLinks.Num() + OffMeshSegmentLinks.Num() ); 
}
