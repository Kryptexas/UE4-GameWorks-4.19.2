// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Assets/ClothingAsset.h"
#include "PhysXPublic.h"

#if WITH_EDITOR
#include "Engine/SkeletalMesh.h"
#include "MeshUtilities.h"
#endif

#include "Components/SkeletalMeshComponent.h"

#include "PhysicsPublic.h"
#include "UObjectIterator.h"
#include "ModuleManager.h"
#include "SNotificationList.h"
#include "NotificationManager.h"
#include "ComponentReregisterContext.h"

DEFINE_LOG_CATEGORY(LogClothingAsset)

#define LOCTEXT_NAMESPACE "ClothingAsset"

UClothingAsset::UClothingAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

#if WITH_EDITOR

bool UClothingAsset::BindToSkeletalMesh(USkeletalMesh* InSkelMesh, int32 InMeshLodIndex, int32 InSectionIndex, int32 InAssetLodIndex)
{
	// If we've been added to the wrong mesh
	if(InSkelMesh != GetOuter())
	{
		UE_LOG(LogClothingAsset, Warning, TEXT("Failed to bind clothing asset %s as the provided mesh is not the owner of this asset."), *GetName());

		return false;
	}

	// If we don't have clothing data
	if(!LodData.IsValidIndex(InAssetLodIndex))
	{
		UE_LOG(LogClothingAsset, Warning, TEXT("Failed to bind clothing asset %s LOD%d as LOD%d does not exist."), *GetName(), InAssetLodIndex, InAssetLodIndex);

		return false;
	}

	// If we don't have a mesh
	if(!InSkelMesh)
	{
		UE_LOG(LogClothingAsset, Warning, TEXT("Failed to bind clothing asset %s as provided skel mesh does not exist."), *GetName());

		return false;
	}

	// If the mesh LOD index is invalid
	if(!InSkelMesh->GetImportedResource()->LODModels.IsValidIndex(InMeshLodIndex))
	{
		UE_LOG(LogClothingAsset, Warning, TEXT("Failed to bind clothing asset %s as mesh LOD%d does not exist."), *GetName(), InMeshLodIndex);

		return false;
	}

	for(const int32& MappedLod : LodMap)
	{
		if(MappedLod == InAssetLodIndex)
		{
			UE_LOG(LogClothingAsset, Warning, TEXT("Failed to bind clothing asset %s LOD%d as LOD%d is already mapped to mesh LOD%d."), *GetName(), InAssetLodIndex, InAssetLodIndex, InMeshLodIndex);
			return false;
		}
	}

	if(LodMap.IsValidIndex(InMeshLodIndex) && LodMap[InMeshLodIndex] != INDEX_NONE)
	{
		// Already mapped
		return false;
	}

	// Grab the clothing and skel lod data
	FClothLODData& ClothLodData = LodData[InAssetLodIndex];
	FStaticLODModel& SkelLod = InSkelMesh->GetImportedResource()->LODModels[InMeshLodIndex];

	FSkelMeshSection* OriginalSection = &SkelLod.Sections[InSectionIndex];
	FSkelMeshSection NewClothSection;

	// Data for mesh to mesh binding
	TArray<FMeshToMeshVertData> MeshToMeshData;
	TArray<FVector> RenderPositions;
	TArray<FVector> RenderNormals;
	TArray<FVector> RenderTangents;

	RenderPositions.Reserve(OriginalSection->SoftVertices.Num());
	RenderNormals.Reserve(OriginalSection->SoftVertices.Num());
	RenderTangents.Reserve(OriginalSection->SoftVertices.Num());

	// Original data to weight to the clothing simulation mesh
	for(FSoftSkinVertex& UnrealVert : OriginalSection->SoftVertices)
	{
		RenderPositions.Add(UnrealVert.Position);
		RenderNormals.Add(UnrealVert.TangentZ);
		RenderTangents.Add(UnrealVert.TangentX);
	}

	ClothingAssetUtils::GenerateMeshToMeshSkinningData(MeshToMeshData,
													  RenderPositions,
													  RenderNormals,
													  RenderTangents,
													  ClothLodData.PhysicalMeshData.Vertices,
													  ClothLodData.PhysicalMeshData.Normals,
													  ClothLodData.PhysicalMeshData.Indices);

	if(MeshToMeshData.Num() == 0)
	{
		// Failed to generate skinning data, the function above will have notified
		// with the cause of the failure, so just exit
		return false;
	}

	// Calculate fixed verts
	for(FMeshToMeshVertData& VertData : MeshToMeshData)
	{
		float TriangleDistanceMax = 0.0f;
		TriangleDistanceMax += ClothLodData.PhysicalMeshData.MaxDistances[VertData.SourceMeshVertIndices[0]];
		TriangleDistanceMax += ClothLodData.PhysicalMeshData.MaxDistances[VertData.SourceMeshVertIndices[1]];
		TriangleDistanceMax += ClothLodData.PhysicalMeshData.MaxDistances[VertData.SourceMeshVertIndices[2]];

		if(TriangleDistanceMax == 0.0f)
		{
			VertData.SourceMeshVertIndices[3] = 0xFFFF;
		}
	}

	NewClothSection.SoftVertices = OriginalSection->SoftVertices;
	NewClothSection.NumVertices = NewClothSection.SoftVertices.Num();
	NewClothSection.BoneMap = OriginalSection->BoneMap;
	NewClothSection.MaxBoneInfluences = OriginalSection->MaxBoneInfluences;

	for(FName& BoneName : UsedBoneNames)
	{
		const int32 BoneIndex = InSkelMesh->RefSkeleton.FindBoneIndex(BoneName);

		if(BoneIndex != INDEX_NONE)
		{
			NewClothSection.BoneMap.AddUnique(BoneIndex);
		}
	}

	// Array of re-import contexts for components using this mesh
	TIndirectArray<FComponentReregisterContext> ComponentContexts;
	for (TObjectIterator<USkeletalMeshComponent> It; It; ++It)
	{
		USkeletalMeshComponent* Component = *It;
		if (Component && !Component->IsTemplate() && Component->SkeletalMesh == InSkelMesh)
		{
			new(ComponentContexts) FComponentReregisterContext(Component);
		}
	}

	// Ready to apply the changes
	InSkelMesh->PreEditChange(nullptr);

	// calculate LOD verts before adding our new section
	uint32 NumLodVertices = 0;
	for(const FSkelMeshSection& CurSection : SkelLod.Sections)
	{
		NumLodVertices += CurSection.GetNumVertices();
	}

	int32 NewSectionIdx = SkelLod.Sections.AddDefaulted();

	SkelLod.Sections[NewSectionIdx] = SkelLod.Sections[InSectionIndex];

	// Re set original here, may have just realloced
	OriginalSection = &SkelLod.Sections[InSectionIndex];
	FSkelMeshSection& ClothSection = SkelLod.Sections[NewSectionIdx];
	
	ClothSection.bDisabled = false;
	ClothSection.CorrespondClothSectionIndex = InSectionIndex;

	OriginalSection->bDisabled = true;
	OriginalSection->CorrespondClothSectionIndex = NewSectionIdx;

	// Geo properties
	ClothSection.SoftVertices = NewClothSection.SoftVertices;
	ClothSection.NumVertices = NewClothSection.NumVertices;
	ClothSection.BoneMap = NewClothSection.BoneMap;
	ClothSection.MaxBoneInfluences = NewClothSection.MaxBoneInfluences;

	// Set the asset index, used during rendering to pick the correct sim mesh buffer
	int32 AssetIndex = INDEX_NONE;
	check(InSkelMesh->MeshClothingAssets.Find(this, AssetIndex));
	ClothSection.CorrespondClothAssetIndex = AssetIndex;
	
	ClothSection.BaseVertexIndex = NumLodVertices;

	// Material properties
	ClothSection.MaterialIndex = OriginalSection->MaterialIndex;
	ClothSection.TriangleSorting = OriginalSection->TriangleSorting;
	ClothSection.bSelected = OriginalSection->bSelected;
	ClothSection.bCastShadow = OriginalSection->bCastShadow;
	ClothSection.bRecomputeTangent = OriginalSection->bRecomputeTangent;
	ClothSection.bEnableClothLOD_DEPRECATED = OriginalSection->bEnableClothLOD_DEPRECATED;

	// sim properties
	ClothSection.ClothMappingData = MeshToMeshData;
	ClothSection.PhysicalMeshVertices = ClothLodData.PhysicalMeshData.Vertices;
	ClothSection.PhysicalMeshNormals = ClothLodData.PhysicalMeshData.Normals;

	// Set asset links on both sections
	ClothSection.ClothingData.AssetLodIndex = InAssetLodIndex;
	ClothSection.ClothingData.AssetGuid = AssetGuid;

	OriginalSection->ClothingData.AssetGuid = AssetGuid;
	OriginalSection->ClothingData.AssetLodIndex = InAssetLodIndex;

	// Copy original indices over
	const int32 BaseIndexToCopy = OriginalSection->BaseIndex;
	const int32 NumIndicesToCopy = OriginalSection->NumTriangles * 3;
	const int32 BaseVertexOffset = ClothSection.BaseVertexIndex - OriginalSection->BaseVertexIndex;

	FMultiSizeIndexContainerData NewIndexData;
	SkelLod.MultiSizeIndexContainer.GetIndexBuffer(NewIndexData.Indices);

	ClothSection.BaseIndex = NewIndexData.Indices.Num();
	ClothSection.NumTriangles = OriginalSection->NumTriangles;

	for(int32 CurrIdx = 0; CurrIdx < NumIndicesToCopy; ++CurrIdx)
	{
		int32 SourceIndex = NewIndexData.Indices[BaseIndexToCopy + CurrIdx];
		NewIndexData.Indices.Add(SourceIndex + BaseVertexOffset);
	}

	SkelLod.NumVertices += ClothSection.GetNumVertices();

	if(SkelLod.NumVertices > MAX_uint16)
	{
		NewIndexData.DataTypeSize = sizeof(uint32);
	}
	else
	{
		NewIndexData.DataTypeSize = sizeof(uint16);
	}

	SkelLod.MultiSizeIndexContainer.RebuildIndexBuffer(NewIndexData);
	
	// Build adjacency info (Only need soft verts for cloth)
	TArray<FSoftSkinVertex> TempVerts;
	FMultiSizeIndexContainerData AdjacencyData;
	SkelLod.GetVertices(TempVerts);
	IMeshUtilities& MeshUtils = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	MeshUtils.BuildSkeletalAdjacencyIndexBuffer(TempVerts, SkelLod.NumTexCoords, NewIndexData.Indices, AdjacencyData.Indices);
	SkelLod.AdjacencyMultiSizeIndexContainer.RebuildIndexBuffer(AdjacencyData);

	bool bRequireBoneChange = false;
	for(FBoneIndexType& BoneIndex : ClothSection.BoneMap)
	{
		if(!SkelLod.RequiredBones.Contains(BoneIndex))
		{
			bRequireBoneChange = true;
			if(InSkelMesh->RefSkeleton.IsValidIndex(BoneIndex))
			{
				SkelLod.RequiredBones.Add(BoneIndex);
				SkelLod.ActiveBoneIndices.AddUnique(BoneIndex);
			}
		}
	}

	if(bRequireBoneChange)
	{
		SkelLod.RequiredBones.Sort();
		SkelLod.ActiveBoneIndices.Sort();
	}

	if(CustomData)
	{
		CustomData->BindToSkeletalMesh(InSkelMesh, InMeshLodIndex, InSectionIndex, InAssetLodIndex);
	}

	// Make sure the LOD map is always big enough for the asset to use.
	// This shouldn't grow to an unwieldy size but maybe consider compacting later.
	while(LodMap.Num() - 1 < InMeshLodIndex)
	{
		LodMap.Add(INDEX_NONE);
	}

	LodMap[InMeshLodIndex] = InAssetLodIndex;

	InSkelMesh->PostEditChange();

	return true;

	// ComponentContexts goes out of scope, causing components to be re-registered
}

