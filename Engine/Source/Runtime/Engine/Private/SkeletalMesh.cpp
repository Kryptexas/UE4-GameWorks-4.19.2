// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SkeletalMesh.cpp: Unreal skeletal mesh and animation implementation.
=============================================================================*/

#include "Engine/SkeletalMesh.h"
#include "Serialization/CustomVersion.h"
#include "UObject/FrameworkObjectVersion.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"
#include "EngineStats.h"
#include "EngineGlobals.h"
#include "RawIndexBuffer.h"
#include "Engine/TextureStreamingTypes.h"
#include "Engine/Brush.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Components/SkinnedMeshComponent.h"
#include "Animation/SmartName.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "ComponentReregisterContext.h"
#include "UObject/EditorObjectVersion.h"
#include "UObject/RenderingObjectVersion.h"
#include "EngineUtils.h"
#include "EditorSupportDelegates.h"
#include "GPUSkinVertexFactory.h"
#include "TessellationRendering.h"
#include "SkeletalRenderPublic.h"
#include "Logging/TokenizedMessage.h"
#include "Logging/MessageLog.h"
#include "SceneManagement.h"
#include "PhysicsPublic.h"
#include "Animation/MorphTarget.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Engine/AssetUserData.h"
#include "Engine/Engine.h"
#include "Animation/NodeMappingContainer.h"
#include "GPUSkinCache.h"
#include "Misc/ConfigCacheIni.h"
#include "SkeletalMeshTypes.h"
#include "Rendering/SkeletalMeshVertexBuffer.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "UObject/PropertyPortFlags.h"
#include "Templates/UniquePtr.h"
#include "Misc/ScopedSlowTask.h" //#nv #Blast Ability to hide bones using a dynamic index buffer

#if WITH_EDITOR
#include "Rendering/SkeletalMeshModel.h"
#include "MeshUtilities.h"

#if WITH_APEX_CLOTHING
#include "ApexClothingUtils.h"
#endif

#endif // #if WITH_EDITOR

#include "Interfaces/ITargetPlatform.h"

#if WITH_APEX
#include "PhysXIncludes.h"
#endif// #if WITH_APEX

#include "EditorFramework/AssetImportData.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/BrushComponent.h"
#include "Streaming/UVChannelDensity.h"
#include "Paths.h"

#include "ClothingAssetInterface.h"

#if WITH_EDITOR
#include "ClothingAssetFactoryInterface.h"
#include "ClothingSystemEditorInterfaceModule.h"
#endif
#include "SkeletalDebugRendering.h"
#include "Misc/RuntimeErrors.h"

#define LOCTEXT_NAMESPACE "SkeltalMesh"

DEFINE_LOG_CATEGORY(LogSkeletalMesh);
DECLARE_CYCLE_STAT(TEXT("GetShadowShapes"), STAT_GetShadowShapes, STATGROUP_Anim);

TAutoConsoleVariable<int32> CVarDebugDrawSimpleBones(TEXT("a.DebugDrawSimpleBones"), 0, TEXT("When drawing bones (using Show Bones), draw bones as simple lines."));
TAutoConsoleVariable<int32> CVarDebugDrawBoneAxes(TEXT("a.DebugDrawBoneAxes"), 0, TEXT("When drawing bones (using Show Bones), draw bone axes."));

const FGuid FSkeletalMeshCustomVersion::GUID(0xD78A4A00, 0xE8584697, 0xBAA819B5, 0x487D46B4);
FCustomVersionRegistration GRegisterSkeletalMeshCustomVersion(FSkeletalMeshCustomVersion::GUID, FSkeletalMeshCustomVersion::LatestVersion, TEXT("SkeletalMeshVer"));

#if WITH_APEX_CLOTHING
/*-----------------------------------------------------------------------------
	utility functions for apex clothing 
-----------------------------------------------------------------------------*/

static apex::ClothingAsset* LoadApexClothingAssetFromBlob(const TArray<uint8>& Buffer)
{
	// Wrap this blob with the APEX read stream class
	physx::PxFileBuf* Stream = GApexSDK->createMemoryReadStream( Buffer.GetData(), Buffer.Num() );
	// Create an NvParameterized serializer
	NvParameterized::Serializer* Serializer = GApexSDK->createSerializer(NvParameterized::Serializer::NST_BINARY);
	// Deserialize into a DeserializedData buffer
	NvParameterized::Serializer::DeserializedData DeserializedData;
	Serializer->deserialize( *Stream, DeserializedData );
	apex::Asset* ApexAsset = NULL;
	if( DeserializedData.size() > 0 )
	{
		// The DeserializedData has something in it, so create an APEX asset from it
		ApexAsset = GApexSDK->createAsset( DeserializedData[0], NULL);
		// Make sure it's a Clothing asset
		if (ApexAsset 
			&& ApexAsset->getObjTypeID() != GApexModuleClothing->getModuleID()
			)
		{
			GPhysCommandHandler->DeferredRelease(ApexAsset);
			ApexAsset = NULL;
		}
	}

	apex::ClothingAsset* ApexClothingAsset = static_cast<apex::ClothingAsset*>(ApexAsset);
	// Release our temporary objects
	Serializer->release();
	GApexSDK->releaseMemoryReadStream( *Stream );

	return ApexClothingAsset;
}

static bool SaveApexClothingAssetToBlob(const apex::ClothingAsset *InAsset, TArray<uint8>& OutBuffer)
{
	bool bResult = false;
	uint32 Size = 0;
	// Get the NvParameterized data for our Clothing asset
	if( InAsset != NULL )
	{
		// Create an APEX write stream
		physx::PxFileBuf* Stream = GApexSDK->createMemoryWriteStream();
		// Create an NvParameterized serializer
		NvParameterized::Serializer* Serializer = GApexSDK->createSerializer(NvParameterized::Serializer::NST_BINARY);

		const NvParameterized::Interface* AssetParameterized = InAsset->getAssetNvParameterized();
		if( AssetParameterized != NULL )
		{
			// Serialize the data into the stream
			Serializer->serialize( *Stream, &AssetParameterized, 1 );
			// Read the stream data into our buffer for UE serialzation
			Size = Stream->getFileLength();
			OutBuffer.AddUninitialized( Size );
			Stream->read( OutBuffer.GetData(), Size );
			bResult = true;
		}

		// Release our temporary objects
		Serializer->release();
		Stream->release();
	}

	return bResult;
}

#endif//#if WITH_APEX_CLOTHING


/*-----------------------------------------------------------------------------
FGPUSkinVertexBase
-----------------------------------------------------------------------------*/

/**
* Serializer
*
* @param Ar - archive to serialize with
*/
void TGPUSkinVertexBase::Serialize(FArchive& Ar)
{
	Ar << TangentX;
	Ar << TangentZ;
}






const FGuid FRecomputeTangentCustomVersion::GUID(0x5579F886, 0x933A4C1F, 0x83BA087B, 0x6361B92F);
// Register the custom version with core
FCustomVersionRegistration GRegisterRecomputeTangentCustomVersion(FRecomputeTangentCustomVersion::GUID, FRecomputeTangentCustomVersion::LatestVersion, TEXT("RecomputeTangentCustomVer"));

const FGuid FOverlappingVerticesCustomVersion::GUID(0x612FBE52, 0xDA53400B, 0x910D4F91, 0x9FB1857C);
// Register the custom version with core
FCustomVersionRegistration GRegisterOverlappingVerticesCustomVersion(FOverlappingVerticesCustomVersion::GUID, FOverlappingVerticesCustomVersion::LatestVersion, TEXT("OverlappingVerticeDetectionVer"));

/*-----------------------------------------------------------------------------
FreeSkeletalMeshBuffersSinkCallback
-----------------------------------------------------------------------------*/

void FreeSkeletalMeshBuffersSinkCallback()
{
	// If r.FreeSkeletalMeshBuffers==1 then CPU buffer copies are to be released.
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FreeSkeletalMeshBuffers"));
	bool bFreeSkeletalMeshBuffers = CVar->GetValueOnGameThread() == 1;
	if(bFreeSkeletalMeshBuffers)
	{
		FlushRenderingCommands();
		for (TObjectIterator<USkeletalMesh> It;It;++It)
		{
			if (!It->GetResourceForRendering()->RequiresCPUSkinning(GMaxRHIFeatureLevel))
			{
				It->ReleaseCPUResources();
			}
		}
	}
}


//#nv begin #Blast Ability to hide bones using a dynamic index buffer
/*-----------------------------------------------------------------------------
FDynamicLODModelOverride
-----------------------------------------------------------------------------*/

void FDynamicLODModelOverride::InitResources(const FSkeletalMeshLODRenderData& InitialData)
{
	Sections.SetNum(InitialData.RenderSections.Num());
	for (int32 S = 0; S < Sections.Num(); S++)
	{
		Sections[S].BaseIndex = InitialData.RenderSections[S].BaseIndex;
		Sections[S].NumTriangles = InitialData.RenderSections[S].NumTriangles;
	}

	FMultiSizeIndexContainerData TempData;
	TempData.DataTypeSize = InitialData.MultiSizeIndexContainer.GetDataTypeSize();
	InitialData.MultiSizeIndexContainer.GetIndexBuffer(TempData.Indices);
	MultiSizeIndexContainer.RebuildIndexBuffer(TempData.DataTypeSize, TempData.Indices);

	INC_DWORD_STAT_BY(STAT_SkeletalMeshIndexMemory, MultiSizeIndexContainer.IsIndexBufferValid() ? (MultiSizeIndexContainer.GetIndexBuffer()->Num() * MultiSizeIndexContainer.GetDataTypeSize()) : 0);

	MultiSizeIndexContainer.InitResources();

	//Need to check if the data was stripped in cooking or not
	if (RHISupportsTessellation(GMaxRHIShaderPlatform) && InitialData.AdjacencyMultiSizeIndexContainer.IsIndexBufferValid())
	{
		TempData.DataTypeSize = InitialData.AdjacencyMultiSizeIndexContainer.GetDataTypeSize();
		InitialData.AdjacencyMultiSizeIndexContainer.GetIndexBuffer(TempData.Indices);
		AdjacencyMultiSizeIndexContainer.RebuildIndexBuffer(TempData.DataTypeSize, TempData.Indices);

		AdjacencyMultiSizeIndexContainer.InitResources();
		INC_DWORD_STAT_BY(STAT_SkeletalMeshIndexMemory, AdjacencyMultiSizeIndexContainer.IsIndexBufferValid() ? (AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->Num() * AdjacencyMultiSizeIndexContainer.GetDataTypeSize()) : 0);
	}
}

void FDynamicLODModelOverride::ReleaseResources()
{
	DEC_DWORD_STAT_BY(STAT_SkeletalMeshIndexMemory, MultiSizeIndexContainer.IsIndexBufferValid() ? (MultiSizeIndexContainer.GetIndexBuffer()->Num() * MultiSizeIndexContainer.GetDataTypeSize()) : 0);
	DEC_DWORD_STAT_BY(STAT_SkeletalMeshIndexMemory, AdjacencyMultiSizeIndexContainer.IsIndexBufferValid() ? (AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->Num() * AdjacencyMultiSizeIndexContainer.GetDataTypeSize()) : 0);

	MultiSizeIndexContainer.ReleaseResources();
	AdjacencyMultiSizeIndexContainer.ReleaseResources();
}

void FDynamicLODModelOverride::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const
{
	CumulativeResourceSize.AddUnknownMemoryBytes(Sections.GetAllocatedSize());
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

	CumulativeResourceSize.AddUnknownMemoryBytes(sizeof(int32));
}

SIZE_T FDynamicLODModelOverride::GetResourceSizeBytes() const
{
	FResourceSizeEx ResSize;
	GetResourceSizeEx(ResSize);
	return ResSize.GetTotalMemoryBytes();
}


/*-----------------------------------------------------------------------------
FSkeletalMeshDynamicOverride
-----------------------------------------------------------------------------*/

void FSkeletalMeshDynamicOverride::InitResources(const FSkeletalMeshRenderData& InitialData)
{
	if (!bInitialized)
	{
		// initialize resources for each lod
		for (int32 LODIndex = 0; LODIndex < InitialData.LODRenderData.Num(); LODIndex++)
		{
			FDynamicLODModelOverride* LODModel = new FDynamicLODModelOverride();
			LODModels.Add(LODModel);
			LODModel->InitResources(InitialData.LODRenderData[LODIndex]);
		}
		bInitialized = true;
	}
}

void FSkeletalMeshDynamicOverride::ReleaseResources()
{
	if (bInitialized)
	{
		// release resources for each lod
		for (int32 LODIndex = 0; LODIndex < LODModels.Num(); LODIndex++)
		{
			LODModels[LODIndex].ReleaseResources();
		}
		bInitialized = false;
	}
}


void FSkeletalMeshDynamicOverride::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	for (int32 LODIndex = 0; LODIndex < LODModels.Num(); ++LODIndex)
	{
		const FDynamicLODModelOverride& Model = LODModels[LODIndex];
		Model.GetResourceSizeEx(CumulativeResourceSize);
	}
}

SIZE_T FSkeletalMeshDynamicOverride::GetResourceSizeBytes()
{
	FResourceSizeEx ResSize;
	GetResourceSizeEx(ResSize);
	return ResSize.GetTotalMemoryBytes();
}
//nv end

/*-----------------------------------------------------------------------------
	FClothingAssetData
-----------------------------------------------------------------------------*/

FArchive& operator<<(FArchive& Ar, FClothingAssetData_Legacy& A)
{
	// Serialization to load and save ApexClothingAsset
	if( Ar.IsLoading() )
	{
		uint32 AssetSize;
		Ar << AssetSize;

		if( AssetSize > 0 )
		{
			// Load the binary blob data
			TArray<uint8> Buffer;
			Buffer.AddUninitialized( AssetSize );
			Ar.Serialize( Buffer.GetData(), AssetSize );
#if WITH_APEX_CLOTHING
			A.ApexClothingAsset = LoadApexClothingAssetFromBlob(Buffer);
#endif //#if WITH_APEX_CLOTHING
		}
	}
	else
	if( Ar.IsSaving() )
	{
#if WITH_APEX_CLOTHING
		if (A.ApexClothingAsset)
		{
			TArray<uint8> Buffer;
			SaveApexClothingAssetToBlob(A.ApexClothingAsset, Buffer);
			uint32 AssetSize = Buffer.Num();
			Ar << AssetSize;
			Ar.Serialize(Buffer.GetData(), AssetSize);
		}
		else
#endif// #if WITH_APEX_CLOTHING
		{
			uint32 AssetSize = 0;
			Ar << AssetSize;
		}
	}

	return Ar;
}


FSkeletalMeshClothBuildParams::FSkeletalMeshClothBuildParams()
	: TargetAsset(nullptr)
	, TargetLod(INDEX_NONE)
	, bRemapParameters(false)
	, AssetName("Clothing")
	, LodIndex(0)
	, SourceSection(0)
	, bRemoveFromMesh(false)
	, PhysicsAsset(nullptr)
{

}

/*-----------------------------------------------------------------------------
	USkeletalMesh
-----------------------------------------------------------------------------*/


USkeletalMesh::USkeletalMesh(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SkelMirrorAxis = EAxis::X;
	SkelMirrorFlipAxis = EAxis::Z;
#if WITH_EDITORONLY_DATA
	ImportedModel = MakeShareable(new FSkeletalMeshModel());
#endif
}

USkeletalMesh::USkeletalMesh(FVTableHelper& Helper)
	: Super(Helper)
{
}

USkeletalMesh::~USkeletalMesh() = default;

void USkeletalMesh::PostInitProperties()
{
#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
	Super::PostInitProperties();
}

FBoxSphereBounds USkeletalMesh::GetBounds()
{
	return ExtendedBounds;
}

FBoxSphereBounds USkeletalMesh::GetImportedBounds()
{
	return ImportedBounds;
}

void USkeletalMesh::SetImportedBounds(const FBoxSphereBounds& InBounds)
{
	ImportedBounds = InBounds;
	CalculateExtendedBounds();
}

void USkeletalMesh::SetPositiveBoundsExtension(const FVector& InExtension)
{
	PositiveBoundsExtension = InExtension;
	CalculateExtendedBounds();
}

void USkeletalMesh::SetNegativeBoundsExtension(const FVector& InExtension)
{
	NegativeBoundsExtension = InExtension;
	CalculateExtendedBounds();
}

void USkeletalMesh::CalculateExtendedBounds()
{
	FBoxSphereBounds CalculatedBounds = ImportedBounds;

	// Convert to Min and Max
	FVector Min = CalculatedBounds.Origin - CalculatedBounds.BoxExtent;
	FVector Max = CalculatedBounds.Origin + CalculatedBounds.BoxExtent;
	// Apply bound extensions
	Min -= NegativeBoundsExtension;
	Max += PositiveBoundsExtension;
	// Convert back to Origin, Extent and update SphereRadius
	CalculatedBounds.Origin = (Min + Max) / 2;
	CalculatedBounds.BoxExtent = (Max - Min) / 2;
	CalculatedBounds.SphereRadius = CalculatedBounds.BoxExtent.GetAbsMax();

	ExtendedBounds = CalculatedBounds;
}

void USkeletalMesh::ValidateBoundsExtension()
{
	FVector HalfExtent = ImportedBounds.BoxExtent;

	PositiveBoundsExtension.X = FMath::Clamp(PositiveBoundsExtension.X, -HalfExtent.X, MAX_flt);
	PositiveBoundsExtension.Y = FMath::Clamp(PositiveBoundsExtension.Y, -HalfExtent.Y, MAX_flt);
	PositiveBoundsExtension.Z = FMath::Clamp(PositiveBoundsExtension.Z, -HalfExtent.Z, MAX_flt);

	NegativeBoundsExtension.X = FMath::Clamp(NegativeBoundsExtension.X, -HalfExtent.X, MAX_flt);
	NegativeBoundsExtension.Y = FMath::Clamp(NegativeBoundsExtension.Y, -HalfExtent.Y, MAX_flt);
	NegativeBoundsExtension.Z = FMath::Clamp(NegativeBoundsExtension.Z, -HalfExtent.Z, MAX_flt);
}

void USkeletalMesh::AddClothingAsset(UClothingAssetBase* InNewAsset)
{
	// Check the outer is us
	if(InNewAsset && InNewAsset->GetOuter() == this)
	{
		// Ok this should be a correctly created asset, we can add it
		MeshClothingAssets.AddUnique(InNewAsset);

#if WITH_EDITOR
		OnClothingChange.Broadcast();
#endif
	}
}

#if WITH_EDITOR
void USkeletalMesh::RemoveClothingAsset(int32 InLodIndex, int32 InSectionIndex)
{
	UClothingAssetBase* Asset = GetSectionClothingAsset(InLodIndex, InSectionIndex);

	if(Asset)
	{
		Asset->UnbindFromSkeletalMesh(this, InLodIndex);
		MeshClothingAssets.Remove(Asset);

		OnClothingChange.Broadcast();
	}
}
#endif

