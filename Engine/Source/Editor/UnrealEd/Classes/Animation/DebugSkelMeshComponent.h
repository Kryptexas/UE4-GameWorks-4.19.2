// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DebugSkelMeshComponent.generated.h"

USTRUCT(transient)
struct FSelectedSocketInfo
{
	GENERATED_USTRUCT_BODY()

	/** Default constructor */
	FSelectedSocketInfo()
		: Socket( NULL )
		, bSocketIsOnSkeleton( false )
	{

	}

	/** Constructor */
	FSelectedSocketInfo( class USkeletalMeshSocket* InSocket, bool bInSocketIsOnSkeleton )
		: Socket( InSocket )
		, bSocketIsOnSkeleton( bInSocketIsOnSkeleton )
	{

	}

	/** The socket we have selected */
	class USkeletalMeshSocket* Socket;

	/** true if on skeleton, false if on mesh */
	bool bSocketIsOnSkeleton;


};

UCLASS(transient, MinimalAPI)
class UDebugSkelMeshComponent : public USkeletalMeshComponent
{
	GENERATED_UCLASS_BODY()

	/** If true, render a wireframe skeleton of the mesh animated with the raw (uncompressed) animation data. */
	UPROPERTY()
	uint32 bRenderRawSkeleton:1;

	/** Holds onto the bone color that will be used to render the bones of its skeletal mesh */
	//var Color		BoneColor;
	
	/** If true then the skeletal mesh associated with the component is drawn. */
	UPROPERTY()
	uint32 bDrawMesh:1;

	/** If true then the bone names associated with the skeletal mesh are displayed */
	UPROPERTY()
	uint32 bShowBoneNames:1;

	/** Bone influences viewing */
	UPROPERTY(transient)
	uint32 bDrawBoneInfluences:1;

	/** Vertex normal viewing */
	UPROPERTY(transient)
	uint32 bDrawNormals:1;

	/** Vertex tangent viewing */
	UPROPERTY(transient)
	uint32 bDrawTangents:1;

	/** Vertex binormal viewing */
	UPROPERTY(transient)
	uint32 bDrawBinormals:1;

	/** Socket hit points viewing */
	UPROPERTY(transient)
	uint32 bDrawSockets:1;

	/** Skeleton sockets visible? */
	UPROPERTY(transient)
	uint32 bSkeletonSocketsVisible:1;

	/** Mesh sockets visible? */
	UPROPERTY(transient)
	uint32 bMeshSocketsVisible:1;

	/** Display raw animation bone transform */
	UPROPERTY(transient)
	uint32 bDisplayRawAnimation:1;

	/** Display non retargeted animation pose */
	UPROPERTY(Transient)
	uint32 bDisplayNonRetargetedPose:1;

	/** Display additive base bone transform */
	UPROPERTY(transient)
	uint32 bDisplayAdditiveBasePose:1;

	/** Display Bound **/
	UPROPERTY(transient)
	uint32 bDisplayBound:1;

	/** Compressed SpaceBases for when bDisplayRawAnimation == true, as raw space bases are stored in SpaceBases **/
	TArray<FTransform> CompressedSpaceBases;

	/** Storage of Additive Base Pose for when bDisplayAdditiveBasePose == true, as they have to be calculated */
	TArray<FTransform> AdditiveBasePoses;

	/** Storage for non retargeted pose. */
	TArray<FTransform> NonRetargetedSpaceBases;

	/** Color render mode enum value - 0 - none, 1 - tangent, 2 - normal, 3 - mirror, 4 - bone weighting */
	//var native transient int ColorRenderMode;
	
	/** Array of bones to render bone weights for */
	UPROPERTY(transient)
	TArray<int32> BonesOfInterest;
	
	/** Array of sockets to render manipulation widgets for
	/	Storing a pointer to the actual socket rather than a name, as we don't care here
	/	whether the socket is on the skeleton or the mesh! */
	UPROPERTY(transient)
	TArray< FSelectedSocketInfo > SocketsOfInterest;

	/** Array of materials to restore when not rendering blend weights */
	UPROPERTY(transient)
	TArray<class UMaterialInterface*> SkelMaterials;
	
	UPROPERTY(transient)
	class UAnimPreviewInstance* PreviewInstance;

	UPROPERTY(transient)
	class UAnimInstance* SavedAnimScriptInstance;

	/** Does this component use in game bounds or does it use bounds calculated from bones */
	UPROPERTY(transient)
	bool bIsUsingInGameBounds;
	
	/** true if wind effects on clothing is enabled */
	bool bEnableWind;

	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// End USceneComponent interface.

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	
	// engine only draw bounds IF selected
	// @todo fix this properly
	// this isn't really the best way to do this, but for now
	// we'll just mark as selected
	virtual bool ShouldRenderSelected() const OVERRIDE
	{
		return bDisplayBound;
	}
	// End UPrimitiveComponent interface.

	// Begin SkinnedMeshComponent Interface
	virtual bool ShouldCPUSkin() OVERRIDE;
	virtual void PostInitMeshObject(class FSkeletalMeshObject* MeshObject) OVERRIDE;
	virtual void RefreshBoneTransforms() OVERRIDE;
	// End SkinnedMeshComponent Interface

	// Begin SkeletalMeshComponent Interface
	virtual void InitAnim(bool bForceReinit) OVERRIDE;
	virtual bool IsWindEnabled() const OVERRIDE;
	// End SkeletalMeshComponent Interface
	// Preview.
	// @todo document
	UNREALED_API bool IsPreviewOn() const;

	// @todo document
	UNREALED_API FString GetPreviewText() const;

	// @todo anim : you still need to give asset, so that we know which one to disable
	// we can disable per asset, so that if some other window disabled before me, I don't accidently turn it off
	UNREALED_API void EnablePreview(bool bEnable, class UAnimationAsset * PreviewAsset, UVertexAnimation* PreviewVertexAnim);

	/**
	 * Update material information depending on color render mode 
	 * Refresh/replace materials 
	 */
	UNREALED_API void SetShowBoneWeight(bool bNewShowBoneWeight);

	/**
	 * Does it use in-game bounds or bounds calculated from bones
	 */
	UNREALED_API bool IsUsingInGameBounds() const;

	/**
	 * Set to use in-game bounds or bounds calculated from bones
	 */
	UNREALED_API void UseInGameBounds(bool bUseInGameBounds);

	/**
	 * Test if in-game bounds are as big as preview bounds
	 */
	UNREALED_API bool CheckIfBoundsAreCorrrect();
};



