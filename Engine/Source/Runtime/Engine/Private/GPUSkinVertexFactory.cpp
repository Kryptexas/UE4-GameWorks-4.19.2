// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GPUVertexFactory.cpp: GPU skin vertex factory implementation
=============================================================================*/

#include "GPUSkinVertexFactory.h"
#include "SceneView.h"
#include "MeshBatch.h"
#include "GPUSkinCache.h"
#include "ShaderParameterUtils.h"

// Changing this is currently unsupported after content has been chunked with the previous setting
// Changing this causes a full shader recompile
static int32 GCVarMaxGPUSkinBones = FGPUBaseSkinVertexFactory::GHardwareMaxGPUSkinBones;
static FAutoConsoleVariableRef CVarMaxGPUSkinBones(
	TEXT("Compat.MAX_GPUSKIN_BONES"),
	GCVarMaxGPUSkinBones,
	TEXT("Max number of bones that can be skinned on the GPU in a single draw call. Cannot be changed at runtime."),
	ECVF_ReadOnly);

// Whether to use 2 bones influence instead of default 4 for GPU skinning
// Changing this causes a full shader recompile
static TAutoConsoleVariable<int32> CVarGPUSkinLimit2BoneInfluences(
	TEXT("r.GPUSkin.Limit2BoneInfluences"),
	0,	
	TEXT("Whether to use 2 bones influence instead of default 4 for GPU skinning. Cannot be changed at runtime."),
	ECVF_ReadOnly);

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FAPEXClothUniformShaderParameters,TEXT("APEXClothParam"));

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FBoneMatricesUniformShaderParameters,TEXT("Bones"));

static FBoneMatricesUniformShaderParameters GBoneUniformStruct;

#define IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE_INTERNAL(FactoryClass, ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,bSupportsDynamicLighting,bPrecisePrevWorldPos,bSupportsPositionOnly) \
	template <bool bExtraBoneInfluencesT> FVertexFactoryType FactoryClass<bExtraBoneInfluencesT>::StaticType( \
	bExtraBoneInfluencesT ? TEXT(#FactoryClass) TEXT("true") : TEXT(#FactoryClass) TEXT("false"), \
	TEXT(ShaderFilename), \
	bUsedWithMaterials, \
	bSupportsStaticLighting, \
	bSupportsDynamicLighting, \
	bPrecisePrevWorldPos, \
	bSupportsPositionOnly, \
	Construct##FactoryClass##ShaderParameters<bExtraBoneInfluencesT>, \
	FactoryClass<bExtraBoneInfluencesT>::ShouldCompilePermutation, \
	FactoryClass<bExtraBoneInfluencesT>::ModifyCompilationEnvironment, \
	FactoryClass<bExtraBoneInfluencesT>::SupportsTessellationShaders \
	); \
	template <bool bExtraBoneInfluencesT> inline FVertexFactoryType* FactoryClass<bExtraBoneInfluencesT>::GetType() const { return &StaticType; }


#define IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE(FactoryClass, ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,bSupportsDynamicLighting,bPrecisePrevWorldPos,bSupportsPositionOnly) \
	template <bool bExtraBoneInfluencesT> FVertexFactoryShaderParameters* Construct##FactoryClass##ShaderParameters(EShaderFrequency ShaderFrequency) { return FactoryClass<bExtraBoneInfluencesT>::ConstructShaderParameters(ShaderFrequency); } \
	IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE_INTERNAL(FactoryClass, ShaderFilename,bUsedWithMaterials,bSupportsStaticLighting,bSupportsDynamicLighting,bPrecisePrevWorldPos,bSupportsPositionOnly) \
	template class FactoryClass<false>;	\
	template class FactoryClass<true>;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarVelocityTest(
	TEXT("r.VelocityTest"),
	0,
	TEXT("Allows to enable some low level testing code for the velocity rendering (Affects object motion blur and TemporalAA).")
	TEXT(" 0: off (default)")
	TEXT(" 1: add random data to the buffer where we store skeletal mesh bone data to test if the code (good to test in PAUSED as well)."),
	ECVF_Cheat | ECVF_RenderThreadSafe);
#endif // if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

/*-----------------------------------------------------------------------------
 FSharedPoolPolicyData
 -----------------------------------------------------------------------------*/
uint32 FSharedPoolPolicyData::GetPoolBucketIndex(uint32 Size)
{
	unsigned long Lower = 0;
	unsigned long Upper = NumPoolBucketSizes;
	unsigned long Middle;
	
	do
	{
		Middle = ( Upper + Lower ) >> 1;
		if( Size <= BucketSizes[Middle-1] )
		{
			Upper = Middle;
		}
		else
		{
			Lower = Middle;
		}
	}
	while( Upper - Lower > 1 );
	
	check( Size <= BucketSizes[Lower] );
	check( (Lower == 0 ) || ( Size > BucketSizes[Lower-1] ) );
	
	return Lower;
}

uint32 FSharedPoolPolicyData::GetPoolBucketSize(uint32 Bucket)
{
	check(Bucket < NumPoolBucketSizes);
	return BucketSizes[Bucket];
}

uint32 FSharedPoolPolicyData::BucketSizes[NumPoolBucketSizes] = {
	16, 48, 96, 192, 384, 768, 1536, 
	3072, 4608, 6144, 7680, 9216, 12288, 
	65536, 131072, 262144, 1048576 // these 4 numbers are added for large cloth simulation vertices, supports up to 65,536 verts
};

/*-----------------------------------------------------------------------------
 FBoneBufferPoolPolicy
 -----------------------------------------------------------------------------*/
FVertexBufferAndSRV FBoneBufferPoolPolicy::CreateResource(CreationArguments Args)
{
	uint32 BufferSize = GetPoolBucketSize(GetPoolBucketIndex(Args));
	// in VisualStudio the copy constructor call on the return argument can be optimized out
	// see https://msdn.microsoft.com/en-us/library/ms364057.aspx#nrvo_cpp05_topic3
	FVertexBufferAndSRV Buffer;
	FRHIResourceCreateInfo CreateInfo;
	Buffer.VertexBufferRHI = RHICreateVertexBuffer( BufferSize, (BUF_Dynamic | BUF_ShaderResource), CreateInfo );
	Buffer.VertexBufferSRV = RHICreateShaderResourceView( Buffer.VertexBufferRHI, sizeof(FVector4), PF_A32B32G32R32F );
	return Buffer;
}

FSharedPoolPolicyData::CreationArguments FBoneBufferPoolPolicy::GetCreationArguments(const FVertexBufferAndSRV& Resource)
{
	return Resource.VertexBufferRHI->GetSize();
}

void FBoneBufferPoolPolicy::FreeResource(FVertexBufferAndSRV Resource)
{
}

FVertexBufferAndSRV FClothBufferPoolPolicy::CreateResource(CreationArguments Args)
{
	uint32 BufferSize = GetPoolBucketSize(GetPoolBucketIndex(Args));
	// in VisualStudio the copy constructor call on the return argument can be optimized out
	// see https://msdn.microsoft.com/en-us/library/ms364057.aspx#nrvo_cpp05_topic3
	FVertexBufferAndSRV Buffer;
	FRHIResourceCreateInfo CreateInfo;
	Buffer.VertexBufferRHI = RHICreateVertexBuffer( BufferSize, (BUF_Dynamic | BUF_ShaderResource), CreateInfo );
	Buffer.VertexBufferSRV = RHICreateShaderResourceView( Buffer.VertexBufferRHI, sizeof(FVector2D), PF_G32R32F );
	return Buffer;
}

/*-----------------------------------------------------------------------------
 FBoneBufferPool
 -----------------------------------------------------------------------------*/
FBoneBufferPool::~FBoneBufferPool()
{
}

TStatId FBoneBufferPool::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FBoneBufferPool, STATGROUP_Tickables);
}

