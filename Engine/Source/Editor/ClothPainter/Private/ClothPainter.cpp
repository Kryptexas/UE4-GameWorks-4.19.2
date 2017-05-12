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
#include "ClothPaintTools.h"

#define LOCTEXT_NAMESPACE "ClothPainter"

FClothPainter::FClothPainter()
	: IMeshPainter()
	, SkeletalMeshComponent(nullptr)
	, PaintSettings(nullptr)
	, BrushSettings(nullptr)
{
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

	Tools.Add(MakeShared<FClothPaintTool_Brush>(AsShared()));
	Tools.Add(MakeShared<FClothPaintTool_Gradient>(AsShared()));

	SelectedTool = Tools[0];

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
			// Generic per-vertex painting operations
			if(!IsPainting())
				{
					BeginTransaction(LOCTEXT("MeshPaint", "Painting Cloth Property Values"));
					bArePainting = true;
					Adapter->PreEdit();
				}

				const FMeshPaintParameters Parameters = CreatePaintParameters(HitResult, InCameraOrigin, InRayOrigin, InRayDirection, PaintStrength);

			FPerVertexPaintActionArgs Args;
			Args.Adapter = Adapter.Get();
			Args.CameraPosition = InCameraOrigin;
			Args.HitResult = HitResult;
			Args.BrushSettings = GetBrushSettings();
			Args.Action = PaintAction;

			if(SelectedTool->IsPerVertex())
					{
				bApplied = MeshPaintHelpers::ApplyPerVertexPaintAction(Args, GetPaintAction(Parameters));
					}
			else
					{
				bApplied = true;
				GetPaintAction(Parameters).ExecuteIfBound(Args, INDEX_NONE);
			}
		}
	}

	return bApplied;
}

FPerVertexPaintAction FClothPainter::GetPaintAction(const FMeshPaintParameters& InPaintParams)
{
	if(SelectedTool.IsValid())
	{
		return SelectedTool->GetPaintAction(InPaintParams, PaintSettings);
	}

	return FPerVertexPaintAction();
}

void FClothPainter::SetTool(TSharedPtr<FClothPaintToolBase> InTool)
{
	if(Tools.Contains(InTool))
			{
		SelectedTool = InTool;
			}
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
	if (IsPainting())
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
		Adapter->LineTraceComponent(HitResult, TraceStart, TraceEnd, FCollisionQueryParams(SCENE_QUERY_STAT(FClothPainter_GetHitResult), true));
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
	if(SelectedTool.IsValid() && SelectedTool->ShouldRenderInteractors())
	{
		RenderInteractors(View, Viewport, PDI, true, SDPG_Foreground);
	}

	const ESceneDepthPriorityGroup DepthPriority = bShowHiddenVerts ? SDPG_Foreground : SDPG_World;
	// Render simulation mesh vertices if not simulating
	if(SkeletalMeshComponent)
	{
		if(!bShouldSimulate)
	{
			const FMatrix ComponentToWorldMatrix = SkeletalMeshComponent->GetComponentTransform().ToMatrixWithScale();
		const TArray<FVector>& AllVertices = Adapter->GetMeshVertices();
			for(const FVector& Vertex : AllVertices)
		{
			const FVector WorldPositionVertex = ComponentToWorldMatrix.TransformPosition(Vertex);
			PDI->DrawPoint(WorldPositionVertex, VertexPointColor, VertexPointSize, DepthPriority);
		}

			if(SelectedTool.IsValid())
					{
				SelectedTool->Render(SkeletalMeshComponent, Adapter.Get(), View, Viewport, PDI);
			}
		}
	}
	
	bShouldSimulate = Viewport->KeyState(EKeys::H);
	bShowHiddenVerts = Viewport->KeyState(EKeys::J);
}

bool FClothPainter::InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent)
{
	bool bHandled = IMeshPainter::InputKey(InViewportClient, InViewport, InKey, InEvent);

	if(SelectedTool.IsValid())
		{
		bHandled |= SelectedTool->InputKey(Adapter.Get(), InViewportClient, InViewport, InKey, InEvent);
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
