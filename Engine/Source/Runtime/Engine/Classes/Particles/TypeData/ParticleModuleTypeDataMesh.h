// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleTypeDataMesh.generated.h"

UENUM()
enum EMeshScreenAlignment
{
	PSMA_MeshFaceCameraWithRoll,
	PSMA_MeshFaceCameraWithSpin,
	PSMA_MeshFaceCameraWithLockedAxis,
	PSMA_MAX,
};

UENUM()
enum EMeshCameraFacingUpAxis
{
	CameraFacing_NoneUP,
	CameraFacing_ZUp,
	CameraFacing_NegativeZUp,
	CameraFacing_YUp,
	CameraFacing_NegativeYUp,
	CameraFacing_MAX,
};

UENUM()
enum EMeshCameraFacingOptions
{
	XAxisFacing_NoUp,
	XAxisFacing_ZUp,
	XAxisFacing_NegativeZUp,
	XAxisFacing_YUp,
	XAxisFacing_NegativeYUp,
	LockedAxis_ZAxisFacing,
	LockedAxis_NegativeZAxisFacing,
	LockedAxis_YAxisFacing,
	LockedAxis_NegativeYAxisFacing,
	VelocityAligned_ZAxisFacing,
	VelocityAligned_NegativeZAxisFacing,
	VelocityAligned_YAxisFacing,
	VelocityAligned_NegativeYAxisFacing,
	EMeshCameraFacingOptions_MAX,
};

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Mesh Data"))
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{
	GENERATED_UCLASS_BODY()

	/** The static mesh to render at the particle positions */
	UPROPERTY(EditAnywhere, Category=Mesh)
	class UStaticMesh* Mesh;

	/** If true, has the meshes cast shadows */
	UPROPERTY()
	uint32 CastShadows:1;

	/** UNUSED (the collision module dictates doing collisions) */
	UPROPERTY()
	uint32 DoCollisions:1;

	/** 
	 *	The alignment to use on the meshes emitted.
	 *	The RequiredModule->ScreenAlignment MUST be set to PSA_TypeSpecific to use.
	 *	One of the following:
	 *	PSMA_MeshFaceCameraWithRoll
	 *		Face the camera allowing for rotation around the mesh-to-camera FVector 
	 *		(amount provided by the standard particle sprite rotation).  
	 *	PSMA_MeshFaceCameraWithSpin
	 *		Face the camera allowing for the mesh to rotate about the tangential axis.  
	 *	PSMA_MeshFaceCameraWithLockedAxis
	 *		Face the camera while maintaining the up FVector as the locked direction.  
	 */
	UPROPERTY(EditAnywhere, Category=Mesh)
	TEnumAsByte<enum EMeshScreenAlignment> MeshAlignment;

	/**
	 *	If true, use the emitter material when rendering rather than the one applied 
	 *	to the static mesh model.
	 */
	UPROPERTY(EditAnywhere, Category=Mesh)
	uint32 bOverrideMaterial:1;

	/** The 'pre' rotation pitch (in degrees) to apply to the static mesh used. */
	UPROPERTY(EditAnywhere, Category=Orientation)
	float Pitch;

	/** The 'pre' rotation roll (in degrees) to apply to the static mesh used. */
	UPROPERTY(EditAnywhere, Category=Orientation)
	float Roll;

	/** The 'pre' rotation yaw (in degrees) to apply to the static mesh used. */
	UPROPERTY(EditAnywhere, Category=Orientation)
	float Yaw;

	/**
	 *	The axis to lock the mesh on. This overrides TypeSpecific mesh alignment as well as the LockAxis module.
	 *		EPAL_NONE		 -	No locking to an axis.
	 *		EPAL_X			 -	Lock the mesh X-axis facing towards +X.
	 *		EPAL_Y			 -	Lock the mesh X-axis facing towards +Y.
	 *		EPAL_Z			 -	Lock the mesh X-axis facing towards +Z.
	 *		EPAL_NEGATIVE_X	 -	Lock the mesh X-axis facing towards -X.
	 *		EPAL_NEGATIVE_Y	 -	Lock the mesh X-axis facing towards -Y.
	 *		EPAL_NEGATIVE_Z	 -	Lock the mesh X-axis facing towards -Z.
	 *		EPAL_ROTATE_X	 -	Ignored for mesh emitters. Treated as EPAL_NONE.
	 *		EPAL_ROTATE_Y	 -	Ignored for mesh emitters. Treated as EPAL_NONE.
	 *		EPAL_ROTATE_Z	 -	Ignored for mesh emitters. Treated as EPAL_NONE.
	 */
	UPROPERTY(EditAnywhere, Category=Orientation)
	TEnumAsByte<enum EParticleAxisLock> AxisLockOption;

	/**
	 *	If true, then point the X-axis of the mesh towards the camera.
	 *	When set, AxisLockOption as well as all other locked axis/screen alignment settings are ignored.
	 */
	UPROPERTY(EditAnywhere, Category=CameraFacing)
	uint32 bCameraFacing:1;

	/**
	 *	The axis of the mesh to point up when camera facing the X-axis.
	 *		CameraFacing_NoneUP			No attempt to face an axis up or down.
	 *		CameraFacing_ZUp			Z-axis of the mesh should attempt to point up.
	 *		CameraFacing_NegativeZUp	Z-axis of the mesh should attempt to point down.
	 *		CameraFacing_YUp			Y-axis of the mesh should attempt to point up.
	 *		CameraFacing_NegativeYUp	Y-axis of the mesh should attempt to point down.
	 */
	UPROPERTY()
	TEnumAsByte<enum EMeshCameraFacingUpAxis> CameraFacingUpAxisOption_DEPRECATED;

	/**
	 *	The camera facing option to use:
	 *	All camera facing options without locked axis assume X-axis will be facing the camera.
	 *		XAxisFacing_NoUp				- X-axis camera facing, no attempt to face an axis up or down.
	 *		XAxisFacing_ZUp					- X-axis camera facing, Z-axis of the mesh should attempt to point up.
	 *		XAxisFacing_NegativeZUp			- X-axis camera facing, Z-axis of the mesh should attempt to point down.
	 *		XAxisFacing_YUp					- X-axis camera facing, Y-axis of the mesh should attempt to point up.
	 *		XAxisFacing_NegativeYUp			- X-axis camera facing, Y-axis of the mesh should attempt to point down.
	 *	All axis-locked camera facing options assume the AxisLockOption is set. EPAL_NONE will be treated as EPAL_X.
	 *		LockedAxis_ZAxisFacing			- X-axis locked on AxisLockOption axis, rotate Z-axis of the mesh to face towards camera.
	 *		LockedAxis_NegativeZAxisFacing	- X-axis locked on AxisLockOption axis, rotate Z-axis of the mesh to face away from camera.
	 *		LockedAxis_YAxisFacing			- X-axis locked on AxisLockOption axis, rotate Y-axis of the mesh to face towards camera.
	 *		LockedAxis_NegativeYAxisFacing	- X-axis locked on AxisLockOption axis, rotate Y-axis of the mesh to face away from camera.
	 *	All velocity-aligned options do NOT require the ScreenAlignment be set to PSA_Velocity.
	 *	Doing so will result in additional work being performed... (it will orient the mesh twice).
	 *		VelocityAligned_ZAxisFacing         - X-axis aligned to the velocity, rotate the Z-axis of the mesh to face towards camera.
	 *		VelocityAligned_NegativeZAxisFacing - X-axis aligned to the velocity, rotate the Z-axis of the mesh to face away from camera.
	 *		VelocityAligned_YAxisFacing         - X-axis aligned to the velocity, rotate the Y-axis of the mesh to face towards camera.
	 *		VelocityAligned_NegativeYAxisFacing - X-axis aligned to the velocity, rotate the Y-axis of the mesh to face away from camera.
	 */
	UPROPERTY(EditAnywhere, Category=CameraFacing)
	TEnumAsByte<enum EMeshCameraFacingOptions> CameraFacingOption;

	/** 
	 *	If true, apply 'sprite' particle rotation about the orientation axis (direction mesh is pointing).
	 *	If false, apply 'sprite' particle rotation about the camera facing axis.
	 */
	UPROPERTY(EditAnywhere, Category=CameraFacing)
	uint32 bApplyParticleRotationAsSpin:1;

	/** 
	*	If true, all camera facing options will point the mesh against the camera's view direction rather than pointing at the cameras location. 
	*	If false, the camera facing will point to the cameras position as normal.
	*/
	UPROPERTY(EditAnywhere, Category=CameraFacing)
	uint32 bFaceCameraDirectionRatherThanPosition:1;

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void	PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	// Begin UParticleModule Interface
	virtual void	SetToSensibleDefaults(UParticleEmitter* Owner) OVERRIDE;
	// End UParticleModule Interface

	// Begin UParticleModuleTypeDataBase Interface
	virtual FParticleEmitterInstance*	CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent) OVERRIDE;
	virtual bool	SupportsSpecificScreenAlignmentFlags() const OVERRIDE {	return true;	}	
	virtual bool	SupportsSubUV() const OVERRIDE { return true; }
	virtual bool	IsAMeshEmitter() const OVERRIDE { return true; }
	// End UParticleModuleTypeDataBase Interface
};