FClothBufferPool::~FClothBufferPool()
{
}

TStatId FClothBufferPool::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FClothBufferPool, STATGROUP_Tickables);
}

TConsoleVariableData<int32>* FGPUBaseSkinVertexFactory::FShaderDataType::MaxBonesVar = NULL;
uint32 FGPUBaseSkinVertexFactory::FShaderDataType::MaxGPUSkinBones = 0;

static TAutoConsoleVariable<int32> CVarRHICmdDeferSkeletalLockAndFillToRHIThread(
	TEXT("r.RHICmdDeferSkeletalLockAndFillToRHIThread"),
	0,
	TEXT("If > 0, then do the bone and cloth copies on the RHI thread. Experimental option."));

static bool DeferSkeletalLockAndFillToRHIThread()
{
	return IsRunningRHIInSeparateThread() && CVarRHICmdDeferSkeletalLockAndFillToRHIThread.GetValueOnRenderThread() > 0;
}

struct FRHICommandUpdateBoneBuffer final : public FRHICommand<FRHICommandUpdateBoneBuffer>
{
	FVertexBufferRHIParamRef VertexBuffer;
	uint32 BufferSize;
	const TArray<FMatrix>& ReferenceToLocalMatrices;
	const TArray<FBoneIndexType>& BoneMap;


	FORCEINLINE_DEBUGGABLE FRHICommandUpdateBoneBuffer(FVertexBufferRHIParamRef InVertexBuffer, uint32 InBufferSize, const TArray<FMatrix>& InReferenceToLocalMatrices, const TArray<FBoneIndexType>& InBoneMap)
		: VertexBuffer(InVertexBuffer)
		, BufferSize(InBufferSize)
		, ReferenceToLocalMatrices(InReferenceToLocalMatrices)
		, BoneMap(InBoneMap)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FRHICommandUpdateBoneBuffer_Execute);
		FSkinMatrix3x4* ChunkMatrices = (FSkinMatrix3x4*)GDynamicRHI->RHILockVertexBuffer(VertexBuffer, 0, BufferSize, RLM_WriteOnly);
		//FSkinMatrix3x4 is sizeof() == 48
		// PLATFORM_CACHE_LINE_SIZE (128) / 48 = 2.6
		//  sizeof(FMatrix) == 64
		// PLATFORM_CACHE_LINE_SIZE (128) / 64 = 2
		const uint32 NumBones = BoneMap.Num();
		check(NumBones > 0 && NumBones < 256); // otherwise maybe some bad threading on BoneMap, maybe we need to copy that
		const int32 PreFetchStride = 2; // FPlatformMisc::Prefetch stride
		for (uint32 BoneIdx = 0; BoneIdx < NumBones; BoneIdx++)
		{
			const FBoneIndexType RefToLocalIdx = BoneMap[BoneIdx];
			check(ReferenceToLocalMatrices.IsValidIndex(RefToLocalIdx)); // otherwise maybe some bad threading on BoneMap, maybe we need to copy that
			FPlatformMisc::Prefetch( ReferenceToLocalMatrices.GetData() + RefToLocalIdx + PreFetchStride );
			FPlatformMisc::Prefetch( ReferenceToLocalMatrices.GetData() + RefToLocalIdx + PreFetchStride, PLATFORM_CACHE_LINE_SIZE );

			FSkinMatrix3x4& BoneMat = ChunkMatrices[BoneIdx];
			const FMatrix& RefToLocal = ReferenceToLocalMatrices[RefToLocalIdx];
			RefToLocal.To3x4MatrixTranspose( (float*)BoneMat.M );
		}
		GDynamicRHI->RHIUnlockVertexBuffer(VertexBuffer);
	}
};

