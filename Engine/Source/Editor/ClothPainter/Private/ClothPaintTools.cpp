// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ClothPaintTools.h"
#include "ClothPaintSettings.h"
#include "ClothMeshAdapter.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ScopedTransaction.h"
#include "SceneManagement.h"
#include "Package.h"

#define LOCTEXT_NAMESPACE "ClothTools"

FClothPaintTool_Brush::~FClothPaintTool_Brush()
{
	if(Settings)
	{
		Settings->RemoveFromRoot();
	}
}

FText FClothPaintTool_Brush::GetDisplayName()
{
	return LOCTEXT("ToolName_Brush", "Brush");
}

FPerVertexPaintAction FClothPaintTool_Brush::GetPaintAction(const FMeshPaintParameters& InPaintParams, UClothPainterSettings* InPainterSettings)
{
	return FPerVertexPaintAction::CreateRaw(this, &FClothPaintTool_Brush::PaintAction, InPaintParams.InverseBrushToWorldMatrix, InPainterSettings->PaintingProperty);
}

UObject* FClothPaintTool_Brush::GetSettingsObject()
{
	if(!Settings)
	{
		Settings = DuplicateObject<UClothPaintTool_BrushSettings>(GetMutableDefault<UClothPaintTool_BrushSettings>(), GetTransientPackage());
		Settings->AddToRoot();
	}

	return Settings;
}

void FClothPaintTool_Brush::PaintAction(FPerVertexPaintActionArgs& InArgs, int32 VertexIndex, FMatrix InverseBrushMatrix, EPaintableClothProperty Property)
{
	FClothMeshPaintAdapter* ClothAdapter = (FClothMeshPaintAdapter*)InArgs.Adapter;
	if(ClothAdapter)
	{
		TSharedPtr<FClothPainter> SharedPainter = Painter.Pin();
		if(SharedPainter.IsValid())
		{
			UPaintBrushSettings* BrushSettings = SharedPainter->GetBrushSettings();
			UClothPainterSettings* PaintSettings = Cast<UClothPainterSettings>(SharedPainter->GetPainterSettings());

			FVector Position;
			ClothAdapter->GetVertexPosition(VertexIndex, Position);
			Position = ClothAdapter->GetComponentToWorldMatrix().TransformPosition(Position);

			float Value = SharedPainter->GetPropertyValue(VertexIndex, Property);
			const float BrushRadius = BrushSettings->GetBrushRadius();
			MeshPaintHelpers::ApplyBrushToVertex(Position, InverseBrushMatrix, BrushRadius * BrushRadius, BrushSettings->BrushFalloffAmount, Settings->PaintValue, Value);
			SharedPainter->SetPropertyValue(VertexIndex, Value, Property);
		}
	}
}

FClothPaintTool_Gradient::~FClothPaintTool_Gradient()
{
	if(Settings)
	{
		Settings->RemoveFromRoot();
	}
}

FText FClothPaintTool_Gradient::GetDisplayName()
{
	return LOCTEXT("ToolName_Gradient", "Gradient");
}

bool FClothPaintTool_Gradient::InputKey(IMeshPaintGeometryAdapter* Adapter, FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent)
{
	bool bHandled = false;

	if (InKey == EKeys::Enter)
	{
		if (bSelectingBeginPoints && GradientStartPoints.Num() > 0)
		{
			bSelectingBeginPoints = false;
			bHandled = true;
		}
		else if (GradientEndPoints.Num() > 0)
		{
			// Apply gradient?
			ApplyGradient(Adapter);
			bHandled = true;
		}
	}

	return bHandled;
}

FPerVertexPaintAction FClothPaintTool_Gradient::GetPaintAction(const FMeshPaintParameters& InPaintParams, UClothPainterSettings* InPainterSettings)
{
	return FPerVertexPaintAction::CreateRaw(this, &FClothPaintTool_Gradient::PaintAction, InPaintParams.InverseBrushToWorldMatrix, InPainterSettings->PaintingProperty);
}

bool FClothPaintTool_Gradient::IsPerVertex()
{
	return false;
}

