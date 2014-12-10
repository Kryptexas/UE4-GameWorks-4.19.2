// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LandscapeEditorPrivatePCH.h"

#include "ObjectTools.h"
#include "LandscapeEdMode.h"
#include "ScopedTransaction.h"
#include "LandscapeEdit.h"
#include "LandscapeRender.h"
#include "Landscape.h"
#include "LandscapeHeightfieldCollisionComponent.h"

#include "LevelUtils.h"
#include "Materials/MaterialInstanceDynamic.h"

// 
// FLandscapeBrush
//
bool GInLandscapeBrushTransaction = false;

void FLandscapeBrush::BeginStroke(float LandscapeX, float LandscapeY, FLandscapeTool* CurrentTool)
{
	if (!GInLandscapeBrushTransaction)
	{
		GEditor->BeginTransaction(FText::Format(NSLOCTEXT("UnrealEd", "LandscapeMode_EditTransaction", "Landscape Editing: {0}"), CurrentTool->GetDisplayName()));
		GInLandscapeBrushTransaction = true;
	}
}

void FLandscapeBrush::EndStroke()
{
	if (ensure(GInLandscapeBrushTransaction))
	{
		GEditor->EndTransaction();
		GInLandscapeBrushTransaction = false;
	}
}

// 
// FLandscapeBrushCircle
//

class FLandscapeBrushCircle : public FLandscapeBrush
{
	TSet<ULandscapeComponent*> BrushMaterialComponents;
	TArray<UMaterialInstanceDynamic*> BrushMaterialFreeInstances;

protected:
	FVector2D LastMousePosition;
	UMaterialInterface* BrushMaterial;
	TMap<ULandscapeComponent*, UMaterialInstanceDynamic*> BrushMaterialInstanceMap;

	virtual float CalculateFalloff(float Distance, float Radius, float Falloff) = 0;

	/** Protected so that only subclasses can create instances of this class. */
	FLandscapeBrushCircle(FEdModeLandscape* InEdMode, UMaterialInterface* InBrushMaterial)
		: LastMousePosition(0, 0)
		, BrushMaterial(NULL)
		, EdMode(InEdMode)
	{
		BrushMaterial = InBrushMaterial; //UMaterialInstanceDynamic::Create(InBrushMaterial, NULL);
	}

public:
	FEdModeLandscape* EdMode;

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(BrushMaterial);

		// Allow any currently unused material instances to be GC'd
		BrushMaterialFreeInstances.Empty();

