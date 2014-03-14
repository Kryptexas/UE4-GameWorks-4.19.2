// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DistributionVectorConstant.generated.h"

UCLASS(collapsecategories, hidecategories=Object, editinlinenew)
class ENGINE_API UDistributionVectorConstant : public UDistributionVector
{
	GENERATED_UCLASS_BODY()

	/** This FVector will be returned for all input times. */
	UPROPERTY(EditAnywhere, Category=DistributionVectorConstant)
	FVector Constant;

	/** If true, X == Y == Z ie. only one degree of freedom. If false, each axis is picked independently. */
	UPROPERTY()
	uint32 bLockAxes:1;

	UPROPERTY(EditAnywhere, Category=DistributionVectorConstant)
	TEnumAsByte<enum EDistributionVectorLockFlags> LockedAxes;


	//Begin UDistributionVector Interface
	virtual FVector	GetValue( float F = 0.f, UObject* Data = NULL, int32 LastExtreme = 0, class FRandomStream* InRandomStream = NULL ) const OVERRIDE;
	virtual	void	GetRange(FVector& OutMin, FVector& OutMax) const OVERRIDE;
	//End UDistributionVector Interface

	// Begin UObject interface
	virtual void PostInitProperties() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	// End UObject interface
	
	// Begin FCurveEdInterface interface
	virtual int32		GetNumKeys() const OVERRIDE;
	virtual int32		GetNumSubCurves() const OVERRIDE;
	virtual FColor	GetSubCurveButtonColor(int32 SubCurveIndex, bool bIsSubCurveHidden) const OVERRIDE;
	virtual float	GetKeyIn(int32 KeyIndex) OVERRIDE;
	virtual float	GetKeyOut(int32 SubIndex, int32 KeyIndex) OVERRIDE;
	virtual FColor	GetKeyColor(int32 SubIndex, int32 KeyIndex, const FColor& CurveColor) OVERRIDE;
	virtual void	GetInRange(float& MinIn, float& MaxIn) const OVERRIDE;
	virtual void	GetOutRange(float& MinOut, float& MaxOut) const OVERRIDE;
	virtual EInterpCurveMode	GetKeyInterpMode(int32 KeyIndex) const OVERRIDE;
	virtual void	GetTangents(int32 SubIndex, int32 KeyIndex, float& ArriveTangent, float& LeaveTangent) const OVERRIDE;
	virtual float	EvalSub(int32 SubIndex, float InVal) OVERRIDE;
	virtual int32		CreateNewKey(float KeyIn) OVERRIDE;
	virtual void	DeleteKey(int32 KeyIndex) OVERRIDE;
	virtual int32		SetKeyIn(int32 KeyIndex, float NewInVal) OVERRIDE;
	virtual void	SetKeyOut(int32 SubIndex, int32 KeyIndex, float NewOutVal) OVERRIDE;
	virtual void	SetKeyInterpMode(int32 KeyIndex, EInterpCurveMode NewMode) OVERRIDE;
	virtual void	SetTangents(int32 SubIndex, int32 KeyIndex, float ArriveTangent, float LeaveTangent) OVERRIDE;
	// End FCurveEdInterface interface

};

