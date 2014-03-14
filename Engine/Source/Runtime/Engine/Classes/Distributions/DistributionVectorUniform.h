// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DistributionVectorUniform.generated.h"

UCLASS(collapsecategories, hidecategories=Object, editinlinenew, MinimalAPI)
class UDistributionVectorUniform : public UDistributionVector
{
	GENERATED_UCLASS_BODY()

	/** Upper end of FVector magnitude range. */
	UPROPERTY(EditAnywhere, Category=DistributionVectorUniform)
	FVector Max;

	/** Lower end of FVector magnitude range. */
	UPROPERTY(EditAnywhere, Category=DistributionVectorUniform)
	FVector Min;

	/** If true, X == Y == Z ie. only one degree of freedom. If false, each axis is picked independently. */
	UPROPERTY()
	uint32 bLockAxes:1;

	UPROPERTY(EditAnywhere, Category=DistributionVectorUniform)
	TEnumAsByte<enum EDistributionVectorLockFlags> LockedAxes;

	UPROPERTY(EditAnywhere, Category=DistributionVectorUniform)
	TEnumAsByte<enum EDistributionVectorMirrorFlags> MirrorFlags[3];

	UPROPERTY(EditAnywhere, Category=DistributionVectorUniform)
	uint32 bUseExtremes:1;


	// Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface

	virtual FVector	GetValue( float F = 0.f, UObject* Data = NULL, int32 LastExtreme = 0, class FRandomStream* InRandomStream = NULL ) const OVERRIDE;

	//Begin UDistributionVector Interface

	//@todo.CONSOLE: Currently, consoles need this? At least until we have some sort of cooking/packaging step!
	virtual ERawDistributionOperation GetOperation() const OVERRIDE;
	virtual uint8 GetLockFlag() const OVERRIDE;
	virtual uint32 InitializeRawEntry(float Time, float* Values) const OVERRIDE;
	virtual	void	GetRange(FVector& OutMin, FVector& OutMax) const OVERRIDE;
	//End UDistributionVector Interface

	/** These two functions will retrieve the Min/Max values respecting the Locked and Mirror flags. */
	virtual FVector GetMinValue() const;
	virtual FVector GetMaxValue() const;

	// We have 6 subs, 3 mins and three maxes. They are assigned as:
	// 0,1 = min/max x
	// 2,3 = min/max y
	// 4,5 = min/max z

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
	// Begin FCurveEdInterface interface

};