bool FGPUBaseSkinVertexFactory::FShaderDataType::UpdateBoneData(FRHICommandListImmediate& RHICmdList, const TArray<FMatrix>& ReferenceToLocalMatrices,
	const TArray<FBoneIndexType>& BoneMap, uint32 RevisionNumber, bool bPrevious, ERHIFeatureLevel::Type InFeatureLevel, bool bUseSkinCache)
{
	const uint32 NumBones = BoneMap.Num();
	check(NumBones <= MaxGPUSkinBones);
	FSkinMatrix3x4* ChunkMatrices = nullptr;

	FVertexBufferAndSRV* CurrentBoneBuffer = 0;

	if (InFeatureLevel >= ERHIFeatureLevel::ES3_1)
	{
		check(IsInRenderingThread());
		
		// make sure current revision is up-to-date
		SetCurrentRevisionNumber(RevisionNumber);

		CurrentBoneBuffer = &GetBoneBufferForWriting(bPrevious);

		static FSharedPoolPolicyData PoolPolicy;
		uint32 NumVectors = NumBones*3;
		check(NumVectors <= (MaxGPUSkinBones*3));
		uint32 VectorArraySize = NumVectors * sizeof(FVector4);
		uint32 PooledArraySize = BoneBufferPool.PooledSizeForCreationArguments(VectorArraySize);

		if(!IsValidRef(*CurrentBoneBuffer) || PooledArraySize != CurrentBoneBuffer->VertexBufferRHI->GetSize())
		{
			if(IsValidRef(*CurrentBoneBuffer))
			{
				BoneBufferPool.ReleasePooledResource(*CurrentBoneBuffer);
			}
			*CurrentBoneBuffer = BoneBufferPool.CreatePooledResource(VectorArraySize);
			check(IsValidRef(*CurrentBoneBuffer));
		}
		if(NumBones)
		{
			if (!bUseSkinCache && DeferSkeletalLockAndFillToRHIThread())
			{
				new (RHICmdList.AllocCommand<FRHICommandUpdateBoneBuffer>()) FRHICommandUpdateBoneBuffer(CurrentBoneBuffer->VertexBufferRHI, VectorArraySize, ReferenceToLocalMatrices, BoneMap);
				return true;
			}
			ChunkMatrices = (FSkinMatrix3x4*)RHILockVertexBuffer(CurrentBoneBuffer->VertexBufferRHI, 0, VectorArraySize, RLM_WriteOnly);
		}
	}
	else
	{
		if(NumBones)
		{
			check(NumBones * sizeof(FSkinMatrix3x4) <= sizeof(GBoneUniformStruct));
			ChunkMatrices = (FSkinMatrix3x4*)&GBoneUniformStruct;
		}
	}

	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FGPUBaseSkinVertexFactory_ShaderDataType_UpdateBoneData_CopyBones);
		//FSkinMatrix3x4 is sizeof() == 48
		// PLATFORM_CACHE_LINE_SIZE (128) / 48 = 2.6
		//  sizeof(FMatrix) == 64
		// PLATFORM_CACHE_LINE_SIZE (128) / 64 = 2
		const int32 PreFetchStride = 2; // FPlatformMisc::Prefetch stride
		for (uint32 BoneIdx = 0; BoneIdx < NumBones; BoneIdx++)
		{
			const FBoneIndexType RefToLocalIdx = BoneMap[BoneIdx];
			FPlatformMisc::Prefetch( ReferenceToLocalMatrices.GetData() + RefToLocalIdx + PreFetchStride );
			FPlatformMisc::Prefetch( ReferenceToLocalMatrices.GetData() + RefToLocalIdx + PreFetchStride, PLATFORM_CACHE_LINE_SIZE );

			FSkinMatrix3x4& BoneMat = ChunkMatrices[BoneIdx];
			const FMatrix& RefToLocal = ReferenceToLocalMatrices[RefToLocalIdx];
			RefToLocal.To3x4MatrixTranspose( (float*)BoneMat.M );
		}
	}
	if (InFeatureLevel >= ERHIFeatureLevel::ES3_1)
	{
		if (NumBones)
		{
			check(CurrentBoneBuffer);
			RHIUnlockVertexBuffer(CurrentBoneBuffer->VertexBufferRHI);
		}
	}
	else
	{
		UniformBuffer = RHICreateUniformBuffer(&GBoneUniformStruct, FBoneMatricesUniformShaderParameters::StaticStruct.GetLayout(), UniformBuffer_MultiFrame);
	}
	return false;
}

int32 FGPUBaseSkinVertexFactory::GetMaxGPUSkinBones()
{
	return GCVarMaxGPUSkinBones;
}

/*-----------------------------------------------------------------------------
TGPUSkinVertexFactory
-----------------------------------------------------------------------------*/

TGlobalResource<FBoneBufferPool> FGPUBaseSkinVertexFactory::BoneBufferPool;

template <bool bExtraBoneInfluencesT>
bool TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const FShaderType* ShaderType)
{
	bool bLimit2BoneInfluences = (CVarGPUSkinLimit2BoneInfluences.GetValueOnAnyThread() != 0);
	
	// Skip trying to use extra bone influences on < SM4 or when project uses 2 bones influence
	if (bExtraBoneInfluencesT && (GetMaxSupportedFeatureLevel(Platform) < ERHIFeatureLevel::ES3_1 || bLimit2BoneInfluences))
	{
		return false;
	}

	return (Material->IsUsedWithSkeletalMesh() || Material->IsSpecialEngineMaterial());
}


template <bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment )
{
	FVertexFactory::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	const int32 MaxGPUSkinBones = GetFeatureLevelMaxNumberOfBones(GetMaxSupportedFeatureLevel(Platform));
	OutEnvironment.SetDefine(TEXT("MAX_SHADER_BONES"), MaxGPUSkinBones);
	const uint32 UseExtraBoneInfluences = bExtraBoneInfluencesT;
	OutEnvironment.SetDefine(TEXT("GPUSKIN_USE_EXTRA_INFLUENCES"), UseExtraBoneInfluences);
	{
		bool bLimit2BoneInfluences = (CVarGPUSkinLimit2BoneInfluences.GetValueOnAnyThread() != 0);
		OutEnvironment.SetDefine(TEXT("GPUSKIN_LIMIT_2BONE_INFLUENCES"), (bLimit2BoneInfluences ? 1 : 0));
	}
}


template<bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::CopyDataTypeForPassthroughFactory(FGPUSkinPassthroughVertexFactory* PassthroughVertexFactory)
{
	FGPUSkinPassthroughVertexFactory::FDataType DestDataType;
	DestDataType.PositionComponent = Data.PositionComponent;
	DestDataType.TangentBasisComponents[0] = Data.TangentBasisComponents[0];
	DestDataType.TangentBasisComponents[1] = Data.TangentBasisComponents[1];
	DestDataType.TextureCoordinates = Data.TextureCoordinates;
	DestDataType.ColorComponent = Data.ColorComponent;
	DestDataType.PositionComponentSRV = Data.PositionComponentSRV;
	DestDataType.TangentsSRV = Data.TangentsSRV;
	DestDataType.ColorComponentsSRV = Data.ColorComponentsSRV;
	DestDataType.TextureCoordinatesSRV = Data.TextureCoordinatesSRV;
	DestDataType.LightMapCoordinateIndex = Data.LightMapCoordinateIndex;
	DestDataType.NumTexCoords = Data.NumTexCoords;
	PassthroughVertexFactory->SetData(DestDataType);
}

