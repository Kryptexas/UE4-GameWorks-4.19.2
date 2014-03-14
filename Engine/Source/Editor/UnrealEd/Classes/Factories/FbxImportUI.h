// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Copyright 2010 Autodesk, Inc. All Rights Reserved.
 * 
 * Fbx Importer UI options.
 */

#pragma once
#include "FbxImportUI.generated.h"

/** Import mesh type */
UENUM()
enum EFBXImportType
{
	/** Static mesh */
	FBXIT_StaticMesh UMETA(DisplayName="Static Mesh"),
	/** Skeletal mesh */
	FBXIT_SkeletalMesh UMETA(DisplayName="Skeletal Mesh"),
	/** Animation */
	FBXIT_Animation UMETA(DisplayName="Animation"),
	FBXIT_MAX,
};

UCLASS(config=EditorUserSettings, AutoExpandCategories=(General, USkeletalMesh, UStaticMesh, Materials), HideCategories=Object)
class UFbxImportUI : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Type of asset to import from the FBX file */
	UPROPERTY(EditAnywhere, Category=General, meta=(DisplayName = "Import Type", ToolTip="The type of mesh being imported"))
	TEnumAsByte<enum EFBXImportType> MeshTypeToImport;

	/** Use the string in "Name" field as full name of mesh. The option only works when the scene contains one mesh. */
	UPROPERTY(EditAnywhere, config, Category=General)
	uint32 bOverrideFullName:1;

	/** For static meshes, enabling this option will combine all meshes in the FBX into a single monolithic mesh in Unreal */
	UPROPERTY(EditAnywhere, config, Category=StaticMesh, meta=(ToolTip="If enabled, combines all meshes into a single mesh"))
	uint32 bCombineMeshes:1;

	/** Create Skeleton from this SkeletalMesh with postfix _Skeleton. Don't want this flag to be saved as config, so removed config. */
	UPROPERTY(EditAnywhere, Category=SkeletalMesh)
	class USkeleton* Skeleton;

	/** If checked, create new PhysicsAsset if it doesn't have it */
	UPROPERTY(EditAnywhere, config, Category=SkeletalMesh)
	uint32 bCreatePhysicsAsset:1;

	/** If this is set, use this PhysicsAsset. It is possible bCreatePhysicsAsset == false, and PhysicsAsset == NULL. It is possible they do not like to create anything. */
	UPROPERTY(EditAnywhere, Category=SkeletalMesh)
	class UPhysicsAsset* PhysicsAsset;

	/** True to import animations from the FBX File */
	UPROPERTY(EditAnywhere, config, Category=SkeletalMesh)
	uint32 bImportAnimations:1;

	/** If you import animation and if you'd like to chnage name, please type here **/
	UPROPERTY(EditAnywhere, Category=SkeletalMesh, meta=(DisplayName = "Animation Name", editcondition = "bImportAnimations")) 
	FString AnimationName;

	/** Enables importing of 'rigid skeletalmesh' (unskinned, hierarchy-based animation) from this FBX file */
	UPROPERTY(EditAnywhere, config, Category=SkeletalMesh, meta=(editcondition = "bImportAnimations"))
	uint32 bImportRigidMesh:1;

	/** Enable this option to use default sample rate for the imported animation at 30 frames per second */
	UPROPERTY(EditAnywhere, config, Category=SkeletalMesh, meta=(editcondition = "bImportAnimations", ToolTip="If enabled, samples all animation curves to 30 FPS"))
	uint32 bUseDefaultSampleRate:1;

	/** Whether to automatically create Unreal materials for materials found in the FBX scene */
	UPROPERTY(EditAnywhere, config, Category=Materials)
	uint32 bImportMaterials:1;

	/** The option works only when option "Import UMaterial" is OFF. If "Import UMaterial" is ON, textures are always imported. */
	UPROPERTY(EditAnywhere, config, Category=Materials)
	uint32 bImportTextures:1;

	/** Import data used when importing static meshes */
	UPROPERTY(EditAnywhere, editinline, Category=StaticMesh)
	class UFbxStaticMeshImportData* StaticMeshImportData;

	/** Import data used when importing skeletal meshes */
	UPROPERTY(EditAnywhere, editinline, Category=SkeletalMesh)
	class UFbxSkeletalMeshImportData* SkeletalMeshImportData;

	/** Import data used when importing animations */
	UPROPERTY(EditAnywhere, editinline, Category=Animation)
	class UFbxAnimSequenceImportData* AnimSequenceImportData;

	/** Type of asset to import from the FBX file */
	UPROPERTY(EditAnywhere, config, Category=ImportSettings)
	bool	bPreserveLocalTransform;

	/** Import data used when importing textures */
	UPROPERTY(EditAnywhere, editinline, Category=Textures)
	class UFbxTextureImportData* TextureImportData;

	// Begin UObject Interface
	virtual bool CanEditChange( const UProperty* InProperty ) const OVERRIDE;
	// End UObject Interface
};