		Collector.AddReferencedObjects(BrushMaterialInstanceMap);
	}

	virtual void LeaveBrush() override
	{
		for (TSet<ULandscapeComponent*>::TIterator It(BrushMaterialComponents); It; ++It)
		{
			if ((*It)->EditToolRenderData != NULL)
			{
				(*It)->EditToolRenderData->Update(NULL);
			}
		}
		TArray<UMaterialInstanceDynamic*> BrushMaterialInstances;
		BrushMaterialInstanceMap.GenerateValueArray(BrushMaterialInstances);
		BrushMaterialFreeInstances += BrushMaterialInstances;
		BrushMaterialInstanceMap.Empty();
		BrushMaterialComponents.Empty();
	}

	virtual void BeginStroke(float LandscapeX, float LandscapeY, FLandscapeTool* CurrentTool) override
	{
		FLandscapeBrush::BeginStroke(LandscapeX, LandscapeY, CurrentTool);
		LastMousePosition = FVector2D(LandscapeX, LandscapeY);
	}

	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override
	{
		ULandscapeInfo* LandscapeInfo = EdMode->CurrentToolTarget.LandscapeInfo.Get();
		ALandscapeProxy* Proxy = LandscapeInfo ? LandscapeInfo->GetLandscapeProxy() : NULL;
		if (Proxy)
		{
			const float ScaleXY = LandscapeInfo->DrawScale.X;
			float Radius = (1.f - EdMode->UISettings->BrushFalloff) * EdMode->UISettings->BrushRadius / ScaleXY;
			float Falloff = EdMode->UISettings->BrushFalloff * EdMode->UISettings->BrushRadius / ScaleXY;

			// Set brush material.
			int32 X1 = FMath::FloorToInt(LastMousePosition.X - (Radius + Falloff));
			int32 Y1 = FMath::FloorToInt(LastMousePosition.Y - (Radius + Falloff));
			int32 X2 = FMath::CeilToInt(LastMousePosition.X + (Radius + Falloff));
			int32 Y2 = FMath::CeilToInt(LastMousePosition.Y + (Radius + Falloff));

			TSet<ULandscapeComponent*> NewComponents;

			if (!ViewportClient->IsMovingCamera())
			{
				LandscapeInfo->GetComponentsInRegion(X1, Y1, X2, Y2, NewComponents);
			}

			// Remove the material from any old components that are no longer in the region
			TSet<ULandscapeComponent*> RemovedComponents = BrushMaterialComponents.Difference(NewComponents);
			for (ULandscapeComponent* RemovedComponent : RemovedComponents)
			{
				BrushMaterialFreeInstances.Push(BrushMaterialInstanceMap.FindAndRemoveChecked(RemovedComponent));

				if (ensure(RemovedComponent->EditToolRenderData != NULL))
				{
					RemovedComponent->EditToolRenderData->Update(NULL);
				}
			}

			// Set brush material for components in new region
			TSet<ULandscapeComponent*> AddedComponents = NewComponents.Difference(BrushMaterialComponents);
			for (ULandscapeComponent* AddedComponent : AddedComponents)
			{
				if (ensure(AddedComponent->EditToolRenderData != NULL))
				{
					UMaterialInstanceDynamic* BrushMaterialInstance = NULL;
					if (BrushMaterialFreeInstances.Num() > 0)
					{
						BrushMaterialInstance = BrushMaterialFreeInstances.Pop();
					}
					else
					{
						BrushMaterialInstance = UMaterialInstanceDynamic::Create(BrushMaterial, NULL);
					}
					BrushMaterialInstanceMap.Add(AddedComponent, BrushMaterialInstance);
					AddedComponent->EditToolRenderData->Update(BrushMaterialInstance);
				}
			}

			BrushMaterialComponents = MoveTemp(NewComponents);

			// Set params for brush material.
			FVector WorldLocation = Proxy->LandscapeActorToWorld().TransformPosition(FVector(LastMousePosition.X, LastMousePosition.Y, 0));

			for (auto It = BrushMaterialInstanceMap.CreateConstIterator(); It; ++It)
			{
				// Painting can cause the EditToolRenderData to be destructed, so update it if necessary
				if (!AddedComponents.Contains(It.Key()) && ensure(It.Key()->EditToolRenderData != NULL))
				{
					if (It.Key()->EditToolRenderData->ToolMaterial == NULL)
					{
						It.Key()->EditToolRenderData->Update(It.Value());
					}
				}

				It.Value()->SetScalarParameterValue(FName(TEXT("LocalRadius")), Radius);
				It.Value()->SetScalarParameterValue(FName(TEXT("LocalFalloff")), Falloff);
				It.Value()->SetVectorParameterValue(FName(TEXT("WorldPosition")), FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z, ScaleXY));

				bool bCanPaint = true;

				const ULandscapeComponent* Component = It.Key();
				const ALandscapeProxy* LandscapeProxy = Component->GetLandscapeProxy();
				const ULandscapeLayerInfoObject* LayerInfo = EdMode->CurrentToolTarget.LayerInfo.Get();

				if (EdMode->CurrentToolTarget.TargetType == ELandscapeToolTargetType::Weightmap &&
					EdMode->UISettings->PaintingRestriction != ELandscapeLayerPaintingRestriction::None)
				{
					bool bExisting = Component->WeightmapLayerAllocations.ContainsByPredicate([LayerInfo](const FWeightmapLayerAllocationInfo& Allocation){ return Allocation.LayerInfo == LayerInfo; });
					if (!bExisting)
					{
						if (EdMode->UISettings->PaintingRestriction == ELandscapeLayerPaintingRestriction::ExistingOnly ||
							(EdMode->UISettings->PaintingRestriction == ELandscapeLayerPaintingRestriction::UseMaxLayers &&
							LandscapeProxy->MaxPaintedLayersPerComponent > 0 && Component->WeightmapLayerAllocations.Num() >= LandscapeProxy->MaxPaintedLayersPerComponent))
						{
							bCanPaint = false;
						}
					}
				}

				It.Value()->SetScalarParameterValue("CanPaint", bCanPaint ? 1.0f : 0.0f);
			}
		}
	}

	virtual void MouseMove(float LandscapeX, float LandscapeY) override
	{
		LastMousePosition = FVector2D(LandscapeX, LandscapeY);
	}

	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override
	{
		bool bUpdate = false;
		return bUpdate;
	}

	virtual bool ApplyBrush(const TArray<FLandscapeToolMousePosition>& MousePositions, TMap<FIntPoint, float>& OutBrush, int32& X1, int32& Y1, int32& X2, int32& Y2) override
	{
		ULandscapeInfo* LandscapeInfo = EdMode->CurrentToolTarget.LandscapeInfo.Get();
		if (LandscapeInfo)
		{
			float ScaleXY = LandscapeInfo->DrawScale.X;
			float Radius = (1.f - EdMode->UISettings->BrushFalloff) * EdMode->UISettings->BrushRadius / ScaleXY;
			float Falloff = EdMode->UISettings->BrushFalloff * EdMode->UISettings->BrushRadius / ScaleXY;

			bool bHasOutput = false;
			for (int32 MoveIdx = 0; MoveIdx < MousePositions.Num(); MoveIdx++)
			{
				float MousePositionX = MousePositions[MoveIdx].PositionX;
				float MousePositionY = MousePositions[MoveIdx].PositionY;

				int32 ThisMoveX1 = FMath::FloorToInt(MousePositionX - (Radius + Falloff));
				int32 ThisMoveY1 = FMath::FloorToInt(MousePositionY - (Radius + Falloff));
				int32 ThisMoveX2 = FMath::CeilToInt(MousePositionX + (Radius + Falloff));
				int32 ThisMoveY2 = FMath::CeilToInt(MousePositionY + (Radius + Falloff));

				if (MoveIdx == 0)
				{
					X1 = ThisMoveX1;
					Y1 = ThisMoveY1;
					X2 = ThisMoveX2;
					Y2 = ThisMoveY2;
				}
				else
				{
					X1 = FMath::Min<int32>(ThisMoveX1, X1);
					Y1 = FMath::Min<int32>(ThisMoveY1, Y1);
					X2 = FMath::Max<int32>(ThisMoveX2, X2);
					Y2 = FMath::Max<int32>(ThisMoveY2, Y2);
				}

				for (int32 Y = ThisMoveY1; Y <= ThisMoveY2; Y++)
				{
					for (int32 X = ThisMoveX1; X <= ThisMoveX2; X++)
					{
						FIntPoint VertexKey(X, Y);

						float* PrevAmount = OutBrush.Find(VertexKey);
						if (!PrevAmount || *PrevAmount < 1.f)
						{
							// Distance from mouse
							float MouseDist = FMath::Sqrt(FMath::Square(MousePositionX - (float)X) + FMath::Square(MousePositionY - (float)Y));

							float PaintAmount = CalculateFalloff(MouseDist, Radius, Falloff);

							if (PaintAmount > 0.f)
							{
								if (EdMode->CurrentTool && EdMode->CurrentTool->GetToolType() != FLandscapeTool::TT_Mask
									&& EdMode->UISettings->bUseSelectedRegion)
								{
									float MaskValue = LandscapeInfo->SelectedRegion.FindRef(VertexKey);
									if (EdMode->UISettings->bUseNegativeMask)
									{
										MaskValue = 1.f - MaskValue;
									}
									PaintAmount *= MaskValue;
								}

								if (!PrevAmount || *PrevAmount < PaintAmount)
								{
									// Set the brush value for this vertex
									OutBrush.Add(VertexKey, PaintAmount);
									bHasOutput = true;
								}
							}
						}
					}
				}
			}

			return bHasOutput;
		}
		return false;
	}
};

// 
// FLandscapeBrushComponent
//

class FLandscapeBrushComponent : public FLandscapeBrush
{
	TSet<ULandscapeComponent*> BrushMaterialComponents;