void UClothingAsset::UnbindFromSkeletalMesh(USkeletalMesh* InSkelMesh)
{
	if(FSkeletalMeshResource* Mesh = InSkelMesh->GetImportedResource())
	{
		const int32 NumLods = Mesh->LODModels.Num();

		for(int32 LodIndex = 0; LodIndex < NumLods; ++LodIndex)
		{
			UnbindFromSkeletalMesh(InSkelMesh, LodIndex);
		}
	}
}

void UClothingAsset::UnbindFromSkeletalMesh(USkeletalMesh* InSkelMesh, int32 InMeshLodIndex)
{
	bool bChangedMesh = false;

	// Find the chunk(s) we created
	if(FSkeletalMeshResource* Mesh = InSkelMesh->GetImportedResource())
	{
		if(!Mesh->LODModels.IsValidIndex(InMeshLodIndex))
		{
			UE_LOG(LogClothingAsset, Warning, TEXT("Failed to remove clothing asset %s from mesh LOD%d as that LOD doesn't exist."), *GetName(), InMeshLodIndex);
			return;
		}

		FStaticLODModel& LodModel = Mesh->LODModels[InMeshLodIndex];

		{
			for(int32 SectionIdx = LodModel.Sections.Num() - 1; SectionIdx >= 0; --SectionIdx)
			{
				FSkelMeshSection& ClothSection = LodModel.Sections[SectionIdx];

				if(ClothSection.ClothingData.IsValid() && ClothSection.ClothingData.AssetGuid == AssetGuid)
				{
					// We made this one.
					int32 OriginalSectionIdx = INDEX_NONE;

					// Find the original section
					for(int32 SearchIdx = 0; SearchIdx < LodModel.Sections.Num(); ++SearchIdx)
					{
						FSkelMeshSection& PossibleOriginalSection = LodModel.Sections[SearchIdx];

						if(PossibleOriginalSection.CorrespondClothSectionIndex == SectionIdx)
						{
							OriginalSectionIdx = SearchIdx;
							break;
						}
					}

					check(OriginalSectionIdx != INDEX_NONE);

					InSkelMesh->PreEditChange(nullptr);

					FSkelMeshSection& OriginalSection = LodModel.Sections[OriginalSectionIdx];

					FMultiSizeIndexContainerData NewIndexData;
					LodModel.MultiSizeIndexContainer.GetIndexBuffer(NewIndexData.Indices);

					uint32 NumIndicesToRemove = ClothSection.NumTriangles * 3;
					uint32 BaseIndexToRemove = ClothSection.BaseIndex;

					uint32 NumVertsToRemove = ClothSection.GetNumVertices();
					uint32 BaseVertToRemove = ClothSection.BaseVertexIndex;

					// Remove from index buffer
					NewIndexData.Indices.RemoveAt(BaseIndexToRemove, NumIndicesToRemove);

					// Fix up remaining indices (can this start at baseindextoremove?)
					for(uint32& Index : NewIndexData.Indices)
					{
						if(Index >= BaseVertToRemove)
						{
							Index -= NumVertsToRemove;
						}
					}

					// Count resulting vertices
					int32 NumVertsAfter = 0;
					for(int32 ResultSectionsIndex = 0; ResultSectionsIndex < SectionIdx; ++ResultSectionsIndex)
					{
						NumVertsAfter += LodModel.Sections[ResultSectionsIndex].NumVertices;
					}

					if(NumVertsAfter > MAX_uint16)
					{
						NewIndexData.DataTypeSize = sizeof(uint16);
					}
					else
					{
						NewIndexData.DataTypeSize = sizeof(uint32);
					}

					// Push back to buffer
					LodModel.MultiSizeIndexContainer.RebuildIndexBuffer(NewIndexData);

					LodModel.Sections.RemoveAt(SectionIdx);

					LodModel.NumVertices -= NumVertsToRemove;

					// Fix up other data
					for(FSkelMeshSection& Section : LodModel.Sections)
					{
						if(Section.CorrespondClothSectionIndex > OriginalSection.CorrespondClothSectionIndex)
						{
							// We removed one here, so knock back
							Section.CorrespondClothSectionIndex -= 1;
						}

						if(Section.BaseIndex > BaseIndexToRemove)
						{
							Section.BaseIndex -= NumIndicesToRemove;
						}

						if(Section.BaseVertexIndex > BaseVertToRemove)
						{
							Section.BaseVertexIndex -= NumVertsToRemove;
						}
					}

					// Enable rendering for section
					OriginalSection.bDisabled = false;

					// Clear cloth link data
					OriginalSection.CorrespondClothSectionIndex = INDEX_NONE;
					OriginalSection.ClothingData.AssetLodIndex = INDEX_NONE;
					OriginalSection.ClothingData.AssetGuid = FGuid();

					// Track the mesh change so we can re-register later
					bChangedMesh = true;

					// Set the LOD map so nothing is mapped for this asset
					LodMap[InMeshLodIndex] = INDEX_NONE;

					InSkelMesh->PostEditChange();
				}
			}
		}
	}

	// If the mesh changed we need to re-register any components that use it to reflect the changes
	if(bChangedMesh)
	{
		for(TObjectIterator<USkeletalMeshComponent> It; It; ++It)
		{
			USkeletalMeshComponent* MeshComponent = *It;
			if(MeshComponent &&
			   !MeshComponent->IsTemplate() &&
			   MeshComponent->SkeletalMesh == InSkelMesh)
			{
				MeshComponent->ReregisterComponent();
			}
		}
	}
}

