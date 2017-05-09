// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ClothPainter.h"
#include "MeshPaintSettings.h"
#include "MeshPaintModule.h"
#include "MeshPaintAdapterFactory.h"
#include "MeshPaintHelpers.h"

#include "Assets/ClothingAsset.h"
#include "Classes/Animation/DebugSkelMeshComponent.h"
#include "ClothPaintSettings.h"
#include "ClothMeshAdapter.h"
#include "SClothPaintWidget.h"

#include "EditorViewportClient.h"
#include "ComponentReregisterContext.h"
#include "Package.h"

#define LOCTEXT_NAMESPACE "ClothPainter"

FClothPainter::FClothPainter()
	: IMeshPainter(),  SkeletalMeshComponent(nullptr), PaintSettings(nullptr), BrushSettings(nullptr), bSelectingFirstPoint(true)
{
	Init();
	VertexPointSize = 3.0f;
	VertexPointColor = FLinearColor::White;
	WidgetLineThickness = .5f;
	bShouldSimulate = false;
	bShowHiddenVerts = false;
}

FClothPainter::~FClothPainter()
{
	PaintSettings->OnAssetSelectionChanged.RemoveAll(this);
	
	SkeletalMeshComponent->ToggleMeshSectionForCloth(SkeletalMeshComponent->SelectedClothingGuidForPainting);

	// Cancel rendering of the paint proxy
	SkeletalMeshComponent->SelectedClothingGuidForPainting = FGuid();
}

void FClothPainter::Init()
{
	BrushSettings = DuplicateObject<UPaintBrushSettings>(GetMutableDefault<UPaintBrushSettings>(), GetTransientPackage());	
	BrushSettings->AddToRoot();
	BrushSettings->bOnlyFrontFacingTriangles = false;
	PaintSettings = DuplicateObject<UClothPainterSettings>(GetMutableDefault<UClothPainterSettings>(), GetTransientPackage());
	PaintSettings->AddToRoot();

	PaintSettings->OnAssetSelectionChanged.AddRaw(this, &FClothPainter::OnAssetSelectionChanged);

	Widget = SNew(SClothPaintWidget, this);
}

bool FClothPainter::PaintInternal(const FVector& InCameraOrigin, const FVector& InRayOrigin, const FVector& InRayDirection, EMeshPaintAction PaintAction, float PaintStrength)
{
	bool bApplied = false;

	if(SkeletalMeshComponent->SelectedClothingGuidForPainting.IsValid())
	{
		USkeletalMesh* SkelMesh = SkeletalMeshComponent->SkeletalMesh;

		const FHitResult& HitResult = GetHitResult(InRayOrigin, InRayDirection);

		if (HitResult.bBlockingHit)
		{
			if (PaintSettings->PaintTool == EClothPaintTool::Brush)
			{
				if (!IsPainting())
				{
					BeginTransaction(LOCTEXT("MeshPaint", "Painting Cloth Property Values"));
					bArePainting = true;
					Adapter->PreEdit();
				}

				const FMeshPaintParameters Parameters = CreatePaintParameters(HitResult, InCameraOrigin, InRayOrigin, InRayDirection, PaintStrength);
				bApplied = MeshPaintHelpers::ApplyPerVertexPaintAction(Adapter.Get(), InCameraOrigin, HitResult.Location, GetBrushSettings(), FPerVertexPaintAction::CreateRaw(this, &FClothPainter::ApplyPropertyPaint, Parameters.InverseBrushToWorldMatrix, PaintSettings->PaintingProperty));
			}
			else if (PaintSettings->PaintTool == EClothPaintTool::Gradient)
			{
				const FMatrix ComponentToWorldMatrix = HitResult.Component->GetComponentTransform().ToMatrixWithScale();
				const FVector ComponentSpaceCameraPosition(ComponentToWorldMatrix.InverseTransformPosition(InCameraOrigin));
				const FVector ComponentSpaceBrushPosition(ComponentToWorldMatrix.InverseTransformPosition(HitResult.Location));
				const float ComponentSpaceBrushRadius = ComponentToWorldMatrix.InverseTransformVector(FVector(PaintSettings->bUseRegularBrushForGradient ? BrushSettings->GetBrushRadius() : 2.0f, 0.0f, 0.0f)).Size();
				const float ComponentSpaceSquaredBrushRadius = ComponentSpaceBrushRadius * ComponentSpaceBrushRadius;

				bApplied = true;
				bArePainting = true;

				TArray<FVector> InRangeVertices = Adapter->SphereIntersectVertices(ComponentSpaceSquaredBrushRadius, ComponentSpaceBrushPosition, ComponentSpaceCameraPosition, GetBrushSettings()->bOnlyFrontFacingTriangles);
				if (InRangeVertices.Num())
				{
					InRangeVertices.Sort([=](const FVector& PointOne, const FVector& PointTwo)
					{
						return (PointOne - ComponentSpaceBrushPosition).SizeSquared() < (PointTwo - ComponentSpaceBrushPosition).SizeSquared();
					});


					InRangeVertices.Sort([=](const FVector& PointOne, const FVector& PointTwo)
					{
						return (PointOne - ComponentSpaceBrushPosition).SizeSquared() < (PointTwo - ComponentSpaceBrushPosition).SizeSquared();
					});

					if (!PaintSettings->bUseRegularBrushForGradient)
					{
						InRangeVertices.SetNum(1);
					}

					TArray<FVector>& GradientPoints = bSelectingFirstPoint ? GradientStartPoints : GradientEndPoints;

					for (const FVector& Vertex : InRangeVertices)
					{
						if (PaintAction == EMeshPaintAction::Erase && GradientPoints.Contains(Vertex))
						{
							GradientPoints.Remove(Vertex);
						}
						else if (PaintAction == EMeshPaintAction::Paint)
						{
							GradientPoints.Add(Vertex);
						}
					}
				}
			}
		}
	}

	return bApplied;
}