UClothingAssetBase* USkeletalMesh::GetSectionClothingAsset(int32 InLodIndex, int32 InSectionIndex)
{
	if(FSkeletalMeshRenderData* SkelResource = GetResourceForRendering())
	{
		if(SkelResource->LODRenderData.IsValidIndex(InLodIndex))
		{
			FSkeletalMeshLODRenderData& LodData = SkelResource->LODRenderData[InLodIndex];
			if(LodData.RenderSections.IsValidIndex(InSectionIndex))
			{
				FSkelMeshRenderSection& Section = LodData.RenderSections[InSectionIndex];

				FGuid ClothingAssetGuid = Section.ClothingData.AssetGuid;

				if(ClothingAssetGuid.IsValid())
				{
					UClothingAssetBase** FoundAsset = MeshClothingAssets.FindByPredicate([&](UClothingAssetBase* InAsset)
					{
						return InAsset->GetAssetGuid() == ClothingAssetGuid;
					});
					
					return FoundAsset ? *FoundAsset : nullptr;
				}
			}
		}
	}

	return nullptr;
}

const UClothingAssetBase* USkeletalMesh::GetSectionClothingAsset(int32 InLodIndex, int32 InSectionIndex) const
{
	if (FSkeletalMeshRenderData* SkelResource = GetResourceForRendering())
	{
		if (SkelResource->LODRenderData.IsValidIndex(InLodIndex))
		{
			FSkeletalMeshLODRenderData& LodData = SkelResource->LODRenderData[InLodIndex];
			if (LodData.RenderSections.IsValidIndex(InSectionIndex))
			{
				FSkelMeshRenderSection& Section = LodData.RenderSections[InSectionIndex];

				FGuid ClothingAssetGuid = Section.ClothingData.AssetGuid;

				if (ClothingAssetGuid.IsValid())
				{
					UClothingAssetBase* const* FoundAsset = MeshClothingAssets.FindByPredicate([&](UClothingAssetBase* InAsset)
					{
						return InAsset->GetAssetGuid() == ClothingAssetGuid;
					});

					return FoundAsset ? *FoundAsset : nullptr;
				}
			}
		}
	}

	return nullptr;
}

UClothingAssetBase* USkeletalMesh::GetClothingAsset(const FGuid& InAssetGuid) const
{
	if(!InAssetGuid.IsValid())
	{
		return nullptr;
	}

	UClothingAssetBase* const* FoundAsset = MeshClothingAssets.FindByPredicate([&](UClothingAssetBase* CurrAsset)
	{
		return CurrAsset->GetAssetGuid() == InAssetGuid;
	});

	return FoundAsset ? *FoundAsset : nullptr;
}

int32 USkeletalMesh::GetClothingAssetIndex(UClothingAssetBase* InAsset) const
{
	if(!InAsset)
	{
		return INDEX_NONE;
	}

	return GetClothingAssetIndex(InAsset->GetAssetGuid());
}

int32 USkeletalMesh::GetClothingAssetIndex(const FGuid& InAssetGuid) const
{
	const int32 NumAssets = MeshClothingAssets.Num();
	for(int32 SearchIndex = 0; SearchIndex < NumAssets; ++SearchIndex)
	{
		if(MeshClothingAssets[SearchIndex]->GetAssetGuid() == InAssetGuid)
		{
			return SearchIndex;
		}
	}

	return INDEX_NONE;
}

bool USkeletalMesh::HasActiveClothingAssets() const
{
#if WITH_EDITOR
	return ComputeActiveClothingAssets();
#else
	return bHasActiveClothingAssets;
#endif
}

bool USkeletalMesh::ComputeActiveClothingAssets() const
{
	if(FSkeletalMeshRenderData* Resource = GetResourceForRendering())
	{
		for(const FSkeletalMeshLODRenderData& LodData : Resource->LODRenderData)
		{
			const int32 NumSections = LodData.RenderSections.Num();
			for(int32 SectionIdx = 0; SectionIdx < NumSections; ++SectionIdx)
			{
				const FSkelMeshRenderSection& Section = LodData.RenderSections[SectionIdx];

				if(Section.ClothingData.AssetGuid.IsValid())
				{
					return true;
				}
			}
		}
	}

	return false;
}

void USkeletalMesh::GetClothingAssetsInUse(TArray<UClothingAssetBase*>& OutClothingAssets) const
{
	OutClothingAssets.Reset();
	
	if(FSkeletalMeshRenderData* Resource = GetResourceForRendering())
	{
		for (FSkeletalMeshLODRenderData& LodData : Resource->LODRenderData)
		{
			const int32 NumSections = LodData.RenderSections.Num();
			for(int32 SectionIdx = 0; SectionIdx < NumSections; ++SectionIdx)
			{
				FSkelMeshRenderSection& Section = LodData.RenderSections[SectionIdx];

				if(Section.ClothingData.AssetGuid.IsValid())
				{
					UClothingAssetBase* Asset = GetClothingAsset(Section.ClothingData.AssetGuid);
					
					if(Asset)
					{
						OutClothingAssets.AddUnique(Asset);
					}
				}
			}
		}
	}
}

bool USkeletalMesh::NeedCPUData(int32 LODIndex)const
{
	return SamplingInfo.IsSamplingEnabled(this, LODIndex);
}

void USkeletalMesh::InitResources()
{
	UpdateUVChannelData(false);

	FSkeletalMeshRenderData* SkelMeshRenderData = GetResourceForRendering();
	if (SkelMeshRenderData)
	{
		SkelMeshRenderData->InitResources(bHasVertexColors, MorphTargets);
	}
}


void USkeletalMesh::ReleaseResources()
{
	FSkeletalMeshRenderData* SkelMeshRenderData = GetResourceForRendering();
	if (SkelMeshRenderData)
	{
		SkelMeshRenderData->ReleaseResources();
	}

	// insert a fence to signal when these commands completed
	ReleaseResourcesFence.BeginFence();
}

#if WITH_EDITORONLY_DATA
static void AccumulateUVDensities(float* OutWeightedUVDensities, float* OutWeights, const FSkeletalMeshLODRenderData& LODData, const FSkelMeshRenderSection& Section)
{
	const int32 NumTotalTriangles = LODData.GetTotalFaces();
	const int32 NumCoordinateIndex = FMath::Min<int32>(LODData.GetNumTexCoords(), TEXSTREAM_MAX_NUM_UVCHANNELS);

	FUVDensityAccumulator UVDensityAccs[TEXSTREAM_MAX_NUM_UVCHANNELS];
	for (int32 CoordinateIndex = 0; CoordinateIndex < NumCoordinateIndex; ++CoordinateIndex)
	{
		UVDensityAccs[CoordinateIndex].Reserve(NumTotalTriangles);
	}

	TArray<uint32> Indices;
	LODData.MultiSizeIndexContainer.GetIndexBuffer( Indices );
	if (!Indices.Num()) return;

	const uint32* SrcIndices = Indices.GetData() + Section.BaseIndex;
	uint32 NumTriangles = Section.NumTriangles;

	// Figure out Unreal unit per texel ratios.
	for (uint32 TriangleIndex=0; TriangleIndex < NumTriangles; TriangleIndex++ )
	{
		//retrieve indices
		uint32 Index0 = SrcIndices[TriangleIndex*3];
		uint32 Index1 = SrcIndices[TriangleIndex*3+1];
		uint32 Index2 = SrcIndices[TriangleIndex*3+2];

		const float Aera = FUVDensityAccumulator::GetTriangleAera(
			LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Index0),
			LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Index1),
			LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Index2));

		if (Aera > SMALL_NUMBER)
		{
			for (int32 CoordinateIndex = 0; CoordinateIndex < NumCoordinateIndex; ++CoordinateIndex)
			{
				const float UVAera = FUVDensityAccumulator::GetUVChannelAera(
					LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index0, CoordinateIndex),
					LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index1, CoordinateIndex),
					LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index2, CoordinateIndex));

				UVDensityAccs[CoordinateIndex].PushTriangle(Aera, UVAera);
			}
		}
	}

	for (int32 CoordinateIndex = 0; CoordinateIndex < NumCoordinateIndex; ++CoordinateIndex)
	{
		UVDensityAccs[CoordinateIndex].AccumulateDensity(OutWeightedUVDensities[CoordinateIndex], OutWeights[CoordinateIndex]);
	}
}
#endif

void USkeletalMesh::UpdateUVChannelData(bool bRebuildAll)
{
#if WITH_EDITORONLY_DATA
	// Once cooked, the data requires to compute the scales will not be CPU accessible.
	FSkeletalMeshRenderData* Resource = GetResourceForRendering();
	if (FPlatformProperties::HasEditorOnlyData() && Resource)
	{
		for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
		{
			FMeshUVChannelInfo& UVChannelData = Materials[MaterialIndex].UVChannelData;

			// Skip it if we want to keep it.
			if (UVChannelData.bInitialized && (!bRebuildAll || UVChannelData.bOverrideDensities))
				continue;

			float WeightedUVDensities[TEXSTREAM_MAX_NUM_UVCHANNELS] = {0, 0, 0, 0};
			float Weights[TEXSTREAM_MAX_NUM_UVCHANNELS] = {0, 0, 0, 0};

			for (const FSkeletalMeshLODRenderData& LODData : Resource->LODRenderData)
			{
				for (const FSkelMeshRenderSection& SectionInfo : LODData.RenderSections)
				{
					if (SectionInfo.MaterialIndex != MaterialIndex)
							continue;

					AccumulateUVDensities(WeightedUVDensities, Weights, LODData, SectionInfo);
				}
			}

			UVChannelData.bInitialized = true;
			UVChannelData.bOverrideDensities = false;
			for (int32 CoordinateIndex = 0; CoordinateIndex < TEXSTREAM_MAX_NUM_UVCHANNELS; ++CoordinateIndex)
			{
				UVChannelData.LocalUVDensities[CoordinateIndex] = (Weights[CoordinateIndex] > KINDA_SMALL_NUMBER) ? (WeightedUVDensities[CoordinateIndex] / Weights[CoordinateIndex]) : 0;
			}
		}

		Resource->SyncUVChannelData(Materials);
	}
#endif
}

const FMeshUVChannelInfo* USkeletalMesh::GetUVChannelData(int32 MaterialIndex) const
{
	if (Materials.IsValidIndex(MaterialIndex))
	{
		ensure(Materials[MaterialIndex].UVChannelData.bInitialized);
		return &Materials[MaterialIndex].UVChannelData;
	}

	return nullptr;
}

void USkeletalMesh::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	Super::GetResourceSizeEx(CumulativeResourceSize);
	// Default implementation handles subobjects

	if (SkeletalMeshRenderData.IsValid())
	{
		SkeletalMeshRenderData->GetResourceSizeEx(CumulativeResourceSize);
	}

	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(RefBasesInvMatrix.GetAllocatedSize());
	CumulativeResourceSize.AddDedicatedSystemMemoryBytes(RefSkeleton.GetDataSize());
}

/**
 * Operator for MemCount only, so it only serializes the arrays that needs to be counted.
*/
FArchive &operator<<( FArchive& Ar, FSkeletalMeshLODInfo& I )
{
	Ar << I.LODMaterialMap;

	if ( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_MOVE_SKELETALMESH_SHADOWCASTING )
	{
		Ar << I.bEnableShadowCasting_DEPRECATED;
	}

	Ar.UsingCustomVersion(FSkeletalMeshCustomVersion::GUID);
	if (Ar.CustomVer(FSkeletalMeshCustomVersion::GUID) < FSkeletalMeshCustomVersion::RemoveTriangleSorting)
	{
		uint8 DummyTriangleSorting;
		Ar << DummyTriangleSorting;

		uint8 DummyCustomLeftRightAxis;
		Ar << DummyCustomLeftRightAxis;

		FName DummyCustomLeftRightBoneName;
		Ar << DummyCustomLeftRightBoneName;
		}


	return Ar;
}

void RefreshSkelMeshOnPhysicsAssetChange(const USkeletalMesh* InSkeletalMesh)
{
	if (InSkeletalMesh)
	{
		for (FObjectIterator Iter(USkeletalMeshComponent::StaticClass()); Iter; ++Iter)
		{
			USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(*Iter);
			// if PhysicsAssetOverride is NULL, it uses SkeletalMesh Physics Asset, so I'll need to update here
			if  (SkeletalMeshComponent->SkeletalMesh == InSkeletalMesh &&
				 SkeletalMeshComponent->PhysicsAssetOverride == NULL)
			{
				// it needs to recreate IF it already has been created
				if (SkeletalMeshComponent->IsPhysicsStateCreated())
				{
					// do not call SetPhysAsset as it will setup physics asset override
					SkeletalMeshComponent->RecreatePhysicsState();
					SkeletalMeshComponent->UpdateHasValidBodies();
				}
			}
		}
#if WITH_EDITOR
		FEditorSupportDelegates::RedrawAllViewports.Broadcast();
#endif // WITH_EDITOR
	}
}

#if WITH_EDITOR
void USkeletalMesh::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	bool bFullPrecisionUVsReallyChanged = false;

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	
	if( GIsEditor &&
		PropertyThatChanged &&
		PropertyThatChanged->GetFName() == FName(TEXT("bUseFullPrecisionUVs")) )
	{
		bFullPrecisionUVsReallyChanged = true;
		if (!bUseFullPrecisionUVs && !GVertexElementTypeSupport.IsSupported(VET_Half2) )
		{
			bUseFullPrecisionUVs = true;
			UE_LOG(LogSkeletalMesh, Warning, TEXT("16 bit UVs not supported. Reverting to 32 bit UVs"));			
			bFullPrecisionUVsReallyChanged = false;
		}
	}
	
	// Don't invalidate render data when dragging sliders, too slow
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		InvalidateRenderData();
	}

	if( GIsEditor &&
		PropertyThatChanged &&
		PropertyThatChanged->GetFName() == FName(TEXT("PhysicsAsset")) )
	{
		RefreshSkelMeshOnPhysicsAssetChange(this);
	}

	if( GIsEditor &&
		Cast<UObjectProperty>(PropertyThatChanged) &&
		Cast<UObjectProperty>(PropertyThatChanged)->PropertyClass == UMorphTarget::StaticClass() )
	{
		// A morph target has changed, reinitialize morph target maps
		InitMorphTargets();
	}

	if ( GIsEditor &&
		 PropertyThatChanged &&
		 PropertyThatChanged->GetFName() == FName(TEXT("bEnablePerPolyCollision"))
		)
	{
		BuildPhysicsData();
	}

	if(UProperty* MemberProperty = PropertyChangedEvent.MemberProperty)
	{
		if(MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USkeletalMesh, PositiveBoundsExtension) ||
			MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USkeletalMesh, NegativeBoundsExtension))
		{
			// If the bounds extensions change, recalculate extended bounds.
			ValidateBoundsExtension();
			CalculateExtendedBounds();
		}
	}

	if(PropertyThatChanged && PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED(USkeletalMesh, PostProcessAnimBlueprint))
	{
		TArray<UActorComponent*> ComponentsToReregister;
		for(TObjectIterator<USkeletalMeshComponent> It; It; ++It)
		{
			USkeletalMeshComponent* MeshComponent = *It;
			if(MeshComponent && !MeshComponent->IsTemplate() && MeshComponent->SkeletalMesh == this)
			{
				ComponentsToReregister.Add(*It);
			}
		}
		FMultiComponentReregisterContext ReregisterContext(ComponentsToReregister);
	}

	if (PropertyThatChanged && PropertyChangedEvent.MemberProperty)
	{
		if (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("SamplingInfo")))
		{
			SamplingInfo.BuildRegions(this);
		}
		else if (PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("LODInfo")))
		{
			SamplingInfo.BuildWholeMesh(this);
		}
	}
	else
	{
		//Rebuild the lot. No property could mean a reimport.
		SamplingInfo.BuildRegions(this);
		SamplingInfo.BuildWholeMesh(this);
	}

	UpdateUVChannelData(true);

#if WITH_EDITOR
	OnMeshChanged.Broadcast();
#endif

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void USkeletalMesh::PostEditUndo()
{
	Super::PostEditUndo();
	for( TObjectIterator<USkinnedMeshComponent> It; It; ++It )
	{
		USkinnedMeshComponent* MeshComponent = *It;
		if( MeshComponent && 
			!MeshComponent->IsTemplate() &&
			MeshComponent->SkeletalMesh == this )
		{
			FComponentReregisterContext Context(MeshComponent);
		}
	}

	if(MorphTargets.Num() > MorphTargetIndexMap.Num())
	{
		// A morph target remove has been undone, reinitialise
		InitMorphTargets();
	}
}
#endif // WITH_EDITOR

void USkeletalMesh::BeginDestroy()
{
	Super::BeginDestroy();

	// remove the cache of link up
	if (Skeleton)
	{
		Skeleton->RemoveLinkup(this);
	}

#if WITH_APEX_CLOTHING
	// release clothing assets
	for (FClothingAssetData_Legacy& Data : ClothingAssets_DEPRECATED)
	{
		if (Data.ApexClothingAsset)
		{
			GPhysCommandHandler->DeferredRelease(Data.ApexClothingAsset);
			Data.ApexClothingAsset = nullptr;
		}
	}
#endif // #if WITH_APEX_CLOTHING

	// Release the mesh's render resources.
	ReleaseResources();
}

bool USkeletalMesh::IsReadyForFinishDestroy()
{
	// see if we have hit the resource flush fence
	return ReleaseResourcesFence.IsFenceComplete();
}

void USkeletalMesh::Serialize( FArchive& Ar )
{
	DECLARE_SCOPE_CYCLE_COUNTER( TEXT("USkeletalMesh::Serialize"), STAT_SkeletalMesh_Serialize, STATGROUP_LoadTime );

	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FFrameworkObjectVersion::GUID);
	Ar.UsingCustomVersion(FEditorObjectVersion::GUID);
	Ar.UsingCustomVersion(FSkeletalMeshCustomVersion::GUID);
	Ar.UsingCustomVersion(FRenderingObjectVersion::GUID);

	FStripDataFlags StripFlags( Ar );

	Ar << ImportedBounds;
	Ar << Materials;

	Ar << RefSkeleton;

	if(Ar.IsLoading())
	{
		const bool bRebuildNameMap = false;
		RefSkeleton.RebuildRefSkeleton(Skeleton, bRebuildNameMap);
	}

#if WITH_EDITORONLY_DATA
	// Serialize the source model (if we want editor data)
	if (!StripFlags.IsEditorDataStripped())
	{
		ImportedModel->Serialize(Ar, this);
	}
