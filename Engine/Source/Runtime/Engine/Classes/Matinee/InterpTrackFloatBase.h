// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InterpTrackFloatBase.generated.h"

UCLASS(HeaderGroup=Interpolation, abstract, MinimalAPI)
class UInterpTrackFloatBase : public UInterpTrack
{
	GENERATED_UCLASS_BODY()
	
	/** Actually track data containing keyframes of float as it varies over time. */
	UPROPERTY()
	FInterpCurveFloat FloatTrack;

	/** Tension of curve, used for keypoints using automatic tangents. */
	UPROPERTY(EditAnywhere, Category=InterpTrackFloatBase)
	float CurveTension;


	// Begin UObject interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
#endif // WITH_EDITOR
	// End UObject interface.

	// Begin UInterpTrack interface.
	virtual int32 GetNumKeyframes() const OVERRIDE;
	virtual void GetTimeRange(float& StartTime, float& EndTime) const OVERRIDE;
	virtual float GetTrackEndTime() const OVERRIDE;
	virtual float GetKeyframeTime(int32 KeyIndex) const OVERRIDE;
	virtual int32 GetKeyframeIndex( float KeyTime ) const OVERRIDE;
	virtual int32 SetKeyframeTime(int32 KeyIndex, float NewKeyTime, bool bUpdateOrder=true) OVERRIDE;
	virtual void RemoveKeyframe(int32 KeyIndex) OVERRIDE;
	virtual int32 DuplicateKeyframe(int32 KeyIndex, float NewKeyTime, UInterpTrack* ToTrack = NULL) OVERRIDE;
	virtual bool GetClosestSnapPosition(float InPosition, TArray<int32> &IgnoreKeys, float& OutPosition) OVERRIDE;
	virtual FColor GetKeyframeColor(int32 KeyIndex) const OVERRIDE;
	// End UInterpTrack interface.

	// Begin FCurveEdInterface interface.
	virtual int32		GetNumKeys() const OVERRIDE;
	virtual int32		GetNumSubCurves() const OVERRIDE;
	virtual float	GetKeyIn(int32 KeyIndex) OVERRIDE;
	virtual float	GetKeyOut(int32 SubIndex, int32 KeyIndex) OVERRIDE;
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
	// End FCurveEdInterface interface.
};