/**
* Add the decl elements for the streams
* @param InData - type with stream components
* @param OutElements - vertex decl list to modify
*/
template <bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::AddVertexElements(FDataType& InData, FVertexDeclarationElementList& OutElements)
{
	// position decls
	OutElements.Add(AccessStreamComponent(InData.PositionComponent,0));

	// tangent basis vector decls
	OutElements.Add(AccessStreamComponent(InData.TangentBasisComponents[0],1));
	OutElements.Add(AccessStreamComponent(InData.TangentBasisComponents[1],2));

	// texture coordinate decls
	if(InData.TextureCoordinates.Num())
	{
		const uint8 BaseTexCoordAttribute = 5;
		for(int32 CoordinateIndex = 0;CoordinateIndex < InData.TextureCoordinates.Num();CoordinateIndex++)
		{
			OutElements.Add(AccessStreamComponent(
				InData.TextureCoordinates[CoordinateIndex],
				BaseTexCoordAttribute + CoordinateIndex
				));
		}

		for(int32 CoordinateIndex = InData.TextureCoordinates.Num();CoordinateIndex < MAX_TEXCOORDS;CoordinateIndex++)
		{
			OutElements.Add(AccessStreamComponent(
				InData.TextureCoordinates[InData.TextureCoordinates.Num() - 1],
				BaseTexCoordAttribute + CoordinateIndex
				));
		}
	}

	if (Data.ColorComponentsSRV == nullptr)
	{
		Data.ColorComponentsSRV = GNullColorVertexBuffer.VertexBufferSRV;
		Data.ColorIndexMask = 0;
	}

	// Account for the possibility that the mesh has no vertex colors
	if( InData.ColorComponent.VertexBuffer )
	{
		OutElements.Add(AccessStreamComponent(InData.ColorComponent, 13));
	}
	else
	{
		//If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
		//This wastes 4 bytes of bandwidth per vertex, but prevents having to compile out twice the number of vertex factories.
		FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color, EVertexStreamUsage::ManualFetch);
		OutElements.Add(AccessStreamComponent(NullColorComponent, 13));
	}

	// bone indices decls
	OutElements.Add(AccessStreamComponent(InData.BoneIndices,3));

	// bone weights decls
	OutElements.Add(AccessStreamComponent(InData.BoneWeights,4));

	if (bExtraBoneInfluencesT)
	{
		// Extra bone indices & weights decls
		OutElements.Add(AccessStreamComponent(InData.ExtraBoneIndices, 14));
		OutElements.Add(AccessStreamComponent(InData.ExtraBoneWeights, 15));
	}
}

/**
* Creates declarations for each of the vertex stream components and
* initializes the device resource
*/
template <bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::InitRHI()
{
	// list of declaration items
	FVertexDeclarationElementList Elements;
	AddVertexElements(Data,Elements);	

	// create the actual device decls
	InitDeclaration(Elements);
}

template <bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::InitDynamicRHI()
{
	FVertexFactory::InitDynamicRHI();
	//ShaderData.UpdateBoneData(GetFeatureLevel());
}

template <bool bExtraBoneInfluencesT>
void TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ReleaseDynamicRHI()
{
	FVertexFactory::ReleaseDynamicRHI();
	ShaderData.ReleaseBoneData();
}

/*-----------------------------------------------------------------------------
TGPUSkinAPEXClothVertexFactory
-----------------------------------------------------------------------------*/

template <bool bExtraBoneInfluencesT>
void TGPUSkinAPEXClothVertexFactory<bExtraBoneInfluencesT>::ReleaseDynamicRHI()
{
	Super::ReleaseDynamicRHI();
	ClothShaderData.ReleaseClothSimulData();
}

/*-----------------------------------------------------------------------------
TGPUSkinVertexFactoryShaderParameters
-----------------------------------------------------------------------------*/

/** Shader parameters for use with TGPUSkinVertexFactory */
class FGPUSkinVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	/**
	* Bind shader constants by name
	* @param	ParameterMap - mapping of named shader constants to indices
	*/
	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		PerBoneMotionBlur.Bind(ParameterMap,TEXT("PerBoneMotionBlur"));
		BoneMatrices.Bind(ParameterMap,TEXT("BoneMatrices"));
		PreviousBoneMatrices.Bind(ParameterMap,TEXT("PreviousBoneMatrices"));
	}
	/**
	* Serialize shader params to an archive
	* @param	Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar) override
	{
		Ar << PerBoneMotionBlur;
		Ar << BoneMatrices;
		Ar << PreviousBoneMatrices;
	}

	/**
	* Set any shader data specific to this vertex factory
	*/
	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override
	{
		FRHIVertexShader* ShaderRHI = Shader->GetVertexShader();

		if(ShaderRHI)
		{
			const FGPUBaseSkinVertexFactory::FShaderDataType& ShaderData = ((const FGPUBaseSkinVertexFactory*)VertexFactory)->GetShaderData();
	
			const auto FeatureLevel = View.GetFeatureLevel();

			bool bLocalPerBoneMotionBlur = false;

			if (FeatureLevel >= ERHIFeatureLevel::ES3_1)
			{
				if(BoneMatrices.IsBound())
				{
					FShaderResourceViewRHIParamRef CurrentData = ShaderData.GetBoneBufferForReading(false).VertexBufferSRV;

					RHICmdList.SetShaderResourceViewParameter(ShaderRHI, BoneMatrices.GetBaseIndex(), CurrentData);
				}
				if(PreviousBoneMatrices.IsBound())
				{
					// todo: Maybe a check for PreviousData!=CurrentData would save some performance (when objects don't have velocty yet) but removing the bool also might save performance
					bLocalPerBoneMotionBlur = true;

					FShaderResourceViewRHIParamRef PreviousData = ShaderData.GetBoneBufferForReading(true).VertexBufferSRV;

					RHICmdList.SetShaderResourceViewParameter(ShaderRHI, PreviousBoneMatrices.GetBaseIndex(), PreviousData);
				}
			}
			else
			{
				SetUniformBufferParameter(RHICmdList, ShaderRHI, Shader->GetUniformBufferParameter<FBoneMatricesUniformShaderParameters>(), ShaderData.GetUniformBuffer());
			}


			SetShaderValue(RHICmdList, ShaderRHI, PerBoneMotionBlur, bLocalPerBoneMotionBlur);
		}
	}

	virtual uint32 GetSize() const override { return sizeof(*this); }