#endif // WITH_EDITORONLY_DATA

	Ar.UsingCustomVersion(FSkeletalMeshCustomVersion::GUID);
	if (Ar.CustomVer(FSkeletalMeshCustomVersion::GUID) >= FSkeletalMeshCustomVersion::SplitModelAndRenderData)
	{
		bool bCooked = Ar.IsCooking();
		Ar << bCooked;

		const bool bIsDuplicating = Ar.HasAnyPortFlags(PPF_Duplicate);

		// Inline the derived data for cooked builds. Never include render data when
		// counting memory as it is included by GetResourceSize.
		if ((bIsDuplicating || bCooked) && !IsTemplate() && !Ar.IsCountingMemory())
		{
			if (Ar.IsLoading())
			{
				SkeletalMeshRenderData = MakeUnique<FSkeletalMeshRenderData>();
				SkeletalMeshRenderData->Serialize(Ar, this);
			}
			else if (Ar.IsSaving())
			{
				if (bCooked)
				{
					int32 MaxBonesPerChunk = SkeletalMeshRenderData->GetMaxBonesPerSection();

					TArray<FName> DesiredShaderFormats;
					Ar.CookingTarget()->GetAllTargetedShaderFormats(DesiredShaderFormats);

					for (int32 FormatIndex = 0; FormatIndex < DesiredShaderFormats.Num(); ++FormatIndex)
					{
						const EShaderPlatform LegacyShaderPlatform = ShaderFormatToLegacyShaderPlatform(DesiredShaderFormats[FormatIndex]);
						const ERHIFeatureLevel::Type FeatureLevelType = GetMaxSupportedFeatureLevel(LegacyShaderPlatform);

						int32 MaxNrBones = GetFeatureLevelMaxNumberOfBones(FeatureLevelType);
						if (MaxBonesPerChunk > MaxNrBones)
						{
							FString FeatureLevelName;
							GetFeatureLevelName(FeatureLevelType, FeatureLevelName);
							UE_LOG(LogSkeletalMesh, Warning, TEXT("Skeletal mesh %s has a LOD section with %d bones and the maximum supported number for feature level %s is %d.\n!This mesh will not be rendered on the specified platform!"),
								*GetFullName(), MaxBonesPerChunk, *FeatureLevelName, MaxNrBones);
						}
					}
				}

				SkeletalMeshRenderData->Serialize(Ar, this);
			}
		}
	}

	// make sure we're counting properly
	if (!Ar.IsLoading() && !Ar.IsSaving())
	{
		Ar << RefBasesInvMatrix;
	}

	if( Ar.UE4Ver() < VER_UE4_REFERENCE_SKELETON_REFACTOR )
	{
		TMap<FName, int32> DummyNameIndexMap;
		Ar << DummyNameIndexMap;
	}

	//@todo legacy
	TArray<UObject*> DummyObjs;
	Ar << DummyObjs;

	if (Ar.IsLoading() && Ar.CustomVer(FRenderingObjectVersion::GUID) < FRenderingObjectVersion::TextureStreamingMeshUVChannelData)
	{
		TArray<float> CachedStreamingTextureFactors;
		Ar << CachedStreamingTextureFactors;
	}

#if WITH_EDITORONLY_DATA
	if ( !StripFlags.IsEditorDataStripped() )
	{
		// Backwards compat for old SourceData member
		if (Ar.IsLoading() && Ar.CustomVer(FSkeletalMeshCustomVersion::GUID) < FSkeletalMeshCustomVersion::RemoveSourceData)
		{
			bool bHaveSourceData = false;
			Ar << bHaveSourceData;
			if (bHaveSourceData)
			{
				FSkeletalMeshLODModel DummyLODModel;
				DummyLODModel.Serialize(Ar, this, INDEX_NONE);
			}
		}
	}

	if ( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_ASSET_IMPORT_DATA_AS_JSON && !AssetImportData)
	{
		// AssetImportData should always be valid
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
	
	// SourceFilePath and SourceFileTimestamp were moved into a subobject
	if ( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_ADDED_FBX_ASSET_IMPORT_DATA && AssetImportData)
	{
		// AssetImportData should always have been set up in the constructor where this is relevant
		FAssetImportInfo Info;
		Info.Insert(FAssetImportInfo::FSourceFile(SourceFilePath_DEPRECATED));
		AssetImportData->SourceData = MoveTemp(Info);
		
		SourceFilePath_DEPRECATED = TEXT("");
		SourceFileTimestamp_DEPRECATED = TEXT("");
	}

	if (Ar.UE4Ver() >= VER_UE4_APEX_CLOTH)
	{
		if(Ar.CustomVer(FSkeletalMeshCustomVersion::GUID) < FSkeletalMeshCustomVersion::NewClothingSystemAdded)
		{
		// Serialize non-UPROPERTY ApexClothingAsset data.
			for(int32 Idx = 0; Idx < ClothingAssets_DEPRECATED.Num(); Idx++)
		{
				Ar << ClothingAssets_DEPRECATED[Idx];
			}
		}

		if (Ar.UE4Ver() < VER_UE4_REFERENCE_SKELETON_REFACTOR)
		{
			RebuildRefSkeletonNameToIndexMap();
		}
	}

	if ( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_MOVE_SKELETALMESH_SHADOWCASTING )
	{
		// Previous to this version, shadowcasting flags were stored in the LODInfo array
		// now they're in the Materials array so we need to move them over
		MoveDeprecatedShadowFlagToMaterials();
	}

	if (Ar.UE4Ver() < VER_UE4_SKELETON_ASSET_PROPERTY_TYPE_CHANGE)
	{
		PreviewAttachedAssetContainer.SaveAttachedObjectsFromDeprecatedProperties();
	}
#endif

	if (bEnablePerPolyCollision)
	{
		Ar << BodySetup;
	}

#if WITH_EDITORONLY_DATA
	if (Ar.CustomVer(FEditorObjectVersion::GUID) < FEditorObjectVersion::RefactorMeshEditorMaterials)
	{
		MoveMaterialFlagsToSections();
	}
#endif

#if WITH_EDITORONLY_DATA
	bRequiresLODScreenSizeConversion = Ar.CustomVer(FFrameworkObjectVersion::GUID) < FFrameworkObjectVersion::LODsUseResolutionIndependentScreenSize;
	bRequiresLODHysteresisConversion = Ar.CustomVer(FFrameworkObjectVersion::GUID) < FFrameworkObjectVersion::LODHysteresisUseResolutionIndependentScreenSize;
#endif

}

void USkeletalMesh::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{	
	USkeletalMesh* This = CastChecked<USkeletalMesh>(InThis);
#if WITH_EDITOR
	if( GIsEditor )
	{
		// Required by the unified GC when running in the editor
		for( int32 Index = 0; Index < This->Materials.Num(); Index++ )
		{
			Collector.AddReferencedObject( This->Materials[ Index ].MaterialInterface, This );
		}
	}
#endif
	Super::AddReferencedObjects( This, Collector );
}

void USkeletalMesh::FlushRenderState()
{
	//TComponentReregisterContext<USkeletalMeshComponent> ReregisterContext;

	// Release the mesh's render resources.
	ReleaseResources();

	// Flush the resource release commands to the rendering thread to ensure that the edit change doesn't occur while a resource is still
	// allocated, and potentially accessing the mesh data.
	ReleaseResourcesFence.Wait();
}

uint32 USkeletalMesh::GetVertexBufferFlags() const
{
	uint32 VertexFlags = ESkeletalMeshVertexFlags::None;
	if (bUseFullPrecisionUVs)
	{
		VertexFlags |= ESkeletalMeshVertexFlags::UseFullPrecisionUVs;
	}
	if (bHasVertexColors)
	{
		VertexFlags |= ESkeletalMeshVertexFlags::HasVertexColors;
	}
	return VertexFlags;
}

void USkeletalMesh::InvalidateRenderData()
{
	// Unregister all instances of this component
	FSkinnedMeshComponentRecreateRenderStateContext RecreateRenderStateContext(this, false);

	// Release the static mesh's resources.
	ReleaseResources();

	// Flush the resource release commands to the rendering thread to ensure that the build doesn't occur while a resource is still
	// allocated, and potentially accessing the USkeletalMesh.
	ReleaseResourcesFence.Wait();

#if WITH_EDITOR
	// Create new 
	GetImportedModel()->GenerateNewGUID();

	// rebuild render data from imported model
	CacheDerivedData();
#endif

	// Do not need to fix up 16-bit UVs here, as we assume all editor platforms support them.
	ensure(GVertexElementTypeSupport.IsSupported(VET_Half2));

	// Note: meshes can be built during automated importing.  We should not create resources in that case
	// as they will never be released when this object is deleted
	if (FApp::CanEverRender())
	{
		// Reinitialize the static mesh's resources.
		InitResources();
	}
}

void USkeletalMesh::PreSave(const class ITargetPlatform* TargetPlatform)
{
	// check the parent index of the root bone is invalid
	check((RefSkeleton.GetNum() == 0) || (RefSkeleton.GetRefBoneInfo()[0].ParentIndex == INDEX_NONE));

	Super::PreSave(TargetPlatform);
}

// Pre-calculate refpose-to-local transforms
void USkeletalMesh::CalculateInvRefMatrices()
{
	const int32 NumRealBones = RefSkeleton.GetRawBoneNum();

	if( RefBasesInvMatrix.Num() != NumRealBones)
	{
		RefBasesInvMatrix.Empty(NumRealBones);
		RefBasesInvMatrix.AddUninitialized(NumRealBones);

		// Reset cached mesh-space ref pose
		CachedComposedRefPoseMatrices.Empty(NumRealBones);
		CachedComposedRefPoseMatrices.AddUninitialized(NumRealBones);

		// Precompute the Mesh.RefBasesInverse.
		for( int32 b=0; b<NumRealBones; b++)
		{
			// Render the default pose.
			CachedComposedRefPoseMatrices[b] = GetRefPoseMatrix(b);

			// Construct mesh-space skeletal hierarchy.
			if( b>0 )
			{
				int32 Parent = RefSkeleton.GetRawParentIndex(b);
				CachedComposedRefPoseMatrices[b] = CachedComposedRefPoseMatrices[b] * CachedComposedRefPoseMatrices[Parent];
			}

			FVector XAxis, YAxis, ZAxis;

			CachedComposedRefPoseMatrices[b].GetScaledAxes(XAxis, YAxis, ZAxis);
			if(	XAxis.IsNearlyZero(SMALL_NUMBER) &&
				YAxis.IsNearlyZero(SMALL_NUMBER) &&
				ZAxis.IsNearlyZero(SMALL_NUMBER))
			{
				// this is not allowed, warn them 
				UE_LOG(LogSkeletalMesh, Warning, TEXT("Reference Pose for joint (%s) includes NIL matrix. Zero scale isn't allowed on ref pose. "), *RefSkeleton.GetBoneName(b).ToString());
			}

			// Precompute inverse so we can use from-refpose-skin vertices.
			RefBasesInvMatrix[b] = CachedComposedRefPoseMatrices[b].Inverse(); 
		}

#if WITH_EDITORONLY_DATA
		if(RetargetBasePose.Num() == 0)
		{
			RetargetBasePose = RefSkeleton.GetRawRefBonePose();
		}
#endif // WITH_EDITORONLY_DATA
	}
}

#if WITH_EDITOR
void USkeletalMesh::CalculateRequiredBones(FSkeletalMeshLODModel& LODModel, const struct FReferenceSkeleton& RefSkeleton, const TMap<FBoneIndexType, FBoneIndexType> * BonesToRemove)
{
	// RequiredBones for base model includes all raw bones.
	int32 RequiredBoneCount = RefSkeleton.GetRawBoneNum();
	LODModel.RequiredBones.Empty(RequiredBoneCount);
	for(int32 i=0; i<RequiredBoneCount; i++)
	{
		// Make sure it's not in BonesToRemove
		// @Todo change this to one TArray
		if (!BonesToRemove || BonesToRemove->Find(i) == NULL)
		{
			LODModel.RequiredBones.Add(i);
		}
	}

	LODModel.RequiredBones.Shrink();	
}

#if WITH_APEX_CLOTHING

void USkeletalMesh::UpgradeOldClothingAssets()
{
	// Can only do an old-> new clothing asset upgrade in the editor.
	// And only if APEX clothing is available to upgrade from
	if (ClothingAssets_DEPRECATED.Num() > 0)
	{
		// Upgrade the old deprecated clothing assets in to new clothing assets
		TMap<int32, TArray<int32>> OldLodMappings; // Map asset index to multiple lod indices
		TMap<int32, TArray<int32>> OldSectionMappings; // Map asset index to a section per LOD
		for (int32 AssetIdx = 0; AssetIdx < ClothingAssets_DEPRECATED.Num(); ++AssetIdx)
		{
			FClothingAssetData_Legacy& OldAssetData = ClothingAssets_DEPRECATED[AssetIdx];

			OldLodMappings.Add(AssetIdx);
			OldSectionMappings.Add(AssetIdx);

			if (ImportedModel.IsValid())
			{
				int32 FoundLod = INDEX_NONE;
				int32 FoundSection = INDEX_NONE;
				for (int32 LodIdx = 0; LodIdx < ImportedModel->LODModels.Num(); ++LodIdx)
				{
					FSkeletalMeshLODModel& LodModel = ImportedModel->LODModels[LodIdx];

					for (int32 SecIdx = 0; SecIdx < LodModel.Sections.Num(); ++SecIdx)
					{
						FSkelMeshSection& Section = LodModel.Sections[SecIdx];

						if (Section.CorrespondClothSectionIndex_DEPRECATED != INDEX_NONE && Section.bLegacyClothingSection_DEPRECATED)
						{
							FSkelMeshSection& ClothSection = LodModel.Sections[Section.CorrespondClothSectionIndex_DEPRECATED];

							if (ClothSection.CorrespondClothAssetIndex == AssetIdx)
							{
								FoundSection = SecIdx;
								break;
							}
						}
					}

					if (FoundSection != INDEX_NONE)
					{
						OldLodMappings[AssetIdx].Add(LodIdx);
						OldSectionMappings[AssetIdx].Add(FoundSection);

						// Reset for next LOD
						FoundSection = INDEX_NONE;
					}
				}
			}

			FClothingSystemEditorInterfaceModule& ClothingEditorModule = FModuleManager::Get().LoadModuleChecked<FClothingSystemEditorInterfaceModule>(TEXT("ClothingSystemEditorInterface"));
			UClothingAssetFactoryBase* Factory = ClothingEditorModule.GetClothingAssetFactory();
			if (Factory)
			{
				UClothingAssetBase* NewAsset = Factory->CreateFromApexAsset(OldAssetData.ApexClothingAsset, this, *FPaths::GetBaseFilename(OldAssetData.ApexFileName));
				check(NewAsset);

				// Pull the path across so reimports work as expected
				NewAsset->ImportedFilePath = OldAssetData.ApexFileName;

				MeshClothingAssets.Add(NewAsset);
			}
		}

		// Go back over the old assets and remove them from the skeletal mesh so the indices are preserved while
		// calculating the LOD and section mappings above.
		for (int32 AssetIdx = ClothingAssets_DEPRECATED.Num() - 1; AssetIdx >= 0; --AssetIdx)
		{
			ApexClothingUtils::RemoveAssetFromSkeletalMesh(this, AssetIdx, false);
		}

		check(OldLodMappings.Num() == OldSectionMappings.Num());

		for (int32 NewAssetIdx = 0; NewAssetIdx < MeshClothingAssets.Num(); ++NewAssetIdx)
		{
			UClothingAssetBase* CurrAsset = MeshClothingAssets[NewAssetIdx];

			for (int32 MappedLodIdx = 0; MappedLodIdx < OldLodMappings[NewAssetIdx].Num(); ++MappedLodIdx)
			{
				const int32 MappedLod = OldLodMappings[NewAssetIdx][MappedLodIdx];
				const int32 MappedSection = OldSectionMappings[NewAssetIdx][MappedLodIdx];

				// Previously Clothing LODs were required to match skeletal mesh LODs, which is why we pass
				// MappedLod for both the mesh and clothing LODs here when doing an upgrade to the new
				// system. This restriction is now lifted and any mapping can be selected in Persona
				CurrAsset->BindToSkeletalMesh(this, MappedLod, MappedSection, MappedLod);
			}
		}
	}
}

#endif // WITH_APEX_CLOTHING

void USkeletalMesh::RemoveLegacyClothingSections()
{
	// Remove duplicate skeletal mesh sections previously used for clothing simulation
	if(GetLinkerCustomVersion(FSkeletalMeshCustomVersion::GUID) < FSkeletalMeshCustomVersion::RemoveDuplicatedClothingSections)
	{
		if(FSkeletalMeshModel* Model = GetImportedModel())
		{
			for(FSkeletalMeshLODModel& LodModel : Model->LODModels)
			{
				int32 ClothingSectionCount = 0;
				uint32 BaseVertex = MAX_uint32;
				int32 VertexCount = 0;
				uint32 BaseIndex = MAX_uint32;
				int32 IndexCount = 0;

				for(int32 SectionIndex = 0; SectionIndex < LodModel.Sections.Num(); ++SectionIndex)
				{
					FSkelMeshSection& Section = LodModel.Sections[SectionIndex];

					// If the section is disabled, it could be a clothing section
					if(Section.bLegacyClothingSection_DEPRECATED && Section.CorrespondClothSectionIndex_DEPRECATED != INDEX_NONE)
					{
						FSkelMeshSection& DuplicatedSection = LodModel.Sections[Section.CorrespondClothSectionIndex_DEPRECATED];

						// Cache the base index for the first clothing section (will be in correct order)
						if(ClothingSectionCount == 0)
						{
							PreEditChange(nullptr);
						}
						
						BaseVertex = FMath::Min(DuplicatedSection.BaseVertexIndex, BaseVertex);
						BaseIndex = FMath::Min(DuplicatedSection.BaseIndex, BaseIndex);

						VertexCount += DuplicatedSection.SoftVertices.Num();
						IndexCount += DuplicatedSection.NumTriangles * 3;

						// Mapping data for clothing could be built either on the source or the
						// duplicated section and has changed a few times, so check here for
						// where to get our data from
						if(DuplicatedSection.ClothMappingData.Num() > 0)
						{
							Section.ClothingData = DuplicatedSection.ClothingData;
							Section.ClothMappingData = DuplicatedSection.ClothMappingData;
						}

						Section.CorrespondClothAssetIndex = MeshClothingAssets.IndexOfByPredicate([&Section](const UClothingAssetBase* CurrAsset) 
						{
							return CurrAsset->GetAssetGuid() == Section.ClothingData.AssetGuid;
						});

						Section.BoneMap = DuplicatedSection.BoneMap;
						Section.bLegacyClothingSection_DEPRECATED = false;

						// Remove the reference index
						Section.CorrespondClothSectionIndex_DEPRECATED = INDEX_NONE;

						ClothingSectionCount++;
					}
				}

				if(BaseVertex != MAX_uint32 && BaseIndex != MAX_uint32)
				{
					// Remove from section list
					LodModel.Sections.RemoveAt(LodModel.Sections.Num() - ClothingSectionCount, ClothingSectionCount);

					// Clean up actual geometry
					LodModel.IndexBuffer.RemoveAt(BaseIndex, IndexCount);
					LodModel.NumVertices -= VertexCount;

					// Clean up index entries above the base we removed.
					// Ideally this shouldn't be unnecessary as clothing was at the end of the buffer
					// but this will always be safe to run to make sure adjacency generates correctly.
					for(uint32& Index : LodModel.IndexBuffer)
					{
						if(Index >= BaseVertex)
						{
							Index -= VertexCount;
						}
					}

					PostEditChange();
				}
			}
		}
	}
}

#endif // WITH_EDITOR

