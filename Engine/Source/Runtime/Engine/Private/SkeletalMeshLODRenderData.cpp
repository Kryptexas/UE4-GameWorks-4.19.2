// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SkeletalMeshLODRenderData.h"
#include "SkeletalMeshRenderData.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/MorphTarget.h"
#include "Misc/ConfigCacheIni.h"
#include "EngineLogs.h"
#include "EngineUtils.h"
#include "Interfaces/ITargetPlatform.h"

#if WITH_EDITOR
#include "SkeletalMeshModel.h"
#include "MeshUtilities.h"
#endif // WITH_EDITOR


// Serialization.
FArchive& operator<<(FArchive& Ar, FSkelMeshRenderSection& S)
{
	// When data is cooked for server platform some of the
	// variables are not serialized so that they're always
	// set to their initial values (for safety)
	FStripDataFlags StripFlags(Ar);

	Ar << S.MaterialIndex;
	Ar << S.BaseIndex;
	Ar << S.NumTriangles;
	Ar << S.bRecomputeTangent;
	Ar << S.bCastShadow;
	Ar << S.bDisabled;
	Ar << S.BaseVertexIndex;
	Ar << S.ClothMappingData;
	Ar << S.BoneMap;
	Ar << S.NumVertices;
	Ar << S.MaxBoneInfluences;
	Ar << S.CorrespondClothAssetIndex;
	Ar << S.ClothingData;

	return Ar;
}