private:
	FShaderParameter PerBoneMotionBlur;
	FShaderResourceParameter BoneMatrices;
	FShaderResourceParameter PreviousBoneMatrices;
};

template <bool bExtraBoneInfluencesT>
FVertexFactoryShaderParameters* TGPUSkinVertexFactory<bExtraBoneInfluencesT>::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return (ShaderFrequency == SF_Vertex) ? new FGPUSkinVertexFactoryShaderParameters() : NULL;
}

/** bind gpu skin vertex factory to its shader file and its shader parameters */
IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE(TGPUSkinVertexFactory, "/Engine/Private/GpuSkinVertexFactory.ush", true, false, true, false, false);

/*-----------------------------------------------------------------------------
TGPUSkinVertexFactoryShaderParameters
-----------------------------------------------------------------------------*/

/** Shader parameters for use with TGPUSkinVertexFactory */
class FGPUSkinVertexPassthroughFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	/**
	* Bind shader constants by name
	* @param	ParameterMap - mapping of named shader constants to indices
	*/
	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		GPUSkinCachePreviousPositionBuffer.Bind(ParameterMap,TEXT("GPUSkinCachePreviousPositionBuffer"));
	}
	/**
	* Serialize shader params to an archive
	* @param	Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar) override
	{
		Ar << GPUSkinCachePreviousPositionBuffer;
	}
	/**
	* Set any shader data specific to this vertex factory
	*/
	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* Shader,const FVertexFactory* VertexFactory,const FSceneView& View,const FMeshBatchElement& BatchElement,uint32 DataFlags) const override
	{
		check(VertexFactory->GetType() == &FGPUSkinPassthroughVertexFactory::StaticType);
		FGPUSkinBatchElementUserData* BatchUserData = (FGPUSkinBatchElementUserData*)BatchElement.VertexFactoryUserData;
		check(BatchUserData);
		FGPUSkinCache::SetVertexStreams(BatchUserData->Entry, BatchUserData->Section, RHICmdList, Shader, (FGPUSkinPassthroughVertexFactory*)VertexFactory, BatchElement.MinVertexIndex, GPUSkinCachePreviousPositionBuffer);
	}

	virtual uint32 GetSize() const override { return sizeof(*this); }

private:
	FShaderResourceParameter GPUSkinCachePreviousPositionBuffer;
};

/*-----------------------------------------------------------------------------
FGPUSkinPassthroughVertexFactory
-----------------------------------------------------------------------------*/
void FGPUSkinPassthroughVertexFactory::ModifyCompilationEnvironment( EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment )
{
	const bool ContainsManualVertexFetch = OutEnvironment.GetDefinitions().Contains("MANUAL_VERTEX_FETCH");
	if (!ContainsManualVertexFetch)
	{
		OutEnvironment.SetDefine(TEXT("MANUAL_VERTEX_FETCH"), TEXT("0"));
	}

	Super::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	OutEnvironment.SetDefine(TEXT("GPUSKIN_PASS_THROUGH"),TEXT("1"));
}

bool FGPUSkinPassthroughVertexFactory::ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const FShaderType* ShaderType)
{
	// Passthrough is only valid on platforms with Compute Shader support AND for (skeletal meshes or default materials)
	return IsGPUSkinCacheAvailable() && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && Super::ShouldCompilePermutation(Platform, Material, ShaderType) && (Material->IsUsedWithSkeletalMesh() || Material->IsSpecialEngineMaterial());
}

void FGPUSkinPassthroughVertexFactory::InternalUpdateVertexDeclaration(FGPUBaseSkinVertexFactory* SourceVertexFactory, struct FRWBuffer* PositionRWBuffer, struct FRWBuffer* TangentRWBuffer)
{
	// Point this vertex buffer to the RWBuffer
	PositionVBAlias.VertexBufferRHI = PositionRWBuffer->Buffer;

	TangentVBAlias.VertexBufferRHI = TangentRWBuffer ? TangentRWBuffer->Buffer : nullptr;

	// Modify the vertex declaration using the RWBuffer for the position & tangent information
	Data.PositionComponent.VertexBuffer = &PositionVBAlias;
	Data.PositionComponent.Offset = 0;
	Data.PositionComponent.VertexStreamUsage = EVertexStreamUsage::Overridden;
	Data.PositionComponent.Stride = 3 * sizeof(float);

	
	{
		Data.TangentsSRV = TangentRWBuffer ? TangentRWBuffer->SRV : SourceVertexFactory->GetTangentsSRV();
		Data.PositionComponentSRV = PositionRWBuffer->SRV;
	}

	if (TangentRWBuffer)
	{
		Data.TangentBasisComponents[0].VertexBuffer = &TangentVBAlias;
		Data.TangentBasisComponents[0].Offset = 0;
		Data.TangentBasisComponents[0].Type = VET_PackedNormal;
		Data.TangentBasisComponents[0].Stride = 8;
		Data.TangentBasisComponents[0].VertexStreamUsage = EVertexStreamUsage::Overridden | EVertexStreamUsage::ManualFetch;

		Data.TangentBasisComponents[1].VertexBuffer = &TangentVBAlias;
		Data.TangentBasisComponents[1].Offset = 4;
		Data.TangentBasisComponents[1].Type = VET_PackedNormal;
		Data.TangentBasisComponents[1].Stride = 8;
		Data.TangentBasisComponents[1].VertexStreamUsage = EVertexStreamUsage::Overridden | EVertexStreamUsage::ManualFetch;
	}

	int32 PrevNumStreams = Streams.Num();
	UpdateRHI();

	// Verify no additional stream was created
	check(Streams.Num() == PrevNumStreams);
	// Find the added stream (usually at 0)
	PositionStreamIndex = -1;
	TangentStreamIndex = -1;
	for (int32 Index = 0; Index < Streams.Num(); ++Index)
	{
		if (Streams[Index].VertexBuffer->VertexBufferRHI.GetReference() == PositionRWBuffer->Buffer.GetReference())
		{
			PositionStreamIndex = Index;
		}

		if (TangentRWBuffer)
		{
			if (Streams[Index].VertexBuffer->VertexBufferRHI.GetReference() == TangentRWBuffer->Buffer.GetReference())
			{
				TangentStreamIndex = Index;
			}
		}
	}
	checkf(PositionStreamIndex != -1, TEXT("Unable to find stream for RWBuffer Vertex buffer!"));
}