void USkeletalMesh::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	// If LODInfo is missing - create array of correct size.
	if( LODInfo.Num() != ImportedModel->LODModels.Num() )
	{
		LODInfo.Empty(ImportedModel->LODModels.Num());
		LODInfo.AddZeroed(ImportedModel->LODModels.Num());

		for(int32 i=0; i<LODInfo.Num(); i++)
		{
			LODInfo[i].LODHysteresis = 0.02f;
		}
	}

	int32 TotalLODNum = LODInfo.Num();
	for (int32 LodIndex = 0; LodIndex<TotalLODNum; LodIndex++)
	{
		FSkeletalMeshLODInfo& ThisLODInfo = LODInfo[LodIndex];
		FSkeletalMeshLODModel& ThisLODModel = ImportedModel->LODModels[LodIndex];

		if (ThisLODInfo.ReductionSettings.BonesToRemove_DEPRECATED.Num() > 0)
		{
			for (auto& BoneToRemove : ThisLODInfo.ReductionSettings.BonesToRemove_DEPRECATED)
			{
				AddBoneToReductionSetting(LodIndex, BoneToRemove.BoneName);
			}

			// since in previous system, we always removed from previous LOD, I'm adding this 
			// here for previous LODs
			for (int32 CurLodIndx = LodIndex + 1; CurLodIndx < TotalLODNum; ++CurLodIndx)
			{
				AddBoneToReductionSetting(CurLodIndx, ThisLODInfo.RemovedBones_DEPRECATED);
			}

			// we don't apply this change here, but this will be applied when you re-gen simplygon
			ThisLODInfo.ReductionSettings.BonesToRemove_DEPRECATED.Empty();
		}

		if (ThisLODInfo.ReductionSettings.BakePose_DEPRECATED != nullptr)
		{
			ThisLODInfo.BakePose = ThisLODInfo.ReductionSettings.BakePose_DEPRECATED;
			ThisLODInfo.ReductionSettings.BakePose_DEPRECATED = nullptr;
		}
	}

	if (GetLinkerUE4Version() < VER_UE4_SORT_ACTIVE_BONE_INDICES)
	{
		for (int32 LodIndex = 0; LodIndex < LODInfo.Num(); LodIndex++)
		{
			FSkeletalMeshLODModel & ThisLODModel = ImportedModel->LODModels[LodIndex];
			ThisLODModel.ActiveBoneIndices.Sort();
			}
		}

#if WITH_APEX_CLOTHING
	UpgradeOldClothingAssets();
#endif // WITH_APEX_CLOTHING

	RemoveLegacyClothingSections();

	if (GetResourceForRendering() == nullptr)
	{
		CacheDerivedData();
	}
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
	if (GetLinkerCustomVersion(FRenderingObjectVersion::GUID) < FRenderingObjectVersion::FixedMeshUVDensity)
	{
		UpdateUVChannelData(true);
	}
#endif // WITH_EDITOR

	// init morph targets. 
	// should do this before InitResource, so that we clear invalid morphtargets
	InitMorphTargets();

	// initialize rendering resources
	if (FApp::CanEverRender())
	{
		InitResources();
	}
	else
	{
		// Update any missing data when cooking.
		UpdateUVChannelData(false);
	}

	CalculateInvRefMatrices();

#if WITH_EDITORONLY_DATA
	if (RetargetBasePose.Num() == 0)
	{
		RetargetBasePose = RefSkeleton.GetRefBonePose();
	}
#endif

	// Bounds have been loaded - apply extensions.
	CalculateExtendedBounds();

#if WITH_EDITORONLY_DATA
	if (bRequiresLODScreenSizeConversion || bRequiresLODHysteresisConversion)
	{
		// Convert screen area to screen size
		ConvertLegacyLODScreenSize();
	}
#endif

	// If inverse masses have never been cached, invalidate data so it will be recalculated
	if(GetLinkerCustomVersion(FSkeletalMeshCustomVersion::GUID) < FSkeletalMeshCustomVersion::CachedClothInverseMasses)
	{
		for(UClothingAssetBase* ClothingAsset : MeshClothingAssets)
		{
			ClothingAsset->InvalidateCachedData();
		}
	}

	bHasActiveClothingAssets = ComputeActiveClothingAssets();
}

#if WITH_EDITORONLY_DATA

void USkeletalMesh::RebuildRefSkeletonNameToIndexMap()
{
	TArray<FBoneIndexType> DuplicateBones;
	// Make sure we have no duplicate bones. Some content got corrupted somehow. :(
	RefSkeleton.RemoveDuplicateBones(this, DuplicateBones);

	// If we have removed any duplicate bones, we need to fix up any broken LODs as well.
	// Duplicate bones are given from highest index to lowest.
	// so it's safe to decrease indices for children, we're not going to lose the index of the remaining duplicate bones.
	for (int32 Index = 0; Index < DuplicateBones.Num(); Index++)
	{
		const FBoneIndexType& DuplicateBoneIndex = DuplicateBones[Index];
		for (int32 LodIndex = 0; LodIndex < LODInfo.Num(); LodIndex++)
		{
			FSkeletalMeshLODModel& ThisLODModel = ImportedModel->LODModels[LodIndex];
			{
				int32 FoundIndex;
				if (ThisLODModel.RequiredBones.Find(DuplicateBoneIndex, FoundIndex))
				{
					ThisLODModel.RequiredBones.RemoveAt(FoundIndex, 1);
					// we need to shift indices of the remaining bones.
					for (int32 j = FoundIndex; j < ThisLODModel.RequiredBones.Num(); j++)
					{
						ThisLODModel.RequiredBones[j] = ThisLODModel.RequiredBones[j] - 1;
					}
				}
			}

			{
				int32 FoundIndex;
				if (ThisLODModel.ActiveBoneIndices.Find(DuplicateBoneIndex, FoundIndex))
				{
					ThisLODModel.ActiveBoneIndices.RemoveAt(FoundIndex, 1);
					// we need to shift indices of the remaining bones.
					for (int32 j = FoundIndex; j < ThisLODModel.ActiveBoneIndices.Num(); j++)
					{
						ThisLODModel.ActiveBoneIndices[j] = ThisLODModel.ActiveBoneIndices[j] - 1;
					}
				}
			}
		}
	}

	// Rebuild name table.
	RefSkeleton.RebuildNameToIndexMap();
}

#endif


void USkeletalMesh::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	int32 NumTriangles = 0;
	FSkeletalMeshRenderData* SkelMeshRenderData = GetResourceForRendering();
	if (SkelMeshRenderData && SkelMeshRenderData->LODRenderData.Num() > 0)
	{
		const FSkeletalMeshLODRenderData& LODData = SkelMeshRenderData->LODRenderData[0];
		NumTriangles = LODData.GetTotalFaces();
	}

	OutTags.Add(FAssetRegistryTag("Triangles", FString::FromInt(NumTriangles), FAssetRegistryTag::TT_Numerical));
	OutTags.Add(FAssetRegistryTag("Bones", FString::FromInt(RefSkeleton.GetRawBoneNum()), FAssetRegistryTag::TT_Numerical));
	OutTags.Add(FAssetRegistryTag("MorphTargets", FString::FromInt(MorphTargets.Num()), FAssetRegistryTag::TT_Numerical));

#if WITH_EDITORONLY_DATA
	if (AssetImportData)
	{
		OutTags.Add( FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden) );
	}
#endif
	
	Super::GetAssetRegistryTags(OutTags);
}

#if WITH_EDITOR
void USkeletalMesh::GetAssetRegistryTagMetadata(TMap<FName, FAssetRegistryTagMetadata>& OutMetadata) const
{
	Super::GetAssetRegistryTagMetadata(OutMetadata);
	OutMetadata.Add("PhysicsAsset", FAssetRegistryTagMetadata().SetImportantValue(TEXT("None")));
}
#endif

void USkeletalMesh::DebugVerifySkeletalMeshLOD()
{
	// if LOD do not have displayfactor set up correctly
	if (LODInfo.Num() > 1)
	{
		for(int32 i=1; i<LODInfo.Num(); i++)
		{
			if (LODInfo[i].ScreenSize <= 0.1f)
			{
				// too small
				UE_LOG(LogSkeletalMesh, Warning, TEXT("SkelMeshLOD (%s) : ScreenSize for LOD %d may be too small (%0.5f)"), *GetPathName(), i, LODInfo[i].ScreenSize);
			}
		}
	}
	else
	{
		// no LODInfo
		UE_LOG(LogSkeletalMesh, Warning, TEXT("SkelMeshLOD (%s) : LOD does not exist"), *GetPathName());
	}
}

void USkeletalMesh::RegisterMorphTarget(UMorphTarget* MorphTarget)
{
	if ( MorphTarget )
	{
		// if MorphTarget has SkelMesh, make sure you unregister before registering yourself
		if ( MorphTarget->BaseSkelMesh && MorphTarget->BaseSkelMesh!=this )
		{
			MorphTarget->BaseSkelMesh->UnregisterMorphTarget(MorphTarget);
		}

		// if the input morphtarget doesn't have valid data, do not add to the base morphtarget
		ensureMsgf(MorphTarget->HasValidData(), TEXT("RegisterMorphTarget: %s has empty data."), *MorphTarget->GetName());

		MorphTarget->BaseSkelMesh = this;

		bool bRegistered = false;

		for ( int32 Index = 0; Index < MorphTargets.Num(); ++Index )
		{
			if ( MorphTargets[Index]->GetFName() == MorphTarget->GetFName() )
			{
				UE_LOG( LogSkeletalMesh, Log, TEXT("RegisterMorphTarget: %s already exists, replacing"), *MorphTarget->GetName() );
				MorphTargets[Index] = MorphTarget;
				bRegistered = true;
				break;
			}
		}

		if (!bRegistered)
		{
			MorphTargets.Add( MorphTarget );
			bRegistered = true;
		}

		if (bRegistered)
		{
			MarkPackageDirty();
			// need to refresh the map
			InitMorphTargets();
			// invalidate render data
			InvalidateRenderData();
		}
	}
}

void USkeletalMesh::UnregisterAllMorphTarget()
{
	MorphTargets.Empty();
	MarkPackageDirty();
	// need to refresh the map
	InitMorphTargets();
	// invalidate render data
	InvalidateRenderData();
}

void USkeletalMesh::UnregisterMorphTarget(UMorphTarget* MorphTarget)
{
	if ( MorphTarget )
	{
		// Do not remove with MorphTarget->GetFName(). The name might have changed
		// Search the value, and delete	
		for ( int32 I=0; I<MorphTargets.Num(); ++I)
		{
			if ( MorphTargets[I] == MorphTarget )
			{
				MorphTargets.RemoveAt(I);
				--I;
				MarkPackageDirty();
				// need to refresh the map
				InitMorphTargets();
				// invalidate render data
				InvalidateRenderData();
				return;
			}
		}

		UE_LOG( LogSkeletalMesh, Log, TEXT("UnregisterMorphTarget: %s not found."), *MorphTarget->GetName() );
	}
}

void USkeletalMesh::InitMorphTargets()
{
	MorphTargetIndexMap.Empty();

	for (int32 Index = 0; Index < MorphTargets.Num(); ++Index)
	{
		UMorphTarget* MorphTarget = MorphTargets[Index];
		// if we don't have a valid data, just remove it
		if (!MorphTarget->HasValidData())
		{
			MorphTargets.RemoveAt(Index);
			--Index;
			continue;
		}

		FName const ShapeName = MorphTarget->GetFName();
		if (MorphTargetIndexMap.Find(ShapeName) == nullptr)
		{
			MorphTargetIndexMap.Add(ShapeName, Index);

			// register as morphtarget curves
			if (Skeleton)
			{
				FSmartName CurveName;
				CurveName.DisplayName = ShapeName;
				
				// verify will make sure it adds to the curve if not found
				// the reason of using this is to make sure it works in editor/non-editor
				Skeleton->VerifySmartName(USkeleton::AnimCurveMappingName, CurveName);
				Skeleton->AccumulateCurveMetaData(ShapeName, false, true);
			}
		}
	}
}

UMorphTarget* USkeletalMesh::FindMorphTarget(FName MorphTargetName) const
{
	int32 Index;
	return FindMorphTargetAndIndex(MorphTargetName, Index);
}

UMorphTarget* USkeletalMesh::FindMorphTargetAndIndex(FName MorphTargetName, int32& OutIndex) const
{
	OutIndex = INDEX_NONE;
	if( MorphTargetName != NAME_None )
	{
		const int32* Found = MorphTargetIndexMap.Find(MorphTargetName);
		if (Found)
		{
			OutIndex = *Found;
			return MorphTargets[*Found];
		}
	}

	return nullptr;
}

USkeletalMeshSocket* USkeletalMesh::FindSocket(FName InSocketName) const
{
	int32 DummyIdx;
	return FindSocketAndIndex(InSocketName, DummyIdx);
}

USkeletalMeshSocket* USkeletalMesh::FindSocketAndIndex(FName InSocketName, int32& OutIndex) const
{
	OutIndex = INDEX_NONE;
	if (InSocketName == NAME_None)
	{
		return nullptr;
	}

	for (int32 i = 0; i < Sockets.Num(); i++)
	{
		USkeletalMeshSocket* Socket = Sockets[i];
		if (Socket && Socket->SocketName == InSocketName)
		{
			OutIndex = i;
			return Socket;
		}
	}

	// If the socket isn't on the mesh, try to find it on the skeleton
	if (Skeleton)
	{
		USkeletalMeshSocket* SkeletonSocket = Skeleton->FindSocketAndIndex(InSocketName, OutIndex);
		if (SkeletonSocket != nullptr)
		{
			OutIndex += Sockets.Num();
		}
		return SkeletonSocket;
	}

	return nullptr;
}

int32 USkeletalMesh::NumSockets() const
{
	return Sockets.Num() + (Skeleton ? Skeleton->Sockets.Num() : 0);
}

USkeletalMeshSocket* USkeletalMesh::GetSocketByIndex(int32 Index) const
{
	if (Index < Sockets.Num())
	{
		return Sockets[Index];
	}

	if (Skeleton && Index < Skeleton->Sockets.Num())
	{
		return Skeleton->Sockets[Index];
	}

	return nullptr;
}



/**
 * This will return detail info about this specific object. (e.g. AudioComponent will return the name of the cue,
 * ParticleSystemComponent will return the name of the ParticleSystem)  The idea here is that in many places
 * you have a component of interest but what you really want is some characteristic that you can use to track
 * down where it came from.  
 */
FString USkeletalMesh::GetDetailedInfoInternal() const
{
	return GetPathName(nullptr);
}


FMatrix USkeletalMesh::GetRefPoseMatrix( int32 BoneIndex ) const
{
	check( BoneIndex >= 0 && BoneIndex < RefSkeleton.GetRawBoneNum() );
	FTransform BoneTransform = RefSkeleton.GetRawRefBonePose()[BoneIndex];
	// Make sure quaternion is normalized!
	BoneTransform.NormalizeRotation();
	return BoneTransform.ToMatrixWithScale();
}

FMatrix USkeletalMesh::GetComposedRefPoseMatrix( FName InBoneName ) const
{
	FMatrix LocalPose( FMatrix::Identity );

	if ( InBoneName != NAME_None )
	{
		int32 BoneIndex = RefSkeleton.FindBoneIndex(InBoneName);
		if (BoneIndex != INDEX_NONE)
		{
			return GetComposedRefPoseMatrix(BoneIndex);
		}
		else
		{
			USkeletalMeshSocket const* Socket = FindSocket(InBoneName);

			if(Socket != NULL)
			{
				BoneIndex = RefSkeleton.FindBoneIndex(Socket->BoneName);

				if(BoneIndex != INDEX_NONE)
				{
					const FRotationTranslationMatrix SocketMatrix(Socket->RelativeRotation, Socket->RelativeLocation);
					LocalPose = SocketMatrix * GetComposedRefPoseMatrix(BoneIndex);
				}
			}
		}
	}

	return LocalPose;
}

FMatrix USkeletalMesh::GetComposedRefPoseMatrix(int32 InBoneIndex) const
{
	return CachedComposedRefPoseMatrices[InBoneIndex];
}

TArray<USkeletalMeshSocket*>& USkeletalMesh::GetMeshOnlySocketList()
{
	return Sockets;
}

const TArray<USkeletalMeshSocket*>& USkeletalMesh::GetMeshOnlySocketList() const
{
	return Sockets;
}

#if WITH_EDITORONLY_DATA
void USkeletalMesh::MoveDeprecatedShadowFlagToMaterials()
{
	// First, the easy case where there's no LOD info (in which case, default to true!)
	if ( LODInfo.Num() == 0 )
	{
		for ( auto Material = Materials.CreateIterator(); Material; ++Material )
		{
			Material->bEnableShadowCasting_DEPRECATED = true;
		}

		return;
	}
	
	TArray<bool> PerLodShadowFlags;
	bool bDifferenceFound = false;

	// Second, detect whether the shadow casting flag is the same for all sections of all lods
	for ( auto LOD = LODInfo.CreateConstIterator(); LOD; ++LOD )
	{
		if ( LOD->bEnableShadowCasting_DEPRECATED.Num() )
		{
			PerLodShadowFlags.Add( LOD->bEnableShadowCasting_DEPRECATED[0] );
		}

		if ( !AreAllFlagsIdentical( LOD->bEnableShadowCasting_DEPRECATED ) )
		{
			// We found a difference in the sections of this LOD!
			bDifferenceFound = true;
			break;
		}
	}

	if ( !bDifferenceFound && !AreAllFlagsIdentical( PerLodShadowFlags ) )
	{
		// Difference between LODs
		bDifferenceFound = true;
	}

	if ( !bDifferenceFound )
	{
		// All the same, so just copy the shadow casting flag to all materials
		for ( auto Material = Materials.CreateIterator(); Material; ++Material )
		{
			Material->bEnableShadowCasting_DEPRECATED = PerLodShadowFlags.Num() ? PerLodShadowFlags[0] : true;
		}
	}
	else
	{
		FSkeletalMeshModel* Resource = GetImportedModel();
		check( Resource->LODModels.Num() == LODInfo.Num() );

		TArray<FSkeletalMaterial> NewMaterialArray;

		// There was a difference, so we need to build a new material list which has all the combinations of UMaterialInterface and shadow casting flag required
		for ( int32 LODIndex = 0; LODIndex < Resource->LODModels.Num(); ++LODIndex )
		{
			check( Resource->LODModels[LODIndex].Sections.Num() == LODInfo[LODIndex].bEnableShadowCasting_DEPRECATED.Num() );

			for ( int32 i = 0; i < Resource->LODModels[LODIndex].Sections.Num(); ++i )
			{
				NewMaterialArray.Add( FSkeletalMaterial( Materials[ Resource->LODModels[LODIndex].Sections[i].MaterialIndex ].MaterialInterface, LODInfo[LODIndex].bEnableShadowCasting_DEPRECATED[i], false, NAME_None, NAME_None ) );
			}
		}

		// Reassign the materials array to the new one
		Materials = NewMaterialArray;
		int32 NewIndex = 0;

		// Remap the existing LODModels to point at the correct new material index
		for ( int32 LODIndex = 0; LODIndex < Resource->LODModels.Num(); ++LODIndex )
		{
			check( Resource->LODModels[LODIndex].Sections.Num() == LODInfo[LODIndex].bEnableShadowCasting_DEPRECATED.Num() );

			for ( int32 i = 0; i < Resource->LODModels[LODIndex].Sections.Num(); ++i )
			{
				Resource->LODModels[LODIndex].Sections[i].MaterialIndex = NewIndex;
				++NewIndex;
			}
		}
	}
}

void USkeletalMesh::MoveMaterialFlagsToSections()
{
	//No LOD we cant set the value
	if (LODInfo.Num() == 0)
	{
		return;
	}

	for (FSkeletalMeshLODModel &StaticLODModel : ImportedModel->LODModels)
	{
		for (int32 SectionIndex = 0; SectionIndex < StaticLODModel.Sections.Num(); ++SectionIndex)
		{
			FSkelMeshSection &Section = StaticLODModel.Sections[SectionIndex];
			//Prior to FEditorObjectVersion::RefactorMeshEditorMaterials Material index match section index
			if (Materials.IsValidIndex(SectionIndex))
			{
				Section.bCastShadow = Materials[SectionIndex].bEnableShadowCasting_DEPRECATED;

				Section.bRecomputeTangent = Materials[SectionIndex].bRecomputeTangent_DEPRECATED;
			}
			else
			{
				//Default cast shadow to true this is a fail safe code path it should not go here if the data
				//is valid
				Section.bCastShadow = true;
				//Recompute tangent is serialize prior to FEditorObjectVersion::RefactorMeshEditorMaterials
				// We just keep the serialize value
			}
		}
	}
}
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
FDelegateHandle USkeletalMesh::RegisterOnClothingChange(const FSimpleMulticastDelegate::FDelegate& InDelegate)
{
	return OnClothingChange.Add(InDelegate);
}

