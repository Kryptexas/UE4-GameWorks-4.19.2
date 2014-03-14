// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "MaterialExpressionSpeedTree.generated.h"

UENUM()
enum ESpeedTreeGeometryType
{
	STG_Branch		UMETA(DisplayName="Branch"),
	STG_Frond		UMETA(DisplayName="Frond"),
	STG_Leaf		UMETA(DisplayName="Leaf"),
	STG_FacingLeaf	UMETA(DisplayName="Facing Leaf"),
	STG_Billboard	UMETA(DisplayName="Billboard")
};

UENUM()
enum ESpeedTreeWindType
{
	STW_None		UMETA(DisplayName="None"),
	STW_Standard	UMETA(DisplayName="Standard"),
	STW_Palm		UMETA(DisplayName="Palm"),
	STW_Hero		UMETA(DisplayName="Hero")
};

UENUM()
enum ESpeedTreeLODType
{
	STLOD_Pop		UMETA(DisplayName="Pop"),
	STLOD_Smooth	UMETA(DisplayName="Smooth")
};


UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionSpeedTree : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()	

	UPROPERTY(EditAnywhere, Category=MaterialExpressionSpeedTree, meta=(DisplayName = "Geometry Type", ToolTip="The type of SpeedTree geometry on which this material will be used"))
	TEnumAsByte<enum ESpeedTreeGeometryType> GeometryType;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionSpeedTree, meta=(DisplayName = "Wind Type", ToolTip="The type of wind effect used on this tree. Make sure this matches how the tree was tuned in the SpeedTree Modeler."))
	TEnumAsByte<enum ESpeedTreeWindType> WindType;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionSpeedTree, meta=(DisplayName = "LOD Type", ToolTip="The type of LOD to use"))
	TEnumAsByte<enum ESpeedTreeLODType> LODType;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionSpeedTree, meta=(DisplayName = "Billboard Threshold", ToolTip="The threshold for triangles to be removed from the bilboard mesh when not facing the camera (0 = none pass, 1 = all pass).", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float BillboardThreshold;

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};