void FClothPaintTool_Gradient::Render(USkeletalMeshComponent* InComponent, IMeshPaintGeometryAdapter* InAdapter, const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	TSharedPtr<FClothPainter> SharedPainter = Painter.Pin();

	if(!SharedPainter.IsValid())
	{
		return;
	}

	const float VertexPointSize = 3.0f;
	const UClothPainterSettings* PaintSettings = CastChecked<UClothPainterSettings>(SharedPainter->GetPainterSettings());
	const UPaintBrushSettings* BrushSettings = SharedPainter->GetBrushSettings();

	TArray<MeshPaintHelpers::FPaintRay> PaintRays;
	MeshPaintHelpers::RetrieveViewportPaintRays(View, Viewport, PDI, PaintRays);
	
	const FMatrix ComponentToWorldMatrix = InComponent->ComponentToWorld.ToMatrixWithScale();
	
	for (const MeshPaintHelpers::FPaintRay& PaintRay : PaintRays)
	{
		for (const FVector& Vertex : GradientStartPoints)
		{
			const FVector WorldPositionVertex = ComponentToWorldMatrix.TransformPosition(Vertex);
			PDI->DrawPoint(WorldPositionVertex, FLinearColor::Green, VertexPointSize * 2.0f, SDPG_World);
		}
	
		for (const FVector& Vertex : GradientEndPoints)
		{
			const FVector WorldPositionVertex = ComponentToWorldMatrix.TransformPosition(Vertex);
			PDI->DrawPoint(WorldPositionVertex, FLinearColor::Red, VertexPointSize * 2.0f, SDPG_World);
		}
	
	
		const FHitResult& HitResult = SharedPainter->GetHitResult(PaintRay.RayStart, PaintRay.RayDirection);
		if (HitResult.Component == InComponent)
		{
			const FVector ComponentSpaceCameraPosition(ComponentToWorldMatrix.InverseTransformPosition(PaintRay.CameraLocation));
			const FVector ComponentSpaceBrushPosition(ComponentToWorldMatrix.InverseTransformPosition(HitResult.Location));
			const float ComponentSpaceBrushRadius = ComponentToWorldMatrix.InverseTransformVector(FVector(Settings->bUseRegularBrush ? BrushSettings->GetBrushRadius() : 2.0f, 0.0f, 0.0f)).Size();
			const float ComponentSpaceSquaredBrushRadius = ComponentSpaceBrushRadius * ComponentSpaceBrushRadius;
	
			// Draw hovered vertex
			TArray<FVector> InRangeVertices = InAdapter->SphereIntersectVertices(ComponentSpaceSquaredBrushRadius, ComponentSpaceBrushPosition, ComponentSpaceCameraPosition, BrushSettings->bOnlyFrontFacingTriangles);
			InRangeVertices.Sort([=](const FVector& PointOne, const FVector& PointTwo)
			{
				return (PointOne - ComponentSpaceBrushPosition).SizeSquared() < (PointTwo - ComponentSpaceBrushPosition).SizeSquared();
			});
	
			
			if (InRangeVertices.Num())
			{
				if (!Settings->bUseRegularBrush)
				{
					InRangeVertices.SetNum(1);
				}
	
				for (const FVector& Vertex : InRangeVertices)
				{
					const FVector WorldPositionVertex = ComponentToWorldMatrix.TransformPosition(Vertex);
					PDI->DrawPoint(WorldPositionVertex, bSelectingBeginPoints ? FLinearColor::Green : FLinearColor::Red, VertexPointSize * 2.0f, SDPG_Foreground);
				}
			}
		}
	}
}

bool FClothPaintTool_Gradient::ShouldRenderInteractors()
{
	return Settings->bUseRegularBrush;
}

UObject* FClothPaintTool_Gradient::GetSettingsObject()
{
	if(!Settings)
	{
		Settings = DuplicateObject<UClothPaintTool_GradientSettings>(GetMutableDefault<UClothPaintTool_GradientSettings>(), GetTransientPackage());
		Settings->AddToRoot();
	}

	return Settings;
}