void USkeletalMesh::UnregisterOnClothingChange(const FDelegateHandle& InHandle)
{
	OnClothingChange.Remove(InHandle);
}
#endif

bool USkeletalMesh::AreAllFlagsIdentical( const TArray<bool>& BoolArray ) const
{
	if ( BoolArray.Num() == 0 )
	{
		return true;
	}

	for ( int32 i = 0; i < BoolArray.Num() - 1; ++i )
	{
		if ( BoolArray[i] != BoolArray[i + 1] )
		{
			return false;
		}
	}

	return true;
}

bool operator== ( const FSkeletalMaterial& LHS, const FSkeletalMaterial& RHS )
{
	return ( LHS.MaterialInterface == RHS.MaterialInterface );
}

bool operator== ( const FSkeletalMaterial& LHS, const UMaterialInterface& RHS )
{
	return ( LHS.MaterialInterface == &RHS );
}

bool operator== ( const UMaterialInterface& LHS, const FSkeletalMaterial& RHS )
{
	return ( RHS.MaterialInterface == &LHS );
}

FArchive& operator<<(FArchive& Ar, FMeshUVChannelInfo& ChannelData)
{
	Ar << ChannelData.bInitialized;
	Ar << ChannelData.bOverrideDensities;

	for (int32 CoordIndex = 0; CoordIndex < TEXSTREAM_MAX_NUM_UVCHANNELS; ++CoordIndex)
	{
		Ar << ChannelData.LocalUVDensities[CoordIndex];
	}

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FSkeletalMaterial& Elem)
{
	Ar.UsingCustomVersion(FEditorObjectVersion::GUID);

	Ar << Elem.MaterialInterface;

	//Use the automatic serialization instead of this custom operator
	if (Ar.CustomVer(FEditorObjectVersion::GUID) >= FEditorObjectVersion::RefactorMeshEditorMaterials)
	{
		Ar << Elem.MaterialSlotName;
#if WITH_EDITORONLY_DATA
		if (!Ar.IsCooking() || Ar.CookingTarget()->HasEditorOnlyData())
		{
			Ar << Elem.ImportedMaterialSlotName;
		}
#endif //#if WITH_EDITORONLY_DATA
	}
	else
	{
		if (Ar.UE4Ver() >= VER_UE4_MOVE_SKELETALMESH_SHADOWCASTING)
		{
			Ar << Elem.bEnableShadowCasting_DEPRECATED;
		}

		Ar.UsingCustomVersion(FRecomputeTangentCustomVersion::GUID);
		if (Ar.CustomVer(FRecomputeTangentCustomVersion::GUID) >= FRecomputeTangentCustomVersion::RuntimeRecomputeTangent)
		{
			Ar << Elem.bRecomputeTangent_DEPRECATED;
		}
	}
	
	if (!Ar.IsLoading() || Ar.CustomVer(FRenderingObjectVersion::GUID) >= FRenderingObjectVersion::TextureStreamingMeshUVChannelData)
	{
		Ar << Elem.UVChannelData;
	}

	return Ar;
}

TArray<USkeletalMeshSocket*> USkeletalMesh::GetActiveSocketList() const
{
	TArray<USkeletalMeshSocket*> ActiveSockets = Sockets;

	// Then the skeleton sockets that aren't in the mesh
	if (Skeleton)
	{
		for (auto SkeletonSocketIt = Skeleton->Sockets.CreateConstIterator(); SkeletonSocketIt; ++SkeletonSocketIt)
		{
			USkeletalMeshSocket* Socket = *(SkeletonSocketIt);

			if (!IsSocketOnMesh(Socket->SocketName))
			{
				ActiveSockets.Add(Socket);
			}
		}
	}
	return ActiveSockets;
}

bool USkeletalMesh::IsSocketOnMesh(const FName& InSocketName) const
{
	for(int32 SocketIdx=0; SocketIdx < Sockets.Num(); SocketIdx++)
	{
		USkeletalMeshSocket* Socket = Sockets[SocketIdx];

		if(Socket != NULL && Socket->SocketName == InSocketName)
		{
			return true;
		}
	}

	return false;
}

void USkeletalMesh::AllocateResourceForRendering()
{
	SkeletalMeshRenderData = MakeUnique<FSkeletalMeshRenderData>();
}

#if WITH_EDITOR

void USkeletalMesh::CacheDerivedData()
{
	AllocateResourceForRendering();
	SkeletalMeshRenderData->Cache(this);
	PostMeshCached.Broadcast(this);
}

int32 USkeletalMesh::ValidatePreviewAttachedObjects()
{
	int32 NumBrokenAssets = PreviewAttachedAssetContainer.ValidatePreviewAttachedObjects();

	if (NumBrokenAssets > 0)
	{
		MarkPackageDirty();
	}
	return NumBrokenAssets;
}

void USkeletalMesh::RemoveMeshSection(int32 InLodIndex, int32 InSectionIndex)
{
	// Need a mesh resource
	if(!ImportedModel.IsValid())
	{
		UE_LOG(LogSkeletalMesh, Warning, TEXT("Failed to remove skeletal mesh section, ImportedResource is invalid."));
		return;
	}

	// Need a valid LOD
	if(!ImportedModel->LODModels.IsValidIndex(InLodIndex))
	{
		UE_LOG(LogSkeletalMesh, Warning, TEXT("Failed to remove skeletal mesh section, LOD%d does not exist in the mesh"), InLodIndex);
		return;
	}

	FSkeletalMeshLODModel& LodModel = ImportedModel->LODModels[InLodIndex];

	// Need a valid section
	if(!LodModel.Sections.IsValidIndex(InSectionIndex))
	{
		UE_LOG(LogSkeletalMesh, Warning, TEXT("Failed to remove skeletal mesh section, Section %d does not exist in LOD%d."), InSectionIndex, InLodIndex);
		return;
	}

	FSkelMeshSection& SectionToRemove = LodModel.Sections[InSectionIndex];

	if(SectionToRemove.HasClothingData())
	{
		// Can't remove this, clothing currently relies on it
		UE_LOG(LogSkeletalMesh, Warning, TEXT("Failed to remove skeletal mesh section, clothing is currently bound to Lod%d Section %d, unbind clothing before removal."), InLodIndex, InSectionIndex);
		return;
	}

	// Valid to remove, dirty the mesh
	Modify();
	PreEditChange(nullptr);

	SectionToRemove.bDisabled = true;

	PostEditChange();
}

//#nv begin #Blast Ability to hide bones using a dynamic index buffer
void USkeletalMesh::RebuildIndexBufferRanges()
{
	IndexBufferRanges.Reset();
	const int32 BoneCount = RefSkeleton.GetNum();
	IndexBufferRanges.SetNum(BoneCount);

	FSkeletalMeshRenderData* Resource = GetResourceForRendering();
	if (!Resource)
	{
		return;
	}

	FScopedSlowTask Progress(float(Resource->LODRenderData.Num()), LOCTEXT("RebuildIndexBufferRangesProgress", "Rebuilding bone index buffer ranges"));

	TArray<uint32> TempBuffer;
	for (int32 I = 0; I < BoneCount; I++)
	{
		IndexBufferRanges[I].LODModels.SetNum(Resource->LODRenderData.Num());
	}

	TArray<int32> TempBoneIndicies;
	for (int32 I = 0; I < Resource->LODRenderData.Num(); I++)
	{
		Progress.EnterProgressFrame();
		const FSkeletalMeshLODRenderData& SrcModel = Resource->LODRenderData[I];
		if (SrcModel.MultiSizeIndexContainer.IsIndexBufferValid())
		{
			SrcModel.MultiSizeIndexContainer.GetIndexBuffer(TempBuffer);
		}

		const int32 SectionCount = SrcModel.RenderSections.Num();
		for (int32 B = 0; B < BoneCount; B++)
		{
			FSkeletalMeshIndexBufferRanges::FPerLODInfo& DestModel = IndexBufferRanges[B].LODModels[I];
			DestModel.Sections.SetNum(SectionCount);
		}

		const FSkeletalMeshLODModel& LODModel = GetImportedModel()->LODModels[I];
		for (int32 S = 0; S < SectionCount; S++)
		{
			const FSkelMeshSection& SkelSection = LODModel.Sections[S];
			for (uint32 TI = 0; TI < SkelSection.NumTriangles; TI++)
			{
				const int32 IndexIndex = SkelSection.BaseIndex + TI * 3;
				//The indices required for this triangle
				FInt32Range CurIndexRange(IndexIndex, IndexIndex + 3);

				TempBoneIndicies.Reset(3);
				for (int32 VI = 0; VI < 3; VI++)
				{
					const int32 VertexIndex = TempBuffer[IndexIndex + VI] - SkelSection.BaseVertexIndex;
					const FSoftSkinVertex& VertexData = SkelSection.SoftVertices[VertexIndex];
					for (int32 WeightIdx = 0; WeightIdx < MAX_TOTAL_INFLUENCES; WeightIdx++)
					{
						if (VertexData.InfluenceWeights[WeightIdx] > 0)
						{
							int32 BoneIdx = SkelSection.BoneMap[VertexData.InfluenceBones[WeightIdx]];
							if (BoneIdx != INDEX_NONE)
							{
								TempBoneIndicies.AddUnique(BoneIdx);
							}
						}
					}
				}

				for (int32 BoneIndex : TempBoneIndicies)
				{
					FSkeletalMeshIndexBufferRanges::FPerLODInfo& DestModel = IndexBufferRanges[BoneIndex].LODModels[I];
					FSkeletalMeshIndexBufferRanges::FPerSectionInfo& DestSection = DestModel.Sections[S];

					bool bJoined = false;
					for (FInt32Range& Existing : DestSection.Regions)
					{
						if (Existing.Contiguous(CurIndexRange))
						{
							Existing = FInt32Range::Hull(Existing, CurIndexRange);
							bJoined = true;
							break;
						}
					}
					if (!bJoined)
					{
						DestSection.Regions.Add(CurIndexRange);
					}
				}
			}
		}
	}
}
//nv end
#endif // #if WITH_EDITOR

void USkeletalMesh::ReleaseCPUResources()
{
	FSkeletalMeshRenderData* SkelMeshRenderData = GetResourceForRendering();
	if (SkelMeshRenderData)
	{
		for(int32 Index = 0; Index < SkelMeshRenderData->LODRenderData.Num(); ++Index)
		{
			if (!NeedCPUData(Index))
			{
				SkelMeshRenderData->LODRenderData[Index].ReleaseCPUResources();
			}
		}
	}
}

/** Allocate and initialise bone mirroring table for this skeletal mesh. Default is source = destination for each bone. */
void USkeletalMesh::InitBoneMirrorInfo()
{
	SkelMirrorTable.Empty(RefSkeleton.GetNum());
	SkelMirrorTable.AddZeroed(RefSkeleton.GetNum());

	// By default, no bone mirroring, and source is ourself.
	for(int32 i=0; i<SkelMirrorTable.Num(); i++)
	{
		SkelMirrorTable[i].SourceIndex = i;
	}
}

/** Utility for copying and converting a mirroring table from another SkeletalMesh. */
void USkeletalMesh::CopyMirrorTableFrom(USkeletalMesh* SrcMesh)
{
	// Do nothing if no mirror table in source mesh
	if(SrcMesh->SkelMirrorTable.Num() == 0)
	{
		return;
	}

	// First, allocate and default mirroring table.
	InitBoneMirrorInfo();

	// Keep track of which entries in the source we have already copied
	TArray<bool> EntryCopied;
	EntryCopied.AddZeroed( SrcMesh->SkelMirrorTable.Num() );

	// Mirror table must always be size of ref skeleton.
	check(SrcMesh->SkelMirrorTable.Num() == SrcMesh->RefSkeleton.GetNum());

	// Iterate over each entry in the source mesh mirror table.
	// We assume that the src table is correct, and don't check for errors here (ie two bones using the same one as source).
	for(int32 i=0; i<SrcMesh->SkelMirrorTable.Num(); i++)
	{
		if(!EntryCopied[i])
		{
			// Get name of source and dest bone for this entry in the source table.
			FName DestBoneName = SrcMesh->RefSkeleton.GetBoneName(i);
			int32 SrcBoneIndex = SrcMesh->SkelMirrorTable[i].SourceIndex;
			FName SrcBoneName = SrcMesh->RefSkeleton.GetBoneName(SrcBoneIndex);
			EAxis::Type FlipAxis = SrcMesh->SkelMirrorTable[i].BoneFlipAxis;

			// Look up bone names in target mesh (this one)
			int32 DestBoneIndexTarget = RefSkeleton.FindBoneIndex(DestBoneName);
			int32 SrcBoneIndexTarget = RefSkeleton.FindBoneIndex(SrcBoneName);

			// If both bones found, copy data to this mesh's mirror table.
			if( DestBoneIndexTarget != INDEX_NONE && SrcBoneIndexTarget != INDEX_NONE )
			{
				SkelMirrorTable[DestBoneIndexTarget].SourceIndex = SrcBoneIndexTarget;
				SkelMirrorTable[DestBoneIndexTarget].BoneFlipAxis = FlipAxis;


				SkelMirrorTable[SrcBoneIndexTarget].SourceIndex = DestBoneIndexTarget;
				SkelMirrorTable[SrcBoneIndexTarget].BoneFlipAxis = FlipAxis;

				// Flag entries as copied, so we don't try and do it again.
				EntryCopied[i] = true;
				EntryCopied[SrcBoneIndex] = true;
			}
		}
	}
}

/** Utility for copying and converting a mirroring table from another SkeletalMesh. */
void USkeletalMesh::ExportMirrorTable(TArray<FBoneMirrorExport> &MirrorExportInfo)
{
	// Do nothing if no mirror table in source mesh
	if( SkelMirrorTable.Num() == 0 )
	{
		return;
	}
	
	// Mirror table must always be size of ref skeleton.
	check(SkelMirrorTable.Num() == RefSkeleton.GetNum());

	MirrorExportInfo.Empty(SkelMirrorTable.Num());
	MirrorExportInfo.AddZeroed(SkelMirrorTable.Num());

	// Iterate over each entry in the source mesh mirror table.
	// We assume that the src table is correct, and don't check for errors here (ie two bones using the same one as source).
	for(int32 i=0; i<SkelMirrorTable.Num(); i++)
	{
		MirrorExportInfo[i].BoneName		= RefSkeleton.GetBoneName(i);
		MirrorExportInfo[i].SourceBoneName	= RefSkeleton.GetBoneName(SkelMirrorTable[i].SourceIndex);
		MirrorExportInfo[i].BoneFlipAxis	= SkelMirrorTable[i].BoneFlipAxis;
	}
}


/** Utility for copying and converting a mirroring table from another SkeletalMesh. */
void USkeletalMesh::ImportMirrorTable(TArray<FBoneMirrorExport> &MirrorExportInfo)
{
	// Do nothing if no mirror table in source mesh
	if( MirrorExportInfo.Num() == 0 )
	{
		return;
	}

	// First, allocate and default mirroring table.
	InitBoneMirrorInfo();

	// Keep track of which entries in the source we have already copied
	TArray<bool> EntryCopied;
	EntryCopied.AddZeroed( RefSkeleton.GetNum() );

	// Mirror table must always be size of ref skeleton.
	check(SkelMirrorTable.Num() == RefSkeleton.GetNum());

	// Iterate over each entry in the source mesh mirror table.
	// We assume that the src table is correct, and don't check for errors here (ie two bones using the same one as source).
	for(int32 i=0; i<MirrorExportInfo.Num(); i++)
	{
		FName DestBoneName	= MirrorExportInfo[i].BoneName;
		int32 DestBoneIndex	= RefSkeleton.FindBoneIndex(DestBoneName);

		if( DestBoneIndex != INDEX_NONE && !EntryCopied[DestBoneIndex] )
		{
			FName SrcBoneName	= MirrorExportInfo[i].SourceBoneName;
			int32 SrcBoneIndex	= RefSkeleton.FindBoneIndex(SrcBoneName);
			EAxis::Type FlipAxis		= MirrorExportInfo[i].BoneFlipAxis;

			// If both bones found, copy data to this mesh's mirror table.
			if( SrcBoneIndex != INDEX_NONE )
			{
				SkelMirrorTable[DestBoneIndex].SourceIndex = SrcBoneIndex;
				SkelMirrorTable[DestBoneIndex].BoneFlipAxis = FlipAxis;

				SkelMirrorTable[SrcBoneIndex].SourceIndex = DestBoneIndex;
				SkelMirrorTable[SrcBoneIndex].BoneFlipAxis = FlipAxis;

				// Flag entries as copied, so we don't try and do it again.
				EntryCopied[DestBoneIndex]	= true;
				EntryCopied[SrcBoneIndex]	= true;
			}
		}
	}
}

/** 
 *	Utility for checking that the bone mirroring table of this mesh is good.
 *	Return true if mirror table is OK, false if there are problems.
 *	@param	ProblemBones	Output string containing information on bones that are currently bad.
 */
bool USkeletalMesh::MirrorTableIsGood(FString& ProblemBones)
{
	TArray<int32>	BadBoneMirror;

	for(int32 i=0; i<SkelMirrorTable.Num(); i++)
	{
		int32 SrcIndex = SkelMirrorTable[i].SourceIndex;
		if( SkelMirrorTable[SrcIndex].SourceIndex != i)
		{
			BadBoneMirror.Add(i);
		}
	}

	if(BadBoneMirror.Num() > 0)
	{
		for(int32 i=0; i<BadBoneMirror.Num(); i++)
		{
			int32 BoneIndex = BadBoneMirror[i];
			FName BoneName = RefSkeleton.GetBoneName(BoneIndex);

			ProblemBones += FString::Printf( TEXT("%s (%d)\n"), *BoneName.ToString(), BoneIndex );
		}

		return false;
	}
	else
	{
		return true;
	}
}

void USkeletalMesh::CreateBodySetup()
{
	if (BodySetup == nullptr)
	{
		BodySetup = NewObject<UBodySetup>(this);
		BodySetup->bSharedCookedData = true;
	}
}

UBodySetup* USkeletalMesh::GetBodySetup()
{
	CreateBodySetup();
	return BodySetup;
}

#if WITH_EDITOR
void USkeletalMesh::BuildPhysicsData()
{
	CreateBodySetup();
	BodySetup->CookedFormatData.FlushData();	//we need to force a re-cook because we're essentially re-creating the bodysetup so that it swaps whether or not it has a trimesh
	BodySetup->InvalidatePhysicsData();
	BodySetup->CreatePhysicsMeshes();
}
#endif

bool USkeletalMesh::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	return bEnablePerPolyCollision;
}

