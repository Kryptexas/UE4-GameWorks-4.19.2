// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InterpTrackVectorMaterialParam.generated.h"

UCLASS(HeaderGroup=Interpolation, MinimalAPI, meta=( DisplayName = "Vector UMaterial Param Track" ) )
class UInterpTrackVectorMaterialParam : public UInterpTrackVectorBase
{
	GENERATED_UCLASS_BODY()

	/** Materials whose parameters we want to change and the references to those materials. */
	UPROPERTY(EditAnywhere, Category=InterpTrackVectorMaterialParam)
	TArray<class UMaterialInterface*> TargetMaterials;

	/** Name of parameter in the MaterialInstance which this track will modify over time. */
	UPROPERTY(EditAnywhere, Category=InterpTrackVectorMaterialParam)
	FName ParamName;


	//Begin UObject Interface
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyThatWillChange) OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	//End UObject Interface

	// Begin UInterpTrack interface.
	virtual int32 AddKeyframe(float Time, UInterpTrackInst* TrInst, EInterpCurveMode InitInterpMode) OVERRIDE;
	virtual void PreviewUpdateTrack(float NewPosition, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void UpdateTrack(float NewPosition, UInterpTrackInst* TrInst, bool bJump) OVERRIDE;
	// End UInterpTrack interface.
};