void FSkeletalMeshLODRenderData::InitResources(bool bNeedsVertexColors, int32 LODIndex, TArray<UMorphTarget*>& InMorphTargets)
{
	INC_DWORD_STAT_BY(STAT_SkeletalMeshIndexMemory, MultiSizeIndexContainer.IsIndexBufferValid() ? (MultiSizeIndexContainer.GetIndexBuffer()->Num() * MultiSizeIndexContainer.GetDataTypeSize()) : 0);

	MultiSizeIndexContainer.InitResources();

	INC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, StaticVertexBuffers.PositionVertexBuffer.GetStride() * StaticVertexBuffers.PositionVertexBuffer.GetNumVertices());
	INC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, StaticVertexBuffers.StaticMeshVertexBuffer.GetResourceSize());
	BeginInitResource(&StaticVertexBuffers.PositionVertexBuffer);
	BeginInitResource(&StaticVertexBuffers.StaticMeshVertexBuffer);

	INC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, SkinWeightVertexBuffer.GetVertexDataSize());
	BeginInitResource(&SkinWeightVertexBuffer);

	if (bNeedsVertexColors)
	{
		// Only init the color buffer if the mesh has vertex colors
		INC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, StaticVertexBuffers.ColorVertexBuffer.GetAllocatedSize());
		BeginInitResource(&StaticVertexBuffers.ColorVertexBuffer);
	}

	if (ClothVertexBuffer.GetNumVertices() > 0)
	{
		// Only init the clothing buffer if the mesh has clothing data
		INC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, ClothVertexBuffer.GetVertexDataSize());
		BeginInitResource(&ClothVertexBuffer);
	}

	if (RHISupportsTessellation(GMaxRHIShaderPlatform))
	{
		AdjacencyMultiSizeIndexContainer.InitResources();
		INC_DWORD_STAT_BY(STAT_SkeletalMeshIndexMemory, AdjacencyMultiSizeIndexContainer.IsIndexBufferValid() ? (AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->Num() * AdjacencyMultiSizeIndexContainer.GetDataTypeSize()) : 0);
	}

	if (RHISupportsComputeShaders(GMaxRHIShaderPlatform) && InMorphTargets.Num() > 0)
	{
		MorphTargetVertexInfoBuffers.VertexIndices.Empty();
		MorphTargetVertexInfoBuffers.MorphDeltas.Empty();
		MorphTargetVertexInfoBuffers.WorkItemsPerMorph.Empty();
		MorphTargetVertexInfoBuffers.StartOffsetPerMorph.Empty();
		MorphTargetVertexInfoBuffers.MaximumValuePerMorph.Empty();
		MorphTargetVertexInfoBuffers.MinimumValuePerMorph.Empty();
		MorphTargetVertexInfoBuffers.NumTotalWorkItems = 0;

		// Populate the arrays to be filled in later in the render thread
		for (int32 AnimIdx = 0; AnimIdx < InMorphTargets.Num(); ++AnimIdx)
		{
			uint32 StartOffset = MorphTargetVertexInfoBuffers.NumTotalWorkItems;
			MorphTargetVertexInfoBuffers.StartOffsetPerMorph.Add(StartOffset);

			float MaximumValues[4] = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
			float MinimumValues[4] = { +FLT_MAX, +FLT_MAX, +FLT_MAX, +FLT_MAX };
			UMorphTarget* MorphTarget = InMorphTargets[AnimIdx];
			int32 NumSrcDeltas = 0;
			FMorphTargetDelta* MorphDeltas = MorphTarget->GetMorphTargetDelta(LODIndex, NumSrcDeltas);
			for (int32 DeltaIndex = 0; DeltaIndex < NumSrcDeltas; DeltaIndex++)
			{
				const auto& MorphDelta = MorphDeltas[DeltaIndex];
				// when import, we do check threshold, and also when adding weight, we do have threshold for how smaller weight can fit in
				// so no reason to check here another threshold
				MaximumValues[0] = FMath::Max(MaximumValues[0], MorphDelta.PositionDelta.X);
				MaximumValues[1] = FMath::Max(MaximumValues[1], MorphDelta.PositionDelta.Y);
				MaximumValues[2] = FMath::Max(MaximumValues[2], MorphDelta.PositionDelta.Z);
				MaximumValues[3] = FMath::Max(MaximumValues[3], FMath::Max(MorphDelta.TangentZDelta.X, FMath::Max(MorphDelta.TangentZDelta.Y, MorphDelta.TangentZDelta.Z)));

				MinimumValues[0] = FMath::Min(MinimumValues[0], MorphDelta.PositionDelta.X);
				MinimumValues[1] = FMath::Min(MinimumValues[1], MorphDelta.PositionDelta.Y);
				MinimumValues[2] = FMath::Min(MinimumValues[2], MorphDelta.PositionDelta.Z);
				MinimumValues[3] = FMath::Min(MinimumValues[3], FMath::Min(MorphDelta.TangentZDelta.X, FMath::Min(MorphDelta.TangentZDelta.Y, MorphDelta.TangentZDelta.Z)));

				MorphTargetVertexInfoBuffers.VertexIndices.Add(MorphDelta.SourceIdx);
				MorphTargetVertexInfoBuffers.MorphDeltas.Emplace(MorphDelta.PositionDelta, MorphDelta.TangentZDelta);
				MorphTargetVertexInfoBuffers.NumTotalWorkItems++;
			}

			uint32 MorphTargetSize = MorphTargetVertexInfoBuffers.NumTotalWorkItems - StartOffset;
			if (MorphTargetSize > 0)
			{
				ensureMsgf(MaximumValues[0] < +32752.0f && MaximumValues[1] < +32752.0f && MaximumValues[2] < +32752.0f && MaximumValues[3] < +32752.0f, TEXT("Huge MorphTarget Delta found in %s at index %i, might break down because we use half float storage"), *MorphTarget->GetName(), AnimIdx);
				ensureMsgf(MinimumValues[0] > -32752.0f && MinimumValues[1] > -32752.0f && MinimumValues[2] > -32752.0f && MaximumValues[3] > -32752.0f, TEXT("Huge MorphTarget Delta found in %s at index %i, might break down because we use half float storage"), *MorphTarget->GetName(), AnimIdx);
			}

			MorphTargetVertexInfoBuffers.WorkItemsPerMorph.Add(MorphTargetSize);
			MorphTargetVertexInfoBuffers.MaximumValuePerMorph.Add(FVector4(MaximumValues[0], MaximumValues[1], MaximumValues[2], MaximumValues[3]));
			MorphTargetVertexInfoBuffers.MinimumValuePerMorph.Add(FVector4(MinimumValues[0], MinimumValues[1], MinimumValues[2], MinimumValues[3]));
		}

		check(MorphTargetVertexInfoBuffers.WorkItemsPerMorph.Num() == MorphTargetVertexInfoBuffers.StartOffsetPerMorph.Num());
		check(MorphTargetVertexInfoBuffers.WorkItemsPerMorph.Num() == MorphTargetVertexInfoBuffers.MaximumValuePerMorph.Num());
		check(MorphTargetVertexInfoBuffers.WorkItemsPerMorph.Num() == MorphTargetVertexInfoBuffers.MinimumValuePerMorph.Num());
		if (MorphTargetVertexInfoBuffers.NumTotalWorkItems > 0)
		{
			BeginInitResource(&MorphTargetVertexInfoBuffers);
		}
	}
}