bool USkeletalMesh::GetPhysicsTriMeshData(FTriMeshCollisionData* CollisionData, bool bInUseAllTriData)
{
#if WITH_EDITORONLY_DATA

	// Fail if no mesh or not per poly collision
	if (!GetResourceForRendering() || !bEnablePerPolyCollision)
	{
		return false;
	}

	FSkeletalMeshRenderData* SkelMeshRenderData = GetResourceForRendering();
	const FSkeletalMeshLODRenderData& LODData = SkelMeshRenderData->LODRenderData[0];

		// Copy all verts into collision vertex buffer.
		CollisionData->Vertices.Empty();
	CollisionData->Vertices.AddUninitialized(LODData.GetNumVertices());

	for (uint32 VertIdx = 0; VertIdx < LODData.GetNumVertices(); ++VertIdx)
	{
		CollisionData->Vertices[VertIdx] = LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertIdx);
	}

	{
		// Copy indices into collision index buffer
		const FMultiSizeIndexContainer& IndexBufferContainer = LODData.MultiSizeIndexContainer;

		TArray<uint32> Indices;
		IndexBufferContainer.GetIndexBuffer(Indices);

		const uint32 NumTris = Indices.Num() / 3;
		CollisionData->Indices.Empty();
		CollisionData->Indices.Reserve(NumTris);

		FTriIndices TriIndex;
		for (int32 SectionIndex = 0; SectionIndex < LODData.RenderSections.Num(); ++SectionIndex)
		{
			const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

			const uint32 OnePastLastIndex = Section.BaseIndex + Section.NumTriangles * 3;

			for (uint32 i = Section.BaseIndex; i < OnePastLastIndex; i += 3)
			{
				TriIndex.v0 = Indices[i];
				TriIndex.v1 = Indices[i + 1];
				TriIndex.v2 = Indices[i + 2];

				CollisionData->Indices.Add(TriIndex);
				CollisionData->MaterialIndices.Add(Section.MaterialIndex);
			}
		}
	}

	CollisionData->bFlipNormals = true;
	CollisionData->bDeformableMesh = true;

	// We only have a valid TriMesh if the CollisionData has vertices AND indices. For meshes with disabled section collision, it
	// can happen that the indices will be empty, in which case we do not want to consider that as valid trimesh data
	return CollisionData->Vertices.Num() > 0 && CollisionData->Indices.Num() > 0;
#else // #if WITH_EDITORONLY_DATA
	return false;
#endif // #if WITH_EDITORONLY_DATA
}

void USkeletalMesh::AddAssetUserData(UAssetUserData* InUserData)
{
	if (InUserData != NULL)
	{
		UAssetUserData* ExistingData = GetAssetUserDataOfClass(InUserData->GetClass());
		if (ExistingData != NULL)
		{
			AssetUserData.Remove(ExistingData);
		}
		AssetUserData.Add(InUserData);
	}
}

UAssetUserData* USkeletalMesh::GetAssetUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass)
{
	for (int32 DataIdx = 0; DataIdx < AssetUserData.Num(); DataIdx++)
	{
		UAssetUserData* Datum = AssetUserData[DataIdx];
		if (Datum != NULL && Datum->IsA(InUserDataClass))
		{
			return Datum;
		}
	}
	return NULL;
}

void USkeletalMesh::RemoveUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass)
{
	for (int32 DataIdx = 0; DataIdx < AssetUserData.Num(); DataIdx++)
	{
		UAssetUserData* Datum = AssetUserData[DataIdx];
		if (Datum != NULL && Datum->IsA(InUserDataClass))
		{
			AssetUserData.RemoveAt(DataIdx);
			return;
		}
	}
}

const TArray<UAssetUserData*>* USkeletalMesh::GetAssetUserDataArray() const
{
	return &AssetUserData;
}

////// SKELETAL MESH THUMBNAIL SUPPORT ////////

/** 
 * Returns a one line description of an object for viewing in the thumbnail view of the generic browser
 */
FString USkeletalMesh::GetDesc()
{
	FString DescString;

	FSkeletalMeshRenderData* Resource = GetResourceForRendering();
	if (Resource)
	{
		check(Resource->LODRenderData.Num() > 0);
		DescString = FString::Printf(TEXT("%d Triangles, %d Bones"), Resource->LODRenderData[0].GetTotalFaces(), RefSkeleton.GetRawBoneNum());
	}
	return DescString;
}

bool USkeletalMesh::IsSectionUsingCloth(int32 InSectionIndex, bool bCheckCorrespondingSections) const
{
	FSkeletalMeshRenderData* SkelMeshRenderData = GetResourceForRendering();
	if(SkelMeshRenderData)
	{
		for (FSkeletalMeshLODRenderData& LodData : SkelMeshRenderData->LODRenderData)
		{
			if(LodData.RenderSections.IsValidIndex(InSectionIndex))
			{
				FSkelMeshRenderSection* SectionToCheck = &LodData.RenderSections[InSectionIndex];
				return SectionToCheck->HasClothingData();
			}
		}
	}

	return false;
}

#if WITH_EDITOR
void USkeletalMesh::AddBoneToReductionSetting(int32 LODIndex, const TArray<FName>& BoneNames)
{
	if (LODInfo.IsValidIndex(LODIndex))
	{
		for (auto& BoneName : BoneNames)
		{
			LODInfo[LODIndex].BonesToRemove.AddUnique(BoneName);
		}
	}
}
void USkeletalMesh::AddBoneToReductionSetting(int32 LODIndex, FName BoneName)
{
	if (LODInfo.IsValidIndex(LODIndex))
	{
		LODInfo[LODIndex].BonesToRemove.AddUnique(BoneName);
	}
}
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
void USkeletalMesh::ConvertLegacyLODScreenSize()
{
	if (LODInfo.Num() == 1)
	{
		// Only one LOD
		LODInfo[0].ScreenSize = 1.0f;
	}
	else
	{
		// Use 1080p, 90 degree FOV as a default, as this should not cause runtime regressions in the common case.
		// LODs will appear different in Persona, however.
		const float HalfFOV = PI * 0.25f;
		const float ScreenWidth = 1920.0f;
		const float ScreenHeight = 1080.0f;
		const FPerspectiveMatrix ProjMatrix(HalfFOV, ScreenWidth, ScreenHeight, 1.0f);
		FBoxSphereBounds Bounds = GetBounds();

		// Multiple models, we should have LOD screen area data.
		for (int32 LODIndex = 0; LODIndex < LODInfo.Num(); ++LODIndex)
		{
			FSkeletalMeshLODInfo& LODInfoEntry = LODInfo[LODIndex];

			if (bRequiresLODScreenSizeConversion)
			{
				if (LODInfoEntry.ScreenSize == 0.0f)
				{
					LODInfoEntry.ScreenSize = 1.0f;
				}
				else
				{
					// legacy screen size was scaled by a fixed constant of 320.0f, so its kinda arbitrary. Convert back to distance based metric first.
					const float ScreenDepth = FMath::Max(ScreenWidth / 2.0f * ProjMatrix.M[0][0], ScreenHeight / 2.0f * ProjMatrix.M[1][1]) * Bounds.SphereRadius / (LODInfoEntry.ScreenSize * 320.0f);

					// Now convert using the query function
					LODInfoEntry.ScreenSize = ComputeBoundsScreenSize(FVector::ZeroVector, Bounds.SphereRadius, FVector(0.0f, 0.0f, ScreenDepth), ProjMatrix);
				}
			}

			if (bRequiresLODHysteresisConversion)
			{
				if (LODInfoEntry.LODHysteresis != 0.0f)
				{
					// Also convert the hysteresis as if it was a screen size topo
					const float ScreenHysteresisDepth = FMath::Max(ScreenWidth / 2.0f * ProjMatrix.M[0][0], ScreenHeight / 2.0f * ProjMatrix.M[1][1]) * Bounds.SphereRadius / (LODInfoEntry.LODHysteresis * 320.0f);
					LODInfoEntry.LODHysteresis = ComputeBoundsScreenSize(FVector::ZeroVector, Bounds.SphereRadius, FVector(0.0f, 0.0f, ScreenHysteresisDepth), ProjMatrix);
				}
			}
		}
	}
}
#endif

class UNodeMappingContainer* USkeletalMesh::GetNodeMappingContainer(class UBlueprint* SourceAsset) const
{
	for (int32 Index = 0; Index < NodeMappingData.Num(); ++Index)
	{
		UNodeMappingContainer* Iter = NodeMappingData[Index];
		if (Iter && Iter->GetSourceAsset() == SourceAsset)
		{
			return Iter;
		}
	}

	return nullptr;
}
/*-----------------------------------------------------------------------------
USkeletalMeshSocket
-----------------------------------------------------------------------------*/
USkeletalMeshSocket::USkeletalMeshSocket(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bForceAlwaysAnimated(true)
{
	RelativeScale = FVector(1.0f, 1.0f, 1.0f);
}

void USkeletalMeshSocket::InitializeSocketFromLocation(const class USkeletalMeshComponent* SkelComp, FVector WorldLocation, FVector WorldNormal)
{
	if (ensureAsRuntimeWarning(SkelComp))
	{
		BoneName = SkelComp->FindClosestBone(WorldLocation);
		if (BoneName != NAME_None)
		{
			SkelComp->TransformToBoneSpace(BoneName, WorldLocation, WorldNormal.Rotation(), RelativeLocation, RelativeRotation);
		}
	}
}

FVector USkeletalMeshSocket::GetSocketLocation(const class USkeletalMeshComponent* SkelComp) const
{
	if (ensureAsRuntimeWarning(SkelComp))
	{
		FMatrix SocketMatrix;
		if (GetSocketMatrix(SocketMatrix, SkelComp))
		{
			return SocketMatrix.GetOrigin();
		}

		// Fall back to MeshComp origin, so it's visible in case of failure.
		return SkelComp->GetComponentLocation();
	}
	return FVector(0.f);
}

bool USkeletalMeshSocket::GetSocketMatrix(FMatrix& OutMatrix, const class USkeletalMeshComponent* SkelComp) const
{
	const int32 BoneIndex = SkelComp ? SkelComp->GetBoneIndex(BoneName) : INDEX_NONE;
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneMatrix = SkelComp->GetBoneMatrix(BoneIndex);
		FScaleRotationTranslationMatrix RelSocketMatrix( RelativeScale, RelativeRotation, RelativeLocation );
		OutMatrix = RelSocketMatrix * BoneMatrix;
		return true;
	}

	return false;
}

FTransform USkeletalMeshSocket::GetSocketLocalTransform() const
{
	return FTransform(RelativeRotation, RelativeLocation, RelativeScale);
}

FTransform USkeletalMeshSocket::GetSocketTransform(const class USkeletalMeshComponent* SkelComp) const
{
	FTransform OutTM;

	const int32 BoneIndex = SkelComp ? SkelComp->GetBoneIndex(BoneName) : INDEX_NONE;
	if(BoneIndex != INDEX_NONE)
	{
		FTransform BoneTM = SkelComp->GetBoneTransform(BoneIndex);
		FTransform RelSocketTM( RelativeRotation, RelativeLocation, RelativeScale );
		OutTM = RelSocketTM * BoneTM;
	}

	return OutTM;
}

bool USkeletalMeshSocket::GetSocketMatrixWithOffset(FMatrix& OutMatrix, class USkeletalMeshComponent* SkelComp, const FVector& InOffset, const FRotator& InRotation) const
{
	const int32 BoneIndex = SkelComp ? SkelComp->GetBoneIndex(BoneName) : INDEX_NONE;
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneMatrix = SkelComp->GetBoneMatrix(BoneIndex);
		FScaleRotationTranslationMatrix RelSocketMatrix(RelativeScale, RelativeRotation, RelativeLocation);
		FRotationTranslationMatrix RelOffsetMatrix(InRotation, InOffset);
		OutMatrix = RelOffsetMatrix * RelSocketMatrix * BoneMatrix;
		return true;
	}

	return false;
}


bool USkeletalMeshSocket::GetSocketPositionWithOffset(FVector& OutPosition, class USkeletalMeshComponent* SkelComp, const FVector& InOffset, const FRotator& InRotation) const
{
	const int32 BoneIndex = SkelComp ? SkelComp->GetBoneIndex(BoneName) : INDEX_NONE;
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneMatrix = SkelComp->GetBoneMatrix(BoneIndex);
		FScaleRotationTranslationMatrix RelSocketMatrix(RelativeScale, RelativeRotation, RelativeLocation);
		FRotationTranslationMatrix RelOffsetMatrix(InRotation, InOffset);
		FMatrix SocketMatrix = RelOffsetMatrix * RelSocketMatrix * BoneMatrix;
		OutPosition = SocketMatrix.GetOrigin();
		return true;
	}

	return false;
}

/** 
 *	Utility to associate an actor with a socket
 *	
 *	@param	Actor			The actor to attach to the socket
 *	@param	SkelComp		The skeletal mesh component that the socket comes from
 *
 *	@return	bool			true if successful, false if not
 */
bool USkeletalMeshSocket::AttachActor(AActor* Actor, class USkeletalMeshComponent* SkelComp) const
{
	bool bAttached = false;
	if (ensureAlways(SkelComp))
	{
		// Don't support attaching to own socket
		if ((Actor != SkelComp->GetOwner()) && Actor->GetRootComponent())
		{
			FMatrix SocketTM;
			if (GetSocketMatrix(SocketTM, SkelComp))
			{
				Actor->Modify();

				Actor->SetActorLocation(SocketTM.GetOrigin(), false);
				Actor->SetActorRotation(SocketTM.Rotator());
				Actor->GetRootComponent()->AttachToComponent(SkelComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);

	#if WITH_EDITOR
				if (GIsEditor)
				{
					Actor->PreEditChange(NULL);
					Actor->PostEditChange();
				}
	#endif // WITH_EDITOR

				bAttached = true;
			}
		}
	}
	return bAttached;
}

#if WITH_EDITOR
void USkeletalMeshSocket::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		ChangedEvent.Broadcast(this, PropertyChangedEvent.MemberProperty);
	}
}

void USkeletalMeshSocket::CopyFrom(const class USkeletalMeshSocket* OtherSocket)
{
	if (OtherSocket)
	{
		SocketName = OtherSocket->SocketName;
		BoneName = OtherSocket->BoneName;
		RelativeLocation = OtherSocket->RelativeLocation;
		RelativeRotation = OtherSocket->RelativeRotation;
		RelativeScale = OtherSocket->RelativeScale;
		bForceAlwaysAnimated = OtherSocket->bForceAlwaysAnimated;
	}
}

#endif

void USkeletalMeshSocket::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FFrameworkObjectVersion::GUID);

	if(Ar.CustomVer(FFrameworkObjectVersion::GUID) < FFrameworkObjectVersion::MeshSocketScaleUtilization)
	{
		// Set the relative scale to 1.0. As it was not used before this should allow existing data
		// to work as expected.
		RelativeScale = FVector(1.0f, 1.0f, 1.0f);
	}
}


/*-----------------------------------------------------------------------------
FSkeletalMeshSceneProxy
-----------------------------------------------------------------------------*/
#include "Engine/LevelStreaming.h"
#include "LevelUtils.h"

const FQuat SphylBasis(FVector(1.0f / FMath::Sqrt(2.0f), 0.0f, 1.0f / FMath::Sqrt(2.0f)), PI);

/** 
 * Constructor. 
 * @param	Component - skeletal mesh primitive being added
 */
FSkeletalMeshSceneProxy::FSkeletalMeshSceneProxy(const USkinnedMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData)
		:	FPrimitiveSceneProxy(Component, Component->SkeletalMesh->GetFName())
		,	Owner(Component->GetOwner())
		,	MeshObject(Component->MeshObject)
		,	SkeletalMeshRenderData(InSkelMeshRenderData)
		,	SkeletalMeshForDebug(Component->SkeletalMesh)
		,	PhysicsAssetForDebug(Component->GetPhysicsAsset())
		,	bForceWireframe(Component->bForceWireframe)
		,	bCanHighlightSelectedSections(Component->bCanHighlightSelectedSections)
		,	MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		,	FeatureLevel(GetScene().GetFeatureLevel())
		,	bMaterialsNeedMorphUsage_GameThread(false)
#if WITH_EDITORONLY_DATA
		,	StreamingDistanceMultiplier(FMath::Max(0.0f, Component->StreamingDistanceMultiplier))
