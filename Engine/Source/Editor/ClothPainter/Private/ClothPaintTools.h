// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ClothPaintToolBase.h"
#include "ClothPaintTools.generated.h"

/** Unique settings for the Brush tool */
UCLASS()
class UClothPaintTool_BrushSettings : public UObject
{
	GENERATED_BODY()

public:

	UClothPaintTool_BrushSettings()
		: PaintValue(100.0f)
	{

	}

	/** Value to paint onto the mesh for this parameter */
	UPROPERTY(EditAnywhere, Category = ToolSettings)
	float PaintValue;
};


/** Standard brush tool for painting onto the mesh */
class FClothPaintTool_Brush : public FClothPaintToolBase
{
public:

	FClothPaintTool_Brush(TWeakPtr<FClothPainter> InPainter)
		: FClothPaintToolBase(InPainter)
		, Settings(nullptr)
	{

	}

	virtual ~FClothPaintTool_Brush();

	/** FClothPaintToolBase interface */
	virtual FText GetDisplayName() override;
	virtual FPerVertexPaintAction GetPaintAction(const FMeshPaintParameters& InPaintParams, UClothPainterSettings* InPainterSettings) override;
	virtual UObject* GetSettingsObject() override;
	/** End FClothPaintToolBase interface */

protected:

	/** The settings object shown in the details panel */
	UClothPaintTool_BrushSettings* Settings;

	/** Called when the paint action is applied */
	void PaintAction(FPerVertexPaintActionArgs& InArgs, int32 VertexIndex, FMatrix InverseBrushMatrix, EPaintableClothProperty Property);

};

/** Unique settings for the Gradient tool */
UCLASS()
class UClothPaintTool_GradientSettings : public UObject
{

	GENERATED_BODY()

public:

	UClothPaintTool_GradientSettings()
		: GradientStartValue(0.0f)
		, GradientEndValue(100.0f)
		, bUseRegularBrush(false)
	{

	}

	/** Value of the gradient at the start points */
	UPROPERTY(EditAnywhere, Category = ToolSettings)
	float GradientStartValue;

	/** Value of the gradient at the end points */
	UPROPERTY(EditAnywhere, Category = ToolSettings)
	float GradientEndValue;

	/** Enables the painting of selected points using a brush rather than just a point */
	UPROPERTY(EditAnywhere, Category = ToolSettings)
	bool bUseRegularBrush;
};

/** 
 * Gradient tool - Allows the user to select begin and end points to apply a gradient to.
 * Pressing Enter will move from selecting begin points, to selecting end points and finally
 * applying the operation to the mesh
 */
class FClothPaintTool_Gradient : public FClothPaintToolBase
{
public:

	FClothPaintTool_Gradient(TWeakPtr<FClothPainter> InPainter)
		: FClothPaintToolBase(InPainter)
		, bSelectingBeginPoints(true)
		, Settings(nullptr)
	{

	}

	virtual ~FClothPaintTool_Gradient();

	/* FClothPaintToolBase interface */
	virtual FText GetDisplayName() override;
	virtual bool InputKey(IMeshPaintGeometryAdapter* Adapter, FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;
	virtual FPerVertexPaintAction GetPaintAction(const FMeshPaintParameters& InPaintParams, UClothPainterSettings* InPainterSettings) override;
	virtual bool IsPerVertex() override;
	virtual void Render(USkeletalMeshComponent* InComponent, IMeshPaintGeometryAdapter* InAdapter, const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool ShouldRenderInteractors() override;
	virtual UObject* GetSettingsObject() override;
	/* End FClothPaintToolBase interface */
	
protected:

	/* Whether we're selecting the start points or end points */
	bool bSelectingBeginPoints;

	/* List of points to consider the start of the gradient */
	TArray<FVector> GradientStartPoints;

	/* List of points to consider the end of the gradient */
	TArray<FVector> GradientEndPoints;

	/** The settings object shown in the details panel */
	UClothPaintTool_GradientSettings* Settings;

	/* Called when the paint action is applied */
	void PaintAction(FPerVertexPaintActionArgs& InArgs, int32 VertexIndex, FMatrix InverseBrushMatrix, EPaintableClothProperty Property);

	/* Applies the gradient to the currently selected points */
	void ApplyGradient(IMeshPaintGeometryAdapter* InAdapter);

};

class FClothPaintTool_Smooth : public FClothPaintToolBase
{
	FClothPaintTool_Smooth(TWeakPtr<FClothPainter> InPainter)
		: FClothPaintToolBase(InPainter)
	{

	}

public:

	virtual FPerVertexPaintAction GetPaintAction(const FMeshPaintParameters& InPaintParams, UClothPainterSettings* InPainterSettings) override;
	virtual FText GetDisplayName() override;
	virtual UObject* GetSettingsObject() override;

protected:

	void PaintAction(FPerVertexPaintActionArgs& InArgs, int32 VertexIndex, FMatrix InverseBrushMatrix, EPaintableClothProperty Property);
};