void FSkeletalMeshLODRenderData::ReleaseResources()
{
	DEC_DWORD_STAT_BY(STAT_SkeletalMeshIndexMemory, MultiSizeIndexContainer.IsIndexBufferValid() ? (MultiSizeIndexContainer.GetIndexBuffer()->Num() * MultiSizeIndexContainer.GetDataTypeSize()) : 0);
	DEC_DWORD_STAT_BY(STAT_SkeletalMeshIndexMemory, AdjacencyMultiSizeIndexContainer.IsIndexBufferValid() ? (AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->Num() * AdjacencyMultiSizeIndexContainer.GetDataTypeSize()) : 0);

	DEC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, StaticVertexBuffers.PositionVertexBuffer.GetStride() * StaticVertexBuffers.PositionVertexBuffer.GetNumVertices());
	DEC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, StaticVertexBuffers.StaticMeshVertexBuffer.GetResourceSize());

	DEC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, SkinWeightVertexBuffer.GetVertexDataSize());
	DEC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, StaticVertexBuffers.ColorVertexBuffer.GetAllocatedSize());
	DEC_DWORD_STAT_BY(STAT_SkeletalMeshVertexMemory, ClothVertexBuffer.GetVertexDataSize());

	MultiSizeIndexContainer.ReleaseResources();
	AdjacencyMultiSizeIndexContainer.ReleaseResources();

	BeginReleaseResource(&StaticVertexBuffers.PositionVertexBuffer);
	BeginReleaseResource(&StaticVertexBuffers.StaticMeshVertexBuffer);
	BeginReleaseResource(&SkinWeightVertexBuffer);
	BeginReleaseResource(&StaticVertexBuffers.ColorVertexBuffer);
	BeginReleaseResource(&ClothVertexBuffer);
	BeginReleaseResource(&MorphTargetVertexInfoBuffers);
}

#if WITH_EDITOR