void UClothingAsset::RefreshBoneMapping(USkeletalMesh* InSkelMesh)
{
	// No mesh, can't remap
	if(!InSkelMesh)
	{
		return;
	}

	if(UsedBoneNames.Num() != UsedBoneIndices.Num())
	{
		UsedBoneIndices.Reset();
		UsedBoneIndices.AddDefaulted(UsedBoneNames.Num());
	}

	// Repopulate the used indices.
	for(int32 BoneNameIndex = 0; BoneNameIndex < UsedBoneNames.Num(); ++BoneNameIndex)
	{
		UsedBoneIndices[BoneNameIndex] = InSkelMesh->RefSkeleton.FindBoneIndex(UsedBoneNames[BoneNameIndex]);
	}
}

void UClothingAsset::InvalidateCachedData()
{
	for(FClothLODData& CurrentLodData : LodData)
	{
		// Recalculate inverse masses for the physical mesh particles
		FClothPhysicalMeshData& PhysMesh = CurrentLodData.PhysicalMeshData;

		check(PhysMesh.Indices.Num() % 3 == 0);

		TArray<float>& InvMasses = PhysMesh.InverseMasses;

		const int32 NumVerts = PhysMesh.Vertices.Num();
		InvMasses.Empty(NumVerts);
		InvMasses.AddZeroed(NumVerts);

		for(int32 TriBaseIndex = 0; TriBaseIndex < PhysMesh.Indices.Num(); TriBaseIndex += 3)
		{
			const int32 Index0 = PhysMesh.Indices[TriBaseIndex];
			const int32 Index1 = PhysMesh.Indices[TriBaseIndex + 1];
			const int32 Index2 = PhysMesh.Indices[TriBaseIndex + 2];

			const FVector AB = PhysMesh.Vertices[Index1] - PhysMesh.Vertices[Index0];
			const FVector AC = PhysMesh.Vertices[Index2] - PhysMesh.Vertices[Index0];
			const float TriArea = FVector::CrossProduct(AB, AC).Size();

			InvMasses[Index0] += TriArea;
			InvMasses[Index1] += TriArea;
			InvMasses[Index2] += TriArea;
		}

		PhysMesh.NumFixedVerts = 0;
		float MassSum = 0.0f;
		for(int32 CurrVertIndex = 0; CurrVertIndex < NumVerts; ++CurrVertIndex)
		{
			float& InvMass = InvMasses[CurrVertIndex];
			const float& MaxDistance = PhysMesh.MaxDistances[CurrVertIndex];

			if(MaxDistance < SMALL_NUMBER)
			{
				InvMass = 0.0f;
				++PhysMesh.NumFixedVerts;
			}
			else
			{
				MassSum += InvMass;
			}
		}

		const float MassScale = (float)(NumVerts - PhysMesh.NumFixedVerts) / MassSum;

		for(float& InvMass : InvMasses)
		{
			InvMass *= MassScale;
			InvMass = 1.0f / InvMass;
		}
	}
}