#endif
{
	check(MeshObject);
	check(SkeletalMeshRenderData);
	check(SkeletalMeshForDebug);

	bIsCPUSkinned = MeshObject->IsCPUSkinned();

	bCastCapsuleDirectShadow = Component->bCastDynamicShadow && Component->CastShadow && Component->bCastCapsuleDirectShadow;
	bCastsDynamicIndirectShadow = Component->bCastDynamicShadow && Component->CastShadow && Component->bCastCapsuleIndirectShadow;

	DynamicIndirectShadowMinVisibility = FMath::Clamp(Component->CapsuleIndirectShadowMinVisibility, 0.0f, 1.0f);

	// Force inset shadows if capsule shadows are requested, as they can't be supported with full scene shadows
	bCastInsetShadow = bCastInsetShadow || bCastCapsuleDirectShadow;

	const USkeletalMeshComponent* SkeletalMeshComponent = Cast<const USkeletalMeshComponent>(Component);
	if(SkeletalMeshComponent && SkeletalMeshComponent->bPerBoneMotionBlur)
	{
		bAlwaysHasVelocity = true;
	}

	// setup materials and performance classification for each LOD.
	extern bool GForceDefaultMaterial;
	bool bCastShadow = Component->CastShadow;
	bool bAnySectionCastsShadow = false;
	LODSections.Reserve(SkeletalMeshRenderData->LODRenderData.Num());
	LODSections.AddZeroed(SkeletalMeshRenderData->LODRenderData.Num());
	for(int32 LODIdx=0; LODIdx < SkeletalMeshRenderData->LODRenderData.Num(); LODIdx++)
	{
		const FSkeletalMeshLODRenderData& LODData = SkeletalMeshRenderData->LODRenderData[LODIdx];
		const FSkeletalMeshLODInfo& Info = Component->SkeletalMesh->LODInfo[LODIdx];

		FLODSectionElements& LODSection = LODSections[LODIdx];

		// Presize the array
		LODSection.SectionElements.Empty(LODData.RenderSections.Num() );
		for(int32 SectionIndex = 0;SectionIndex < LODData.RenderSections.Num();SectionIndex++)
		{
			const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

			// If we are at a dropped LOD, route material index through the LODMaterialMap in the LODInfo struct.
			int32 UseMaterialIndex = Section.MaterialIndex;			
			if(LODIdx > 0)
			{
				if(Section.MaterialIndex < Info.LODMaterialMap.Num())
				{
					UseMaterialIndex = Info.LODMaterialMap[Section.MaterialIndex];
					UseMaterialIndex = FMath::Clamp( UseMaterialIndex, 0, Component->SkeletalMesh->Materials.Num() );
				}
			}

			// If Section is hidden, do not cast shadow
			bool bSectionHidden = MeshObject->IsMaterialHidden(LODIdx,UseMaterialIndex);

			// If the material is NULL, or isn't flagged for use with skeletal meshes, it will be replaced by the default material.
			UMaterialInterface* Material = Component->GetMaterial(UseMaterialIndex);
			if (GForceDefaultMaterial && Material && !IsTranslucentBlendMode(Material->GetBlendMode()))
			{
				Material = UMaterial::GetDefaultMaterial(MD_Surface);
				MaterialRelevance |= Material->GetRelevance(FeatureLevel);
			}

			// if this is a clothing section, then enabled and will be drawn but the corresponding original section should be disabled
			bool bClothSection = Section.HasClothingData();

			if(!Material || !Material->CheckMaterialUsage_Concurrent(MATUSAGE_SkeletalMesh) ||
			  (bClothSection && !Material->CheckMaterialUsage_Concurrent(MATUSAGE_Clothing)))
			{
				Material = UMaterial::GetDefaultMaterial(MD_Surface);
				MaterialRelevance |= Material->GetRelevance(FeatureLevel);
			}

			const bool bRequiresAdjacencyInformation = RequiresAdjacencyInformation( Material, &TGPUSkinVertexFactory<false>::StaticType, FeatureLevel );
			if ( bRequiresAdjacencyInformation && LODData.AdjacencyMultiSizeIndexContainer.IsIndexBufferValid() == false )
			{
				UE_LOG(LogSkeletalMesh, Warning, 
					TEXT("Material %s requires adjacency information, but skeletal mesh %s does not have adjacency information built. The mesh must be rebuilt to be used with this material. The mesh will be rendered with DefaultMaterial."), 
					*Material->GetPathName(), 
					*Component->SkeletalMesh->GetPathName() )
				Material = UMaterial::GetDefaultMaterial(MD_Surface);
				MaterialRelevance |= UMaterial::GetDefaultMaterial(MD_Surface)->GetRelevance(FeatureLevel);
			}

			bool bSectionCastsShadow = !bSectionHidden && bCastShadow &&
				(Component->SkeletalMesh->Materials.IsValidIndex(UseMaterialIndex) == false || Section.bCastShadow);

			bAnySectionCastsShadow |= bSectionCastsShadow;
			LODSection.SectionElements.Add(
				FSectionElementInfo(
					Material,
					bSectionCastsShadow,
					UseMaterialIndex
					));
			MaterialsInUse_GameThread.Add(Material);
		}
	}

	bCastDynamicShadow = bCastDynamicShadow && bAnySectionCastsShadow;

	// Try to find a color for level coloration.
	if( Owner )
	{
		ULevel* Level = Owner->GetLevel();
		ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel( Level );
		if ( LevelStreaming )
		{
			LevelColor = LevelStreaming->LevelColor;
		}
	}

	// Get a color for property coloration
	FColor NewPropertyColor;
	GEngine->GetPropertyColorationColor( (UObject*)Component, NewPropertyColor );
	PropertyColor = NewPropertyColor;

	// Copy out shadow physics asset data
	if(SkeletalMeshComponent)
	{
		UPhysicsAsset* ShadowPhysicsAsset = SkeletalMeshComponent->SkeletalMesh->ShadowPhysicsAsset;

		if (ShadowPhysicsAsset
			&& SkeletalMeshComponent->CastShadow
			&& (SkeletalMeshComponent->bCastCapsuleDirectShadow || SkeletalMeshComponent->bCastCapsuleIndirectShadow))
		{
			for (int32 BodyIndex = 0; BodyIndex < ShadowPhysicsAsset->SkeletalBodySetups.Num(); BodyIndex++)
			{
				UBodySetup* BodySetup = ShadowPhysicsAsset->SkeletalBodySetups[BodyIndex];
				int32 BoneIndex = SkeletalMeshComponent->GetBoneIndex(BodySetup->BoneName);

				if (BoneIndex != INDEX_NONE)
				{
					const FMatrix& RefBoneMatrix = SkeletalMeshComponent->SkeletalMesh->GetComposedRefPoseMatrix(BoneIndex);

					const int32 NumSpheres = BodySetup->AggGeom.SphereElems.Num();
					for (int32 SphereIndex = 0; SphereIndex < NumSpheres; SphereIndex++)
					{
						const FKSphereElem& SphereShape = BodySetup->AggGeom.SphereElems[SphereIndex];
						ShadowCapsuleData.Emplace(BoneIndex, FCapsuleShape(RefBoneMatrix.TransformPosition(SphereShape.Center), SphereShape.Radius, FVector(0.0f, 0.0f, 1.0f), 0.0f));
					}

					const int32 NumCapsules = BodySetup->AggGeom.SphylElems.Num();
					for (int32 CapsuleIndex = 0; CapsuleIndex < NumCapsules; CapsuleIndex++)
					{
						const FKSphylElem& SphylShape = BodySetup->AggGeom.SphylElems[CapsuleIndex];
						ShadowCapsuleData.Emplace(BoneIndex, FCapsuleShape(RefBoneMatrix.TransformPosition(SphylShape.Center), SphylShape.Radius, RefBoneMatrix.TransformVector((SphylShape.Rotation.Quaternion() * SphylBasis).Vector()), SphylShape.Length));
					}

					if (NumSpheres > 0 || NumCapsules > 0)
					{
						ShadowCapsuleBoneIndices.AddUnique(BoneIndex);
					}
				}
			}
		}
	}

	// Sort to allow merging with other bone hierarchies
	if (ShadowCapsuleBoneIndices.Num())
	{
		ShadowCapsuleBoneIndices.Sort();
	}
}


// FPrimitiveSceneProxy interface.

/** 
 * Iterates over sections,chunks,elements based on current instance weight usage 
 */
class FSkeletalMeshSectionIter
{
public:
	FSkeletalMeshSectionIter(const int32 InLODIdx, const FSkeletalMeshObject& InMeshObject, const FSkeletalMeshLODRenderData& InLODData, const FSkeletalMeshSceneProxy::FLODSectionElements& InLODSectionElements)
		: SectionIndex(0)
		, MeshObject(InMeshObject)
		, LODSectionElements(InLODSectionElements)
		, Sections(InLODData.RenderSections)
#if WITH_EDITORONLY_DATA
		, SectionIndexPreview(InMeshObject.SectionIndexPreview)
		, MaterialIndexPreview(InMeshObject.MaterialIndexPreview)
#endif
	{
		while (NotValidPreviewSection())
		{
			SectionIndex++;
		}
	}
	FORCEINLINE FSkeletalMeshSectionIter& operator++()
	{
		do 
		{
		SectionIndex++;
		} while (NotValidPreviewSection());
		return *this;
	}
	FORCEINLINE operator bool() const
	{
		return ((SectionIndex < Sections.Num()) && LODSectionElements.SectionElements.IsValidIndex(GetSectionElementIndex()));
	}
	FORCEINLINE const FSkelMeshRenderSection& GetSection() const
	{
		return Sections[SectionIndex];
	}
	FORCEINLINE const int32 GetSectionElementIndex() const
	{
		return SectionIndex;
	}
	FORCEINLINE const FSkeletalMeshSceneProxy::FSectionElementInfo& GetSectionElementInfo() const
	{
		int32 SectionElementInfoIndex = GetSectionElementIndex();
		return LODSectionElements.SectionElements[SectionElementInfoIndex];
	}
	FORCEINLINE bool NotValidPreviewSection()
	{
#if WITH_EDITORONLY_DATA
		if (MaterialIndexPreview == INDEX_NONE)
		{
			int32 ActualPreviewSectionIdx = SectionIndexPreview;

			return	(SectionIndex < Sections.Num()) &&
				((ActualPreviewSectionIdx >= 0) && (ActualPreviewSectionIdx != SectionIndex));
		}
		else
		{
			int32 ActualPreviewMaterialIdx = MaterialIndexPreview;
			int32 ActualPreviewSectionIdx = INDEX_NONE;
			if (ActualPreviewMaterialIdx != INDEX_NONE && Sections.IsValidIndex(SectionIndex))
			{
				const FSkeletalMeshSceneProxy::FSectionElementInfo& SectionInfo = LODSectionElements.SectionElements[SectionIndex];
				if (SectionInfo.UseMaterialIndex == ActualPreviewMaterialIdx)
				{
					ActualPreviewSectionIdx = SectionIndex;
				}
			}

			return	(SectionIndex < Sections.Num()) &&
				((ActualPreviewMaterialIdx >= 0) && (ActualPreviewSectionIdx != SectionIndex));
		}
#else
		return false;
#endif
	}
private:
	int32 SectionIndex;
	const FSkeletalMeshObject& MeshObject;
	const FSkeletalMeshSceneProxy::FLODSectionElements& LODSectionElements;
	const TArray<FSkelMeshRenderSection>& Sections;
#if WITH_EDITORONLY_DATA
	const int32 SectionIndexPreview;
	const int32 MaterialIndexPreview;
#endif
};

#if WITH_EDITOR
HHitProxy* FSkeletalMeshSceneProxy::CreateHitProxies(UPrimitiveComponent* Component, TArray<TRefCountPtr<HHitProxy> >& OutHitProxies)
{
	if ( Component->GetOwner() )
	{
		if ( LODSections.Num() > 0 )
		{
			for ( int32 LODIndex = 0; LODIndex < SkeletalMeshRenderData->LODRenderData.Num(); LODIndex++ )
			{
				const FSkeletalMeshLODRenderData& LODData = SkeletalMeshRenderData->LODRenderData[LODIndex];

				FLODSectionElements& LODSection = LODSections[LODIndex];

				check(LODSection.SectionElements.Num() == LODData.RenderSections.Num());

				for ( int32 SectionIndex = 0; SectionIndex < LODData.RenderSections.Num(); SectionIndex++ )
				{
					HHitProxy* ActorHitProxy;

					int32 MaterialIndex = LODData.RenderSections[SectionIndex].MaterialIndex;
					if ( Component->GetOwner()->IsA(ABrush::StaticClass()) && Component->IsA(UBrushComponent::StaticClass()) )
					{
						ActorHitProxy = new HActor(Component->GetOwner(), Component, HPP_Wireframe, SectionIndex, MaterialIndex);
					}
					else
					{
						ActorHitProxy = new HActor(Component->GetOwner(), Component, SectionIndex, MaterialIndex);
					}

					// Set the hitproxy.
					check(LODSection.SectionElements[SectionIndex].HitProxy == NULL);
					LODSection.SectionElements[SectionIndex].HitProxy = ActorHitProxy;
					OutHitProxies.Add(ActorHitProxy);
				}
			}
		}
		else
		{
			return FPrimitiveSceneProxy::CreateHitProxies(Component, OutHitProxies);
		}
	}

	return NULL;
}
#endif

void FSkeletalMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FSkeletalMeshSceneProxy_GetMeshElements);
	GetMeshElementsConditionallySelectable(Views, ViewFamily, true, VisibilityMap, Collector);
}

void FSkeletalMeshSceneProxy::GetMeshElementsConditionallySelectable(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, bool bInSelectable, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	if( !MeshObject )
	{
		return;
	}	
	MeshObject->PreGDMECallback(ViewFamily.Scene->GetGPUSkinCache(), ViewFamily.FrameNumber);

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];
			MeshObject->UpdateMinDesiredLODLevel(View, GetBounds(), ViewFamily.FrameNumber);
		}
	}

	const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;

	const int32 LODIndex = MeshObject->GetLOD();
	check(LODIndex < SkeletalMeshRenderData->LODRenderData.Num());
	const FSkeletalMeshLODRenderData& LODData = SkeletalMeshRenderData->LODRenderData[LODIndex];

	if( LODSections.Num() > 0 )
	{
		const FLODSectionElements& LODSection = LODSections[LODIndex];

		check(LODSection.SectionElements.Num() == LODData.RenderSections.Num());

		for (FSkeletalMeshSectionIter Iter(LODIndex, *MeshObject, LODData, LODSection); Iter; ++Iter)
		{
			const FSkelMeshRenderSection& Section = Iter.GetSection();
			const int32 SectionIndex = Iter.GetSectionElementIndex();
			const FSectionElementInfo& SectionElementInfo = Iter.GetSectionElementInfo();

			bool bSectionSelected = false;

#if WITH_EDITORONLY_DATA
			// TODO: This is not threadsafe! A render command should be used to propagate SelectedEditorSection to the scene proxy.
			if (MeshObject->SelectedEditorMaterial != INDEX_NONE)
			{
				bSectionSelected = (MeshObject->SelectedEditorMaterial == SectionElementInfo.UseMaterialIndex);
			}
			else
			{
				bSectionSelected = (MeshObject->SelectedEditorSection == SectionIndex);
			}
			
#endif
			// If hidden skip the draw
			if (MeshObject->IsMaterialHidden(LODIndex, SectionElementInfo.UseMaterialIndex) || Section.bDisabled)
			{
				continue;
			}

			GetDynamicElementsSection(Views, ViewFamily, VisibilityMap, LODData, LODIndex, SectionIndex, bSectionSelected, SectionElementInfo, bInSelectable, Collector);
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			if( PhysicsAssetForDebug )
			{
				DebugDrawPhysicsAsset(ViewIndex, Collector, ViewFamily.EngineShowFlags);
			}

			if (EngineShowFlags.MassProperties && DebugMassData.Num() > 0)
			{
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
				const TArray<FTransform>& ComponentSpaceTransforms = *MeshObject->GetComponentSpaceTransforms();

				for (const FDebugMassData& DebugMass : DebugMassData)
				{
					if(ComponentSpaceTransforms.IsValidIndex(DebugMass.BoneIndex))
					{			
						const FTransform BoneToWorld = ComponentSpaceTransforms[DebugMass.BoneIndex] * FTransform(GetLocalToWorld());
						DebugMass.DrawDebugMass(PDI, BoneToWorld);
					}
				}
			}

			if (ViewFamily.EngineShowFlags.SkeletalMeshes)
			{
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}

			if (ViewFamily.EngineShowFlags.Bones)
			{
				DebugDrawSkeleton(ViewIndex, Collector, ViewFamily.EngineShowFlags);
			}
		}
	}
#endif
}

void FSkeletalMeshSceneProxy::GetDynamicElementsSection(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, 
	const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, bool bSectionSelected,
	const FSectionElementInfo& SectionElementInfo, bool bInSelectable, FMeshElementCollector& Collector ) const
{
	const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

	//// If hidden skip the draw
	//if (Section.bDisabled || MeshObject->IsMaterialHidden(LODIndex,SectionElementInfo.UseMaterialIndex))
	//{
	//	return;
	//}

#if !WITH_EDITOR
	const bool bIsSelected = false;
#else // #if !WITH_EDITOR
	bool bIsSelected = IsSelected();

	// if the mesh isn't selected but the mesh section is selected in the AnimSetViewer, find the mesh component and make sure that it can be highlighted (ie. are we rendering for the AnimSetViewer or not?)
	if( !bIsSelected && bSectionSelected && bCanHighlightSelectedSections )
	{
		bIsSelected = true;
	}
#endif // #if WITH_EDITOR

	const bool bIsWireframe = ViewFamily.EngineShowFlags.Wireframe;

//#nv begin #Blast Ability to hide bones using a dynamic index buffer
	FSkeletalMeshDynamicOverride* DynamicOverride = MeshObject->GetSkeletalMeshDynamicOverride();
	FDynamicLODModelOverride* LODModelDynamicOverride = nullptr;
	FSkelMeshSectionOverride* SectionDynamicOverride = nullptr;
	if (DynamicOverride)
	{
		LODModelDynamicOverride = &DynamicOverride->LODModels[LODIndex];
		SectionDynamicOverride = &LODModelDynamicOverride->Sections[SectionIndex];
	}
//nv end

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];

			FMeshBatch& Mesh = Collector.AllocateMesh();
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			Mesh.LCI = NULL;
			Mesh.bWireframe |= bForceWireframe;
			Mesh.Type = PT_TriangleList;
			Mesh.VertexFactory = MeshObject->GetSkinVertexFactory(View, LODIndex, SectionIndex);
			
			if(!Mesh.VertexFactory)
			{
				// hide this part
				continue;
			}

			Mesh.bSelectable = bInSelectable;

//#nv begin #Blast Ability to hide bones using a dynamic index buffer
			int32 SectionNumTriangles;
			if (SectionDynamicOverride) //If one is valid both are valid, no need to check both
			{
				BatchElement.FirstIndex = SectionDynamicOverride->BaseIndex;
				BatchElement.IndexBuffer = LODModelDynamicOverride->MultiSizeIndexContainer.GetIndexBuffer();
				SectionNumTriangles = SectionDynamicOverride->NumTriangles;
				if (SectionNumTriangles == 0)
				{
					continue;
				}
			}
			else
			{
				BatchElement.FirstIndex = Section.BaseIndex;
				BatchElement.IndexBuffer = LODData.MultiSizeIndexContainer.GetIndexBuffer();
				SectionNumTriangles = Section.NumTriangles;
			}
//nv end

			BatchElement.MaxVertexIndex = LODData.GetNumVertices() - 1;
			BatchElement.VertexFactoryUserData = FGPUSkinCache::GetFactoryUserData(MeshObject->SkinCacheEntry, SectionIndex);

			const bool bRequiresAdjacencyInformation = RequiresAdjacencyInformation( SectionElementInfo.Material, Mesh.VertexFactory->GetType(), ViewFamily.GetFeatureLevel() );
			if ( bRequiresAdjacencyInformation )
			{
//#nv begin #Blast Ability to hide bones using a dynamic index buffer
				if (LODModelDynamicOverride)
				{
					check(LODModelDynamicOverride->AdjacencyMultiSizeIndexContainer.IsIndexBufferValid());
					BatchElement.IndexBuffer = LODModelDynamicOverride->AdjacencyMultiSizeIndexContainer.GetIndexBuffer();
				}
				else
				{
					check(LODData.AdjacencyMultiSizeIndexContainer.IsIndexBufferValid());
					BatchElement.IndexBuffer = LODData.AdjacencyMultiSizeIndexContainer.GetIndexBuffer();
				}
//nv end
				Mesh.Type = PT_12_ControlPointPatchList;
				BatchElement.FirstIndex *= 4;
			}

			Mesh.MaterialRenderProxy = SectionElementInfo.Material->GetRenderProxy(false, IsHovered());
		#if WITH_EDITOR
			Mesh.BatchHitProxyId = SectionElementInfo.HitProxy ? SectionElementInfo.HitProxy->Id : FHitProxyId();

			if (bSectionSelected && bCanHighlightSelectedSections)
			{
				Mesh.bUseSelectionOutline = true;
			}
			else
			{
				Mesh.bUseSelectionOutline = !bCanHighlightSelectedSections && bIsSelected;
			}
		#endif

			BatchElement.PrimitiveUniformBufferResource = &GetUniformBuffer();

			BatchElement.NumPrimitives = SectionNumTriangles;	//#nv #Blast Ability to hide bones using a dynamic index buffer

