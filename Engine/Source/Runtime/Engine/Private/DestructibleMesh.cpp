// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DestructibleMesh.cpp: UDestructibleMesh methods.
=============================================================================*/

#include "EnginePrivate.h"
#include "PhysicsEngine/PhysXSupport.h"

#if WITH_APEX && WITH_EDITOR
#include "ApexDestructibleAssetImport.h"
#endif

DEFINE_LOG_CATEGORY(LogDestructible)

UDestructibleMesh::UDestructibleMesh(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UDestructibleMesh::PostLoad()
{
	Super::PostLoad();

	// BodySetup is used for uniform lookup of PhysicalMaterials.
	CreateBodySetup();

	// Fix location of destructible physical material
	if (GetLinker() && (GetLinker()->UE4Ver() < VER_UE4_DESTRUCTIBLE_MESH_BODYSETUP_HOLDS_PHYSICAL_MATERIAL))
	{
		if (BodySetup != NULL)
		{
			BodySetup->PhysMaterial = DestructiblePhysicalMaterial_DEPRECATED;
		}
	}

#if WITH_APEX
	// destructible mesh needs to re-evaluate the number of max gpu bone count for each chunk
	// we'll have to rechunk if that exceeds project setting
	// since this might not work well if outside of editor
	// you'll have to save re-chunked asset here, but I don't have a good way to let user
	// know here since PostLoad invalidate any dirty package mark. 
	FSkeletalMeshResource* ImportedResource = GetImportedResource();
	static IConsoleVariable* MaxBonesVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Compat.MAX_GPUSKIN_BONES"));
	const int32 MaxGPUSkinBones = MaxBonesVar->GetInt();
	// if this doesn't have the right MAX GPU Bone count, recreate it. 
	for(int32 LodIndex=0; LodIndex<LODInfo.Num(); LodIndex++)
	{
		FSkeletalMeshLODInfo& ThisLODInfo = LODInfo[LodIndex];
		FStaticLODModel& ThisLODModel = ImportedResource->LODModels[LodIndex];

		for (int32 ChunkIndex = 0; ChunkIndex < ThisLODModel.Chunks.Num(); ++ChunkIndex)
		{
			if (ThisLODModel.Chunks[ChunkIndex].BoneMap.Num() > MaxGPUSkinBones)
			{
#if WITH_EDITOR
				// re create destructible asset if it exceeds
				if (ApexDestructibleAsset != NULL)
				{
					SetApexDestructibleAsset(*this, *ApexDestructibleAsset, NULL, EImportOptions::PreserveSettings);
				}
#else
				UE_LOG(LogDestructible, Warning, TEXT("Can't render %s asset because it exceeds max GPU skin bones supported(%d). You'll need to resave this in the editor."), *GetName(), MaxGPUSkinBones);
#endif
				// we don't have to do this multiple times, if one failes, do it once, and get out
				break;
			}
		}
	}
#endif

}

#if WITH_EDITOR
void UDestructibleMesh::PreEditChange(UProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	CreateBodySetup();
}
#endif

void UDestructibleMesh::Serialize(FArchive& Ar)
{
	Super::Serialize( Ar );

	if( Ar.IsLoading() )
	{
		// Deserializing the name of the NxDestructibleAsset
		TArray<uint8> NameBuffer;
		uint32 NameBufferSize;
		Ar << NameBufferSize;
		NameBuffer.AddUninitialized( NameBufferSize );
		Ar.Serialize( NameBuffer.GetData(), NameBufferSize );

		// Buffer for the NxDestructibleAsset
		TArray<uint8> Buffer;
		uint32 Size;
		Ar << Size;
		if( Size > 0 )
		{
			// Size is non-zero, so a binary blob follows
			Buffer.AddUninitialized( Size );
			Ar.Serialize( Buffer.GetData(), Size );
#if WITH_APEX
			// Wrap this blob with the APEX read stream class
			physx::PxFileBuf* Stream = GApexSDK->createMemoryReadStream( Buffer.GetData(), Size );
			// Create an NxParameterized serializer
			NxParameterized::Serializer* Serializer = GApexSDK->createSerializer(NxParameterized::Serializer::NST_BINARY);
			// Deserialize into a DeserializedData buffer
			NxParameterized::Serializer::DeserializedData DeserializedData;
			Serializer->deserialize( *Stream, DeserializedData );
			NxApexAsset* ApexAsset = NULL;
			if( DeserializedData.size() > 0 )
			{
				// The DeserializedData has something in it, so create an APEX asset from it
				ApexAsset = GApexSDK->createAsset( DeserializedData[0], NULL );//(const char*)NameBuffer.GetData() );	// BRG - don't require it to have a given name
				// Make sure it's a Destructible asset
				if (ApexAsset && ApexAsset->getObjTypeID() != GApexModuleDestructible->getModuleID())
				{
					GPhysCommandHandler->DeferredRelease(ApexAsset);
					ApexAsset = NULL;
				}
			}
			ApexDestructibleAsset = (NxDestructibleAsset*)ApexAsset;
			// Release our temporary objects
			Serializer->release();
			GApexSDK->releaseMemoryReadStream( *Stream );

			// Fix chunk bounds which were not transformed upon import before version VER_UE4_NX_DESTRUCTIBLE_ASSET_CHUNK_BOUNDS_FIX
			if (ApexDestructibleAsset != NULL && Ar.UE4Ver() < VER_UE4_NX_DESTRUCTIBLE_ASSET_CHUNK_BOUNDS_FIX)
			{
				// Name buffer used for parameterized array element lookup
				char ArrayElementName[1024];

				// Get the NxParameterized interface to the asset
				NxParameterized::Interface* AssetParams = const_cast<NxParameterized::Interface*>(ApexDestructibleAsset->getAssetNxParameterized());
				if (AssetParams != NULL)
				{
					/* render mesh asset (bounding boxes only) */
					const physx::PxMat33 Basis = physx::PxMat33::createDiagonal(physx::PxVec3(1.0f, -1.0f, 1.0f));
					NxParameterized::Interface* RenderMeshAssetParams;
					const bool bFoundRMA = NxParameterized::getParamRef(*AssetParams, "renderMeshAsset", RenderMeshAssetParams);
					check(bFoundRMA);
					if (bFoundRMA)
					{
						physx::PxI32 PartBoundsCount;
						const bool bFoundBounds = NxParameterized::getParamArraySize(*RenderMeshAssetParams, "partBounds", PartBoundsCount);
						check(bFoundBounds);
						if (bFoundBounds)
						{
							for (physx::PxI32 i = 0; i < PartBoundsCount; ++i)
							{
								FCStringAnsi::Sprintf( ArrayElementName, "partBounds[%d]", i );
								NxParameterized::Handle PartBoundsHandle(*RenderMeshAssetParams);
								bool bFoundBoundsHandle = NxParameterized::findParam(*RenderMeshAssetParams, ArrayElementName, PartBoundsHandle) != NULL;
								check(bFoundBoundsHandle);
								if (bFoundBoundsHandle)
								{
									physx::PxBounds3 PartBounds;
									PartBoundsHandle.getParamBounds3( PartBounds );
									PartBounds = physx::PxBounds3::transformSafe(Basis, PartBounds);
									PartBoundsHandle.setParamBounds3( PartBounds );
								}
							}
						}
					}
				}
			}
#endif
		}
	}
	else if ( Ar.IsSaving() )
	{
		const char* Name = "NO_APEX";
#if WITH_APEX
		// Get our Destructible asset's name
		Name = NULL;
		if( ApexDestructibleAsset != NULL )
		{
			Name = ApexDestructibleAsset->getName();
		}
		if( Name == NULL )
		{
			Name = "";
		}
#endif
		// Serialize the name
		uint32 NameBufferSize = FCStringAnsi::Strlen( Name )+1;
		Ar << NameBufferSize;
		Ar.Serialize( (void*)Name, NameBufferSize );
#if WITH_APEX
		// Create an APEX write stream
		physx::PxFileBuf* Stream = GApexSDK->createMemoryWriteStream();
		// Create an NxParameterized serializer
		NxParameterized::Serializer* Serializer = GApexSDK->createSerializer(NxParameterized::Serializer::NST_BINARY);
		uint32 Size = 0;
		TArray<uint8> Buffer;
		// Get the NxParameterized data for our Destructible asset
		if( ApexDestructibleAsset != NULL )
		{
			const NxParameterized::Interface* AssetParameterized = ApexDestructibleAsset->getAssetNxParameterized();
			if( AssetParameterized != NULL )
			{
				// Serialize the data into the stream
				Serializer->serialize( *Stream, &AssetParameterized, 1 );
				// Read the stream data into our buffer for UE serialzation
				Size = Stream->getFileLength();
				Buffer.AddUninitialized( Size );
				Stream->read( Buffer.GetTypedData(), Size );
			}
		}
		Ar << Size;
		if ( Size > 0 )
		{
			Ar.Serialize( Buffer.GetData(), Size );
		}
		// Release our temporary objects
		Serializer->release();
		Stream->release();
#else
		uint32 size=0;
		Ar << size;
#endif
	}
}

void UDestructibleMesh::FinishDestroy()
{
#if WITH_APEX
	if (ApexDestructibleAsset != NULL)
	{
		GPhysCommandHandler->DeferredRelease(ApexDestructibleAsset);
		ApexDestructibleAsset = NULL;
	}
#endif

	Super::FinishDestroy();
}

#if WITH_APEX
NxParameterized::Interface* UDestructibleMesh::GetDestructibleActorDesc(UPhysicalMaterial* PhysMat)
{
	NxParameterized::Interface* Params = NULL;

	if (ApexDestructibleAsset != NULL)
	{
		Params = ApexDestructibleAsset->getDefaultActorDesc();
	}
	if (Params != NULL)
	{
		// Damage parameters
		verify( NxParameterized::setParamF32(*Params,"defaultBehaviorGroup.damageThreshold",				DefaultDestructibleParameters.DamageParameters.DamageThreshold * PhysMat->DestructibleDamageThresholdScale) );
		verify( NxParameterized::setParamF32(*Params,"defaultBehaviorGroup.damageToRadius",					DefaultDestructibleParameters.DamageParameters.DamageSpread) );
		verify( NxParameterized::setParamF32(*Params,"destructibleParameters.forceToDamage",				DefaultDestructibleParameters.DamageParameters.ImpactDamage) );
		verify( NxParameterized::setParamF32(*Params,"defaultBehaviorGroup.materialStrength",				DefaultDestructibleParameters.DamageParameters.ImpactResistance) );
		verify( NxParameterized::setParamI32(*Params,"destructibleParameters.impactDamageDefaultDepth",		DefaultDestructibleParameters.DamageParameters.DefaultImpactDamageDepth) );

		// Special depths
		verify( NxParameterized::setParamU32(*Params,"supportDepth",										DefaultDestructibleParameters.SpecialHierarchyDepths.SupportDepth) );
		verify( NxParameterized::setParamU32(*Params,"destructibleParameters.minimumFractureDepth",			DefaultDestructibleParameters.SpecialHierarchyDepths.MinimumFractureDepth) );
		verify( NxParameterized::setParamI32(*Params,"destructibleParameters.debrisDepth",					DefaultDestructibleParameters.SpecialHierarchyDepths.DebrisDepth) );
		verify( NxParameterized::setParamU32(*Params,"destructibleParameters.essentialDepth",				DefaultDestructibleParameters.SpecialHierarchyDepths.EssentialDepth) );

		// Advanced parameters
		verify( NxParameterized::setParamF32(*Params,"destructibleParameters.damageCap",					DefaultDestructibleParameters.AdvancedParameters.DamageCap) );
		verify( NxParameterized::setParamF32(*Params,"destructibleParameters.impactVelocityThreshold",		DefaultDestructibleParameters.AdvancedParameters.ImpactVelocityThreshold) );
		verify( NxParameterized::setParamF32(*Params,"destructibleParameters.maxChunkSpeed",				DefaultDestructibleParameters.AdvancedParameters.MaxChunkSpeed) );
		verify( NxParameterized::setParamF32(*Params,"destructibleParameters.fractureImpulseScale",			DefaultDestructibleParameters.AdvancedParameters.FractureImpulseScale) );

		// Debris parameters
		verify( NxParameterized::setParamF32(*Params,"destructibleParameters.debrisLifetimeMin",			DefaultDestructibleParameters.DebrisParameters.DebrisLifetimeMin) );
		verify( NxParameterized::setParamF32(*Params,"destructibleParameters.debrisLifetimeMax",			DefaultDestructibleParameters.DebrisParameters.DebrisLifetimeMax) );
		verify( NxParameterized::setParamF32(*Params,"destructibleParameters.debrisMaxSeparationMin",		DefaultDestructibleParameters.DebrisParameters.DebrisMaxSeparationMin) );
		verify( NxParameterized::setParamF32(*Params,"destructibleParameters.debrisMaxSeparationMax",		DefaultDestructibleParameters.DebrisParameters.DebrisMaxSeparationMax) );
		physx::PxBounds3 ValidBounds;
		ValidBounds.minimum.x = DefaultDestructibleParameters.DebrisParameters.ValidBounds.Min.X;
		ValidBounds.minimum.y = DefaultDestructibleParameters.DebrisParameters.ValidBounds.Min.Y;
		ValidBounds.minimum.z = DefaultDestructibleParameters.DebrisParameters.ValidBounds.Min.Z;															   
		ValidBounds.maximum.x = DefaultDestructibleParameters.DebrisParameters.ValidBounds.Max.X;
		ValidBounds.maximum.y = DefaultDestructibleParameters.DebrisParameters.ValidBounds.Max.Y;
		ValidBounds.maximum.z = DefaultDestructibleParameters.DebrisParameters.ValidBounds.Max.Z;
		verify( NxParameterized::setParamBounds3(*Params,"destructibleParameters.validBounds",					ValidBounds) );

		// Flags
		verify( NxParameterized::setParamBool(*Params,"destructibleParameters.flags.ACCUMULATE_DAMAGE",			DefaultDestructibleParameters.Flags.bAccumulateDamage) );
		verify( NxParameterized::setParamBool(*Params,"useAssetDefinedSupport",									DefaultDestructibleParameters.Flags.bAssetDefinedSupport) );
		verify( NxParameterized::setParamBool(*Params,"useWorldSupport",										DefaultDestructibleParameters.Flags.bWorldSupport) );
		verify( NxParameterized::setParamBool(*Params,"destructibleParameters.flags.DEBRIS_TIMEOUT",			DefaultDestructibleParameters.Flags.bDebrisTimeout) );
		verify( NxParameterized::setParamBool(*Params,"destructibleParameters.flags.DEBRIS_MAX_SEPARATION",		DefaultDestructibleParameters.Flags.bDebrisMaxSeparation) );
		verify( NxParameterized::setParamBool(*Params,"destructibleParameters.flags.CRUMBLE_SMALLEST_CHUNKS",	DefaultDestructibleParameters.Flags.bCrumbleSmallestChunks) );
		verify( NxParameterized::setParamBool(*Params,"destructibleParameters.flags.ACCURATE_RAYCASTS",			DefaultDestructibleParameters.Flags.bAccurateRaycasts) );
		verify( NxParameterized::setParamBool(*Params,"destructibleParameters.flags.USE_VALID_BOUNDS",			DefaultDestructibleParameters.Flags.bUseValidBounds) );
		verify( NxParameterized::setParamBool(*Params,"formExtendedStructures",									DefaultDestructibleParameters.Flags.bFormExtendedStructures) );

		// Depth parameters
		for (int32 Depth = 0; Depth < DefaultDestructibleParameters.DepthParameters.Num(); ++Depth)
		{
			char OverrideName[MAX_SPRINTF];
			FCStringAnsi::Sprintf(OverrideName, "depthParameters[%d].OVERRIDE_IMPACT_DAMAGE", Depth);
			char OverrideValueName[MAX_SPRINTF];
			FCStringAnsi::Sprintf(OverrideValueName, "depthParameters[%d].OVERRIDE_IMPACT_DAMAGE_VALUE", Depth);

			switch (DefaultDestructibleParameters.DepthParameters[Depth].ImpactDamageOverride)
			{
			case IDO_None:
				verify( NxParameterized::setParamBool(*Params, OverrideName, false) );
				break;
			case IDO_On:
				verify( NxParameterized::setParamBool(*Params, OverrideName, true) );
				verify( NxParameterized::setParamBool(*Params, OverrideValueName, true) );
				break;
			case IDO_Off:
				verify( NxParameterized::setParamBool(*Params, OverrideName, true) );
				verify( NxParameterized::setParamBool(*Params, OverrideValueName, false) );
				break;
			default:
				break;
			}
		}
	}

	return Params;
}
#endif // WITH_APEX


void UDestructibleMesh::LoadDefaultDestructibleParametersFromApexAsset()
{
#if WITH_APEX
	const NxParameterized::Interface* Params = ApexDestructibleAsset->getAssetNxParameterized();

	if (Params != NULL)
	{
		// Damage parameters
		verify( NxParameterized::getParamF32(*Params,"defaultBehaviorGroup.damageThreshold",				DefaultDestructibleParameters.DamageParameters.DamageThreshold) );
		verify( NxParameterized::getParamF32(*Params,"defaultBehaviorGroup.damageToRadius",				DefaultDestructibleParameters.DamageParameters.DamageSpread) );
		verify( NxParameterized::getParamF32(*Params,"destructibleParameters.forceToDamage",				DefaultDestructibleParameters.DamageParameters.ImpactDamage) );
		verify( NxParameterized::getParamF32(*Params,"defaultBehaviorGroup.materialStrength",				DefaultDestructibleParameters.DamageParameters.ImpactResistance) );
		verify( NxParameterized::getParamI32(*Params,"destructibleParameters.impactDamageDefaultDepth",		DefaultDestructibleParameters.DamageParameters.DefaultImpactDamageDepth) );

		// Special depths
		physx::PxU32 SupportDepth;
		verify( NxParameterized::getParamU32(*Params,"supportDepth",										SupportDepth) );
		DefaultDestructibleParameters.SpecialHierarchyDepths.SupportDepth = (int32)SupportDepth;
		physx::PxU32 MinimumFractureDepth;
		verify( NxParameterized::getParamU32(*Params,"destructibleParameters.minimumFractureDepth",			MinimumFractureDepth) );
		DefaultDestructibleParameters.SpecialHierarchyDepths.MinimumFractureDepth = (int32)MinimumFractureDepth;
		verify( NxParameterized::getParamI32(*Params,"destructibleParameters.debrisDepth",					DefaultDestructibleParameters.SpecialHierarchyDepths.DebrisDepth) );
		physx::PxU32 EssentialDepth;
		verify( NxParameterized::getParamU32(*Params,"destructibleParameters.essentialDepth",				EssentialDepth) );
		DefaultDestructibleParameters.SpecialHierarchyDepths.EssentialDepth = (int32)EssentialDepth;

		// Advanced parameters
		verify( NxParameterized::getParamF32(*Params,"destructibleParameters.damageCap",					DefaultDestructibleParameters.AdvancedParameters.DamageCap) );
		verify( NxParameterized::getParamF32(*Params,"destructibleParameters.impactVelocityThreshold",		DefaultDestructibleParameters.AdvancedParameters.ImpactVelocityThreshold) );
		verify( NxParameterized::getParamF32(*Params,"destructibleParameters.maxChunkSpeed",				DefaultDestructibleParameters.AdvancedParameters.MaxChunkSpeed) );
		verify( NxParameterized::getParamF32(*Params,"destructibleParameters.fractureImpulseScale",			DefaultDestructibleParameters.AdvancedParameters.FractureImpulseScale) );

		// Debris parameters
		verify( NxParameterized::getParamF32(*Params,"destructibleParameters.debrisLifetimeMin",			DefaultDestructibleParameters.DebrisParameters.DebrisLifetimeMin) );
		verify( NxParameterized::getParamF32(*Params,"destructibleParameters.debrisLifetimeMax",			DefaultDestructibleParameters.DebrisParameters.DebrisLifetimeMax) );
		verify( NxParameterized::getParamF32(*Params,"destructibleParameters.debrisMaxSeparationMin",		DefaultDestructibleParameters.DebrisParameters.DebrisMaxSeparationMin) );
		verify( NxParameterized::getParamF32(*Params,"destructibleParameters.debrisMaxSeparationMax",		DefaultDestructibleParameters.DebrisParameters.DebrisMaxSeparationMax) );
		physx::PxBounds3 ValidBounds;
		verify( NxParameterized::getParamBounds3(*Params,"destructibleParameters.validBounds",				ValidBounds) );
		DefaultDestructibleParameters.DebrisParameters.ValidBounds.Min.X = ValidBounds.minimum.x;
		DefaultDestructibleParameters.DebrisParameters.ValidBounds.Min.Y = ValidBounds.minimum.y;
		DefaultDestructibleParameters.DebrisParameters.ValidBounds.Min.Z = ValidBounds.minimum.z;															   
		DefaultDestructibleParameters.DebrisParameters.ValidBounds.Max.X = ValidBounds.maximum.x;
		DefaultDestructibleParameters.DebrisParameters.ValidBounds.Max.Y = ValidBounds.maximum.y;
		DefaultDestructibleParameters.DebrisParameters.ValidBounds.Max.Z = ValidBounds.maximum.z;

		// Flags
		bool bFlag;
		verify( NxParameterized::getParamBool(*Params,"destructibleParameters.flags.ACCUMULATE_DAMAGE",			bFlag) );
		DefaultDestructibleParameters.Flags.bAccumulateDamage = bFlag ? 1 : 0;
		verify( NxParameterized::getParamBool(*Params,"useAssetDefinedSupport",									bFlag) );
		DefaultDestructibleParameters.Flags.bAssetDefinedSupport = bFlag ? 1 : 0;
		verify( NxParameterized::getParamBool(*Params,"useWorldSupport",										bFlag) );
		DefaultDestructibleParameters.Flags.bWorldSupport = bFlag ? 1 : 0;
		verify( NxParameterized::getParamBool(*Params,"destructibleParameters.flags.DEBRIS_TIMEOUT",			bFlag) );
		DefaultDestructibleParameters.Flags.bDebrisTimeout = bFlag ? 1 : 0;
		verify( NxParameterized::getParamBool(*Params,"destructibleParameters.flags.DEBRIS_MAX_SEPARATION",		bFlag) );
		DefaultDestructibleParameters.Flags.bDebrisMaxSeparation = bFlag ? 1 : 0;
		verify( NxParameterized::getParamBool(*Params,"destructibleParameters.flags.CRUMBLE_SMALLEST_CHUNKS",	bFlag) );
		DefaultDestructibleParameters.Flags.bCrumbleSmallestChunks = bFlag ? 1 : 0;
		verify( NxParameterized::getParamBool(*Params,"destructibleParameters.flags.ACCURATE_RAYCASTS",			bFlag) );
		DefaultDestructibleParameters.Flags.bAccurateRaycasts = bFlag ? 1 : 0;
		verify( NxParameterized::getParamBool(*Params,"destructibleParameters.flags.USE_VALID_BOUNDS",			bFlag) );
		DefaultDestructibleParameters.Flags.bUseValidBounds = bFlag ? 1 : 0;
		verify( NxParameterized::getParamBool(*Params,"formExtendedStructures",									bFlag) );
		DefaultDestructibleParameters.Flags.bFormExtendedStructures = bFlag ? 1 : 0;

		// Depth parameters
		for (int32 Depth = 0; Depth < DefaultDestructibleParameters.DepthParameters.Num(); ++Depth)
		{
			char OverrideName[MAX_SPRINTF];
			FCStringAnsi::Sprintf(OverrideName, "depthParameters[%d].OVERRIDE_IMPACT_DAMAGE", Depth);
			char OverrideValueName[MAX_SPRINTF];
			FCStringAnsi::Sprintf(OverrideValueName, "depthParameters[%d].OVERRIDE_IMPACT_DAMAGE_VALUE", Depth);

			bool bOverride;
			bool bOverrideValue;
			verify( NxParameterized::getParamBool(*Params, OverrideName, bOverride) );
			verify( NxParameterized::getParamBool(*Params, OverrideValueName, bOverrideValue) );

			if (!bOverride)
			{
				DefaultDestructibleParameters.DepthParameters[Depth].ImpactDamageOverride = IDO_None;
			}
			else
			{
				DefaultDestructibleParameters.DepthParameters[Depth].ImpactDamageOverride = bOverrideValue ? IDO_On : IDO_Off;
			}
		}
	}
#endif
}

void UDestructibleMesh::CreateBodySetup()
{
	if (BodySetup == NULL)
	{
		BodySetup = ConstructObject<UBodySetup>(UBodySetup::StaticClass(), this);
	}
}

void UDestructibleMesh::CreateFractureSettings()
{
#if WITH_EDITORONLY_DATA
	if (FractureSettings == NULL)
	{
		FractureSettings = CastChecked<UDestructibleFractureSettings>(StaticConstructObject(UDestructibleFractureSettings::StaticClass(), this));
		check(FractureSettings);
	}
#endif	// WITH_EDITORONLY_DATA
}

#if WITH_APEX && WITH_EDITORONLY_DATA

bool CreateSubmeshFromSMSection(FStaticMeshLODResources& RenderMesh, int32 SubmeshIdx, FStaticMeshSection& Section, physx::NxExplicitSubmeshData& SubmeshData, TArray<physx::NxExplicitRenderTriangle>& Triangles)
{
	// Create submesh descriptor, just a material name and a vertex format
	FCStringAnsi::Strncpy(SubmeshData.mMaterialName, TCHAR_TO_ANSI(*FString::Printf(TEXT("Material%d"),Section.MaterialIndex)), physx::NxExplicitSubmeshData::MaterialNameBufferSize);
	SubmeshData.mVertexFormat.mHasStaticPositions = SubmeshData.mVertexFormat.mHasStaticNormals = SubmeshData.mVertexFormat.mHasStaticTangents = true;
	SubmeshData.mVertexFormat.mBonesPerVertex = 1;
	SubmeshData.mVertexFormat.mUVCount =  FMath::Min((physx::PxU32)RenderMesh.VertexBuffer.GetNumTexCoords(), (physx::PxU32)NxVertexFormat::MAX_UV_COUNT);

	const uint32 NumVertexColors = RenderMesh.ColorVertexBuffer.GetNumVertices();

	FIndexArrayView StaticMeshIndices = RenderMesh.IndexBuffer.GetArrayView();

	// Now pull in the triangles from this element, all belonging to this submesh
	const int32 TriangleCount = Section.NumTriangles;
	for (int32 TriangleIndex = 0; TriangleIndex < TriangleCount; ++TriangleIndex)
	{
		physx::NxExplicitRenderTriangle Triangle;
		for (int32 PointIndex = 0; PointIndex < 3; PointIndex++)
		{
			physx::NxVertex& Vertex = Triangle.vertices[PointIndex];
			const uint32 UnrealVertIndex = StaticMeshIndices[Section.FirstIndex + ((TriangleIndex * 3) + PointIndex)];
			Vertex.position = U2PVector(RenderMesh.PositionVertexBuffer.VertexPosition(UnrealVertIndex));	Vertex.position.y *= -1;
			Vertex.normal = U2PVector((FVector)RenderMesh.VertexBuffer.VertexTangentZ(UnrealVertIndex));	Vertex.normal.y *= -1;
			Vertex.tangent = U2PVector((FVector)RenderMesh.VertexBuffer.VertexTangentX(UnrealVertIndex));	Vertex.tangent.y *= -1;
			// Vertex.binormal = // Not recoding the bitangent/binormal
			for (int32 TexCoordSourceIndex = 0; TexCoordSourceIndex < (int32)SubmeshData.mVertexFormat.mUVCount; ++TexCoordSourceIndex)
			{
				const FVector2D& TexCoord = RenderMesh.VertexBuffer.GetVertexUV(UnrealVertIndex, TexCoordSourceIndex);
				Vertex.uv[TexCoordSourceIndex].set(TexCoord.X, -TexCoord.Y + 1.0);
			}
			FLinearColor VertColor(1.0f, 1.0f, 1.0f);
			if (UnrealVertIndex < NumVertexColors)
			{
				VertColor = RenderMesh.ColorVertexBuffer.VertexColor(UnrealVertIndex).ReinterpretAsLinear();
			}
			
			Vertex.color.set(VertColor.R, VertColor.G, VertColor.B, VertColor.A);
			// Don't need to write the bone index, since there is only one bone (index zero), and the default value is zero
			// Nor do we need to write the bone weights, since there is only one bone per vertex (the weight is assumed to be 1)
		}
		Triangle.submeshIndex = SubmeshIdx;
		Triangle.smoothingMask = 0;
		Triangle.extraDataIndex = 0xFFFFFFFF;
		Triangles.Add(Triangle);
	}

	return true;
}

#endif // WITH_APEX && WITH_EDITORONLY_DATA

bool UDestructibleMesh::BuildFractureSettingsFromStaticMesh(UStaticMesh* StaticMesh)
{
#if WITH_APEX && WITH_EDITORONLY_DATA
	// Make sure we have the authoring data container
	CreateFractureSettings();

	// Array of render meshes to create the fracture settings from. The first RenderMesh is the root mesh, while the other ones are 
	// imported to form the chunk meshes ( if no FBX was imported as level 1 chunks, this will just contain the root mesh )
	TArray<FStaticMeshLODResources*> RenderMeshes;
	TArray<UStaticMesh*> StaticMeshes;
	TArray<uint32> MeshPartitions;

	// Do we have a main static mesh?
	FStaticMeshLODResources& MainRenderMesh = StaticMesh->GetLODForExport(0);

	// Keep track of overall triangle and submesh count
	int32 OverallTriangleCount = MainRenderMesh.GetNumTriangles();
	int32 OverallSubmeshCount  = MainRenderMesh.Sections.Num();

	//MeshPartitions.Add(OverallTriangleCount);
		
 	RenderMeshes.Add(&MainRenderMesh);
 	StaticMeshes.Add(StaticMesh);
	
	UE_LOG(LogDestructible, Warning, TEXT("MainMesh Tris: %d"), MainRenderMesh.GetNumTriangles());

	// Get the rendermeshes for the chunks ( if any )
	for (int32 ChunkMeshIdx=0; ChunkMeshIdx < FractureChunkMeshes.Num(); ++ChunkMeshIdx)
	{
		FStaticMeshLODResources& ChunkRM = FractureChunkMeshes[ChunkMeshIdx]->GetLODForExport(0);
		
		RenderMeshes.Add(&ChunkRM);
		StaticMeshes.Add(FractureChunkMeshes[ChunkMeshIdx]);

		UE_LOG(LogDestructible, Warning, TEXT("Chunk: %d Tris: %d"), ChunkMeshIdx, ChunkRM.GetNumTriangles());

		OverallTriangleCount += ChunkRM.GetNumTriangles();
		OverallSubmeshCount  += ChunkRM.Sections.Num();
	}

	// Build an array of NxExplicitRenderTriangles and NxExplicitSubmeshData for authoring
	TArray<physx::NxExplicitRenderTriangle> Triangles;
	Triangles.Reserve(OverallTriangleCount);
	TArray<physx::NxExplicitSubmeshData> Submeshes;	// Elements <-> Submeshes
	Submeshes.AddUninitialized(OverallSubmeshCount);

	// UE Materials
	TArray<UMaterialInterface*> Materials;

	// Counter for submesh index
	int32 SubmeshIndexCounter = 0;

	

	// Start creating apex data
	for (int32 MeshIdx=0; MeshIdx < RenderMeshes.Num(); ++MeshIdx)
	{
		FStaticMeshLODResources& RenderMesh = *RenderMeshes[MeshIdx];

		// Get the current static mesh to retrieve the material from
		UStaticMesh* CurrentStaticMesh = StaticMeshes[MeshIdx];

		// Loop through elements
		for (int32 SectionIndex = 0; SectionIndex < RenderMesh.Sections.Num(); ++SectionIndex, ++SubmeshIndexCounter)
		{
			FStaticMeshSection& Section = RenderMesh.Sections[SectionIndex];
			physx::NxExplicitSubmeshData& SubmeshData = Submeshes[SubmeshIndexCounter];

			// Parallel materials array
			Materials.Add(CurrentStaticMesh->GetMaterial(Section.MaterialIndex));

			CreateSubmeshFromSMSection(RenderMesh, SubmeshIndexCounter, Section, SubmeshData, Triangles);
		}

		//if (MeshIdx > 0)
		{
			MeshPartitions.Add(Triangles.Num());
		}
	}

	FractureSettings->SetRootMesh(Triangles, Materials, Submeshes, MeshPartitions, true);
	return true;
#else // WITH_APEX && WITH_EDITORONLY_DATA
	return false;
#endif // WITH_APEX && WITH_EDITORONLY_DATA
}

ENGINE_API bool UDestructibleMesh::BuildFromStaticMesh( UStaticMesh& StaticMesh )
{
#if WITH_EDITORONLY_DATA
	PreEditChange(NULL);

	// Import the StaticMesh
	if (!BuildFractureSettingsFromStaticMesh(&StaticMesh))
	{
		return false;
	}

	SourceStaticMesh = &StaticMesh;
	if ( !FDateTime::Parse(StaticMesh.AssetImportData->SourceFileTimestamp, SourceSMImportTimestamp) )
	{
		SourceSMImportTimestamp = FDateTime::MinValue();
	}

#if WITH_APEX
	BuildDestructibleMeshFromFractureSettings(*this, NULL);
#endif

	PostEditChange();
	MarkPackageDirty();
#endif // WITH_EDITORONLY_DATA
	return true;
}

ENGINE_API bool UDestructibleMesh::SetupChunksFromStaticMeshes( const TArray<UStaticMesh*>& ChunkMeshes )
{
#if WITH_EDITORONLY_DATA
	if (SourceStaticMesh == NULL)
	{
		UE_LOG(LogDestructible, Warning, TEXT("Unable to import FBX as level 1 chunks if the DM was not created from a static mesh."));
		return false;
	}

	PreEditChange(NULL);

	FractureChunkMeshes.Empty(ChunkMeshes.Num());
	FractureChunkMeshes.Append(ChunkMeshes);
	
	// Import the StaticMesh
	if (!BuildFractureSettingsFromStaticMesh(SourceStaticMesh))
	{
		return false;
	}

#if WITH_APEX
	BuildDestructibleMeshFromFractureSettings(*this, NULL);
#endif

	// Clear the fracture chunk meshes again
	FractureChunkMeshes.Empty();

	PostEditChange();
	MarkPackageDirty();
#endif // WITH_EDITORONLY_DATA
	return true;
}