void UClothingAsset::BuildLodTransitionData()
{
	// Temporary until mesh to mesh is refactored to allow not having all inputs available
	TArray<FVector> DummyTangentArray;

	const int32 NumLods = LodData.Num();
	for(int32 LodIndex = 0; LodIndex < NumLods; ++LodIndex)
	{
		const bool bHasPrevLod = LodIndex > 0;
		const bool bHasNextLod = LodIndex < NumLods - 1;

		FClothLODData& CurrentLod = LodData[LodIndex];
		FClothPhysicalMeshData& CurrentPhysMesh = CurrentLod.PhysicalMeshData;

		FClothLODData* PrevLod = bHasPrevLod ? &LodData[LodIndex - 1] : nullptr;
		FClothLODData* NextLod = bHasNextLod ? &LodData[LodIndex + 1] : nullptr;

		const int32 CurrentLodNumVerts = CurrentPhysMesh.Vertices.Num();

		DummyTangentArray.Empty(CurrentLodNumVerts);
		DummyTangentArray.AddDefaulted(CurrentLodNumVerts);

		if(PrevLod)
		{
			FClothPhysicalMeshData& PrevPhysMesh = PrevLod->PhysicalMeshData;

			CurrentLod.TransitionUpSkinData.Empty(CurrentLodNumVerts);

			ClothingAssetUtils::GenerateMeshToMeshSkinningData(CurrentLod.TransitionUpSkinData, CurrentPhysMesh.Vertices, CurrentPhysMesh.Normals, DummyTangentArray, PrevPhysMesh.Vertices, PrevPhysMesh.Normals, PrevPhysMesh.Indices);
		}

		if(NextLod)
		{
			FClothPhysicalMeshData& NextPhysMesh = NextLod->PhysicalMeshData;

			CurrentLod.TransitionDownSkinData.Empty(CurrentLodNumVerts);

			ClothingAssetUtils::GenerateMeshToMeshSkinningData(CurrentLod.TransitionDownSkinData, CurrentPhysMesh.Vertices, CurrentPhysMesh.Normals, DummyTangentArray, NextPhysMesh.Vertices, NextPhysMesh.Normals, NextPhysMesh.Indices);
		}
	}
}