FVertexFactoryShaderParameters* FGPUSkinPassthroughVertexFactory::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return (ShaderFrequency == SF_Vertex) ? new FGPUSkinVertexPassthroughFactoryShaderParameters() : nullptr;
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FGPUSkinPassthroughVertexFactory, "/Engine/Private/LocalVertexFactory.ush", true, false, true, false, false);

/*-----------------------------------------------------------------------------
TGPUSkinMorphVertexFactory
-----------------------------------------------------------------------------*/

/**
* Modify compile environment to enable the morph blend codepath
* @param OutEnvironment - shader compile environment to modify
*/
template <bool bExtraBoneInfluencesT>
void TGPUSkinMorphVertexFactory<bExtraBoneInfluencesT>::ModifyCompilationEnvironment( EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment )
{
	Super::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	OutEnvironment.SetDefine(TEXT("GPUSKIN_MORPH_BLEND"),TEXT("1"));
}

template <bool bExtraBoneInfluencesT>
bool TGPUSkinMorphVertexFactory<bExtraBoneInfluencesT>::ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return (Material->IsUsedWithMorphTargets() || Material->IsSpecialEngineMaterial()) 
		&& Super::ShouldCompilePermutation(Platform, Material, ShaderType);
}

/**
* Add the decl elements for the streams
* @param InData - type with stream components
* @param OutElements - vertex decl list to modify
*/
template <bool bExtraBoneInfluencesT>
void TGPUSkinMorphVertexFactory<bExtraBoneInfluencesT>::AddVertexElements(FDataType& InData, FVertexDeclarationElementList& OutElements)
{
	// add the base gpu skin elements
	TGPUSkinVertexFactory<bExtraBoneInfluencesT>::AddVertexElements(InData,OutElements);
	// add the morph delta elements
	OutElements.Add(FVertexFactory::AccessStreamComponent(InData.DeltaPositionComponent,9));
	OutElements.Add(FVertexFactory::AccessStreamComponent(InData.DeltaTangentZComponent,10));
}

/**
* Creates declarations for each of the vertex stream components and
* initializes the device resource
*/
template <bool bExtraBoneInfluencesT>
void TGPUSkinMorphVertexFactory<bExtraBoneInfluencesT>::InitRHI()
{
	// list of declaration items
	FVertexDeclarationElementList Elements;	
	AddVertexElements(MorphData,Elements);

	// create the actual device decls
	FVertexFactory::InitDeclaration(Elements);
}

template <bool bExtraBoneInfluencesT>
FVertexFactoryShaderParameters* TGPUSkinMorphVertexFactory<bExtraBoneInfluencesT>::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return ShaderFrequency == SF_Vertex ? new FGPUSkinVertexFactoryShaderParameters() : NULL;
}

/** bind morph target gpu skin vertex factory to its shader file and its shader parameters */
IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE(TGPUSkinMorphVertexFactory, "/Engine/Private/GpuSkinVertexFactory.ush", true, false, true, false, false);


/*-----------------------------------------------------------------------------
	TGPUSkinAPEXClothVertexFactoryShaderParameters
-----------------------------------------------------------------------------*/
/** Shader parameters for use with TGPUSkinAPEXClothVertexFactory */
class TGPUSkinAPEXClothVertexFactoryShaderParameters : public FGPUSkinVertexFactoryShaderParameters
{
public:

	/**
	* Bind shader constants by name
	* @param	ParameterMap - mapping of named shader constants to indices
	*/
	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		FGPUSkinVertexFactoryShaderParameters::Bind(ParameterMap);
		ClothSimulVertsPositionsNormalsParameter.Bind(ParameterMap,TEXT("ClothSimulVertsPositionsNormals"));
		PreviousClothSimulVertsPositionsNormalsParameter.Bind(ParameterMap,TEXT("PreviousClothSimulVertsPositionsNormals"));
		ClothLocalToWorldParameter.Bind(ParameterMap, TEXT("ClothLocalToWorld"));
		ClothBlendWeightParameter.Bind(ParameterMap, TEXT("ClothBlendWeight"));
		GPUSkinApexClothParameter.Bind(ParameterMap, TEXT("GPUSkinApexCloth"));
		GPUSkinApexClothStartIndexOffsetParameter.Bind(ParameterMap, TEXT("GPUSkinApexClothStartIndexOffset"));
	}
	/**
	* Serialize shader params to an archive
	* @param	Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar) override
	{ 
		FGPUSkinVertexFactoryShaderParameters::Serialize(Ar);
		Ar << ClothSimulVertsPositionsNormalsParameter;
		Ar << PreviousClothSimulVertsPositionsNormalsParameter;
		Ar << ClothLocalToWorldParameter;
		Ar << ClothBlendWeightParameter;
		Ar << GPUSkinApexClothParameter;
		Ar << GPUSkinApexClothStartIndexOffsetParameter;
	}

	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override
	{
		FRHIVertexShader* VertexShader = Shader->GetVertexShader();
		if (VertexShader)
		{
			// Call regular GPU skinning shader parameters
			FGPUSkinVertexFactoryShaderParameters::SetMesh(RHICmdList, Shader, VertexFactory, View, BatchElement, DataFlags);
			const auto* GPUSkinVertexFactory = (const FGPUBaseSkinVertexFactory*)VertexFactory;
			// A little hacky; problem is we can't upcast from FGPUBaseSkinVertexFactory to FGPUBaseSkinAPEXClothVertexFactory as they are unrelated; a nice solution would be
			// to use virtual inheritance, but that requires RTTI and complicates things further...
			const FGPUBaseSkinAPEXClothVertexFactory::ClothShaderType& ClothShaderData = GPUSkinVertexFactory->UsesExtraBoneInfluences()
				? ((const TGPUSkinAPEXClothVertexFactory<true>*)GPUSkinVertexFactory)->GetClothShaderData()
				: ((const TGPUSkinAPEXClothVertexFactory<false>*)GPUSkinVertexFactory)->GetClothShaderData();

			SetUniformBufferParameter(RHICmdList, VertexShader, Shader->GetUniformBufferParameter<FAPEXClothUniformShaderParameters>(),ClothShaderData.GetClothUniformBuffer());

			uint32 FrameNumber = View.Family->FrameNumber;

			// we tell the shader where to pickup the data
			if(ClothSimulVertsPositionsNormalsParameter.IsBound())
			{
				RHICmdList.SetShaderResourceViewParameter(VertexShader, ClothSimulVertsPositionsNormalsParameter.GetBaseIndex(),
														  ClothShaderData.GetClothBufferForReading(false, FrameNumber).VertexBufferSRV);
			}
			if(PreviousClothSimulVertsPositionsNormalsParameter.IsBound())
			{
				RHICmdList.SetShaderResourceViewParameter(VertexShader, PreviousClothSimulVertsPositionsNormalsParameter.GetBaseIndex(),
														  ClothShaderData.GetClothBufferForReading(true, FrameNumber).VertexBufferSRV);
			}
			
			SetShaderValue(
				RHICmdList,
				VertexShader,
				ClothLocalToWorldParameter,
				ClothShaderData.ClothLocalToWorld
				);

			SetShaderValue(
				RHICmdList,
				VertexShader,
				ClothBlendWeightParameter,
				ClothShaderData.ClothBlendWeight
				);

			if (GPUSkinApexClothParameter.IsBound())
			{
				RHICmdList.SetShaderResourceViewParameter(
					VertexShader,
					GPUSkinApexClothParameter.GetBaseIndex(),
					GPUSkinVertexFactory->UsesExtraBoneInfluences()
					? ((const TGPUSkinAPEXClothVertexFactory<true>*)GPUSkinVertexFactory)->GetClothBuffer()
					: ((const TGPUSkinAPEXClothVertexFactory<false>*)GPUSkinVertexFactory)->GetClothBuffer());
				int32 ClothIndexOffset =
					GPUSkinVertexFactory->UsesExtraBoneInfluences()
					? ((const TGPUSkinAPEXClothVertexFactory<true>*)GPUSkinVertexFactory)->GetClothIndexOffset(BatchElement.MinVertexIndex)
					: ((const TGPUSkinAPEXClothVertexFactory<false>*)GPUSkinVertexFactory)->GetClothIndexOffset(BatchElement.MinVertexIndex);
				FIntVector4 GPUSkinApexClothStartIndexOffset(BatchElement.MinVertexIndex, ClothIndexOffset, 0, 0);
				SetShaderValue(RHICmdList, VertexShader, GPUSkinApexClothStartIndexOffsetParameter, GPUSkinApexClothStartIndexOffset);
			}
		}
	}

protected:
	FShaderResourceParameter ClothSimulVertsPositionsNormalsParameter;
	FShaderResourceParameter PreviousClothSimulVertsPositionsNormalsParameter;
	FShaderParameter ClothLocalToWorldParameter;
	FShaderParameter ClothBlendWeightParameter;
	FShaderResourceParameter GPUSkinApexClothParameter;
	FShaderParameter GPUSkinApexClothStartIndexOffsetParameter;
};

/*-----------------------------------------------------------------------------
	TGPUSkinAPEXClothVertexFactory::ClothShaderType
-----------------------------------------------------------------------------*/

struct FRHICommandUpdateClothBuffer final : public FRHICommand<FRHICommandUpdateClothBuffer>
{
	FVertexBufferRHIParamRef VertexBuffer;
	uint32 BufferSize;
	const TArray<FVector>& SimulPositions;
	const TArray<FVector>& SimulNormals;


	FORCEINLINE_DEBUGGABLE FRHICommandUpdateClothBuffer(FVertexBufferRHIParamRef InVertexBuffer, uint32 InBufferSize, const TArray<FVector>& InSimulPositions, const TArray<FVector>& InSimulNormals)
		: VertexBuffer(InVertexBuffer)
		, BufferSize(InBufferSize)
		, SimulPositions(InSimulPositions)
		, SimulNormals(InSimulNormals)
	{
	}
	void Execute(FRHICommandListBase& CmdList)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FRHICommandUpdateBoneBuffer_Execute);
		float* RESTRICT Data = (float* RESTRICT)GDynamicRHI->RHILockVertexBuffer(VertexBuffer, 0, BufferSize, RLM_WriteOnly);
		uint32 NumSimulVerts = SimulPositions.Num();
		check(NumSimulVerts > 0 && NumSimulVerts <= MAX_APEXCLOTH_VERTICES_FOR_VB);
		float* RESTRICT Pos = (float* RESTRICT) &SimulPositions[0].X;
		float* RESTRICT Normal = (float* RESTRICT) &SimulNormals[0].X;
		for (uint32 Index = 0; Index < NumSimulVerts; Index++)
		{
			FPlatformMisc::Prefetch(Pos + PLATFORM_CACHE_LINE_SIZE);
			FPlatformMisc::Prefetch(Normal + PLATFORM_CACHE_LINE_SIZE);

			FMemory::Memcpy(Data, Pos, sizeof(float) * 3);
			FMemory::Memcpy(Data + 3, Normal, sizeof(float) * 3);
			Data += 6;
			Pos += 3;
			Normal += 3;
		}
		GDynamicRHI->RHIUnlockVertexBuffer(VertexBuffer);
	}
};