void FSkeletalMeshLODRenderData::BuildFromLODModel(const FSkeletalMeshLODModel* ImportedModel, uint32 BuildFlags)
{
	bool bUseFullPrecisionUVs = (BuildFlags & ESkeletalMeshVertexFlags::UseFullPrecisionUVs) != 0;
	bool bHasVertexColors = (BuildFlags & ESkeletalMeshVertexFlags::HasVertexColors) != 0;

	// Copy required info from source sections
	RenderSections.Empty();
	for (int32 SectionIndex = 0; SectionIndex < ImportedModel->Sections.Num(); SectionIndex++)
	{
		const FSkelMeshSection& ModelSection = ImportedModel->Sections[SectionIndex];

		FSkelMeshRenderSection NewRenderSection;
		NewRenderSection.MaterialIndex = ModelSection.MaterialIndex;
		NewRenderSection.BaseIndex = ModelSection.BaseIndex;
		NewRenderSection.NumTriangles = ModelSection.NumTriangles;
		NewRenderSection.bRecomputeTangent = ModelSection.bRecomputeTangent;
		NewRenderSection.bCastShadow = ModelSection.bCastShadow;
		NewRenderSection.bDisabled = ModelSection.bDisabled;
		NewRenderSection.BaseVertexIndex = ModelSection.BaseVertexIndex;
		NewRenderSection.ClothMappingData = ModelSection.ClothMappingData;
		NewRenderSection.BoneMap = ModelSection.BoneMap;
		NewRenderSection.NumVertices = ModelSection.NumVertices;
		NewRenderSection.MaxBoneInfluences = ModelSection.MaxBoneInfluences;
		NewRenderSection.CorrespondClothAssetIndex = ModelSection.CorrespondClothAssetIndex;
		NewRenderSection.ClothingData = ModelSection.ClothingData;

		RenderSections.Add(NewRenderSection);
	}

	TArray<FSoftSkinVertex> Vertices;
	ImportedModel->GetVertices(Vertices);

	// match UV precision for mesh vertex buffer to setting from parent mesh
	StaticVertexBuffers.StaticMeshVertexBuffer.SetUseFullPrecisionUVs(bUseFullPrecisionUVs);
	// init vertex buffer with the vertex array
	StaticVertexBuffers.PositionVertexBuffer.Init(Vertices.Num());
	StaticVertexBuffers.StaticMeshVertexBuffer.Init(Vertices.Num(), ImportedModel->NumTexCoords);

	for (int32 i = 0; i < Vertices.Num(); i++)
	{
		StaticVertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertices[i].Position;
		StaticVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertices[i].TangentX, Vertices[i].TangentY, Vertices[i].TangentZ);
		for (uint32 j = 0; j < ImportedModel->NumTexCoords; j++)
		{
			StaticVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, j, Vertices[i].UVs[j]);
		}
	}

	// Init skin weight buffer
	SkinWeightVertexBuffer.SetNeedsCPUAccess(true);
	SkinWeightVertexBuffer.SetHasExtraBoneInfluences(ImportedModel->DoSectionsNeedExtraBoneInfluences());
	SkinWeightVertexBuffer.Init(Vertices);

	// Init the color buffer if this mesh has vertex colors.
	if (bHasVertexColors && Vertices.Num() > 0 && StaticVertexBuffers.ColorVertexBuffer.GetAllocatedSize() == 0)
	{
		StaticVertexBuffers.ColorVertexBuffer.InitFromColorArray(&Vertices[0].Color, Vertices.Num(), sizeof(FSoftSkinVertex));
		StaticVertexBuffers.ColorVertexBuffer.InitFromColorArray(&Vertices[0].Color, Vertices.Num(), sizeof(FSoftSkinVertex));
	}

	if (ImportedModel->HasClothData())
	{
		TArray<FMeshToMeshVertData> MappingData;
		TArray<uint64> ClothIndexMapping;
		ImportedModel->GetClothMappingData(MappingData, ClothIndexMapping);
		ClothVertexBuffer.Init(MappingData, ClothIndexMapping);
	}

	const uint8 DataTypeSize = (ImportedModel->NumVertices < MAX_uint16) ? sizeof(uint16) : sizeof(uint32);

	MultiSizeIndexContainer.RebuildIndexBuffer(DataTypeSize, ImportedModel->IndexBuffer);
	
	TArray<uint32> BuiltAdjacencyIndices;
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	MeshUtilities.BuildSkeletalAdjacencyIndexBuffer(Vertices, ImportedModel->NumTexCoords, ImportedModel->IndexBuffer, BuiltAdjacencyIndices);
	AdjacencyMultiSizeIndexContainer.RebuildIndexBuffer(DataTypeSize, BuiltAdjacencyIndices);

	// MorphTargetVertexInfoBuffers are created in InitResources

	ActiveBoneIndices = ImportedModel->ActiveBoneIndices;
	RequiredBones = ImportedModel->RequiredBones;
}

#endif // WITH_EDITOR