void FClothPainter::ApplyGradient()
{
	BeginTransaction(LOCTEXT("MeshPaintGradientApply", "Gradient Property Painting"));
	Adapter->PreEdit();

	// Apply gradient
	const TArray<FVector> Vertices = Adapter->GetMeshVertices();
	for (int32 VertexIndex = 0; VertexIndex < Vertices.Num(); ++VertexIndex)
	{
		const FVector& Vertex = Vertices[VertexIndex];

		// Find distances to closest begin and end gradient vert
		float DistanceToStart = FLT_MAX;
		for (int32 GradientIndex = 0; GradientIndex < GradientStartPoints.Num(); ++GradientIndex)
		{
			const float CurrentDistance = (GradientStartPoints[GradientIndex] - Vertex).SizeSquared();
			if (CurrentDistance < DistanceToStart)
			{
				DistanceToStart = CurrentDistance;
			}
		}

		float DistanceToEnd = FLT_MAX;
		for (int32 GradientIndex = 0; GradientIndex < GradientEndPoints.Num(); ++GradientIndex)
		{
			const float CurrentDistance = (GradientEndPoints[GradientIndex] - Vertex).SizeSquared();
			if (CurrentDistance < DistanceToEnd)
			{
				DistanceToEnd = CurrentDistance;
			}
		}
		
		// Lerp between the two gradient values according to the distances
		const float Value = FMath::LerpStable(PaintSettings->GradientStartValue, PaintSettings->GradientEndValue, DistanceToStart / (DistanceToStart + DistanceToEnd));
		SetPropertyValue(VertexIndex, Value, PaintSettings->PaintingProperty);
	}
	
	EndTransaction();

	Adapter->PostEdit();

	GradientStartPoints.Empty();
	GradientEndPoints.Empty();

	bSelectingFirstPoint = true;
}

void FClothPainter::SetSkeletalMeshComponent(UDebugSkelMeshComponent* InSkeletalMeshComponent)
{
	TSharedPtr<FClothMeshPaintAdapter> Result = MakeShareable(new FClothMeshPaintAdapter());
	Result->Construct(InSkeletalMeshComponent, 0);
	Adapter = Result;

	SkeletalMeshComponent = InSkeletalMeshComponent;

	RefreshClothingAssets();

	if (Widget.IsValid())
	{
		Widget->OnRefresh();
	}
}

void FClothPainter::RefreshClothingAssets()
{
	if(!PaintSettings || !SkeletalMeshComponent)
	{
		return;
	}

	PaintSettings->ClothingAssets.Reset();

	if(USkeletalMesh* Mesh = SkeletalMeshComponent->SkeletalMesh)
	{
		for(UClothingAssetBase* BaseClothingAsset : Mesh->MeshClothingAssets)
		{
			if(UClothingAsset* ActualAsset = Cast<UClothingAsset>(BaseClothingAsset))
			{
				PaintSettings->ClothingAssets.AddUnique(ActualAsset);
			}
		}
	}
}