	virtual const TCHAR* GetBrushName() override { return TEXT("Component"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_Component", "Component"); };

protected:
	FVector2D LastMousePosition;
	UMaterial* BrushMaterial;
public:
	FEdModeLandscape* EdMode;

	FLandscapeBrushComponent(FEdModeLandscape* InEdMode)
		: BrushMaterial(NULL)
		, EdMode(InEdMode)
	{
		BrushMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorLandscapeResources/SelectBrushMaterial.SelectBrushMaterial"), NULL, LOAD_None, NULL);
	}

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(BrushMaterial);
	}

	virtual EBrushType GetBrushType() override { return BT_Component; }

	virtual void LeaveBrush() override
	{
		for (TSet<ULandscapeComponent*>::TIterator It(BrushMaterialComponents); It; ++It)
		{
			if ((*It)->EditToolRenderData != NULL)
			{
				(*It)->EditToolRenderData->Update(NULL);
			}
		}
		BrushMaterialComponents.Empty();
	}

	virtual void BeginStroke(float LandscapeX, float LandscapeY, FLandscapeTool* CurrentTool) override
	{
		FLandscapeBrush::BeginStroke(LandscapeX, LandscapeY, CurrentTool);
		LastMousePosition = FVector2D(LandscapeX, LandscapeY);
	}

	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override
	{
		TSet<ULandscapeComponent*> NewComponents;

		if (!ViewportClient->IsMovingCamera())
		{
			ULandscapeInfo* LandscapeInfo = EdMode->CurrentToolTarget.LandscapeInfo.Get();
			if (LandscapeInfo && LandscapeInfo->ComponentSizeQuads > 0)
			{
				int32 X = FMath::FloorToInt(LastMousePosition.X);
				int32 Y = FMath::FloorToInt(LastMousePosition.Y);

				int32 ComponentIndexX = (X >= 0.f) ? X / LandscapeInfo->ComponentSizeQuads : (X + 1) / LandscapeInfo->ComponentSizeQuads - 1;
				int32 ComponentIndexY = (Y >= 0.f) ? Y / LandscapeInfo->ComponentSizeQuads : (Y + 1) / LandscapeInfo->ComponentSizeQuads - 1;
				int32 BrushSize = FMath::Max(EdMode->UISettings->BrushComponentSize - 1, 0);
				for (int32 YIndex = -(BrushSize >> 1); YIndex <= (BrushSize >> 1) + (BrushSize % 2); ++YIndex)
				{
					for (int32 XIndex = -(BrushSize >> 1); XIndex <= (BrushSize >> 1) + (BrushSize % 2); ++XIndex)
					{
						ULandscapeComponent* Component = LandscapeInfo->XYtoComponentMap.FindRef(FIntPoint((ComponentIndexX + XIndex), (ComponentIndexY + YIndex)));
						if (Component && FLevelUtils::IsLevelVisible(Component->GetLandscapeProxy()->GetLevel()))
						{
							// For MoveToLevel
							if (EdMode->CurrentTool->GetToolName() == FName("MoveToLevel"))
							{
								if (Component->GetLandscapeProxy() && !Component->GetLandscapeProxy()->GetLevel()->IsCurrentLevel())
								{
									NewComponents.Add(Component);
								}
							}
							else
							{
								NewComponents.Add(Component);
							}
						}
					}
				}

				// Set brush material for components in new region
				for (ULandscapeComponent* NewComponent : NewComponents)
				{
					if (NewComponent->EditToolRenderData != NULL)
					{
						NewComponent->EditToolRenderData->Update(BrushMaterial);
					}
				}
			}
		}

		// Remove the material from any old components that are no longer in the region
		TSet<ULandscapeComponent*> RemovedComponents = BrushMaterialComponents.Difference(NewComponents);
		for (ULandscapeComponent* RemovedComponent : RemovedComponents)
		{
			if (RemovedComponent->EditToolRenderData != NULL)
			{
				RemovedComponent->EditToolRenderData->Update(NULL);
			}
		}

		BrushMaterialComponents = MoveTemp(NewComponents);
	}

	virtual void MouseMove(float LandscapeX, float LandscapeY) override
	{
		LastMousePosition = FVector2D(LandscapeX, LandscapeY);
	}

	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override
	{
		bool bUpdate = false;


		return bUpdate;
	}

	virtual bool ApplyBrush(const TArray<FLandscapeToolMousePosition>& MousePositions, TMap<FIntPoint, float>& OutBrush, int32& X1, int32& Y1, int32& X2, int32& Y2) override
	{
		// Selection Brush only works for 
		ULandscapeInfo* LandscapeInfo = EdMode->CurrentToolTarget.LandscapeInfo.Get();
		X1 = INT_MAX;
		Y1 = INT_MAX;
		X2 = INT_MIN;
		Y2 = INT_MIN;

		// Check for add component...
		if (EdMode->LandscapeRenderAddCollision)
		{
			// Apply Brush size..
			int32 X = FMath::FloorToInt(LastMousePosition.X);
			int32 Y = FMath::FloorToInt(LastMousePosition.Y);

			int32 ComponentIndexX = (X >= 0.f) ? X / LandscapeInfo->ComponentSizeQuads : (X + 1) / LandscapeInfo->ComponentSizeQuads - 1;
			int32 ComponentIndexY = (Y >= 0.f) ? Y / LandscapeInfo->ComponentSizeQuads : (Y + 1) / LandscapeInfo->ComponentSizeQuads - 1;

			int32 BrushSize = FMath::Max(EdMode->UISettings->BrushComponentSize - 1, 0);

			X1 = (ComponentIndexX - (BrushSize >> 1)) * LandscapeInfo->ComponentSizeQuads;
			X2 = (ComponentIndexX + (BrushSize >> 1) + (BrushSize % 2) + 1) * LandscapeInfo->ComponentSizeQuads;
			Y1 = (ComponentIndexY - (BrushSize >> 1)) * LandscapeInfo->ComponentSizeQuads;
			Y2 = (ComponentIndexY + (BrushSize >> 1) + (BrushSize % 2) + 1) * LandscapeInfo->ComponentSizeQuads;
		}
		else
		{
			// Get extent for all components
			for (TSet<ULandscapeComponent*>::TIterator It(BrushMaterialComponents); It; ++It)
			{
				if (*It)
				{
					(*It)->GetComponentExtent(X1, Y1, X2, Y2);
				}
			}
		}

		// Should not be possible...
		//check(X1 <= X2 && Y1 <= Y2);

		for (int32 Y = Y1; Y <= Y2; Y++)
		{
			for (int32 X = X1; X <= X2; X++)
			{
				FIntPoint VertexKey = ALandscape::MakeKey(X, Y);

				float PaintAmount = 1.0f;
				if (EdMode->CurrentTool && EdMode->CurrentTool->GetToolType() != FLandscapeTool::TT_Mask
					&& EdMode->UISettings->bUseSelectedRegion)
				{
					float MaskValue = LandscapeInfo->SelectedRegion.FindRef(VertexKey);
					if (EdMode->UISettings->bUseNegativeMask)
					{
						MaskValue = 1.f - MaskValue;
					}
					PaintAmount *= MaskValue;
				}

				// Set the brush value for this vertex
				OutBrush.Add(VertexKey, PaintAmount);
			}
		}

		return (X1 <= X2 && Y1 <= Y2);
	}
};

// 
// FLandscapeBrushGizmo
//

class FLandscapeBrushGizmo : public FLandscapeBrush
{
	TSet<ULandscapeComponent*> BrushMaterialComponents;