#if WITH_EDITORONLY_DATA
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bIsSelected)
			{
				if (ViewFamily.EngineShowFlags.VertexColors && AllowDebugViewmodes())
				{
					// Override the mesh's material with our material that draws the vertex colors
					UMaterial* VertexColorVisualizationMaterial = NULL;
					switch (GVertexColorViewMode)
					{
					case EVertexColorViewMode::Color:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_ColorOnly;
						break;

					case EVertexColorViewMode::Alpha:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_AlphaAsColor;
						break;

					case EVertexColorViewMode::Red:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_RedOnly;
						break;

					case EVertexColorViewMode::Green:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_GreenOnly;
						break;

					case EVertexColorViewMode::Blue:
						VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_BlueOnly;
						break;
					}
					check(VertexColorVisualizationMaterial != NULL);

					auto VertexColorVisualizationMaterialInstance = new FColoredMaterialRenderProxy(
						VertexColorVisualizationMaterial->GetRenderProxy(Mesh.MaterialRenderProxy->IsSelected(), Mesh.MaterialRenderProxy->IsHovered()),
						GetSelectionColor(FLinearColor::White, bSectionSelected, IsHovered())
					);

					Collector.RegisterOneFrameMaterialProxy(VertexColorVisualizationMaterialInstance);
					Mesh.MaterialRenderProxy = VertexColorVisualizationMaterialInstance;
				}
			}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif // WITH_EDITORONLY_DATA

			BatchElement.MinVertexIndex = Section.BaseVertexIndex;
			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
			Mesh.CastShadow = SectionElementInfo.bEnableShadowCasting;

			Mesh.bCanApplyViewModeOverrides = true;
			Mesh.bUseWireframeSelectionColoring = bIsSelected;

		#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			BatchElement.VisualizeElementIndex = SectionIndex;
			Mesh.VisualizeLODIndex = LODIndex;
		#endif

			if ( ensureMsgf(Mesh.MaterialRenderProxy, TEXT("GetDynamicElementsSection with invalid MaterialRenderProxy. Owner:%s LODIndex:%d UseMaterialIndex:%d"), *GetOwnerName().ToString(), LODIndex, SectionElementInfo.UseMaterialIndex) &&
				 ensureMsgf(Mesh.MaterialRenderProxy->GetMaterial(FeatureLevel), TEXT("GetDynamicElementsSection with invalid FMaterial. Owner:%s LODIndex:%d UseMaterialIndex:%d"), *GetOwnerName().ToString(), LODIndex, SectionElementInfo.UseMaterialIndex) )
			{
				Collector.AddMesh(ViewIndex, Mesh);
			}

			const int32 NumVertices = Section.GetNumVertices();
			INC_DWORD_STAT_BY(STAT_GPUSkinVertices,(uint32)(bIsCPUSkinned ? 0 : NumVertices));
			INC_DWORD_STAT_BY(STAT_SkelMeshTriangles,Mesh.GetNumPrimitives());
			INC_DWORD_STAT(STAT_SkelMeshDrawCalls);
		}
	}
}

SIZE_T FSkeletalMeshSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

bool FSkeletalMeshSceneProxy::HasDynamicIndirectShadowCasterRepresentation() const
{
	return CastsDynamicShadow() && CastsDynamicIndirectShadow();
}

void FSkeletalMeshSceneProxy::GetShadowShapes(TArray<FCapsuleShape>& CapsuleShapes) const 
{
	SCOPE_CYCLE_COUNTER(STAT_GetShadowShapes);

	const TArray<FMatrix>& ReferenceToLocalMatrices = MeshObject->GetReferenceToLocalMatrices();
	const FMatrix& ProxyLocalToWorld = GetLocalToWorld();

	int32 CapsuleIndex = CapsuleShapes.Num();
	CapsuleShapes.SetNum(CapsuleShapes.Num() + ShadowCapsuleData.Num(), false);

	for(const TPair<int32, FCapsuleShape>& CapsuleData : ShadowCapsuleData)
	{
		FMatrix ReferenceToWorld = ReferenceToLocalMatrices[CapsuleData.Key] * ProxyLocalToWorld;
		const float MaxScale = ReferenceToWorld.GetScaleVector().GetMax();

		FCapsuleShape& NewCapsule = CapsuleShapes[CapsuleIndex++];

		NewCapsule.Center = ReferenceToWorld.TransformPosition(CapsuleData.Value.Center);
		NewCapsule.Radius = CapsuleData.Value.Radius * MaxScale;
		NewCapsule.Orientation = ReferenceToWorld.TransformVector(CapsuleData.Value.Orientation).GetSafeNormal();
		NewCapsule.Length = CapsuleData.Value.Length * MaxScale;
	}
}

/**
 * Returns the world transform to use for drawing.
 * @param OutLocalToWorld - Will contain the local-to-world transform when the function returns.
 * @param OutWorldToLocal - Will contain the world-to-local transform when the function returns.
 */
bool FSkeletalMeshSceneProxy::GetWorldMatrices( FMatrix& OutLocalToWorld, FMatrix& OutWorldToLocal ) const
{
	OutLocalToWorld = GetLocalToWorld();
	if (OutLocalToWorld.GetScaledAxis(EAxis::X).IsNearlyZero(SMALL_NUMBER) &&
		OutLocalToWorld.GetScaledAxis(EAxis::Y).IsNearlyZero(SMALL_NUMBER) &&
		OutLocalToWorld.GetScaledAxis(EAxis::Z).IsNearlyZero(SMALL_NUMBER))
	{
		return false;
	}
	OutWorldToLocal = GetLocalToWorld().InverseFast();
	return true;
}

/**
 * Relevance is always dynamic for skel meshes unless they are disabled
 */
FPrimitiveViewRelevance FSkeletalMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View) && View->Family->EngineShowFlags.SkeletalMeshes;
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = true;
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	
	MaterialRelevance.SetPrimitiveViewRelevance(Result);

#if !UE_BUILD_SHIPPING
	Result.bSeparateTranslucencyRelevance |= View->Family->EngineShowFlags.Constraints;
#endif

	return Result;
}

bool FSkeletalMeshSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest && !ShouldRenderCustomDepth();
}

/** Util for getting LOD index currently used by this SceneProxy. */
int32 FSkeletalMeshSceneProxy::GetCurrentLODIndex()
{
	if(MeshObject)
	{
		return MeshObject->GetLOD();
	}
	else
	{
		return 0;
	}
}


/** 
 * Render physics asset for debug display
 */
void FSkeletalMeshSceneProxy::DebugDrawPhysicsAsset(int32 ViewIndex, FMeshElementCollector& Collector, const FEngineShowFlags& EngineShowFlags) const
{
	FMatrix ProxyLocalToWorld, WorldToLocal;
	if (!GetWorldMatrices(ProxyLocalToWorld, WorldToLocal))
	{
		return; // Cannot draw this, world matrix not valid
	}

	FMatrix ScalingMatrix = ProxyLocalToWorld;
	FVector TotalScale = ScalingMatrix.ExtractScaling();

	// Only if valid
	if( !TotalScale.IsNearlyZero() )
	{
		FTransform LocalToWorldTransform(ProxyLocalToWorld);

		TArray<FTransform>* BoneSpaceBases = MeshObject->GetComponentSpaceTransforms();
		if(BoneSpaceBases)
		{
			//TODO: These data structures are not double buffered. This is not thread safe!
			check(PhysicsAssetForDebug);
			if (EngineShowFlags.Collision && IsCollisionEnabled())
			{
				PhysicsAssetForDebug->GetCollisionMesh(ViewIndex, Collector, SkeletalMeshForDebug, *BoneSpaceBases, LocalToWorldTransform, TotalScale);
			}
			if (EngineShowFlags.Constraints)
			{
				PhysicsAssetForDebug->DrawConstraints(ViewIndex, Collector, SkeletalMeshForDebug, *BoneSpaceBases, LocalToWorldTransform, TotalScale.X);
			}
		}
	}
}

void FSkeletalMeshSceneProxy::DebugDrawSkeleton(int32 ViewIndex, FMeshElementCollector& Collector, const FEngineShowFlags& EngineShowFlags) const
{
	FMatrix ProxyLocalToWorld, WorldToLocal;
	if (!GetWorldMatrices(ProxyLocalToWorld, WorldToLocal))
	{
		return; // Cannot draw this, world matrix not valid
	}

	FTransform LocalToWorldTransform(ProxyLocalToWorld);

	auto MakeRandomColorForSkeleton = [](uint32 InUID)
	{
		FRandomStream Stream((int32)InUID);
		const uint8 Hue = (uint8)(Stream.FRand()*255.f);
		return FLinearColor::FGetHSV(Hue, 0, 255);
	};

	FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
	TArray<FTransform>& ComponentSpaceTransforms = *MeshObject->GetComponentSpaceTransforms();

	for (int32 Index = 0; Index < ComponentSpaceTransforms.Num(); ++Index)
	{
		const int32 ParentIndex = SkeletalMeshForDebug->RefSkeleton.GetParentIndex(Index);
		FVector Start, End;
		
		FLinearColor LineColor = MakeRandomColorForSkeleton(GetPrimitiveComponentId().PrimIDValue);
		const FTransform Transform = ComponentSpaceTransforms[Index] * LocalToWorldTransform;

		if (ParentIndex >= 0)
		{
			Start = (ComponentSpaceTransforms[ParentIndex] * LocalToWorldTransform).GetLocation();
			End = Transform.GetLocation();
		}
		else
		{
			Start = LocalToWorldTransform.GetLocation();
			End = Transform.GetLocation();
		}

		if(EngineShowFlags.Bones)
		{
			if(CVarDebugDrawSimpleBones.GetValueOnRenderThread() != 0)
			{
				PDI->DrawLine(Start, End, LineColor, SDPG_Foreground, 0.0f, 1.0f);
			}
			else
			{
				SkeletalDebugRendering::DrawWireBone(PDI, Start, End, LineColor, SDPG_Foreground);
			}

			if(CVarDebugDrawBoneAxes.GetValueOnRenderThread() != 0)
			{
				SkeletalDebugRendering::DrawAxes(PDI, Transform, SDPG_Foreground);
			}
		}
	}
}

/**
* Updates morph material usage for materials referenced by each LOD entry
*
* @param bNeedsMorphUsage - true if the materials used by this skeletal mesh need morph target usage
*/
void FSkeletalMeshSceneProxy::UpdateMorphMaterialUsage_GameThread(TArray<UMaterialInterface*>& MaterialUsingMorphTarget)
{
	bool bNeedsMorphUsage = MaterialUsingMorphTarget.Num() > 0;
	if( bNeedsMorphUsage != bMaterialsNeedMorphUsage_GameThread )
	{
		// keep track of current morph material usage for the proxy
		bMaterialsNeedMorphUsage_GameThread = bNeedsMorphUsage;

		TSet<UMaterialInterface*> MaterialsToSwap;
		for (auto It = MaterialsInUse_GameThread.CreateConstIterator(); It; ++It)
		{
			UMaterialInterface* Material = *It;
			if (Material)
			{
				const bool bCheckSkelUsage = Material->CheckMaterialUsage_Concurrent(MATUSAGE_SkeletalMesh);
				if (!bCheckSkelUsage)
				{
					MaterialsToSwap.Add(Material);
				}
				else if(MaterialUsingMorphTarget.Contains(Material))
				{
					const bool bCheckMorphUsage = !bMaterialsNeedMorphUsage_GameThread || (bMaterialsNeedMorphUsage_GameThread && Material->CheckMaterialUsage_Concurrent(MATUSAGE_MorphTargets));
					// make sure morph material usage and default skeletal usage are both valid
					if (!bCheckMorphUsage)
					{
						MaterialsToSwap.Add(Material);
					}
				}
			}
		}

		// update the new LODSections on the render thread proxy
		if (MaterialsToSwap.Num())
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(
			UpdateSkelProxyLODSectionElementsCmd,
				TSet<UMaterialInterface*>,MaterialsToSwap,MaterialsToSwap,
				UMaterialInterface*,DefaultMaterial,UMaterial::GetDefaultMaterial(MD_Surface),
				ERHIFeatureLevel::Type, FeatureLevel, GetScene().GetFeatureLevel(),
			FSkeletalMeshSceneProxy*,SkelMeshSceneProxy,this,
			{
					for( int32 LodIdx=0; LodIdx < SkelMeshSceneProxy->LODSections.Num(); LodIdx++ )
					{
						FLODSectionElements& LODSection = SkelMeshSceneProxy->LODSections[LodIdx];
						for( int32 SectIdx=0; SectIdx < LODSection.SectionElements.Num(); SectIdx++ )
						{
							FSectionElementInfo& SectionElement = LODSection.SectionElements[SectIdx];
							if( MaterialsToSwap.Contains(SectionElement.Material) )
							{
								// fallback to default material if needed
								SectionElement.Material = DefaultMaterial;
							}
						}
					}
					SkelMeshSceneProxy->MaterialRelevance |= DefaultMaterial->GetRelevance(FeatureLevel);
			});
		}
	}
}

#if WITH_EDITORONLY_DATA

bool FSkeletalMeshSceneProxy::GetPrimitiveDistance(int32 LODIndex, int32 SectionIndex, const FVector& ViewOrigin, float& PrimitiveDistance) const
{

	if (FPrimitiveSceneProxy::GetPrimitiveDistance(LODIndex, SectionIndex, ViewOrigin, PrimitiveDistance))
	{
		const float OneOverDistanceMultiplier = 1.f / FMath::Max<float>(SMALL_NUMBER, StreamingDistanceMultiplier);
		PrimitiveDistance *= OneOverDistanceMultiplier;
		return true;
	}
	return false;
}

bool FSkeletalMeshSceneProxy::GetMeshUVDensities(int32 LODIndex, int32 SectionIndex, FVector4& WorldUVDensities) const
{
	if (LODSections.IsValidIndex(LODIndex) && LODSections[LODIndex].SectionElements.IsValidIndex(SectionIndex))
	{
		// The LOD-section data is stored per material index as it is only used for texture streaming currently.
		const int32 MaterialIndex = LODSections[LODIndex].SectionElements[SectionIndex].UseMaterialIndex;
		if (SkeletalMeshRenderData && SkeletalMeshRenderData->UVChannelDataPerMaterial.IsValidIndex(MaterialIndex))
		{
			const float TransformScale = GetLocalToWorld().GetMaximumAxisScale();
			const float* LocalUVDensities = SkeletalMeshRenderData->UVChannelDataPerMaterial[MaterialIndex].LocalUVDensities;

			WorldUVDensities.Set(
				LocalUVDensities[0] * TransformScale,
				LocalUVDensities[1] * TransformScale,
				LocalUVDensities[2] * TransformScale,
				LocalUVDensities[3] * TransformScale);
			
			return true;
	}
		}
	return FPrimitiveSceneProxy::GetMeshUVDensities(LODIndex, SectionIndex, WorldUVDensities);
	}

bool FSkeletalMeshSceneProxy::GetMaterialTextureScales(int32 LODIndex, int32 SectionIndex, const FMaterialRenderProxy* MaterialRenderProxy, FVector4* OneOverScales, FIntVector4* UVChannelIndices) const
	{
	if (LODSections.IsValidIndex(LODIndex) && LODSections[LODIndex].SectionElements.IsValidIndex(SectionIndex))
	{
		const UMaterialInterface* Material = LODSections[LODIndex].SectionElements[SectionIndex].Material;
		if (Material)
		{
			// This is thread safe because material texture data is only updated while the renderthread is idle.
			for (const FMaterialTextureInfo TextureData : Material->GetTextureStreamingData())
	{
				const int32 TextureIndex = TextureData.TextureIndex;
				if (TextureData.IsValid(true))
		{
					OneOverScales[TextureIndex / 4][TextureIndex % 4] = 1.f / TextureData.SamplingScale;
					UVChannelIndices[TextureIndex / 4][TextureIndex % 4] = TextureData.UVChannelIndex;
		}
	}
			return true;
	}
	}
	return false;
}
#endif

FSkinnedMeshComponentRecreateRenderStateContext::FSkinnedMeshComponentRecreateRenderStateContext(USkeletalMesh* InSkeletalMesh, bool InRefreshBounds /*= false*/)
	: bRefreshBounds(InRefreshBounds)
{
	for (TObjectIterator<USkinnedMeshComponent> It; It; ++It)
	{
		if (It->SkeletalMesh == InSkeletalMesh)
		{
			checkf(!It->IsUnreachable(), TEXT("%s"), *It->GetFullName());

			if (It->IsRenderStateCreated())
			{
				check(It->IsRegistered());
				It->DestroyRenderState_Concurrent();
				MeshComponents.Add(*It);
			}
		}
	}

	// Flush the rendering commands generated by the detachments.
	// The static mesh scene proxies reference the UStaticMesh, and this ensures that they are cleaned up before the UStaticMesh changes.
	FlushRenderingCommands();
}

FSkinnedMeshComponentRecreateRenderStateContext::~FSkinnedMeshComponentRecreateRenderStateContext()
{
	const int32 ComponentCount = MeshComponents.Num();
	for (int32 ComponentIndex = 0; ComponentIndex < ComponentCount; ++ComponentIndex)
	{
		USkinnedMeshComponent* Component = MeshComponents[ComponentIndex];

		if (bRefreshBounds)
		{
			Component->UpdateBounds();
		}

		if (Component->IsRegistered() && !Component->IsRenderStateCreated())
		{
			Component->CreateRenderState_Concurrent();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

template <bool bExtraBoneInfluencesT>
FVector GetRefVertexLocationTyped(
	const USkeletalMesh* Mesh,
	const FSkelMeshRenderSection& Section,
	const FPositionVertexBuffer& PositionBuffer,
	const FSkinWeightVertexBuffer& SkinWeightVertexBuffer,
	const int32 VertIndex
)
{
	FVector SkinnedPos(0, 0, 0);

	// Do soft skinning for this vertex.
	int32 BufferVertIndex = Section.GetVertexBufferIndex() + VertIndex;
	const TSkinWeightInfo<bExtraBoneInfluencesT>* SrcSkinWeights = SkinWeightVertexBuffer.GetSkinWeightPtr<bExtraBoneInfluencesT>(BufferVertIndex);
	int32 MaxBoneInfluences = bExtraBoneInfluencesT ? MAX_TOTAL_INFLUENCES : MAX_INFLUENCES_PER_STREAM;

#if !PLATFORM_LITTLE_ENDIAN
	// uint8[] elements in LOD.VertexBufferGPUSkin have been swapped for VET_UBYTE4 vertex stream use
	for (int32 InfluenceIndex = MAX_INFLUENCES - 1; InfluenceIndex >= MAX_INFLUENCES - MaxBoneInfluences; InfluenceIndex--)
#else
	for (int32 InfluenceIndex = 0; InfluenceIndex < MaxBoneInfluences; InfluenceIndex++)
#endif
	{
		const int32 MeshBoneIndex = Section.BoneMap[SrcSkinWeights->InfluenceBones[InfluenceIndex]];
		const float	Weight = (float)SrcSkinWeights->InfluenceWeights[InfluenceIndex] / 255.0f;
		{
			const FMatrix BoneTransformMatrix = FMatrix::Identity;//Mesh->GetComposedRefPoseMatrix(MeshBoneIndex);
			const FMatrix RefToLocal = Mesh->RefBasesInvMatrix[MeshBoneIndex] * BoneTransformMatrix;

			SkinnedPos += BoneTransformMatrix.TransformPosition(PositionBuffer.VertexPosition(BufferVertIndex)) * Weight;
		}
	}

	return SkinnedPos;
}

FVector GetSkeletalMeshRefVertLocation(const USkeletalMesh* Mesh, const FSkeletalMeshLODRenderData& LODData, const FSkinWeightVertexBuffer& SkinWeightVertexBuffer, const int32 VertIndex)
{
	int32 SectionIndex;
	int32 VertIndexInChunk;
	LODData.GetSectionFromVertexIndex(VertIndex, SectionIndex, VertIndexInChunk);
	const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];
	if (SkinWeightVertexBuffer.HasExtraBoneInfluences())
	{
		return GetRefVertexLocationTyped<true>(Mesh, Section, LODData.StaticVertexBuffers.PositionVertexBuffer, SkinWeightVertexBuffer, VertIndexInChunk);
	}
	else
	{
		return GetRefVertexLocationTyped<false>(Mesh, Section, LODData.StaticVertexBuffers.PositionVertexBuffer, SkinWeightVertexBuffer, VertIndexInChunk);
	}
}

#undef LOCTEXT_NAMESPACE