void FClothPainter::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	IMeshPainter::Tick(ViewportClient, DeltaTime);
	SkeletalMeshComponent->SetVisibleClothProperty((int32)PaintSettings->PaintingProperty);

	if ((bShouldSimulate && SkeletalMeshComponent->bDisableClothSimulation) || (!bShouldSimulate && !SkeletalMeshComponent->bDisableClothSimulation))
	{
		FComponentReregisterContext ReregisterContext(SkeletalMeshComponent);
		SkeletalMeshComponent->ToggleMeshSectionForCloth(SkeletalMeshComponent->SelectedClothingGuidForPainting);
		SkeletalMeshComponent->bDisableClothSimulation = !bShouldSimulate;		
		SkeletalMeshComponent->bShowClothData = !bShouldSimulate;
		ViewportClient->Invalidate();
	}

	
	// We always want up to date CPU skinned verts, so each tick we reinitialize the adapter
	if(Adapter.IsValid())
	{
		Adapter->Initialize();
	}
}

void FClothPainter::FinishPainting()
{
	if (IsPainting() && PaintSettings->PaintTool == EClothPaintTool::Brush)
	{		
		EndTransaction();
		Adapter->PostEdit();
	}

	if(SkeletalMeshComponent)
	{
		FComponentReregisterContext ReregisterContext(SkeletalMeshComponent);

		if(USkeletalMesh* SkelMesh = SkeletalMeshComponent->SkeletalMesh)
		{
			for(UClothingAssetBase* AssetBase : SkelMesh->MeshClothingAssets)
			{
				AssetBase->InvalidateCachedData();
			}
		}
	}

	bArePainting = false;
}

void FClothPainter::Reset()
{	
	bArePainting = false;
	SkeletalMeshComponent->ToggleMeshSectionForCloth(SkeletalMeshComponent->SelectedClothingGuidForPainting);
	SkeletalMeshComponent->SelectedClothingGuidForPainting = FGuid();
}

TSharedPtr<IMeshPaintGeometryAdapter> FClothPainter::GetMeshAdapterForComponent(const UMeshComponent* Component)
{
	if (Component == SkeletalMeshComponent)
	{
		return Adapter;
	}

	return nullptr;
}

void FClothPainter::AddReferencedObjects(FReferenceCollector& Collector)
{	
	Collector.AddReferencedObject(SkeletalMeshComponent);
	Collector.AddReferencedObject(BrushSettings);
	Collector.AddReferencedObject(PaintSettings);
}

UPaintBrushSettings* FClothPainter::GetBrushSettings()
{
	return BrushSettings;
}

UMeshPaintSettings* FClothPainter::GetPainterSettings()
{
	return Cast<UMeshPaintSettings>(PaintSettings);
}

TSharedPtr<class SWidget> FClothPainter::GetWidget()
{
	return Widget;
}

const FHitResult FClothPainter::GetHitResult(const FVector& Origin, const FVector& Direction)
{
	FHitResult HitResult(1.0f);
	const FVector TraceStart(Origin);
	const FVector TraceEnd(Origin + Direction * HALF_WORLD_MAX);

	if (Adapter.IsValid())
	{
		static FName TraceName(TEXT("FClothPainter::GetHitResult"));
		Adapter->LineTraceComponent(HitResult, TraceStart, TraceEnd, FCollisionQueryParams(TraceName, true));
	}
	
	return HitResult;
}

void FClothPainter::Refresh()
{
	RefreshClothingAssets();

	if(Widget.IsValid())
	{
		Widget->OnRefresh();
	}
}