	const TCHAR* GetBrushName() override { return TEXT("Gizmo"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_Gizmo", "Gizmo"); };

protected:
	FVector2D LastMousePosition;
	UMaterialInstanceDynamic* BrushMaterial;
public:
	FEdModeLandscape* EdMode;

	FLandscapeBrushGizmo(FEdModeLandscape* InEdMode)
		: BrushMaterial(NULL)
		, EdMode(InEdMode)
	{
		UMaterialInstanceConstant* GizmoMaterial = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/MaskBrushMaterial_Gizmo.MaskBrushMaterial_Gizmo"), NULL, LOAD_None, NULL);
		BrushMaterial = UMaterialInstanceDynamic::Create(GizmoMaterial, NULL);
	}

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(BrushMaterial);
	}

	virtual EBrushType GetBrushType() override { return BT_Gizmo; }

	virtual void LeaveBrush() override
	{
		for (TSet<ULandscapeComponent*>::TIterator It(BrushMaterialComponents); It; ++It)
		{
			if ((*It)->EditToolRenderData != NULL)
			{
				(*It)->EditToolRenderData->Update(NULL);
			}
		}
		BrushMaterialComponents.Empty();
	}

	virtual void BeginStroke(float LandscapeX, float LandscapeY, FLandscapeTool* CurrentTool) override
	{
		FLandscapeBrush::BeginStroke(LandscapeX, LandscapeY, CurrentTool);
		LastMousePosition = FVector2D(LandscapeX, LandscapeY);
	}

	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override
	{
		if (GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo || GLandscapeEditRenderMode & ELandscapeEditRenderMode::Select)
		{
			ALandscapeGizmoActiveActor* Gizmo = EdMode->CurrentGizmoActor.Get();

			if (Gizmo && Gizmo->TargetLandscapeInfo && (Gizmo->TargetLandscapeInfo == EdMode->CurrentToolTarget.LandscapeInfo.Get()) && Gizmo->GizmoTexture && Gizmo->GetRootComponent())
			{
				ULandscapeInfo* LandscapeInfo = Gizmo->TargetLandscapeInfo;
				if (LandscapeInfo && LandscapeInfo->GetLandscapeProxy())
				{
					float ScaleXY = LandscapeInfo->DrawScale.X;
					FMatrix LToW = LandscapeInfo->GetLandscapeProxy()->LandscapeActorToWorld().ToMatrixWithScale();
					FMatrix WToL = LToW.InverseFast();

					UTexture2D* DataTexture = Gizmo->GizmoTexture;
					int32 MinX = MAX_int32, MaxX = MIN_int32, MinY = MAX_int32, MaxY = MIN_int32;
					FVector LocalPos[4];
					//FMatrix WorldToLocal = Proxy->LocalToWorld().Inverse();
					for (int32 i = 0; i < 4; ++i)
					{
						//LocalPos[i] = WorldToLocal.TransformPosition(Gizmo->FrustumVerts[i]);
						LocalPos[i] = WToL.TransformPosition(Gizmo->FrustumVerts[i]);
						MinX = FMath::Min(MinX, (int32)LocalPos[i].X);
						MinY = FMath::Min(MinY, (int32)LocalPos[i].Y);
						MaxX = FMath::Max(MaxX, (int32)LocalPos[i].X);
						MaxY = FMath::Max(MaxY, (int32)LocalPos[i].Y);
					}

					TSet<ULandscapeComponent*> NewComponents;
					LandscapeInfo->GetComponentsInRegion(MinX, MinY, MaxX, MaxY, NewComponents);

					float SquaredScaleXY = FMath::Square(ScaleXY);
					FLinearColor AlphaScaleBias(
						SquaredScaleXY / (Gizmo->GetWidth() * DataTexture->GetSizeX()),
						SquaredScaleXY / (Gizmo->GetHeight() * DataTexture->GetSizeY()),
						Gizmo->TextureScale.X,
						Gizmo->TextureScale.Y
						);
					BrushMaterial->SetVectorParameterValue(FName(TEXT("AlphaScaleBias")), AlphaScaleBias);

					float Angle = (-EdMode->CurrentGizmoActor->GetActorRotation().Euler().Z) * PI / 180.f;
					FLinearColor LandscapeLocation(EdMode->CurrentGizmoActor->GetActorLocation().X, EdMode->CurrentGizmoActor->GetActorLocation().Y, EdMode->CurrentGizmoActor->GetActorLocation().Z, Angle);
					BrushMaterial->SetVectorParameterValue(FName(TEXT("LandscapeLocation")), LandscapeLocation);
					BrushMaterial->SetTextureParameterValue(FName(TEXT("AlphaTexture")), DataTexture);

					// Set brush material for components in new region
					for (TSet<ULandscapeComponent*>::TIterator It(NewComponents); It; ++It)
					{
						if ((*It)->EditToolRenderData != NULL)
						{
							(*It)->EditToolRenderData->UpdateGizmo((Gizmo->DataType != LGT_None) && (GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo) ? BrushMaterial : NULL);
						}
					}

					// Remove the material from any old components that are no longer in the region
					TSet<ULandscapeComponent*> RemovedComponents = BrushMaterialComponents.Difference(NewComponents);
					for (TSet<ULandscapeComponent*>::TIterator It(RemovedComponents); It; ++It)
					{
						if ((*It)->EditToolRenderData != NULL)
						{
							(*It)->EditToolRenderData->UpdateGizmo(NULL);
						}
					}

					BrushMaterialComponents = NewComponents;
				}
			}
		}
	}

	virtual void MouseMove(float LandscapeX, float LandscapeY) override
	{
		LastMousePosition = FVector2D(LandscapeX, LandscapeY);
	}

	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override
	{
		bool bUpdate = false;
		return bUpdate;
	}

	virtual bool ApplyBrush(const TArray<FLandscapeToolMousePosition>& MousePositions, TMap<FIntPoint, float>& OutBrush, int32& X1, int32& Y1, int32& X2, int32& Y2) override
	{
		// Selection Brush only works for 
		ALandscapeGizmoActiveActor* Gizmo = EdMode->CurrentGizmoActor.Get();
		ULandscapeInfo* LandscapeInfo = EdMode->CurrentToolTarget.LandscapeInfo.Get();

		if (Gizmo && Gizmo->GetRootComponent())
		{
			Gizmo->TargetLandscapeInfo = LandscapeInfo;
			float ScaleXY = LandscapeInfo->DrawScale.X;

			X1 = INT_MAX;
			Y1 = INT_MAX;
			X2 = INT_MIN;
			Y2 = INT_MIN;

			// Get extent for all components
			for (TSet<ULandscapeComponent*>::TIterator It(BrushMaterialComponents); It; ++It)
			{
				if (*It)
				{
					(*It)->GetComponentExtent(X1, Y1, X2, Y2);
				}
			}

			// Should not be possible...
			//check(X1 <= X2 && Y1 <= Y2);

			//FMatrix LandscapeToGizmoLocal = Landscape->LocalToWorld() * Gizmo->WorldToLocal();
			const float LW = Gizmo->GetWidth() / (2 * ScaleXY);
			const float LH = Gizmo->GetHeight() / (2 * ScaleXY);

			FMatrix WToL = LandscapeInfo->GetLandscapeProxy()->LandscapeActorToWorld().ToMatrixWithScale().InverseFast();
			FVector BaseLocation = WToL.TransformPosition(Gizmo->GetActorLocation());
			FMatrix LandscapeToGizmoLocal =
				(FTranslationMatrix(FVector(-LW + 0.5, -LH + 0.5, 0)) * FRotationTranslationMatrix(FRotator(0, Gizmo->GetActorRotation().Yaw, 0), FVector(BaseLocation.X, BaseLocation.Y, 0))).InverseFast();

			float W = Gizmo->GetWidth() / ScaleXY; //Gizmo->GetWidth() / (Gizmo->DrawScale * Gizmo->DrawScale3D.X);
			float H = Gizmo->GetHeight() / ScaleXY; //Gizmo->GetHeight() / (Gizmo->DrawScale * Gizmo->DrawScale3D.Y);

			for (int32 Y = Y1; Y <= Y2; Y++)
			{
				for (int32 X = X1; X <= X2; X++)
				{
					FIntPoint VertexKey = ALandscape::MakeKey(X, Y);

					FVector GizmoLocal = LandscapeToGizmoLocal.TransformPosition(FVector(X, Y, 0));
					if (GizmoLocal.X < W && GizmoLocal.X > 0 && GizmoLocal.Y < H && GizmoLocal.Y > 0)
					{
						float PaintAmount = 1.f;
						// Transform in 0,0 origin LW radius
						if (EdMode->UISettings->bSmoothGizmoBrush)
						{
							FVector TransformedLocal(FMath::Abs(GizmoLocal.X - LW), FMath::Abs(GizmoLocal.Y - LH) * (W / H), 0);
							float FalloffRadius = LW * EdMode->UISettings->BrushFalloff;
							float SquareRadius = LW - FalloffRadius;
							float Cos = FMath::Abs(TransformedLocal.X) / TransformedLocal.Size2D();
							float Sin = FMath::Abs(TransformedLocal.Y) / TransformedLocal.Size2D();
							float RatioX = FalloffRadius > 0.f ? 1.f - FMath::Clamp((FMath::Abs(TransformedLocal.X) - Cos*SquareRadius) / FalloffRadius, 0.f, 1.f) : 1.f;
							float RatioY = FalloffRadius > 0.f ? 1.f - FMath::Clamp((FMath::Abs(TransformedLocal.Y) - Sin*SquareRadius) / FalloffRadius, 0.f, 1.f) : 1.f;
							float Ratio = TransformedLocal.Size2D() > SquareRadius ? RatioX * RatioY : 1.f; //TransformedLocal.X / LW * TransformedLocal.Y / LW;
							PaintAmount = Ratio*Ratio*(3 - 2 * Ratio); //FMath::Lerp(SquareFalloff, RectFalloff*RectFalloff, Ratio);
						}

						if (PaintAmount)
						{
							if (EdMode->CurrentTool && EdMode->CurrentTool->GetToolType() != FLandscapeTool::TT_Mask
								&& EdMode->UISettings->bUseSelectedRegion)
							{
								float MaskValue = LandscapeInfo->SelectedRegion.FindRef(VertexKey);
								if (EdMode->UISettings->bUseNegativeMask)
								{
									MaskValue = 1.f - MaskValue;
								}
								PaintAmount *= MaskValue;
							}

							// Set the brush value for this vertex
							OutBrush.Add(VertexKey, PaintAmount);
						}
					}
				}
			}
		}
		return (X1 <= X2 && Y1 <= Y2);
	}
};

// 
// FLandscapeBrushSplines
//
class FLandscapeBrushSplines : public FLandscapeBrush
{
public:
	const TCHAR* GetBrushName() override { return TEXT("Splines"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_Splines", "Splines"); };

	FEdModeLandscape* EdMode;

	FLandscapeBrushSplines(FEdModeLandscape* InEdMode)
		: EdMode(InEdMode)
	{
	}

	virtual ~FLandscapeBrushSplines()
	{
	}

	virtual EBrushType GetBrushType() override { return BT_Splines; }

	virtual void MouseMove(float LandscapeX, float LandscapeY) override
	{
	}

	virtual bool ApplyBrush(const TArray<FLandscapeToolMousePosition>& MousePositions, TMap<FIntPoint, float>& OutBrush, int32& X1, int32& Y1, int32& X2, int32& Y2) override
	{
		return false;
	}
};

// 
// FLandscapeBrushDummy
//
class FLandscapeBrushDummy : public FLandscapeBrush
{
public:
	const TCHAR* GetBrushName() { return TEXT("None"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_None", "None"); };

	FEdModeLandscape* EdMode;

	FLandscapeBrushDummy(FEdModeLandscape* InEdMode)
		: EdMode(InEdMode)
	{
	}

	virtual ~FLandscapeBrushDummy()
	{
	}

	virtual EBrushType GetBrushType() override { return BT_Normal; }

	virtual void MouseMove(float LandscapeX, float LandscapeY) override
	{
	}

	virtual bool ApplyBrush(const TArray<FLandscapeToolMousePosition>& MousePositions, TMap<FIntPoint, float>& OutBrush, int32& X1, int32& Y1, int32& X2, int32& Y2) override
	{
		return false;
	}
};

class FLandscapeBrushCircle_Linear : public FLandscapeBrushCircle
{
protected:
	/** Protected so only subclasses can create instances. */
	FLandscapeBrushCircle_Linear(FEdModeLandscape* InEdMode, UMaterialInterface* InBrushMaterial)
		: FLandscapeBrushCircle(InEdMode, InBrushMaterial)
	{
	}

	virtual float CalculateFalloff(float Distance, float Radius, float Falloff) override
	{
		return Distance < Radius ? 1.f :
			Falloff > 0.f ? FMath::Max<float>(0.f, 1.f - (Distance - Radius) / Falloff) :
			0.f;
	}

public:
	static FLandscapeBrushCircle_Linear* Create(FEdModeLandscape* InEdMode)
	{
		UMaterialInstanceConstant* CircleBrushMaterial_Linear = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/CircleBrushMaterial_Linear.CircleBrushMaterial_Linear"), NULL, LOAD_None, NULL);
		return new FLandscapeBrushCircle_Linear(InEdMode, CircleBrushMaterial_Linear);
	}
	virtual const TCHAR* GetBrushName() override { return TEXT("Circle_Linear"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_Falloff_Linear", "Linear falloff"); };

};

class FLandscapeBrushCircle_Smooth : public FLandscapeBrushCircle_Linear
{
protected:
	FLandscapeBrushCircle_Smooth(FEdModeLandscape* InEdMode, UMaterialInterface* InBrushMaterial)
		: FLandscapeBrushCircle_Linear(InEdMode, InBrushMaterial)
	{
	}

	virtual float CalculateFalloff(float Distance, float Radius, float Falloff) override
	{
		float y = FLandscapeBrushCircle_Linear::CalculateFalloff(Distance, Radius, Falloff);
		// Smooth-step it
		return y*y*(3 - 2 * y);
	}

public:
	static FLandscapeBrushCircle_Smooth* Create(FEdModeLandscape* InEdMode)
	{
		UMaterialInstanceConstant* CircleBrushMaterial_Smooth = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/CircleBrushMaterial_Smooth.CircleBrushMaterial_Smooth"), NULL, LOAD_None, NULL);
		return new FLandscapeBrushCircle_Smooth(InEdMode, CircleBrushMaterial_Smooth);
	}
	virtual const TCHAR* GetBrushName() override { return TEXT("Circle_Smooth"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_Falloff_Smooth", "Smooth falloff"); };

};

class FLandscapeBrushCircle_Spherical : public FLandscapeBrushCircle
{
protected:
	FLandscapeBrushCircle_Spherical(FEdModeLandscape* InEdMode, UMaterialInterface* InBrushMaterial)
		: FLandscapeBrushCircle(InEdMode, InBrushMaterial)
	{
	}

	virtual float CalculateFalloff(float Distance, float Radius, float Falloff) override
	{
		if (Distance <= Radius)
		{
			return 1.f;
		}

		if (Distance > Radius + Falloff)
		{
			return 0.f;
		}

		// Elliptical falloff
		return FMath::Sqrt(1.f - FMath::Square((Distance - Radius) / Falloff));
	}

public:
	static FLandscapeBrushCircle_Spherical* Create(FEdModeLandscape* InEdMode)
	{
		UMaterialInstanceConstant* CircleBrushMaterial_Spherical = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/CircleBrushMaterial_Spherical.CircleBrushMaterial_Spherical"), NULL, LOAD_None, NULL);
		return new FLandscapeBrushCircle_Spherical(InEdMode, CircleBrushMaterial_Spherical);
	}
	virtual const TCHAR* GetBrushName() override { return TEXT("Circle_Spherical"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_Falloff_Spherical", "Spherical falloff"); };
};

class FLandscapeBrushCircle_Tip : public FLandscapeBrushCircle
{
protected:
	FLandscapeBrushCircle_Tip(FEdModeLandscape* InEdMode, UMaterialInterface* InBrushMaterial)
		: FLandscapeBrushCircle(InEdMode, InBrushMaterial)
	{
	}

	virtual float CalculateFalloff(float Distance, float Radius, float Falloff) override
	{
		if (Distance <= Radius)
		{
			return 1.f;
		}

		if (Distance > Radius + Falloff)
		{
			return 0.f;
		}

		// inverse elliptical falloff
		return 1.f - FMath::Sqrt(1.f - FMath::Square((Falloff + Radius - Distance) / Falloff));
	}

public:
	static FLandscapeBrushCircle_Tip* Create(FEdModeLandscape* InEdMode)
	{
		UMaterialInstanceConstant* CircleBrushMaterial_Tip = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/CircleBrushMaterial_Tip.CircleBrushMaterial_Tip"), NULL, LOAD_None, NULL);
		return new FLandscapeBrushCircle_Tip(InEdMode, CircleBrushMaterial_Tip);
	}
	virtual const TCHAR* GetBrushName() override { return TEXT("Circle_Tip"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_Falloff_Tip", "Tip falloff"); };
};


// FLandscapeBrushAlphaBase
class FLandscapeBrushAlphaBase : public FLandscapeBrushCircle_Smooth
{
public:
	FLandscapeBrushAlphaBase(FEdModeLandscape* InEdMode, UMaterialInterface* InBrushMaterial)
		: FLandscapeBrushCircle_Smooth(InEdMode, InBrushMaterial)
	{
	}

	float GetAlphaSample(float SampleX, float SampleY)
	{
		int32 SizeX = EdMode->UISettings->AlphaTextureSizeX;
		int32 SizeY = EdMode->UISettings->AlphaTextureSizeY;

		// Bilinear interpolate the values from the alpha texture
		int32 SampleX0 = FMath::FloorToInt(SampleX);
		int32 SampleX1 = (SampleX0 + 1) % SizeX;
		int32 SampleY0 = FMath::FloorToInt(SampleY);
		int32 SampleY1 = (SampleY0 + 1) % SizeY;

		const uint8* AlphaData = EdMode->UISettings->AlphaTextureData.GetData();

		float Alpha00 = (float)AlphaData[SampleX0 + SampleY0 * SizeX] / 255.f;
		float Alpha01 = (float)AlphaData[SampleX0 + SampleY1 * SizeX] / 255.f;
		float Alpha10 = (float)AlphaData[SampleX1 + SampleY0 * SizeX] / 255.f;
		float Alpha11 = (float)AlphaData[SampleX1 + SampleY1 * SizeX] / 255.f;

		return FMath::Lerp(
			FMath::Lerp(Alpha00, Alpha01, FMath::Fractional(SampleX)),
			FMath::Lerp(Alpha10, Alpha11, FMath::Fractional(SampleX)),
			FMath::Fractional(SampleY)
			);
	}

};

//
// FLandscapeBrushAlphaPattern
//
class FLandscapeBrushAlphaPattern : public FLandscapeBrushAlphaBase
{
protected:
	FLandscapeBrushAlphaPattern(FEdModeLandscape* InEdMode, UMaterialInterface* InBrushMaterial)
		: FLandscapeBrushAlphaBase(InEdMode, InBrushMaterial)
	{
	}

public:
	static FLandscapeBrushAlphaPattern* Create(FEdModeLandscape* InEdMode)
	{
		UMaterialInstanceConstant* PatternBrushMaterial = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/PatternBrushMaterial_Smooth.PatternBrushMaterial_Smooth"), NULL, LOAD_None, NULL);
		return new FLandscapeBrushAlphaPattern(InEdMode, PatternBrushMaterial);
	}

	virtual EBrushType GetBrushType() override { return BT_Alpha; }

	virtual bool ApplyBrush(const TArray<FLandscapeToolMousePosition>& MousePositions, TMap<FIntPoint, float>& OutBrush, int32& X1, int32& Y1, int32& X2, int32& Y2) override
	{
		ULandscapeInfo* LandscapeInfo = EdMode->CurrentToolTarget.LandscapeInfo.Get();
		float ScaleXY = LandscapeInfo->DrawScale.X;

		float Radius = (1.f - EdMode->UISettings->BrushFalloff) * EdMode->UISettings->BrushRadius / ScaleXY;
		float Falloff = EdMode->UISettings->BrushFalloff * EdMode->UISettings->BrushRadius / ScaleXY;

		int32 SizeX = EdMode->UISettings->AlphaTextureSizeX;
		int32 SizeY = EdMode->UISettings->AlphaTextureSizeY;

		X1 = FMath::FloorToInt(LastMousePosition.X - (Radius + Falloff));
		Y1 = FMath::FloorToInt(LastMousePosition.Y - (Radius + Falloff));
		X2 = FMath::CeilToInt(LastMousePosition.X + (Radius + Falloff));
		Y2 = FMath::CeilToInt(LastMousePosition.Y + (Radius + Falloff));

		for (int32 Y = Y1; Y <= Y2; Y++)
		{
			for (int32 X = X1; X <= X2; X++)
			{
				FIntPoint VertexKey = ALandscape::MakeKey(X, Y);

				// Find alphamap sample location
				float SampleX = (float)X / EdMode->UISettings->AlphaBrushScale + (float)SizeX * EdMode->UISettings->AlphaBrushPanU;
				float SampleY = (float)Y / EdMode->UISettings->AlphaBrushScale + (float)SizeY * EdMode->UISettings->AlphaBrushPanV;

				float Angle = PI * EdMode->UISettings->AlphaBrushRotation / 180.f;

				float ModSampleX = FMath::Fmod(SampleX * FMath::Cos(Angle) - SampleY * FMath::Sin(Angle), (float)SizeX);
				float ModSampleY = FMath::Fmod(SampleY * FMath::Cos(Angle) + SampleX * FMath::Sin(Angle), (float)SizeY);

				if (ModSampleX < 0.f)
				{
					ModSampleX += (float)SizeX;
				}
				if (ModSampleY < 0.f)
				{
					ModSampleY += (float)SizeY;
				}

				// Sample the alpha texture
				float Alpha = GetAlphaSample(ModSampleX, ModSampleY);

				// Distance from mouse
				float MouseDist = FMath::Sqrt(FMath::Square(LastMousePosition.X - (float)X) + FMath::Square(LastMousePosition.Y - (float)Y));

				float PaintAmount = CalculateFalloff(MouseDist, Radius, Falloff) * Alpha;

				if (PaintAmount > 0.f)
				{
					if (EdMode->CurrentTool && EdMode->CurrentTool->GetToolType() != FLandscapeTool::TT_Mask
						&& EdMode->UISettings->bUseSelectedRegion)
					{
						float MaskValue = LandscapeInfo->SelectedRegion.FindRef(VertexKey);
						if (EdMode->UISettings->bUseNegativeMask)
						{
							MaskValue = 1.f - MaskValue;
						}
						PaintAmount *= MaskValue;
					}
					// Set the brush value for this vertex
					OutBrush.Add(VertexKey, PaintAmount);
				}
			}
		}
		return (X1 <= X2 && Y1 <= Y2);
	}

	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override
	{
		FLandscapeBrushCircle::Tick(ViewportClient, DeltaTime);

		ALandscapeProxy* Proxy = EdMode->CurrentToolTarget.LandscapeInfo.IsValid() ? EdMode->CurrentToolTarget.LandscapeInfo->GetLandscapeProxy() : NULL;
		if (Proxy)
		{
			const float ScaleXY = EdMode->CurrentToolTarget.LandscapeInfo->DrawScale.X;
			int32 SizeX = EdMode->UISettings->AlphaTextureSizeX;
			int32 SizeY = EdMode->UISettings->AlphaTextureSizeY;

			FLinearColor AlphaScaleBias(
				1.0f / (EdMode->UISettings->AlphaBrushScale * SizeX),
				1.0f / (EdMode->UISettings->AlphaBrushScale * SizeY),
				EdMode->UISettings->AlphaBrushPanU,
				EdMode->UISettings->AlphaBrushPanV
				);

			float Angle = FMath::DegreesToRadians(EdMode->UISettings->AlphaBrushRotation);
			FLinearColor LandscapeLocation(Proxy->GetActorLocation().X, Proxy->GetActorLocation().Y, Proxy->GetActorLocation().Z, Angle);

			int32 Channel = EdMode->UISettings->AlphaTextureChannel;
			FLinearColor AlphaTextureMask(Channel == 0 ? 1 : 0, Channel == 1 ? 1 : 0, Channel == 2 ? 1 : 0, Channel == 3 ? 1 : 0);

			for (auto It = BrushMaterialInstanceMap.CreateConstIterator(); It; ++It)
			{
				It.Value()->SetVectorParameterValue(FName(TEXT("AlphaScaleBias")), AlphaScaleBias);
				It.Value()->SetVectorParameterValue(FName(TEXT("LandscapeLocation")), LandscapeLocation);
				It.Value()->SetVectorParameterValue(FName(TEXT("AlphaTextureMask")), AlphaTextureMask);
				It.Value()->SetTextureParameterValue(FName(TEXT("AlphaTexture")), EdMode->UISettings->AlphaTexture);
			}
		}
	}

	virtual const TCHAR* GetBrushName() override { return TEXT("Pattern"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_PatternAlpha", "Pattern Alpha"); };
};


//
// FLandscapeBrushAlpha
//
class FLandscapeBrushAlpha : public FLandscapeBrushAlphaBase
{
	float LastMouseAngle;
	FVector2D OldMousePosition;	// a previous mouse position, kept until we move a certain distance away, for smoothing deltas
	double LastMouseSampleTime;

protected:
	FLandscapeBrushAlpha(FEdModeLandscape* InEdMode, UMaterialInterface* InBrushMaterial)
		: FLandscapeBrushAlphaBase(InEdMode, InBrushMaterial)
		, LastMouseAngle(0.f)
		, OldMousePosition(0.f, 0.f)
		, LastMouseSampleTime(FPlatformTime::Seconds())
	{
	}

public:
	static FLandscapeBrushAlpha* Create(FEdModeLandscape* InEdMode)
	{
		UMaterialInstanceConstant* AlphaBrushMaterial = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/AlphaBrushMaterial_Smooth.AlphaBrushMaterial_Smooth"), NULL, LOAD_None, NULL);
		return new FLandscapeBrushAlpha(InEdMode, AlphaBrushMaterial);
	}

	virtual bool ApplyBrush(const TArray<FLandscapeToolMousePosition>& MousePositions, TMap<FIntPoint, float>& OutBrush, int32& X1, int32& Y1, int32& X2, int32& Y2) override
	{
		ULandscapeInfo* LandscapeInfo = EdMode->CurrentToolTarget.LandscapeInfo.Get();
		if (EdMode->UISettings->bAlphaBrushAutoRotate && OldMousePosition.IsZero())
		{
			X1 = FMath::FloorToInt(LastMousePosition.X);
			Y1 = FMath::FloorToInt(LastMousePosition.Y);
			X2 = FMath::CeilToInt(LastMousePosition.X);
			Y2 = FMath::CeilToInt(LastMousePosition.Y);
			OldMousePosition = LastMousePosition;
			LastMouseAngle = 0.f;
			LastMouseSampleTime = FPlatformTime::Seconds();
		}
		else
		{
			float ScaleXY = LandscapeInfo->DrawScale.X;
			float Radius = EdMode->UISettings->BrushRadius / ScaleXY;
			int32 SizeX = EdMode->UISettings->AlphaTextureSizeX;
			int32 SizeY = EdMode->UISettings->AlphaTextureSizeY;
			float MaxSize = 2.f * FMath::Sqrt(FMath::Square(Radius) / 2.f);
			float AlphaBrushScale = MaxSize / (float)FMath::Max<int32>(SizeX, SizeY);
			const float BrushAngle = EdMode->UISettings->bAlphaBrushAutoRotate ? LastMouseAngle : FMath::DegreesToRadians(EdMode->UISettings->AlphaBrushRotation);

			X1 = FMath::FloorToInt(LastMousePosition.X - Radius);
			Y1 = FMath::FloorToInt(LastMousePosition.Y - Radius);
			X2 = FMath::CeilToInt(LastMousePosition.X + Radius);
			Y2 = FMath::CeilToInt(LastMousePosition.Y + Radius);

			for (int32 Y = Y1; Y <= Y2; Y++)
			{
				for (int32 X = X1; X <= X2; X++)
				{
					// Find alphamap sample location
					float ScaleSampleX = ((float)X - LastMousePosition.X) / AlphaBrushScale;
					float ScaleSampleY = ((float)Y - LastMousePosition.Y) / AlphaBrushScale;

					// Rotate around center to match angle
					float SampleX = ScaleSampleX * FMath::Cos(BrushAngle) - ScaleSampleY * FMath::Sin(BrushAngle);
					float SampleY = ScaleSampleY * FMath::Cos(BrushAngle) + ScaleSampleX * FMath::Sin(BrushAngle);

					SampleX += (float)SizeX * 0.5f;
					SampleY += (float)SizeY * 0.5f;

					if (SampleX >= 0 && SampleX <= (SizeX - 1) &&
						SampleY >= 0 && SampleY <= (SizeY - 1))
					{
						// Sample the alpha texture
						float Alpha = GetAlphaSample(SampleX, SampleY);

						if (Alpha > 0.f)
						{
							// Set the brush value for this vertex
							FIntPoint VertexKey = ALandscape::MakeKey(X, Y);

							if (EdMode->CurrentTool && EdMode->CurrentTool->GetToolType() != FLandscapeTool::TT_Mask
								&& EdMode->UISettings->bUseSelectedRegion)
							{
								float MaskValue = LandscapeInfo->SelectedRegion.FindRef(VertexKey);
								if (EdMode->UISettings->bUseNegativeMask)
								{
									MaskValue = 1.f - MaskValue;
								}
								Alpha *= MaskValue;
							}

							OutBrush.Add(VertexKey, Alpha);
						}
					}
				}
			}
		}

		return (X1 <= X2 && Y1 <= Y2);
	}

	virtual void MouseMove(float LandscapeX, float LandscapeY) override
	{
		FLandscapeBrushAlphaBase::MouseMove(LandscapeX, LandscapeY);

		if (EdMode->UISettings->bAlphaBrushAutoRotate)
		{
			// don't do anything with the angle unless we move at least 0.1 units.
			FVector2D MouseDelta = LastMousePosition - OldMousePosition;
			if (MouseDelta.SizeSquared() >= FMath::Square(0.5f))
			{
				double SampleTime = FPlatformTime::Seconds();
				float DeltaTime = (float)(SampleTime - LastMouseSampleTime);
				FVector2D MouseDirection = MouseDelta.GetSafeNormal();
				float MouseAngle = FMath::Lerp(LastMouseAngle, FMath::Atan2(-MouseDirection.Y, MouseDirection.X), FMath::Min<float>(10.f * DeltaTime, 1.f));		// lerp over 100ms
				LastMouseAngle = MouseAngle;
				LastMouseSampleTime = SampleTime;
				OldMousePosition = LastMousePosition;
				// UE_LOG(LogLandscape, Log, TEXT("(%f,%f) delta (%f,%f) angle %f"), LandscapeX, LandscapeY, MouseDirection.X, MouseDirection.Y, MouseAngle);
			}
		}
	}

	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override
	{
		FLandscapeBrushCircle::Tick(ViewportClient, DeltaTime);

		ULandscapeInfo* LandscapeInfo = EdMode->CurrentToolTarget.LandscapeInfo.Get();
		if (LandscapeInfo)
		{
			float ScaleXY = LandscapeInfo->DrawScale.X;
			int32 SizeX = EdMode->UISettings->AlphaTextureSizeX;
			int32 SizeY = EdMode->UISettings->AlphaTextureSizeY;
			float Radius = EdMode->UISettings->BrushRadius / ScaleXY;
			float MaxSize = 2.f * FMath::Sqrt(FMath::Square(Radius) / 2.f);
			float AlphaBrushScale = MaxSize / (float)FMath::Max<int32>(SizeX, SizeY);

			FLinearColor BrushScaleRot(
				1.0f / (AlphaBrushScale * SizeX),
				1.0f / (AlphaBrushScale * SizeY),
				0.f,
				EdMode->UISettings->bAlphaBrushAutoRotate ? LastMouseAngle : FMath::DegreesToRadians(EdMode->UISettings->AlphaBrushRotation)
				);

			int32 Channel = EdMode->UISettings->AlphaTextureChannel;
			FLinearColor AlphaTextureMask(Channel == 0 ? 1 : 0, Channel == 1 ? 1 : 0, Channel == 2 ? 1 : 0, Channel == 3 ? 1 : 0);

			for (auto It = BrushMaterialInstanceMap.CreateConstIterator(); It; ++It)
			{
				It.Value()->SetVectorParameterValue(FName(TEXT("BrushScaleRot")), BrushScaleRot);
				It.Value()->SetVectorParameterValue(FName(TEXT("AlphaTextureMask")), AlphaTextureMask);
				It.Value()->SetTextureParameterValue(FName(TEXT("AlphaTexture")), EdMode->UISettings->AlphaTexture);
			}
		}
	}

	virtual const TCHAR* GetBrushName() override { return TEXT("Alpha"); }
	virtual FText GetDisplayName() override { return NSLOCTEXT("UnrealEd", "LandscapeMode_Brush_Alpha", "Alpha"); };

};


void FEdModeLandscape::InitializeBrushes()
{
	FLandscapeBrushSet* BrushSet;
	BrushSet = new(LandscapeBrushSets)FLandscapeBrushSet(TEXT("BrushSet_Circle"));
	BrushSet->Brushes.Add(FLandscapeBrushCircle_Smooth::Create(this));
	BrushSet->Brushes.Add(FLandscapeBrushCircle_Linear::Create(this));
	BrushSet->Brushes.Add(FLandscapeBrushCircle_Spherical::Create(this));
	BrushSet->Brushes.Add(FLandscapeBrushCircle_Tip::Create(this));

	BrushSet = new(LandscapeBrushSets)FLandscapeBrushSet(TEXT("BrushSet_Alpha"));
	BrushSet->Brushes.Add(FLandscapeBrushAlpha::Create(this));

	BrushSet = new(LandscapeBrushSets)FLandscapeBrushSet(TEXT("BrushSet_Pattern"));
	BrushSet->Brushes.Add(FLandscapeBrushAlphaPattern::Create(this));

	BrushSet = new(LandscapeBrushSets)FLandscapeBrushSet(TEXT("BrushSet_Component"));
	BrushSet->Brushes.Add(new FLandscapeBrushComponent(this));

	BrushSet = new(LandscapeBrushSets)FLandscapeBrushSet(TEXT("BrushSet_Gizmo"));
	GizmoBrush = new FLandscapeBrushGizmo(this);
	BrushSet->Brushes.Add(GizmoBrush);

	BrushSet = new(LandscapeBrushSets)FLandscapeBrushSet(TEXT("BrushSet_Splines"));
	BrushSet->Brushes.Add(new FLandscapeBrushSplines(this));

	BrushSet = new(LandscapeBrushSets)FLandscapeBrushSet(TEXT("BrushSet_Dummy"));
	BrushSet->Brushes.Add(new FLandscapeBrushDummy(this));
}