bool FGPUBaseSkinAPEXClothVertexFactory::ClothShaderType::UpdateClothSimulData(FRHICommandListImmediate& RHICmdList, const TArray<FVector>& InSimulPositions,
	const TArray<FVector>& InSimulNormals, uint32 FrameNumberToPrepare, ERHIFeatureLevel::Type FeatureLevel)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FGPUBaseSkinAPEXClothVertexFactory_UpdateClothSimulData);

	uint32 NumSimulVerts = InSimulPositions.Num();

	FVertexBufferAndSRV* CurrentClothBuffer = 0;

	if (FeatureLevel >= ERHIFeatureLevel::SM4)
	{
		check(IsInRenderingThread());
		
		CurrentClothBuffer = &GetClothBufferForWriting(FrameNumberToPrepare);

		NumSimulVerts = FMath::Min(NumSimulVerts, (uint32)MAX_APEXCLOTH_VERTICES_FOR_VB);

		uint32 VectorArraySize = NumSimulVerts * sizeof(float) * 6;
		uint32 PooledArraySize = ClothSimulDataBufferPool.PooledSizeForCreationArguments(VectorArraySize);
		if(!IsValidRef(*CurrentClothBuffer) || PooledArraySize != CurrentClothBuffer->VertexBufferRHI->GetSize())
		{
			if(IsValidRef(*CurrentClothBuffer))
			{
				ClothSimulDataBufferPool.ReleasePooledResource(*CurrentClothBuffer);
			}
			*CurrentClothBuffer = ClothSimulDataBufferPool.CreatePooledResource(VectorArraySize);
			check(IsValidRef(*CurrentClothBuffer));
		}

		if(NumSimulVerts)
		{
			if (DeferSkeletalLockAndFillToRHIThread())
			{
				new (RHICmdList.AllocCommand<FRHICommandUpdateClothBuffer>()) FRHICommandUpdateClothBuffer(CurrentClothBuffer->VertexBufferRHI, VectorArraySize, InSimulPositions, InSimulNormals);
				return true;
			}
			float* RESTRICT Data = (float* RESTRICT)RHILockVertexBuffer(CurrentClothBuffer->VertexBufferRHI, 0, VectorArraySize, RLM_WriteOnly);
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_FGPUBaseSkinAPEXClothVertexFactory_UpdateClothSimulData_CopyData);
				float* RESTRICT Pos = (float* RESTRICT) &InSimulPositions[0].X;
				float* RESTRICT Normal = (float* RESTRICT) &InSimulNormals[0].X;
				for (uint32 Index = 0; Index < NumSimulVerts; Index++)
				{
					FPlatformMisc::Prefetch(Pos + PLATFORM_CACHE_LINE_SIZE);
					FPlatformMisc::Prefetch(Normal + PLATFORM_CACHE_LINE_SIZE);

					FMemory::Memcpy(Data, Pos, sizeof(float) * 3);
					FMemory::Memcpy(Data + 3, Normal, sizeof(float) * 3);
					Data += 6;
					Pos += 3;
					Normal += 3;
				}
			}
			RHIUnlockVertexBuffer(CurrentClothBuffer->VertexBufferRHI);
		}
	}
	return false;
}

/*-----------------------------------------------------------------------------
	TGPUSkinAPEXClothVertexFactory
-----------------------------------------------------------------------------*/
TGlobalResource<FClothBufferPool> FGPUBaseSkinAPEXClothVertexFactory::ClothSimulDataBufferPool;

/**
* Modify compile environment to enable the apex clothing path
* @param OutEnvironment - shader compile environment to modify
*/
template <bool bExtraBoneInfluencesT>
void TGPUSkinAPEXClothVertexFactory<bExtraBoneInfluencesT>::ModifyCompilationEnvironment( EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment )
{
	Super::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	OutEnvironment.SetDefine(TEXT("GPUSKIN_APEX_CLOTH"),TEXT("1"));
}

template <bool bExtraBoneInfluencesT>
bool TGPUSkinAPEXClothVertexFactory<bExtraBoneInfluencesT>::ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return GetMaxSupportedFeatureLevel(Platform) >= ERHIFeatureLevel::SM4
		&& (Material->IsUsedWithAPEXCloth() || Material->IsSpecialEngineMaterial()) 
		&& Super::ShouldCompilePermutation(Platform, Material, ShaderType);
}

/**
* Add the decl elements for the streams
* @param InData - type with stream components
* @param OutElements - vertex decl list to modify
*/
template <bool bExtraBoneInfluencesT>
void TGPUSkinAPEXClothVertexFactory<bExtraBoneInfluencesT>::AddVertexElements(FDataType& InData, FVertexDeclarationElementList& OutElements)
{
	// add the base gpu skin elements
	TGPUSkinVertexFactory<bExtraBoneInfluencesT>::AddVertexElements(InData,OutElements);
	// add the morph delta elements
//	return;
	if(InData.CoordNormalComponent.VertexBuffer)
	{
		OutElements.Add(FVertexFactory::AccessStreamComponent(InData.CoordPositionComponent,9));
		OutElements.Add(FVertexFactory::AccessStreamComponent(InData.CoordNormalComponent,10));
		OutElements.Add(FVertexFactory::AccessStreamComponent(InData.CoordTangentComponent,11));
		OutElements.Add(FVertexFactory::AccessStreamComponent(InData.SimulIndicesComponent,12));
	}
}

/**
* Creates declarations for each of the vertex stream components and
* initializes the device resource
*/
template <bool bExtraBoneInfluencesT>
void TGPUSkinAPEXClothVertexFactory<bExtraBoneInfluencesT>::InitRHI()
{
	// list of declaration items
	FVertexDeclarationElementList Elements;	
	AddVertexElements(MeshMappingData,Elements);

	// create the actual device decls
	FVertexFactory::InitDeclaration(Elements);
}

template <bool bExtraBoneInfluencesT>
FVertexFactoryShaderParameters* TGPUSkinAPEXClothVertexFactory<bExtraBoneInfluencesT>::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return ShaderFrequency == SF_Vertex ? new TGPUSkinAPEXClothVertexFactoryShaderParameters() : NULL;
}

/** bind cloth gpu skin vertex factory to its shader file and its shader parameters */
IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE(TGPUSkinAPEXClothVertexFactory, "/Engine/Private/GpuSkinVertexFactory.ush", true, false, true, false, false);


#undef IMPLEMENT_GPUSKINNING_VERTEX_FACTORY_TYPE