bool UClothingAsset::IsValidLod(int32 InLodIndex)
{
	return LodData.IsValidIndex(InLodIndex);
}

int32 UClothingAsset::GetNumLods()
{
	return LodData.Num();
}

#endif

void ClothingAssetUtils::GetMeshClothingAssetBindings(USkeletalMesh* InSkelMesh, TArray<FClothingAssetMeshBinding>& OutBindings)
{
	OutBindings.Empty();

	if(!InSkelMesh)
	{
		return;
	}

	if(FSkeletalMeshResource* Resource = InSkelMesh->GetImportedResource())
	{
		const int32 NumLods = Resource->LODModels.Num();

		for(int32 LodIndex = 0; LodIndex < NumLods; ++LodIndex)
		{
			TArray<FClothingAssetMeshBinding> LodBindings;
			GetMeshClothingAssetBindings(InSkelMesh, LodBindings, LodIndex);

			OutBindings.Append(LodBindings);
		}
	}
}

void ClothingAssetUtils::GetMeshClothingAssetBindings(USkeletalMesh* InSkelMesh, TArray<FClothingAssetMeshBinding>& OutBindings, int32 InLodIndex)
{
	OutBindings.Empty();

	if(!InSkelMesh)
	{
		return;
	}

	if(FSkeletalMeshResource* Resource = InSkelMesh->GetImportedResource())
	{
		if(Resource->LODModels.IsValidIndex(InLodIndex))
		{
			FStaticLODModel& LodModel = Resource->LODModels[InLodIndex];

			const int32 NumSections = LodModel.Sections.Num();

			for(int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
			{
				FSkelMeshSection& Section = LodModel.Sections[SectionIndex];

				if(Section.bDisabled && Section.CorrespondClothSectionIndex != INDEX_NONE)
				{
					UClothingAsset* SectionAsset = Cast<UClothingAsset>(InSkelMesh->GetSectionClothingAsset(InLodIndex, SectionIndex));

					if(SectionAsset)
					{
						// This is the original section of a clothing section pair
						OutBindings.AddDefaulted();
						FClothingAssetMeshBinding& Binding = OutBindings.Last();

						Binding.Asset = SectionAsset;
						Binding.LODIndex = InLodIndex;
						Binding.SectionIndex = SectionIndex;
						Binding.AssetInternalLodIndex = Section.ClothingData.AssetLodIndex;
					}
				}
			}
		}
	}
}

void ClothingAssetUtils::GenerateMeshToMeshSkinningData(TArray<FMeshToMeshVertData>& OutSkinningData, const TArray<FVector>& Mesh0Verts, const TArray<FVector>& Mesh0Normals, const TArray<FVector>& Mesh0Tangents, const TArray<FVector>& Mesh1Verts, const TArray<FVector>& Mesh1Normals, const TArray<uint32>& Mesh1Indices)
{
	const int32 NumMesh0Verts = Mesh0Verts.Num();
	const int32 NumMesh0Normals = Mesh0Normals.Num();
	const int32 NumMesh0Tangents = Mesh0Tangents.Num();

	const int32 NumMesh1Verts = Mesh1Verts.Num();
	const int32 NumMesh1Normals = Mesh1Normals.Num();
	const int32 NumMesh1Indices = Mesh1Indices.Num();

	// Check we have properly formed triangles
	check(NumMesh1Indices % 3 == 0);

	const int32 NumMesh1Triangles = NumMesh1Indices / 3;

	// Check mesh data to make sure we have the same number of each element
	if((NumMesh0Verts + NumMesh0Normals) / NumMesh0Tangents != 2)
	{
		UE_LOG(LogClothingAsset, Warning, TEXT("Can't generate mesh to mesh skinning data, Mesh0 data is missing verts."));
		return;
	}

	if(NumMesh1Verts != NumMesh1Normals)
	{
		UE_LOG(LogClothingAsset, Warning, TEXT("Can't generate mesh to mesh skinning data, Mesh1 data is missing verts."));
		return;
	}

	OutSkinningData.Reserve(NumMesh0Verts);

	// For all mesh0 verts
	for(int32 VertIdx0 = 0; VertIdx0 < NumMesh0Verts; ++VertIdx0)
	{
		OutSkinningData.AddZeroed();
		FMeshToMeshVertData& SkinningData = OutSkinningData.Last();

		const FVector& VertPosition = Mesh0Verts[VertIdx0];
		const FVector& VertNormal = Mesh0Normals[VertIdx0];
		const FVector& VertTangent = Mesh0Tangents[VertIdx0];

		float MinimumDistanceSq = MAX_flt;
		int32 ClosestTriangleBaseIdx = INDEX_NONE;
		// For all mesh1 triangles
		for(int32 Mesh1TriangleIdx = 0; Mesh1TriangleIdx < NumMesh1Triangles; ++Mesh1TriangleIdx)
		{
			int32 TriangleBaseIdx = Mesh1TriangleIdx * 3;
			const FVector& A = Mesh1Verts[Mesh1Indices[TriangleBaseIdx]];
			const FVector& B = Mesh1Verts[Mesh1Indices[TriangleBaseIdx + 1]];
			const FVector& C = Mesh1Verts[Mesh1Indices[TriangleBaseIdx + 2]];

			FVector PointOnTri = FMath::ClosestPointOnTriangleToPoint(VertPosition, A, B, C);
			float DistSq = (PointOnTri - VertPosition).SizeSquared();

			if(DistSq < MinimumDistanceSq)
			{
				MinimumDistanceSq = DistSq;
				ClosestTriangleBaseIdx = TriangleBaseIdx;
			}
		}

		// Should have found at least one triangle
		check(ClosestTriangleBaseIdx != INDEX_NONE);

		const FVector& A = Mesh1Verts[Mesh1Indices[ClosestTriangleBaseIdx]];
		const FVector& B = Mesh1Verts[Mesh1Indices[ClosestTriangleBaseIdx + 1]];
		const FVector& C = Mesh1Verts[Mesh1Indices[ClosestTriangleBaseIdx + 2]];

		const FVector& NA = Mesh1Normals[Mesh1Indices[ClosestTriangleBaseIdx]];
		const FVector& NB = Mesh1Normals[Mesh1Indices[ClosestTriangleBaseIdx + 1]];
		const FVector& NC = Mesh1Normals[Mesh1Indices[ClosestTriangleBaseIdx + 2]];

		// Before generating the skinning data we need to check for a degenerate triangle.
		// If we find _any_ degenerate triangles we will notify and fail to generate the skinning data
		const FVector TriNormal = FVector::CrossProduct(B - A, C - A);
		if(TriNormal.SizeSquared() < SMALL_NUMBER)
		{
			// Failed, we have 2 identical vertices
			OutSkinningData.Reset();

			// Log and toast
			FText Error = FText::Format(LOCTEXT("DegenerateTriangleError", "Failed to generate skinning data, found conincident vertices in triangle A={0} B={1} C={2}"), FText::FromString(A.ToString()), FText::FromString(B.ToString()), FText::FromString(C.ToString()));

			UE_LOG(LogClothingAsset, Warning, TEXT("%s"), *Error.ToString());

#if WITH_EDITOR
			FNotificationInfo Info(Error);
			Info.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
#endif
			return;
		}

		SkinningData.PositionBaryCoordsAndDist = GetPointBaryAndDist(A, B, C, NA, NB, NC, VertPosition);
		SkinningData.NormalBaryCoordsAndDist = GetPointBaryAndDist(A, B, C, NA, NB, NC, VertPosition + VertNormal);
		SkinningData.TangentBaryCoordsAndDist = GetPointBaryAndDist(A, B, C, NA, NB, NC, VertPosition + VertTangent);
		SkinningData.SourceMeshVertIndices[0] = Mesh1Indices[ClosestTriangleBaseIdx];
		SkinningData.SourceMeshVertIndices[1] = Mesh1Indices[ClosestTriangleBaseIdx + 1];
		SkinningData.SourceMeshVertIndices[2] = Mesh1Indices[ClosestTriangleBaseIdx + 2];
		SkinningData.SourceMeshVertIndices[3] = 0;
	}
}

FVector4 ClothingAssetUtils::GetPointBaryAndDist(const FVector& A, const FVector& B, const FVector& C, const FVector& NA, const FVector& NB, const FVector& NC, const FVector& Point)
{
	FPlane TrianglePlane(A, B, C);
	const FVector PointOnTriPlane = FVector::PointPlaneProject(Point, TrianglePlane);
	const FVector BaryCoords = FMath::ComputeBaryCentric2D(PointOnTriPlane, A, B, C);
	const FVector NormalAtPoint = TrianglePlane;
	FVector TriPointToVert = Point - PointOnTriPlane;
	TriPointToVert = TriPointToVert.ProjectOnTo(NormalAtPoint);
	float Dist = TriPointToVert.Size();

	float Sign = TrianglePlane.PlaneDot(Point) < 0.0f ? -1.0f : 1.0f;

	return FVector4(BaryCoords, TrianglePlane.PlaneDot(Point));
}

bool FClothConfig::HasSelfCollision() const
{
	return SelfCollisionRadius > 0.0f && SelfCollisionStiffness > 0.0f;
}

void FClothCollisionData::Reset()
{
	Spheres.Reset();
	SphereConnections.Reset();
	Convexes.Reset();
}

void FClothCollisionData::Append(const FClothCollisionData& InOther)
{
	const int32 NumSpheresBefore = Spheres.Num();
	const int32 NumSphereConnectionsBefore = SphereConnections.Num();

	Spheres.Append(InOther.Spheres);
	SphereConnections.Append(InOther.SphereConnections);

	const int32 NumSphereConnectionsAfter = SphereConnections.Num();

	if(NumSpheresBefore > 0)
	{
		// Each connection that was added needs to have its sphere indices increased to match the new spheres that were added
		for(int32 NewConnectionIndex = NumSphereConnectionsBefore; NewConnectionIndex < NumSphereConnectionsAfter; ++NewConnectionIndex)
		{
			FClothCollisionPrim_SphereConnection& Connection = SphereConnections[NewConnectionIndex];
			Connection.SphereIndices[0] += NumSpheresBefore;
			Connection.SphereIndices[1] += NumSpheresBefore;
		}
	}

	Convexes.Append(InOther.Convexes);
}

void FClothPhysicalMeshData::Reset(const int32 InNumVerts)
{
	Vertices.Reset();
	Normals.Reset();
	MaxDistances.Reset();
	BackstopDistances.Reset();
	BackstopRadiuses.Reset();
	InverseMasses.Reset();
	BoneData.Reset();

	Vertices.AddDefaulted(InNumVerts);
	Normals.AddDefaulted(InNumVerts);
	MaxDistances.AddDefaulted(InNumVerts);
	BackstopDistances.AddDefaulted(InNumVerts);
	BackstopRadiuses.AddDefaulted(InNumVerts);
	InverseMasses.AddDefaulted(InNumVerts);
	BoneData.AddDefaulted(InNumVerts);

	MaxBoneWeights = 0;
	NumFixedVerts = 0;
}

void FClothParameterMask_PhysMesh::Initialize(const FClothPhysicalMeshData& InMeshData)
{
	const int32 NumVerts = InMeshData.Vertices.Num();

	// Set up value array
	Values.Reset(NumVerts);
	Values.AddZeroed(NumVerts);
}

void FClothParameterMask_PhysMesh::SetValue(int32 InVertexIndex, float InValue)
{
	if(InVertexIndex < Values.Num())
	{
		Values[InVertexIndex] = InValue;

		float TempMax = 0.0f;
		for(const float& Value : Values)
		{
			TempMax = FMath::Max(TempMax, Value);
		}

		MaxValue = TempMax;
	}
}

float FClothParameterMask_PhysMesh::GetValue(int32 InVertexIndex) const
{
	return InVertexIndex < Values.Num() ? Values[InVertexIndex] : 0.0f;
}

#if WITH_EDITOR

void FClothParameterMask_PhysMesh::CalcRanges()
{
	MinValue = MAX_flt;
	MaxValue = -MinValue;

	for(const float& Value : Values)
	{
		MaxValue = FMath::Max(Value, MaxValue);

		if(Value > 0.0f)
		{
			MinValue = FMath::Min(Value, MinValue);
		}
	}
}

FColor FClothParameterMask_PhysMesh::GetValueAsColor(int32 InVertexIndex) const
{
	if(InVertexIndex < Values.Num())
	{
		const float& Value = Values[InVertexIndex];

		if(Value == 0.0f)
		{
			return FColor::Magenta;
		}

		uint8 ScaledValue = uint8(((Value - MinValue) / (MaxValue - MinValue)) * 255.0f);
		return FColor(ScaledValue, ScaledValue, ScaledValue);
	}

	return FColor::Red;
}
#endif

void FClothParameterMask_PhysMesh::Apply(FClothPhysicalMeshData& InTargetMesh)
{
	if(CurrentTarget == MaskTarget_PhysMesh::None)
	{
		// Nothing to do here, just return
		return;
	}

	const int32 NumValues = Values.Num();
	const int32 NumTargetMeshVerts = InTargetMesh.Vertices.Num();

	if(NumTargetMeshVerts == NumValues)
	{
		TArray<float>* TargetArray = nullptr;
		switch(CurrentTarget)
		{
			case MaskTarget_PhysMesh::MaxDistance:
				TargetArray = &InTargetMesh.MaxDistances;
				break;
			case MaskTarget_PhysMesh::BackstopDistance:
				TargetArray = &InTargetMesh.BackstopDistances;
				break;
			case MaskTarget_PhysMesh::BackstopRadius:
				TargetArray = &InTargetMesh.BackstopRadiuses;
				break;
			default:
				break;
		}

		if(!TargetArray)
		{
			return;
		}

		check((*TargetArray).Num() == NumValues);

		for(int32 Index = 0; Index < NumTargetMeshVerts; ++Index)
		{
			(*TargetArray)[Index] = Values[Index];
		}
	}
	else
	{
		UE_LOG(LogClothingAsset, Warning, TEXT("Aborted applying mask to physical mesh at %p, value mismatch (NumValues: %d, NumVerts: %d)."), &InTargetMesh, NumValues, NumTargetMeshVerts);
	}
}

#undef LOCTEXT_NAMESPACE