void FClothPainter::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	if (PaintSettings->PaintTool == EClothPaintTool::Brush || (PaintSettings->PaintTool == EClothPaintTool::Gradient && PaintSettings->bUseRegularBrushForGradient))
	{
		RenderInteractors(View, Viewport, PDI, true, ( bShowHiddenVerts && PaintSettings->PaintTool == EClothPaintTool::Brush) ? SDPG_Foreground : SDPG_World);
	}

	const ESceneDepthPriorityGroup DepthPriority = bShowHiddenVerts ? SDPG_Foreground : SDPG_World;
	// Render simulation mesh vertices if not simulating
	if (SkeletalMeshComponent && !bShouldSimulate)
	{
		const FMatrix ComponentToWorldMatrix = SkeletalMeshComponent->GetComponentTransform().ToMatrixWithScale();
		const TArray<FVector>& AllVertices = Adapter->GetMeshVertices();
		for (const FVector& Vertex : AllVertices)
		{
			const FVector WorldPositionVertex = ComponentToWorldMatrix.TransformPosition(Vertex);
			PDI->DrawPoint(WorldPositionVertex, VertexPointColor, VertexPointSize, DepthPriority);
		}
	}

	if (SkeletalMeshComponent && PaintSettings->PaintTool == EClothPaintTool::Gradient && !bShouldSimulate)
	{
		TArray<MeshPaintHelpers::FPaintRay> PaintRays;
		MeshPaintHelpers::RetrieveViewportPaintRays(View, Viewport, PDI, PaintRays);

		const FMatrix ComponentToWorldMatrix = SkeletalMeshComponent->GetComponentTransform().ToMatrixWithScale();

		for (const MeshPaintHelpers::FPaintRay& PaintRay : PaintRays)
		{
			for (const FVector& Vertex : GradientStartPoints)
			{
				const FVector WorldPositionVertex = ComponentToWorldMatrix.TransformPosition(Vertex);
				PDI->DrawPoint(WorldPositionVertex, FLinearColor::Green, VertexPointSize * 2.0f, DepthPriority);
			}

			for (const FVector& Vertex : GradientEndPoints)
			{
				const FVector WorldPositionVertex = ComponentToWorldMatrix.TransformPosition(Vertex);
				PDI->DrawPoint(WorldPositionVertex, FLinearColor::Red, VertexPointSize * 2.0f, DepthPriority);
			}


			const FHitResult& HitResult = GetHitResult(PaintRay.RayStart, PaintRay.RayDirection);
			if (HitResult.Component == SkeletalMeshComponent)
			{
				const FVector ComponentSpaceCameraPosition(ComponentToWorldMatrix.InverseTransformPosition(PaintRay.CameraLocation));
				const FVector ComponentSpaceBrushPosition(ComponentToWorldMatrix.InverseTransformPosition(HitResult.Location));
				const float ComponentSpaceBrushRadius = ComponentToWorldMatrix.InverseTransformVector(FVector(PaintSettings->bUseRegularBrushForGradient ? BrushSettings->GetBrushRadius() : 2.0f, 0.0f, 0.0f)).Size();
				const float ComponentSpaceSquaredBrushRadius = ComponentSpaceBrushRadius * ComponentSpaceBrushRadius;

				// Draw hovered vertex
				TArray<FVector> InRangeVertices = Adapter->SphereIntersectVertices(ComponentSpaceSquaredBrushRadius, ComponentSpaceBrushPosition, ComponentSpaceCameraPosition, GetBrushSettings()->bOnlyFrontFacingTriangles);
				InRangeVertices.Sort([=](const FVector& PointOne, const FVector& PointTwo)
				{
					return (PointOne - ComponentSpaceBrushPosition).SizeSquared() < (PointTwo - ComponentSpaceBrushPosition).SizeSquared();
				});

				
				if (InRangeVertices.Num())
				{
					if (!PaintSettings->bUseRegularBrushForGradient)
					{
						InRangeVertices.SetNum(1);
					}

					for (const FVector& Vertex : InRangeVertices)
					{
						const FVector WorldPositionVertex = ComponentToWorldMatrix.TransformPosition(Vertex);
						PDI->DrawPoint(WorldPositionVertex, bSelectingFirstPoint ? FLinearColor::Green : FLinearColor::Red, VertexPointSize * 2.0f, SDPG_Foreground);
					}					
				}
			}
		}
	}
	
	bShouldSimulate = Viewport->KeyState(EKeys::H);
	bShowHiddenVerts = Viewport->KeyState(EKeys::J);
}

bool FClothPainter::InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent)
{
	bool bHandled = IMeshPainter::InputKey(InViewportClient, InViewport, InKey, InEvent);
	if (InKey == EKeys::Enter)
	{
		if (PaintSettings->PaintTool == EClothPaintTool::Gradient)
		{
			if (bSelectingFirstPoint == true && GradientStartPoints.Num() > 0)
			{
				bSelectingFirstPoint = false;
				bHandled = true;
			}
			else if (GradientEndPoints.Num() > 0)
			{
				// Apply gradient?
				ApplyGradient();
				bHandled = true;
			}
		}
	}

	return bHandled;
}

