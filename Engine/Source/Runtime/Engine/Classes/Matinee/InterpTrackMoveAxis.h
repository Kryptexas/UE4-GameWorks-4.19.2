// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InterpTrackMoveAxis.generated.h"

/**
  *
  * Subtrack for UInterpTrackMove
  * Transforms an interp actor on one axis
  */

/** List of axies this track can use */
UENUM()
enum EInterpMoveAxis
{
	AXIS_TranslationX,
	AXIS_TranslationY,
	AXIS_TranslationZ,
	AXIS_RotationX,
	AXIS_RotationY,
	AXIS_RotationZ,
};

UCLASS(dependson=UInterpTrackMove, HeaderGroup=Interpolation, MinimalAPI, meta=( DisplayName = "Move Axis Track" ) )
class UInterpTrackMoveAxis : public UInterpTrackFloatBase
{
	GENERATED_UCLASS_BODY()

	/** The axis which this track will use when transforming an actor */
	UPROPERTY()
	TEnumAsByte<enum EInterpMoveAxis> MoveAxis;

	/** Lookup track to use when looking at different groups for transform information*/
	UPROPERTY()
	struct FInterpLookupTrack LookupTrack;


	// Begin UInterpTrack Interface
	virtual int32 AddKeyframe( float Time, UInterpTrackInst* TrInst, EInterpCurveMode InitInterpMode ) OVERRIDE;
	virtual void UpdateKeyframe(int32 KeyIndex, UInterpTrackInst* TrInst) OVERRIDE;
	virtual int32 SetKeyframeTime( int32 KeyIndex, float NewKeyTime, bool bUpdateOrder ) OVERRIDE;
	virtual void RemoveKeyframe( int32 KeyIndex ) OVERRIDE;
	virtual int32 DuplicateKeyframe( int32 KeyIndex, float NewKeyTime, UInterpTrack* ToTrack = NULL ) OVERRIDE;
	virtual class UTexture2D* GetTrackIcon() const OVERRIDE;
	virtual void ReduceKeys( float IntervalStart, float IntervalEnd, float Tolerance ) OVERRIDE;
	// End UInterpTrack Interface
	


	// Begin FCurveEdInterface interface.
	virtual FColor GetSubCurveButtonColor( int32 SubCurveIndex, bool bIsSubCurveHidden ) const OVERRIDE;
	virtual int32 CreateNewKey( float KeyIn ) OVERRIDE;
	virtual void DeleteKey( int32 KeyIndex ) OVERRIDE;
	virtual int32 SetKeyIn( int32 KeyIndex, float NewInVal ) OVERRIDE;
	virtual FColor GetKeyColor(int32 SubIndex, int32 KeyIndex, const FColor& CurveColor) OVERRIDE;
	// End FCurveEdInterface interface.

	/** @todo document */
	void GetKeyframeValue( UInterpTrackInst* TrInst, int32 KeyIndex, float& OutTime, float &OutValue, float* OutArriveTangent, float* OutLeaveTangent );

	/** @todo document */
	float EvalValueAtTime( UInterpTrackInst* TrInst, float Time );

	/**
	 * @param KeyIndex	Index of the key to retrieve the lookup group name for.
	 *
	 * @return Returns the groupname for the keyindex specified.
	 */
	FName GetLookupKeyGroupName( int32 KeyIndex );

	/**
	 * Sets the lookup group name for a movement track keyframe.
	 *
	 * @param KeyIndex			Index of the key to modify.
	 * @param NewGroupName		Group name to set the keyframe's lookup group to.
	 */
	ENGINE_API void SetLookupKeyGroupName( int32 KeyIndex, const FName& NewGroupName );

	/**
	 * Clears the lookup group name for a movement track keyframe.
	 *
	 * @param KeyIndex			Index of the key to modify.
	 */
	ENGINE_API void ClearLookupKeyGroupName( int32 KeyIndex );

};