void FSkeletalMeshLODRenderData::ReleaseCPUResources()
{
	if (!GIsEditor && !IsRunningCommandlet())
	{
		if (MultiSizeIndexContainer.IsIndexBufferValid())
		{
			MultiSizeIndexContainer.GetIndexBuffer()->Empty();
		}
		if (AdjacencyMultiSizeIndexContainer.IsIndexBufferValid())
		{
			AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->Empty();
		}
		if (SkinWeightVertexBuffer.IsWeightDataValid())
		{
			SkinWeightVertexBuffer.CleanUp();
		}

		StaticVertexBuffers.PositionVertexBuffer.CleanUp();
		StaticVertexBuffers.StaticMeshVertexBuffer.CleanUp();
	}
}


void FSkeletalMeshLODRenderData::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const
{
	if (MultiSizeIndexContainer.IsIndexBufferValid())
	{
		const FRawStaticIndexBuffer16or32Interface* IndexBuffer = MultiSizeIndexContainer.GetIndexBuffer();
		if (IndexBuffer)
		{
			CumulativeResourceSize.AddUnknownMemoryBytes(IndexBuffer->GetResourceDataSize());
		}
	}

	if (AdjacencyMultiSizeIndexContainer.IsIndexBufferValid())
	{
		const FRawStaticIndexBuffer16or32Interface* AdjacentIndexBuffer = AdjacencyMultiSizeIndexContainer.GetIndexBuffer();
		if (AdjacentIndexBuffer)
		{
			CumulativeResourceSize.AddUnknownMemoryBytes(AdjacentIndexBuffer->GetResourceDataSize());
		}
	}

	CumulativeResourceSize.AddUnknownMemoryBytes(StaticVertexBuffers.PositionVertexBuffer.GetNumVertices() * StaticVertexBuffers.PositionVertexBuffer.GetStride());
	CumulativeResourceSize.AddUnknownMemoryBytes(StaticVertexBuffers.StaticMeshVertexBuffer.GetResourceSize());
	CumulativeResourceSize.AddUnknownMemoryBytes(SkinWeightVertexBuffer.GetVertexDataSize());
	CumulativeResourceSize.AddUnknownMemoryBytes(StaticVertexBuffers.ColorVertexBuffer.GetAllocatedSize());
	CumulativeResourceSize.AddUnknownMemoryBytes(ClothVertexBuffer.GetVertexDataSize());
}

void FSkeletalMeshLODRenderData::Serialize(FArchive& Ar, UObject* Owner, int32 Idx)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FSkeletalMeshLODRenderData::Serialize"), STAT_SkeletalMeshLODRenderData_Serialize, STATGROUP_LoadTime);

	// Defined class flags for possible stripping
	const uint8 LodAdjacencyStripFlag = 1;

	// Actual flags used during serialization
	uint8 ClassDataStripFlags = 0;

	extern int32 GForceStripMeshAdjacencyDataDuringCooking;
	const bool bWantToStripTessellation = Ar.IsCooking() && ((GForceStripMeshAdjacencyDataDuringCooking != 0) || !Ar.CookingTarget()->SupportsFeature(ETargetPlatformFeatures::Tessellation));
	ClassDataStripFlags |= bWantToStripTessellation ? LodAdjacencyStripFlag : 0;

	FStripDataFlags StripFlags(Ar, ClassDataStripFlags);

	// Skeletal mesh buffers are kept in CPU memory after initialization to support merging of skeletal meshes.
	bool bKeepBuffersInCPUMemory = true;
#if !WITH_EDITOR
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FreeSkeletalMeshBuffers"));
	if (CVar)
	{
		bKeepBuffersInCPUMemory = !CVar->GetValueOnAnyThread();
	}