FMeshPaintParameters FClothPainter::CreatePaintParameters(const FHitResult& HitResult, const FVector& InCameraOrigin, const FVector& InRayOrigin, const FVector& InRayDirection, float PaintStrength)
{
	const float BrushStrength = BrushSettings->BrushStrength *  BrushSettings->BrushStrength * PaintStrength;

	const float BrushRadius = BrushSettings->GetBrushRadius();
	const float BrushDepth = BrushRadius * .5f;

	FVector BrushXAxis, BrushYAxis;
	HitResult.Normal.FindBestAxisVectors(BrushXAxis, BrushYAxis);
	// Display settings
	const float VisualBiasDistance = 0.15f;
	const FVector BrushVisualPosition = HitResult.Location + HitResult.Normal * VisualBiasDistance;

	FMeshPaintParameters Params;
	{
		Params.BrushPosition = HitResult.Location;
		
		Params.SquaredBrushRadius = BrushRadius * BrushRadius;
		Params.BrushRadialFalloffRange = BrushSettings->BrushFalloffAmount * BrushRadius;
		Params.InnerBrushRadius = BrushRadius - Params.BrushRadialFalloffRange;
		Params.BrushDepth = BrushDepth;
		Params.BrushDepthFalloffRange = BrushSettings->BrushFalloffAmount * BrushDepth;
		Params.InnerBrushDepth = BrushDepth - Params.BrushDepthFalloffRange;
		Params.BrushStrength = BrushStrength;
		Params.BrushToWorldMatrix = FMatrix(BrushXAxis, BrushYAxis, Params.BrushNormal, Params.BrushPosition);
		Params.InverseBrushToWorldMatrix = Params.BrushToWorldMatrix.InverseFast();		
		Params.BrushNormal = HitResult.Normal;
	}

	return Params;
}

void FClothPainter::ApplyPropertyPaint(IMeshPaintGeometryAdapter* InAdapter, int32 VertexIndex, FMatrix InverseBrushMatrix, EPaintableClothProperty Property)
{
	FClothMeshPaintAdapter* ClothAdapter = (FClothMeshPaintAdapter*)InAdapter;
	if (ClothAdapter)
	{
		FVector Position;
		Adapter->GetVertexPosition(VertexIndex, Position);
		Position = Adapter->GetComponentToWorldMatrix().TransformPosition(Position);

		float Value = GetPropertyValue(VertexIndex, Property);
		const float BrushRadius = BrushSettings->GetBrushRadius();
		MeshPaintHelpers::ApplyBrushToVertex(Position, InverseBrushMatrix, BrushRadius * BrushRadius, BrushSettings->BrushFalloffAmount, PaintSettings->PaintValue, Value);
		SetPropertyValue(VertexIndex, Value, Property);
	}
}

float FClothPainter::GetPropertyValue(int32 VertexIndex, EPaintableClothProperty Property)
{
	FClothMeshPaintAdapter* ClothAdapter = (FClothMeshPaintAdapter*)Adapter.Get();
	switch (Property)
	{
		case EPaintableClothProperty::MaxDistances:
		{
			return ClothAdapter->GetMaxDistanceValue(VertexIndex);
		}

		case EPaintableClothProperty::BackstopDistances:
		{
			return ClothAdapter->GetBackstopDistanceValue(VertexIndex);
		}

		case EPaintableClothProperty::BackstopRadius:
		{
			return ClothAdapter->GetBackstopRadiusValue(VertexIndex);
		}
	}

	return 0.0f;
}

void FClothPainter::SetPropertyValue(int32 VertexIndex, const float Value, EPaintableClothProperty Property)
{
	FClothMeshPaintAdapter* ClothAdapter = (FClothMeshPaintAdapter*)Adapter.Get();	
	switch (Property)
	{
		case EPaintableClothProperty::MaxDistances:
		{
			ClothAdapter->SetMaxDistanceValue(VertexIndex, Value);
			break;
		}

		case EPaintableClothProperty::BackstopDistances:
		{
			ClothAdapter->SetBackstopDistanceValue(VertexIndex, Value);
			break;
		}

		case EPaintableClothProperty::BackstopRadius:
		{
			ClothAdapter->SetBackstopRadiusValue(VertexIndex, Value);
			break;
		}		
	}
}

void FClothPainter::OnAssetSelectionChanged(UClothingAsset* InNewSelectedAsset, int32 InAssetLod)
{
	TSharedPtr<FClothMeshPaintAdapter> ClothAdapter = StaticCastSharedPtr<FClothMeshPaintAdapter>(Adapter);
	if(ClothAdapter.IsValid() && InNewSelectedAsset && InNewSelectedAsset->IsValidLod(InAssetLod))
	{
		const FGuid NewGuid = InNewSelectedAsset->GetAssetGuid();
		SkeletalMeshComponent->ToggleMeshSectionForCloth(SkeletalMeshComponent->SelectedClothingGuidForPainting);
		SkeletalMeshComponent->ToggleMeshSectionForCloth(NewGuid);

		SkeletalMeshComponent->SelectedClothingGuidForPainting = NewGuid;
		SkeletalMeshComponent->SelectedClothingLodForPainting = InAssetLod;
		SkeletalMeshComponent->RefreshSelectedClothingSkinnedPositions();

		ClothAdapter->SetSelectedClothingAsset(NewGuid, InAssetLod);
	}
}

#undef LOCTEXT_NAMESPACE // "ClothPainter"
