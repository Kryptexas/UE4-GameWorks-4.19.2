// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMeshPainter.h"

#include "MeshPaintTypes.h"

class SClothPaintWidget;
class UClothPainterSettings;
class UPaintBrushSettings;
class UClothingAsset;
class UDebugSkelMeshComponent;
enum class EPaintableClothProperty;

class FClothPainter : public IMeshPainter
{
public:
	FClothPainter();
	~FClothPainter();
	void Init();
protected:
	virtual bool PaintInternal(const FVector& InCameraOrigin, const FVector& InRayOrigin, const FVector& InRayDirection, EMeshPaintAction PaintAction, float PaintStrength) override;
public:
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;	
	virtual void FinishPainting() override;
	virtual void ActorSelected(AActor* Actor) override {};
	virtual void ActorDeselected(AActor* Actor) override {};
	virtual void Reset() override;
	virtual TSharedPtr<IMeshPaintGeometryAdapter> GetMeshAdapterForComponent(const UMeshComponent* Component) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual UPaintBrushSettings* GetBrushSettings() override;
	virtual UMeshPaintSettings* GetPainterSettings() override;
	virtual TSharedPtr<class SWidget> GetWidget() override;
	virtual const FHitResult GetHitResult(const FVector& Origin, const FVector& Direction) override;
	virtual void Refresh() override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;

	/** Sets the debug skeletal mesh to which we should currently paint */
	void SetSkeletalMeshComponent(UDebugSkelMeshComponent* SkeletalMeshComponent);

	/** Creates paint parameters for the current setup */
	FMeshPaintParameters CreatePaintParameters(const struct FHitResult& HitResult, const FVector& InCameraOrigin, const FVector& InRayOrigin, const FVector& InRayDirection, float PaintStrength);

protected:
	/** Apply per vertex painting of the given Clothing Property */
	void ApplyPropertyPaint(IMeshPaintGeometryAdapter* InAdapter, int32 Vertexindex, FMatrix InverseBrushMatrix, EPaintableClothProperty Property);

	/** Retrieves the property value from the Cloth asset for the given EPaintableClothProperty */
	float GetPropertyValue(int32 VertexIndex, EPaintableClothProperty Property);
	/** Sets the EPaintableClothProperty property within the Clothing asset to Value */
	void SetPropertyValue(int32 VertexIndex, const float Value, EPaintableClothProperty Property);

	/** When a different clothing asset is selected in the UI the painter should refresh the adapter */
	void OnAssetSelectionChanged(UClothingAsset* InNewSelectedAsset, int32 InAssetLod);
	void OnAssetMaskSelectionChanged()
	{};

	/** Rebuild the list of editable clothing assets from the current mesh */
	void RefreshClothingAssets();

	/** Applies a gradient to all sim points between GradientStartPoints and GradientEndPoints */
	void ApplyGradient();

protected:
	/** Current adapter used to paint the clothing properties */
	TSharedPtr<IMeshPaintGeometryAdapter> Adapter;	
	/** Debug skeletal mesh to which painting should be applied */
	UDebugSkelMeshComponent* SkeletalMeshComponent;
	/** Widget used to represent the state/functionality of the painter */
	TSharedPtr<SClothPaintWidget> Widget;
	/** Cloth paint settings instance */
	UClothPainterSettings* PaintSettings;
	/** Cloth brush settings instance */
	UPaintBrushSettings* BrushSettings;

	/** Array of points which define the start of the gradient */
	TArray<FVector> GradientStartPoints;
	/** Array of points which define the end of the gradient */
	TArray<FVector> GradientEndPoints;
	/** Toggle for selecting start and end gradient points*/
	bool bSelectingFirstPoint;

	/** Flag whether or not the simulation should run */
	bool bShouldSimulate;
	/** Flag to render (hidden) sim verts during gradient painting */
	bool bShowHiddenVerts;
};