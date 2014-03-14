// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * CameraAnim: defines a pre-packaged animation to be played on a camera.
 */

#pragma once
#include "CameraAnim.generated.h"

UCLASS(notplaceable, MinimalAPI)
class UCameraAnim : public UObject
{
	GENERATED_UCLASS_BODY()

	/** The UInterpGroup that holds our actual interpolation data. */
	UPROPERTY()
	class UInterpGroup* CameraInterpGroup;

#if WITH_EDITORONLY_DATA
	/** This is to preview and they only exists in editor */
	UPROPERTY(transient)
	class UInterpGroup* PreviewInterpGroup;

#endif // WITH_EDITORONLY_DATA
	/** Length, in seconds. */
	UPROPERTY()
	float AnimLength;

	/** AABB in local space. */
	UPROPERTY()
	FBox BoundingBox;

	/** The */
	UPROPERTY()
	float BaseFOV;


protected:
	// @todo document
	void CalcLocalAABB();

public:
	// Begin UObject Interface
	virtual void PreSave() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	// End UObject Interface

	/** 
	 * Construct a camera animation from an InterpGroup.  The InterpGroup must control a CameraActor.  
	 * Used by the editor to "export" a camera animation from a normal Matinee scene.
	 */
	ENGINE_API bool CreateFromInterpGroup(class UInterpGroup* SrcGroup, class AMatineeActor* InMatineeActor);
	
	// @todo document
	FBox GetAABB(FVector const& BaseLoc, FRotator const& BaseRot, float Scale) const;

	// @todo document
	bool InitializeCamera(class UInterpGroup* SrcGroup, class USeqAct_Interp* Interp);
};



