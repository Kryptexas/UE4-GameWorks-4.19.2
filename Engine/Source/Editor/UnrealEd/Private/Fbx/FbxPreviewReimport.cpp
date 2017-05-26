// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Static mesh creation from FBX data.
	Largely based on StaticMeshEdit.cpp
=============================================================================*/

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "UObject/Object.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Editor.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "FbxImporter.h"
#include "Misc/FbxErrors.h"

//Meshes includes
#include "MeshUtilities.h"
#include "RawMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "GeomFitUtils.h"
#include "PhysicsAssetUtils.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"

//Static mesh includes
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "StaticMeshResources.h"

//Skeletal mesh includes
#include "SkelImport.h"
#include "SkeletalMeshTypes.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AnimEncoding.h"
#include "ApexClothingUtils.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Assets/ClothingAsset.h"



#define LOCTEXT_NAMESPACE "FbxPreviewReimport"

using namespace UnFbx;

class FCompMaterial
{
public:
	FCompMaterial(FName InMaterialSlotName, FName InImportedMaterialSlotName)
		: MaterialSlotName(InMaterialSlotName)
		, ImportedMaterialSlotName(InImportedMaterialSlotName)
	{}
	FName MaterialSlotName;
	FName ImportedMaterialSlotName;
};

class FCompSection
{
public:
	int32 MaterialIndex;
};

class FCompLOD
{
public:
	TArray<FCompSection> Sections;
};

class FCompMesh
{
public:
	TArray<FCompMaterial> Materials;
	TArray<FCompLOD> Lods;
};

class FCompSkeletalMesh : public FCompMesh
{
public:
	FCompSkeletalMesh(USkeletalMesh*InSkeletalMesh)
		: SkeletalMesh(InSkeletalMesh)
	{
		//Fill the material array
		for (const FSkeletalMaterial &Material : SkeletalMesh->Materials)
		{
			FCompMaterial CompMaterial(Material.MaterialSlotName, Material.ImportedMaterialSlotName);
			Materials.Add(CompMaterial);
		}
		if (SkeletalMesh->GetResourceForRendering())
		{
			//Fill sections data
			for (int32 LodIndex = 0; LodIndex < SkeletalMesh->GetResourceForRendering()->LODModels.Num(); ++LodIndex)
			{
				//Find the LodMaterialMap, which must be use for all LOD except the base
				TArray<int32> LODMaterialMap;
				if(LodIndex > 0 && SkeletalMesh->LODInfo.IsValidIndex(LodIndex))
				{
					const FSkeletalMeshLODInfo &SkeletalMeshLODInfo = SkeletalMesh->LODInfo[LodIndex];
					LODMaterialMap = SkeletalMeshLODInfo.LODMaterialMap;
				}

				const FStaticLODModel &StaticLodModel = SkeletalMesh->GetResourceForRendering()->LODModels[LodIndex];
				for (int32 SectionIndex = 0; SectionIndex < StaticLodModel.Sections.Num(); ++SectionIndex)
				{
					const FSkelMeshSection &SkelMeshSection = StaticLodModel.Sections[SectionIndex];
					int32 MaterialIndex = SkelMeshSection.MaterialIndex;
					if (LodIndex > 0 && LODMaterialMap.IsValidIndex(MaterialIndex))
					{
						MaterialIndex = LODMaterialMap[MaterialIndex];
					}
				}
			}
		}
		else
		{
			
		}
	}

	USkeletalMesh *GetSkeletalMesh() const { return SkeletalMesh; }
	const FCompMesh &GetCompMesh() const { return *((FCompMesh *)this); }
private:
	USkeletalMesh *SkeletalMesh;
};

class FCompStaticMesh : public FCompMesh
{
public:
	FCompStaticMesh(UStaticMesh* InStaticMesh)
		: StaticMesh(InStaticMesh)
	{
		//Fill the material array
		for (const FStaticMaterial &Material : StaticMesh->StaticMaterials)
		{
			FCompMaterial CompMaterial(Material.MaterialSlotName, Material.ImportedMaterialSlotName);
			Materials.Add(CompMaterial);
		}
	}

	UStaticMesh *GetStaticMesh() const { return StaticMesh; }
	const FCompMesh &GetCompMesh() const { return *((FCompMesh *)this); }
private:
	UStaticMesh *StaticMesh;
};

#undef LOCTEXT_NAMESPACE