void FClothPaintTool_Gradient::PaintAction(FPerVertexPaintActionArgs& InArgs, int32 VertexIndex, FMatrix InverseBrushMatrix, EPaintableClothProperty Property)
{
	TSharedPtr<FClothPainter> SharedPainter = Painter.Pin();
	if(!SharedPainter.IsValid())
	{
		return;
	}

	const FHitResult HitResult = InArgs.HitResult;
	UClothPainterSettings* PaintSettings = CastChecked<UClothPainterSettings>(SharedPainter->GetPainterSettings());
	const UPaintBrushSettings* BrushSettings = InArgs.BrushSettings;
	FClothMeshPaintAdapter* Adapter = (FClothMeshPaintAdapter*)InArgs.Adapter;

	const FMatrix ComponentToWorldMatrix = HitResult.Component->ComponentToWorld.ToMatrixWithScale();
	const FVector ComponentSpaceCameraPosition(ComponentToWorldMatrix.InverseTransformPosition(InArgs.CameraPosition));
	const FVector ComponentSpaceBrushPosition(ComponentToWorldMatrix.InverseTransformPosition(HitResult.Location));
	const float ComponentSpaceBrushRadius = ComponentToWorldMatrix.InverseTransformVector(FVector(Settings->bUseRegularBrush ? BrushSettings->GetBrushRadius() : 2.0f, 0.0f, 0.0f)).Size();
	const float ComponentSpaceSquaredBrushRadius = ComponentSpaceBrushRadius * ComponentSpaceBrushRadius;
	
	// Override these flags becuase we're not actually "painting" so the state would be incorrect
	SharedPainter->SetIsPainting(true);
	
	TArray<FVector> InRangeVertices = Adapter->SphereIntersectVertices(ComponentSpaceSquaredBrushRadius, ComponentSpaceBrushPosition, ComponentSpaceCameraPosition, BrushSettings->bOnlyFrontFacingTriangles);
	if (InRangeVertices.Num())
	{
		// Sort based on distance from brush centre
		InRangeVertices.Sort([=](const FVector& PointOne, const FVector& PointTwo)
		{
			return (PointOne - ComponentSpaceBrushPosition).SizeSquared() < (PointTwo - ComponentSpaceBrushPosition).SizeSquared();
		});
	
		// Selection mode
		if (!Settings->bUseRegularBrush)
		{
			InRangeVertices.SetNum(1);
		}
	
		TArray<FVector>& GradientPoints = bSelectingBeginPoints ? GradientStartPoints : GradientEndPoints;
	
		// Add selected verts to current list
		for (const FVector& Vertex : InRangeVertices)
		{
			if (InArgs.Action == EMeshPaintAction::Erase && GradientPoints.Contains(Vertex))
			{
				GradientPoints.Remove(Vertex);
			}
			else if (InArgs.Action == EMeshPaintAction::Paint)
			{
				GradientPoints.AddUnique(Vertex);
			}
		}
	}
}

void FClothPaintTool_Gradient::ApplyGradient(IMeshPaintGeometryAdapter* InAdapter)
{
	TSharedPtr<FClothPainter> SharedPainter = Painter.Pin();
	if(!SharedPainter.IsValid())
	{
		return;
	}

	if(!InAdapter)
	{
		return;
	}

	FClothMeshPaintAdapter* Adapter = (FClothMeshPaintAdapter*)InAdapter;

	{
		FScopedTransaction Transaction(LOCTEXT("ApplyGradientTransaction", "Apply gradient"));

		Adapter->PreEdit();

		const TArray<FVector> Verts = Adapter->GetMeshVertices();

		const int32 NumVerts = Verts.Num();
		for(int32 VertIndex = 0; VertIndex < NumVerts; ++VertIndex)
		{
			const FVector& Vert = Verts[VertIndex];

			// Get distances
			// TODO: Look into surface distance instead of 3D distance? May be necessary for some complex shapes
			float DistanceToStartSq = MAX_flt;
			for(FVector& StartPoint : GradientStartPoints)
			{
				const float DistanceSq = (StartPoint - Vert).SizeSquared();
				if(DistanceSq < DistanceToStartSq)
				{
					DistanceToStartSq = DistanceSq;
				}
			}

			float DistanceToEndSq = MAX_flt;
			for(FVector& EndPoint : GradientEndPoints)
			{
				const float DistanceSq = (EndPoint - Vert).SizeSquared();
				if(DistanceSq < DistanceToEndSq)
				{
					DistanceToEndSq = DistanceSq;
				}
			}

			// Apply to the mesh
			UClothPainterSettings* PaintSettings = CastChecked<UClothPainterSettings>(SharedPainter->GetPainterSettings());
			const float Value = FMath::LerpStable(Settings->GradientStartValue, Settings->GradientEndValue, DistanceToStartSq / (DistanceToStartSq + DistanceToEndSq));
			SharedPainter->SetPropertyValue(VertIndex, Value, PaintSettings->PaintingProperty);
		}
		
		// Finished edit
		Adapter->PostEdit();
	}

	// Empty the point lists as the operation is complete
	GradientStartPoints.Reset();
	GradientEndPoints.Reset();

	// Move back to selecting begin points
	bSelectingBeginPoints = true;
}

FPerVertexPaintAction FClothPaintTool_Smooth::GetPaintAction(const FMeshPaintParameters& InPaintParams, UClothPainterSettings* InPainterSettings)
{
	return FPerVertexPaintAction::CreateRaw(this, &FClothPaintTool_Smooth::PaintAction, InPaintParams.InverseBrushToWorldMatrix, InPainterSettings->PaintingProperty);
}

FText FClothPaintTool_Smooth::GetDisplayName()
{
	return LOCTEXT("ToolName_Smooth", "Smooth");
}

UObject* FClothPaintTool_Smooth::GetSettingsObject()
{
	return nullptr;
}

void FClothPaintTool_Smooth::PaintAction(FPerVertexPaintActionArgs& InArgs, int32 VertexIndex, FMatrix InverseBrushMatrix, EPaintableClothProperty Property)
{

}

#undef LOCTEXT_NAMESPACE
