// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/**
 * Fbx Importer UI options.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Factories/ImportSettings.h"
#include "FbxImportUI.generated.h"

/** Import mesh type */
UENUM()
enum EFBXImportType
{
	/** Select Static Mesh if you'd like to import static mesh. */
	FBXIT_StaticMesh UMETA(DisplayName="Static Mesh"),
	/** Select Skeletal Mesh if you'd like to import skeletal mesh. */
	FBXIT_SkeletalMesh UMETA(DisplayName="Skeletal Mesh"),
	/** Select Animation if you'd like to import only animation. */
	FBXIT_Animation UMETA(DisplayName="Animation"),
	/** Subdivision Surface (Experimental, Early work in progress) */
	FBXIT_SubDSurface UMETA(DisplayName="SubDSurface"),

	FBXIT_MAX,
};

UCLASS(config=EditorPerProjectUserSettings, AutoExpandCategories=(FTransform), HideCategories=Object, MinimalAPI)
class UFbxImportUI : public UObject, public IImportSettingsParser
{
	GENERATED_UCLASS_BODY()

public:
	/** Whether or not the imported file is in OBJ format */
	UPROPERTY()
	bool bIsObjImport;

	/** The original detected type of this import */
	UPROPERTY()
	TEnumAsByte<enum EFBXImportType> OriginalImportType;

	/** Type of asset to import from the FBX file */
	UPROPERTY()
	TEnumAsByte<enum EFBXImportType> MeshTypeToImport;

	/** Use the string in "Name" field as full name of mesh. The option only works when the scene contains one mesh. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, config, Category=Miscellaneous, meta=(OBJRestrict="true"))
	uint32 bOverrideFullName:1;

	/** Whether to import the incoming FBX as a skeletal object */
	UPROPERTY(EditAnywhere, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Skeletal Mesh"))
	bool bImportAsSkeletal;
	
	/** Whether to import the incoming FBX as a Subdivision Surface (could be made a combo box together with bImportAsSkeletal) (Experimental, Early work in progress) */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Mesh, meta = (ImportType = "StaticMesh|SkeletalMesh", DisplayName="Subdivison Surface"))
	bool bImportAsSubDSurface;

	/** Whether to import the mesh. Allows animation only import when importing a skeletal mesh. */
	UPROPERTY(EditAnywhere, Category=Mesh, meta=(ImportType="SkeletalMesh"))
	bool bImportMesh;

	/** Skeleton to use for imported asset. When importing a mesh, leaving this as "None" will create a new skeleton. When importing an animation this MUST be specified to import the asset. */
	UPROPERTY(EditAnywhere, Category=Mesh, meta=(ImportType="SkeletalMesh|Animation"))
	class USkeleton* Skeleton;

	/** If checked, create new PhysicsAsset if it doesn't have it */
	UPROPERTY(EditAnywhere, AdvancedDisplay, config, Category=Mesh, meta=(ImportType="SkeletalMesh"))
	uint32 bCreatePhysicsAsset:1;

	/** If this is set, use this PhysicsAsset. It is possible bCreatePhysicsAsset == false, and PhysicsAsset == NULL. It is possible they do not like to create anything. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Mesh, meta=(ImportType="SkeletalMesh", editcondition="!bCreatePhysicsAsset"))
	class UPhysicsAsset* PhysicsAsset;

	/** True to import animations from the FBX File */
	UPROPERTY(EditAnywhere, config, Category=Animation, meta=(ImportType="SkeletalMesh|Animation"))
	uint32 bImportAnimations:1;

	/** Override for the name of the animation to import. By default, it will be the name of FBX **/
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Animation, meta=(editcondition="bImportAnimations", ImportType="SkeletalMesh")) 
	FString OverrideAnimationName;

	/** Enables importing of 'rigid skeletalmesh' (unskinned, hierarchy-based animation) from this FBX file, no longer shown, used behind the scenes */
	UPROPERTY()
	uint32 bImportRigidMesh:1;

	/** Whether to automatically create Unreal materials for materials found in the FBX scene */
	UPROPERTY(EditAnywhere, config, Category = Material, meta = (OBJRestrict="true"))
	uint32 bImportMaterials:1;

	/** The option works only when option "Import UMaterial" is OFF. If "Import UMaterial" is ON, textures are always imported. */
	UPROPERTY(EditAnywhere, config, Category=Material, meta=(OBJRestrict="true"))
	uint32 bImportTextures:1;

	/** Import data used when importing static meshes */
	UPROPERTY(EditAnywhere, Transient, Instanced, Category = Mesh, meta=(ImportType = "StaticMesh"))
	class UFbxStaticMeshImportData* StaticMeshImportData;

	/** Import data used when importing skeletal meshes */
	UPROPERTY(EditAnywhere, Transient, Instanced, Category=Mesh, meta=(ImportType = "SkeletalMesh"))
	class UFbxSkeletalMeshImportData* SkeletalMeshImportData;

	/** Import data used when importing animations */
	UPROPERTY(EditAnywhere, Transient, Instanced, Category=Animation, meta=(editcondition="bImportAnimations", ImportType = "Animation"))
	class UFbxAnimSequenceImportData* AnimSequenceImportData;

	/** Import data used when importing textures */
	UPROPERTY(EditAnywhere, Transient, Instanced, Category=Material)
	class UFbxTextureImportData* TextureImportData;

	/** If true the automated import path should detect the import type.  If false the import type was specified by the user */
	UPROPERTY()
	bool bAutomatedImportShouldDetectType;

	/** UObject Interface */
	virtual bool CanEditChange( const UProperty* InProperty ) const override;

	/** IImportSettings Interface */
	virtual void ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson) override;

	/** sets MeshTypeToImport */
	void SetMeshTypeToImport()
	{
		MeshTypeToImport = bImportAsSkeletal ? FBXIT_SkeletalMesh : FBXIT_StaticMesh;

		if(bImportAsSubDSurface)
		{
			MeshTypeToImport = FBXIT_SubDSurface;
		}
	}
};