#endif

	if (StripFlags.IsDataStrippedForServer())
	{
		TArray<FSkelMeshRenderSection> DummySections;
		Ar << DummySections;

		FMultiSizeIndexContainer DummyIndexBuffer;
		DummyIndexBuffer.Serialize(Ar, bKeepBuffersInCPUMemory);

		TArray<FBoneIndexType> TempActiveBoneIndices;
		Ar << TempActiveBoneIndices;
	}
	else
	{
		Ar << RenderSections;

		MultiSizeIndexContainer.Serialize(Ar, bKeepBuffersInCPUMemory);

		Ar << ActiveBoneIndices;
	}

	Ar << RequiredBones;


	if (!StripFlags.IsDataStrippedForServer())
	{
		USkeletalMesh* SkelMeshOwner = CastChecked<USkeletalMesh>(Owner);

		// set cpu skinning flag on the vertex buffer so that the resource arrays know if they need to be CPU accessible
		bool bNeedsCPUAccess = bKeepBuffersInCPUMemory || SkelMeshOwner->GetResourceForRendering()->RequiresCPUSkinning(GMaxRHIFeatureLevel);

		if (Ar.IsLoading())
		{
			SkinWeightVertexBuffer.SetNeedsCPUAccess(bNeedsCPUAccess);
		}

		StaticVertexBuffers.PositionVertexBuffer.Serialize(Ar, bNeedsCPUAccess);
		StaticVertexBuffers.StaticMeshVertexBuffer.Serialize(Ar, bNeedsCPUAccess);
		Ar << SkinWeightVertexBuffer;

		if (SkelMeshOwner->bHasVertexColors)
		{
			StaticVertexBuffers.ColorVertexBuffer.Serialize(Ar, bKeepBuffersInCPUMemory);
		}

		if (!StripFlags.IsClassDataStripped(LodAdjacencyStripFlag))
		{
			AdjacencyMultiSizeIndexContainer.Serialize(Ar, bKeepBuffersInCPUMemory);
		}

		if (HasClothData())
		{
			Ar << ClothVertexBuffer;
		}
	}
}

int32 FSkeletalMeshLODRenderData::NumNonClothingSections() const
{
	int32 NumSections = RenderSections.Num();
	int32 Count = 0;

	for (int32 i = 0; i < NumSections; i++)
	{
		const FSkelMeshRenderSection& Section = RenderSections[i];

		// If we have found the start of the clothing section, return that index, since it is equal to the number of non-clothing entries.
		if (!Section.HasClothingData())
		{
			Count++;
		}
	}

	return Count;
}

uint32 FSkeletalMeshLODRenderData::FindSectionIndex(const FSkelMeshRenderSection& Section) const
{
	const FSkelMeshRenderSection* Start = RenderSections.GetData();

	if (Start == nullptr)
	{
		return -1;
	}

	uint32 Ret = &Section - Start;

	if (Ret >= (uint32)RenderSections.Num())
	{
		Ret = -1;
	}

	return Ret;
}

int32 FSkeletalMeshLODRenderData::GetTotalFaces() const
{
	int32 TotalFaces = 0;
	for (int32 i = 0; i < RenderSections.Num(); i++)
	{
		TotalFaces += RenderSections[i].NumTriangles;
	}

	return TotalFaces;
}

bool FSkeletalMeshLODRenderData::HasClothData() const
{
	for (int32 SectionIdx = 0; SectionIdx<RenderSections.Num(); SectionIdx++)
	{
		if (RenderSections[SectionIdx].HasClothingData())
		{
			return true;
		}
	}
	return false;
}

void FSkeletalMeshLODRenderData::GetSectionFromVertexIndex(int32 InVertIndex, int32& OutSectionIndex, int32& OutVertIndex) const
{
	OutSectionIndex = 0;
	OutVertIndex = 0;

	int32 VertCount = 0;

	// Iterate over each chunk
	for (int32 SectionCount = 0; SectionCount < RenderSections.Num(); SectionCount++)
	{
		const FSkelMeshRenderSection& Section = RenderSections[SectionCount];
		OutSectionIndex = SectionCount;

		// Is it in Soft vertex range?
		if (InVertIndex < VertCount + Section.GetNumVertices())
		{
			OutVertIndex = InVertIndex - VertCount;
			return;
		}
		VertCount += Section.NumVertices;
	}

	// InVertIndex should always be in some chunk!
	//check(false);
}