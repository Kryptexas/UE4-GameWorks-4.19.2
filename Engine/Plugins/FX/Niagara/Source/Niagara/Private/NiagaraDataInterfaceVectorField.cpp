// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataInterfaceVectorField.h"
#include "Internationalization.h"
#include "VectorField/VectorFieldStatic.h"
#include "VectorField/VectorFieldAnimated.h"
#include "Math/Float16Color.h"
#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceVectorField"


#if 0
//////////////////////////////////////////////////////////////////////////
//FNDIVectorField_InstanceData

bool FNDIVectorField_InstanceData::Init(UNiagaraDataInterfaceVectorField* Interface, FNiagaraSystemInstance* SystemInstance)
{
	UE_LOG(LogNiagara, Log, TEXT("FNDIVectorField_InstanceData %p"), this);
	check(SystemInstance);
	/*
	USkeletalMesh* PrevMesh = Mesh;
	Component = nullptr;
	Mesh = nullptr;
	Transform = FMatrix::Identity;
	TransformInverseTransposed = FMatrix::Identity;
	PrevTransform = FMatrix::Identity;
	PrevTransformInverseTransposed = FMatrix::Identity;
	*/
	DeltaSeconds = 0.0f;
	Field = Interface->Field;

	UVectorFieldStatic* StaticVectorField = Cast<UVectorFieldStatic>(Field);
	UVectorFieldAnimated* AnimatedVectorField = Cast<UVectorFieldAnimated>(Field);
	SizeX = 0;
	SizeY = 0;
	SizeZ = 0;

	/*
	OutParameters.WorldToVolume[Index] = VectorFieldInstance->WorldToVolume;
	OutParameters.VolumeToWorld[Index] = VectorFieldInstance->VolumeToWorldNoScale;
	OutParameters.VolumeSize[Index] = FVector4(Resource->SizeX, Resource->SizeY, Resource->SizeZ, 0);
	OutParameters.IntensityAndTightness[Index] = FVector4(Intensity, Tightness, 0, 0);
	OutParameters.TilingAxes[Index].X = VectorFieldInstance->bTileX ? 1.0f : 0.0f;
	OutParameters.TilingAxes[Index].Y = VectorFieldInstance->bTileY ? 1.0f : 0.0f;
	OutParameters.TilingAxes[Index].Z = VectorFieldInstance->bTileZ ? 1.0f : 0.0f;
	*/
	TilingAxes.X = Interface->bTileX ? 1.0f : 0.0f;
	TilingAxes.Y = Interface->bTileY ? 1.0f : 0.0f;
	TilingAxes.Z = Interface->bTileZ ? 1.0f : 0.0f;


	if (StaticVectorField)
	{
		SizeX = (uint32)StaticVectorField->SizeX;
		SizeY = (uint32)StaticVectorField->SizeY;
		SizeZ = (uint32)StaticVectorField->SizeZ;
		LocalBounds = StaticVectorField->Bounds;
	}
	else if (AnimatedVectorField)
	{
		SizeX = (uint32)AnimatedVectorField->VolumeSizeX;
		SizeY = (uint32)AnimatedVectorField->VolumeSizeY;
		SizeZ = (uint32)AnimatedVectorField->VolumeSizeZ;
		LocalBounds = AnimatedVectorField->Bounds;
	}
#if 0
	USkeletalMeshComponent* NewSkelComp = nullptr;
	if (Interface->Source)
	{
		ASkeletalMeshActor* MeshActor = Cast<ASkeletalMeshActor>(Interface->Source);
		USkeletalMeshComponent* SourceComp = nullptr;
		if (MeshActor != nullptr)
		{
			SourceComp = MeshActor->GetSkeletalMeshComponent();
		}
		else
		{
			SourceComp = Interface->Source->FindComponentByClass<USkeletalMeshComponent>();
		}

		if (SourceComp)
		{
			Mesh = SourceComp->SkeletalMesh;
			NewSkelComp = SourceComp;
		}
		else
		{
			Component = Interface->Source->GetRootComponent();
		}
	}
	else
	{
		if (UNiagaraComponent* SimComp = SystemInstance->GetComponent())
		{
			if (USkeletalMeshComponent* ParentComp = Cast<USkeletalMeshComponent>(SimComp->GetAttachParent()))
			{
				NewSkelComp = ParentComp;
				Mesh = ParentComp->SkeletalMesh;
			}
			else if (USkeletalMeshComponent* OuterComp = SimComp->GetTypedOuter<USkeletalMeshComponent>())
			{
				NewSkelComp = OuterComp;
				Mesh = OuterComp->SkeletalMesh;
			}
			else if (AActor* Owner = SimComp->GetAttachmentRootActor())
			{
				TArray<UActorComponent*> SourceComps = Owner->GetComponentsByClass(USkeletalMeshComponent::StaticClass());
				for (UActorComponent* ActorComp : SourceComps)
				{
					USkeletalMeshComponent* SourceComp = Cast<USkeletalMeshComponent>(ActorComp);
					if (SourceComp)
					{
						USkeletalMesh* PossibleMesh = SourceComp->SkeletalMesh;
						if (PossibleMesh != nullptr/* && PossibleMesh->bAllowCPUAccess*/)
						{
							Mesh = PossibleMesh;
							NewSkelComp = SourceComp;
							break;
						}
					}
				}
			}

			if (!Component.IsValid())
			{
				Component = SimComp;
			}
		}
	}

	if (NewSkelComp)
	{
		Component = NewSkelComp;
	}

	check(Component.IsValid());

	if (!Mesh && Interface->DefaultMesh)
	{
		Mesh = Interface->DefaultMesh;
	}

	if (Component.IsValid() && Mesh)
	{
		PrevTransform = Transform;
		PrevTransformInverseTransposed = TransformInverseTransposed;
		Transform = Component->GetComponentToWorld().ToMatrixWithScale();
		TransformInverseTransposed = Transform.InverseFast().GetTransposed();
	}

	if (!Mesh)
	{
		UE_LOG(LogNiagara, Log, TEXT("SkeletalMesh data interface has no valid mesh. Failed InitPerInstanceData - %s"), *Interface->GetFullName());
		return false;
	}

	// 	if (!Mesh->bAllowCPUAccess)
	// 	{
	// 		UE_LOG(LogNiagara, Log, TEXT("SkeletalMesh data interface using a mesh that does not allow CPU access. Failed InitPerInstanceData - Mesh: %s"), *Mesh->GetFullName());
	// 		return false;
	// 	}

	if (!Component.IsValid())
	{
		UE_LOG(LogNiagara, Log, TEXT("SkeletalMesh data interface has no valid component. Failed InitPerInstanceData - %s"), *Interface->GetFullName());
		return false;
	}

	//	bIsAreaWeightedSampling = Mesh->bSupportUniformlyDistributedSampling;

	// 	//Init the instance filter
	// 	ValidSections.Empty();
	// 	FStaticMeshLODResources& Res = Mesh->RenderData->LODResources[0];
	// 	for (int32 i = 0; i < Res.Sections.Num(); ++i)
	// 	{
	// 		if (Interface->SectionFilter.AllowedMaterialSlots.Num() == 0 || Interface->SectionFilter.AllowedMaterialSlots.Contains(Res.Sections[i].MaterialIndex))
	// 		{
	// 			ValidSections.Add(i);
	// 		}
	// 	}
	// 
	// 	if (GetValidSections().Num() == 0)
	// 	{
	// 		UE_LOG(LogNiagara, Log, TEXT("StaticMesh data interface has a section filter preventing any spawning. Failed InitPerInstanceData - %s"), *Interface->GetFullName());
	// 		return false;
	// 	}
	// 
	// 	Sampler.Init(&Res, this);

	if (NewSkelComp)
	{
		TWeakObjectPtr<USkeletalMeshComponent> SkelWeakCompPtr = NewSkelComp;
		SkinningData = SystemInstance->GetWorldManager()->GetSkeletalMeshGeneratedData().GetCachedSkinningData(SkelWeakCompPtr);
	}
#endif

	return true;
}

void FNDIVectorField_InstanceData::UpdateTransforms(const FMatrix& LocalToWorld, FMatrix& OutVolumeToWorld, FMatrix& OutWorldToVolume)
{
	const FVector VolumeOffset = LocalBounds.Min;
	const FVector VolumeScale = LocalBounds.Max - LocalBounds.Min;
	//VolumeToWorldNoScale = LocalToWorld.GetMatrixWithoutScale().RemoveTranslation();
	OutVolumeToWorld = FScaleMatrix(VolumeScale) * FTranslationMatrix(VolumeOffset)
		* LocalToWorld;
	OutWorldToVolume = OutVolumeToWorld.InverseFast();
}

const void* FNDIVectorField_InstanceData::Lock()
{
	UVectorFieldStatic* StaticVectorField = Cast<UVectorFieldStatic>(Field);
	UVectorFieldAnimated* AnimatedVectorField = Cast<UVectorFieldAnimated>(Field);

	if (StaticVectorField)
	{
		return StaticVectorField->SourceData.LockReadOnly();
	}
	return nullptr;
}

void FNDIVectorField_InstanceData::Unlock()
{
	UVectorFieldStatic* StaticVectorField = Cast<UVectorFieldStatic>(Field);
	UVectorFieldAnimated* AnimatedVectorField = Cast<UVectorFieldAnimated>(Field);

	if (StaticVectorField)
	{
		StaticVectorField->SourceData.Unlock();
	}
}


bool FNDIVectorField_InstanceData::ResetRequired(UNiagaraDataInterfaceVectorField* Interface)const
{
	//USceneComponent* Comp = Component.Get();
	//if (!Comp)
	//{
	//	//The component we were bound to is no longer valid so we have to trigger a reset.
	//	return true;
	//}

	//if (USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(Comp))
	//{
	//	if (!SkelComp->SkeletalMesh)
	//	{
	//		return true;
	//	}
	//}
	//else
	//{
		if (!Interface->Field || Interface->Field != Field)
		{
			return true;
		}
	//}

	return false;
}

bool FNDIVectorField_InstanceData::Tick(UNiagaraDataInterfaceVectorField* Interface, FNiagaraSystemInstance* SystemInstance, float InDeltaSeconds)
{
	if (ResetRequired(Interface))
	{
		return true;
	}
	else
	{
		DeltaSeconds = InDeltaSeconds;
		/*if (Component.IsValid() && Mesh)
		{
			PrevTransform = Transform;
			PrevTransformInverseTransposed = TransformInverseTransposed;
			Transform = Component->GetComponentToWorld().ToMatrixWithScale();
			TransformInverseTransposed = Transform.InverseFast().GetTransposed();
		}
		else
		{
			PrevTransform = FMatrix::Identity;
			PrevTransformInverseTransposed = FMatrix::Identity;
			Transform = FMatrix::Identity;
			TransformInverseTransposed = FMatrix::Identity;
		}*/
		return false;
	}
}
#endif

//Instance Data END
//////////////////////////////////////////////////////////////////////////
UNiagaraDataInterfaceVectorField::UNiagaraDataInterfaceVectorField(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
	, Field(nullptr)
	, bTileX(false)
	, bTileY(false)
	, bTileZ(false)
	, bGPUBufferDirty(true)
{
}

#if WITH_EDITOR

void UNiagaraDataInterfaceVectorField::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	InitField();
}

void UNiagaraDataInterfaceVectorField::PostLoad()
{
	Super::PostLoad();
	if (Field)
	{
		Field->ConditionalPostLoad();
	}
	InitField();
}

void UNiagaraDataInterfaceVectorField::PostInitProperties()
{
	Super::PostInitProperties();

	//Can we register data interfaces as regular types and fold them into the FNiagaraVariable framework for UI and function calls etc?
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
	}
}

#endif //WITH_EDITOR

static const FName SampleVectorFieldName("SampleField");
static const FName GetVectorDimsName("FieldDimensions");
static const FName GetVectorFieldBoundsName("FieldBounds");

void UNiagaraDataInterfaceVectorField::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = SampleVectorFieldName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Vector Field")));
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Sample Point")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Sampled Value")));

		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = GetVectorDimsName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Vector Field")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Dimensions")));

		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = GetVectorFieldBoundsName;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Vector Field")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("BoundMin")));
		Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("BoundMax")));

		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		OutFunctions.Add(Sig);
	}
}

DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceVectorField, SampleVectorField);
FVMExternalFunction UNiagaraDataInterfaceVectorField::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData)
{
	FVMExternalFunction Function;
	if (BindingInfo.Name == SampleVectorFieldName && BindingInfo.GetNumInputs() == 3 && BindingInfo.GetNumOutputs() == 3)
	{
		Function = TNDIParamBinder<0, float, TNDIParamBinder<1, float, TNDIParamBinder<2, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceVectorField, SampleVectorField)>>>::Bind(this, BindingInfo, InstanceData);
	}
	else if (BindingInfo.Name == GetVectorDimsName && BindingInfo.GetNumInputs() == 0 && BindingInfo.GetNumOutputs() == 3)
	{
		Function = FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterfaceVectorField::GetFieldDimensions);
	}
	else if (BindingInfo.Name == GetVectorFieldBoundsName && BindingInfo.GetNumInputs() == 0 && BindingInfo.GetNumOutputs() == 6)
	{
		Function = FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterfaceVectorField::GetFieldBounds);
	}
	return Function;
}

void UNiagaraDataInterfaceVectorField::GetFieldDimensions(FVectorVMContext& Context)
{
	/*FUserPtrHandler<FNDIVectorField_InstanceData> InstData(Context);
	if (!InstData.Get())
	{
		UE_LOG(LogNiagara, Warning, TEXT("FNDIVectorField_InstanceData has invalid instance data. %s"), *GetPathName());
	}*/

	FRegisterHandler<float> OutSizeX(Context);	
	FRegisterHandler<float> OutSizeY(Context);
	FRegisterHandler<float> OutSizeZ(Context);
	
	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		*OutSizeX.GetDest() = (float)SizeX;		
		*OutSizeY.GetDest() = (float)SizeY;
		*OutSizeZ.GetDest() = (float)SizeZ;

		OutSizeX.Advance();
		OutSizeY.Advance();
		OutSizeZ.Advance();
	}
}

void UNiagaraDataInterfaceVectorField::GetFieldBounds(FVectorVMContext& Context)
{
	/*FUserPtrHandler<FNDIVectorField_InstanceData> InstData(Context);
	if (!InstData.Get())
	{
		UE_LOG(LogNiagara, Warning, TEXT("FNDIVectorField_InstanceData has invalid instance data. %s"), *GetPathName());
	}*/

	FRegisterHandler<float> OutMinX(Context);
	FRegisterHandler<float> OutMinY(Context);
	FRegisterHandler<float> OutMinZ(Context);
	FRegisterHandler<float> OutMaxX(Context);
	FRegisterHandler<float> OutMaxY(Context);
	FRegisterHandler<float> OutMaxZ(Context);
	
	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		*OutMinX.GetDest() = LocalBounds.Min.X;
		*OutMinY.GetDest() = LocalBounds.Min.Y;
		*OutMinZ.GetDest() = LocalBounds.Min.Z;
		*OutMaxX.GetDest() = LocalBounds.Max.X;
		*OutMaxY.GetDest() = LocalBounds.Max.Y;
		*OutMaxZ.GetDest() = LocalBounds.Max.Z;

		OutMinX.Advance();
		OutMinY.Advance();
		OutMinZ.Advance();
		OutMaxX.Advance();
		OutMaxY.Advance();
		OutMaxZ.Advance();
	}
}

/*
* Compute the influence of vector fields on a particle at the given position.
* @param OutForce - Force to apply to the particle.
* @param OutVelocity - Direct velocity influence on the particle.
* @param Position - Position of the particle.
* @param PerParticleScale - Amount by which to scale the influence on this particle.
void EvaluateVectorFields(out float3 OutForce, out float4 OutVelocity, float3 Position, float PerParticleScale)
{
	float3 TotalForce = 0;
	float3 WeightedVelocity = 0;
	float TotalWeight = 0;
	float FinalWeight = 0;

	for (int VectorFieldIndex = 0; VectorFieldIndex < VectorFields.Count; ++VectorFieldIndex)
	{
		float2 IntensityAndTightness = VectorFields.IntensityAndTightness[VectorFieldIndex].xy;
		float Intensity = IntensityAndTightness.x * PerParticleScale;
		float Tightness = IntensityAndTightness.y;
		float3 VolumeSize = VectorFields.VolumeSize[VectorFieldIndex].xyz;
		float3 VolumeUV = mul(float4(Position.xyz, 1), VectorFields.WorldToVolume[VectorFieldIndex]).xyz;
		//Tile the UVs if needed. TilingAxes will be 1.0 or 0.0 in each channel depending on which axes are being tiled, if any.
		VolumeUV -= floor(VolumeUV * VectorFields.TilingAxes[VectorFieldIndex].xyz);

		float3 AxisWeights =
			saturate(VolumeUV * VolumeSize.xyz) *
			saturate((1.0f - VolumeUV) * VolumeSize.xyz);
		float DistanceWeight = min(AxisWeights.x, min(AxisWeights.y, AxisWeights.z));

		// @todo compat hack: Some compilers only allow constant indexing into a texture array
		//		float3 VectorSample = Texture3DSample(VectorFieldTextures[VectorFieldIndex], VectorFieldTexturesSampler, saturate(VolumeUV)).xyz;
		float3 VectorSample = SampleVectorFieldTexture(VectorFieldIndex, saturate(VolumeUV));

		float3 Vec = mul(float4(VectorSample, 0), VectorFields.VolumeToWorld[VectorFieldIndex]).xyz;
		TotalForce += (Vec * DistanceWeight * Intensity);
		WeightedVelocity += (Vec * Intensity * DistanceWeight * Tightness);
		TotalWeight += (DistanceWeight * Tightness);
		FinalWeight = max(FinalWeight, DistanceWeight * Tightness);
	}

	// Forces are additive.
	OutForce = TotalForce;
	// Velocities use a weighted average.
	OutVelocity.xyz = WeightedVelocity / (TotalWeight + 0.001f);
	OutVelocity.w = FinalWeight;
}
*/

void UNiagaraDataInterfaceVectorField::InitField()
{
	UVectorFieldStatic* StaticVectorField = Cast<UVectorFieldStatic>(Field);
	UVectorFieldAnimated* AnimatedVectorField = Cast<UVectorFieldAnimated>(Field);
	SizeX = 0;
	SizeY = 0;
	SizeZ = 0;

	/*
	OutParameters.WorldToVolume[Index] = VectorFieldInstance->WorldToVolume;
	OutParameters.VolumeToWorld[Index] = VectorFieldInstance->VolumeToWorldNoScale;
	OutParameters.VolumeSize[Index] = FVector4(Resource->SizeX, Resource->SizeY, Resource->SizeZ, 0);
	OutParameters.IntensityAndTightness[Index] = FVector4(Intensity, Tightness, 0, 0);
	OutParameters.TilingAxes[Index].X = VectorFieldInstance->bTileX ? 1.0f : 0.0f;
	OutParameters.TilingAxes[Index].Y = VectorFieldInstance->bTileY ? 1.0f : 0.0f;
	OutParameters.TilingAxes[Index].Z = VectorFieldInstance->bTileZ ? 1.0f : 0.0f;
	*/
	TilingAxes.X = bTileX ? 1.0f : 0.0f;
	TilingAxes.Y = bTileY ? 1.0f : 0.0f;
	TilingAxes.Z = bTileZ ? 1.0f : 0.0f;


	if (StaticVectorField)
	{
		SizeX = (uint32)StaticVectorField->SizeX;
		SizeY = (uint32)StaticVectorField->SizeY;
		SizeZ = (uint32)StaticVectorField->SizeZ;
		LocalBounds = StaticVectorField->Bounds;
	}
	else if (AnimatedVectorField)
	{
		SizeX = (uint32)AnimatedVectorField->VolumeSizeX;
		SizeY = (uint32)AnimatedVectorField->VolumeSizeY;
		SizeZ = (uint32)AnimatedVectorField->VolumeSizeZ;
		LocalBounds = AnimatedVectorField->Bounds;
	}
}

const void* UNiagaraDataInterfaceVectorField::Lock()
{
	UVectorFieldStatic* StaticVectorField = Cast<UVectorFieldStatic>(Field);
	UVectorFieldAnimated* AnimatedVectorField = Cast<UVectorFieldAnimated>(Field);

	if (StaticVectorField)
	{
		return StaticVectorField->SourceData.LockReadOnly();
	}
	return nullptr;
}

void UNiagaraDataInterfaceVectorField::Unlock()
{
	UVectorFieldStatic* StaticVectorField = Cast<UVectorFieldStatic>(Field);
	UVectorFieldAnimated* AnimatedVectorField = Cast<UVectorFieldAnimated>(Field);

	if (StaticVectorField)
	{
		StaticVectorField->SourceData.Unlock();
	}
}


template<typename XType, typename YType, typename ZType>
void UNiagaraDataInterfaceVectorField::SampleVectorField(FVectorVMContext& Context)
{
	// Input arguments...
	XType XParam(Context);
	YType YParam(Context);
	ZType ZParam(Context);

	// User pointer buffer (as requested)
	//FUserPtrHandler<FNDIVectorField_InstanceData> InstData(Context);

	// Outputs...
	FRegisterHandler<float> OutSampleX(Context);
	FRegisterHandler<float> OutSampleY(Context);
	FRegisterHandler<float> OutSampleZ(Context);

	/*UE_LOG(LogNiagara, Log, TEXT("Registers: In: %d %d %d %d Out: %d %d %d"),
		XParam.RegisterIndex, YParam.RegisterIndex, ZParam.RegisterIndex, InstData.RegisterIndex,
		OutSampleX.RegisterIndex, OutSampleY.RegisterIndex, OutSampleZ.RegisterIndex);*/

	UVectorFieldStatic* StaticVectorField = Cast<UVectorFieldStatic>(Field);
	UVectorFieldAnimated* AnimatedVectorField = Cast<UVectorFieldAnimated>(Field);

	//FMatrix VolumeToWorld;
	//FMatrix WorldToVolume;
	//FMatrix LocalToWorld = FMatrix::Identity;
	//InstData->UpdateTransforms(LocalToWorld, VolumeToWorld, WorldToVolume);

	const FFloat16Color* Data = (const FFloat16Color*)Lock();

	if (Data == nullptr || Field == nullptr || (StaticVectorField == nullptr && AnimatedVectorField == nullptr) || AnimatedVectorField != nullptr)
	{
		// Set the default vector to positive X axis...
		for (int32 InstanceIdx = 0; InstanceIdx < Context.NumInstances; ++InstanceIdx)
		{
			VectorRegister Dst = MakeVectorRegister(1.0f, 0.0f, 0.0f, 0.0f);

			float *RegPtr = reinterpret_cast<float*>(&Dst);
			*OutSampleX.GetDest() = RegPtr[0];
			*OutSampleY.GetDest() = RegPtr[1];
			*OutSampleZ.GetDest() = RegPtr[2];

			XParam.Advance();
			YParam.Advance();
			ZParam.Advance();
			OutSampleX.Advance();
			OutSampleY.Advance();
			OutSampleZ.Advance();
		}
	}
	else if (StaticVectorField != nullptr)
	{
		FVector VolumeSize((float)SizeX, (float)SizeY, (float)SizeZ);
		uint32 VolumeSizeX = (uint32)SizeX;
		uint32 VolumeSizeY = (uint32)SizeY;
		uint32 VolumeSizeZ = (uint32)SizeZ;

		for (int32 InstanceIdx = 0; InstanceIdx < Context.NumInstances; ++InstanceIdx)
		{
			FVector Pos(XParam.Get(), YParam.Get(), ZParam.Get());
			
			// float3 VolumeUV = mul(float4(Position.xyz, 1), VectorFields.WorldToVolume[VectorFieldIndex]).xyz;
			//FNDITransformHandler Handler;
			FVector VolumeUV = Pos;
			//Handler.TransformPosition(VolumeUV, WorldToVolume);

			// VolumeUV -= floor(VolumeUV * VectorFields.TilingAxes[VectorFieldIndex].xyz);
			FVector TiledVolume = VolumeUV * TilingAxes;
			VolumeUV.X -= FMath::FloorToFloat(TiledVolume.X);
			VolumeUV.Y -= FMath::FloorToFloat(TiledVolume.Y);
			VolumeUV.Z -= FMath::FloorToFloat(TiledVolume.Z);

			// Saturate(VolumeUV) 
			VolumeUV.X = FMath::Clamp(VolumeUV.X, 0.0f, 1.0f);
			VolumeUV.Y = FMath::Clamp(VolumeUV.Y, 0.0f, 1.0f);
			VolumeUV.Z = FMath::Clamp(VolumeUV.Z, 0.0f, 1.0f);

			// Sample texture...
			VolumeUV = VolumeUV * VolumeSize;

			/**
			int3 IntCoord = int3(ModXYZ.x, ModXYZ.y, ModXYZ.z);\n");
			float3 frc = frac(ModXYZ);\n");
			*/
			FVector VecIndex;
			FVector VecFrac;
			VecFrac.X = FGenericPlatformMath::Modf(VolumeUV.X, &VecIndex.X);
			VecFrac.Y = FGenericPlatformMath::Modf(VolumeUV.Y, &VecIndex.Y);
			VecFrac.Z = FGenericPlatformMath::Modf(VolumeUV.Z, &VecIndex.Z);

			uint32 IndexX = (uint32)VecIndex.X;
			uint32 IndexY = (uint32)VecIndex.Y;
			uint32 IndexZ = (uint32)VecIndex.Z;

#define VF_MakeIndex(X, Y, Z) ((X)  + ((Y) * VolumeSizeX) + ((Z) * VolumeSizeX * VolumeSizeY))
#define VF_Tile(I, Range)  (((I) >= Range) ? 0 : (I))

			// Trilinear interpolation, as defined by https://en.wikipedia.org/wiki/Trilinear_interpolation
			/** 
			float3 frc = frac(ModXYZ);
			float3 V1 = Buffer[IntCoord.x + IntCoord.y*VolumeSizeX + IntCoord.z*VolumeSizeX*VolumeSizeY].xyz;  // V(x0, y0, z0)
			float3 V2 = Buffer[IntCoord.x + 1 + IntCoord.y*VolumeSizeX + IntCoord.z*VolumeSizeX*VolumeSizeY].xyz; // V(x1, y0, z0)
			float3 C00 = lerp(V1, V2, frc.xxx);
			V1 = Buffer[IntCoord.x + IntCoord.y*VolumeSizeX + (IntCoord.z+1)*VolumeSizeX*VolumeSizeY].xyz;  // V(x0, y0, z1)
			V2 = Buffer[IntCoord.x + 1 + IntCoord.y*VolumeSizeX + (IntCoord.z+1)*VolumeSizeX*VolumeSizeY].xyz; // V(x1, y0, z1)
			float3 C01 = lerp(V1, V2, frc.xxx);
			V1 = Buffer[IntCoord.x + (IntCoord.y+1)*VolumeSizeX + IntCoord.z*VolumeSizeX*VolumeSizeY].xyz;  // V(x0, y1, z0)
			V2 = Buffer[IntCoord.x + 1 + (IntCoord.y+1)*VolumeSizeX + IntCoord.z*VolumeSizeX*VolumeSizeY].xyz; // V(x1, y1, z0)
			float3 C10 = lerp(V1, V2, frc.xxx);
			V1 = Buffer[IntCoord.x + (IntCoord.y+1)*VolumeSizeX + (IntCoord.z+1)*VolumeSizeX*VolumeSizeY].xyz;  // V(x0, y1, z1)
			V2 = Buffer[IntCoord.x + 1 + (IntCoord.y+1)*VolumeSizeX + (IntCoord.z+1)*VolumeSizeX*VolumeSizeY].xyz; // V(x1, y1, z1)
			float3 C11 = lerp(V1, V2, frc.xxx);
			float3 C0 = lerp(C00, C10, frc.yyy);
			float3 C1 = lerp(c01, C11, frc.yyy);
			Out_Value = lerp(C0, C1, frc.zzz);
			*/
			uint32 Index = VF_MakeIndex(VF_Tile(IndexX, VolumeSizeX), VF_Tile(IndexY, VolumeSizeY), VF_Tile(IndexZ, VolumeSizeZ));
			check(Index < (uint32)(VolumeSizeX*VolumeSizeY*VolumeSizeZ));
			FVector V1 ((float)Data[Index].R, (float)Data[Index].G, (float)Data[Index].B);  // V(x0, y0, z0)
			Index = VF_MakeIndex(VF_Tile(IndexX + 1, VolumeSizeX), VF_Tile(IndexY, VolumeSizeY), VF_Tile(IndexZ, VolumeSizeZ));
			check(Index < (uint32)(VolumeSizeX*VolumeSizeY*VolumeSizeZ));
			FVector V2((float)Data[Index].R, (float)Data[Index].G, (float)Data[Index].B); // V(x1, y0, z0)
			FVector C00 = FMath::Lerp(V1, V2, VecFrac.X);

			Index = VF_MakeIndex(IndexX, VF_Tile(IndexY, VolumeSizeY), VF_Tile(IndexZ + 1, VolumeSizeZ));
			check(Index < (uint32)(VolumeSizeX*VolumeSizeY*VolumeSizeZ));
			V1 = FVector((float)Data[Index].R, (float)Data[Index].G, (float)Data[Index].B);// V(x0, y0, z1)
			Index = VF_MakeIndex(VF_Tile(IndexX + 1, VolumeSizeX), VF_Tile(IndexY, VolumeSizeY), VF_Tile(IndexZ + 1, VolumeSizeZ));
			check(Index < (uint32)(VolumeSizeX*VolumeSizeY*VolumeSizeZ));
			V1 = FVector((float)Data[Index].R, (float)Data[Index].G, (float)Data[Index].B);// V(x1, y0, z1)
			FVector C01 = FMath::Lerp(V1, V2, VecFrac.X);

			Index = VF_MakeIndex(VF_Tile(IndexX, VolumeSizeX), VF_Tile(IndexY + 1, VolumeSizeY), VF_Tile(IndexZ, VolumeSizeZ));
			check(Index < (uint32)(VolumeSizeX*VolumeSizeY*VolumeSizeZ));
			V1 = FVector((float)Data[Index].R, (float)Data[Index].G, (float)Data[Index].B);// V(x0, y1, z0)
			Index = VF_MakeIndex(VF_Tile(IndexX + 1, VolumeSizeX), VF_Tile(IndexY + 1, VolumeSizeY), VF_Tile(IndexZ, VolumeSizeZ));
			check(Index < (uint32)(VolumeSizeX*VolumeSizeY*VolumeSizeZ));
			V1 = FVector((float)Data[Index].R, (float)Data[Index].G, (float)Data[Index].B); // V(x1, y1, z0)
			FVector C10 = FMath::Lerp(V1, V2, VecFrac.X);

			Index = VF_MakeIndex(VF_Tile(IndexX, VolumeSizeX), VF_Tile(IndexY + 1, VolumeSizeY), VF_Tile(IndexZ + 1, VolumeSizeZ));
			check(Index < (uint32)(VolumeSizeX*VolumeSizeY*VolumeSizeZ));
			V1 = FVector((float)Data[Index].R, (float)Data[Index].G, (float)Data[Index].B);// V(x0, y1, z1)
			Index = VF_MakeIndex(VF_Tile(IndexX + 1, VolumeSizeX), VF_Tile(IndexY + 1, VolumeSizeY), VF_Tile(IndexZ + 1, VolumeSizeZ));
			check(Index < (uint32)(VolumeSizeX*VolumeSizeY*VolumeSizeZ));
			V1 = FVector((float)Data[Index].R, (float)Data[Index].G, (float)Data[Index].B);// V(x1, y1, z1)
			FVector C11 = FMath::Lerp(V1, V2, VecFrac.X);

			FVector C0 = FMath::Lerp(C00, C10, VecFrac.Y);
			FVector C1 = FMath::Lerp(C01, C11, VecFrac.Y);
			FVector C = FMath::Lerp(C0, C1, VecFrac.Z);

			// Write final output...
			*OutSampleX.GetDest() = C.X;
			*OutSampleY.GetDest() = C.Y;
			*OutSampleZ.GetDest() = C.Z;

			XParam.Advance();
			YParam.Advance();
			ZParam.Advance();
			OutSampleX.Advance();
			OutSampleY.Advance();
			OutSampleZ.Advance();
		}
	}

	Unlock();
}


bool UNiagaraDataInterfaceVectorField::CanExecuteOnTarget(ENiagaraSimTarget Target)const 
{
	return true; 
}

bool UNiagaraDataInterfaceVectorField::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceVectorField* OtherTyped = CastChecked<UNiagaraDataInterfaceVectorField>(Destination);
	OtherTyped->Field = Field;
	OtherTyped->bTileX = bTileX;
	OtherTyped->bTileY = bTileY;
	OtherTyped->bTileZ = bTileZ;
	OtherTyped->bGPUBufferDirty = true;
	return true;
}

bool UNiagaraDataInterfaceVectorField::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}
	const UNiagaraDataInterfaceVectorField* OtherTyped = CastChecked<const UNiagaraDataInterfaceVectorField>(Other);
	return OtherTyped->Field == Field 
		&& OtherTyped->bTileX == bTileX
		&& OtherTyped->bTileY == bTileY
		&& OtherTyped->bTileZ == bTileZ;
}


int32 UNiagaraDataInterfaceVectorField::PerInstanceDataSize()const  
{
	return 0;
}

bool UNiagaraDataInterfaceVectorField::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	//FNDIVectorField_InstanceData* Inst = new (PerInstanceData) FNDIVectorField_InstanceData();
	//return Inst->Init(this, SystemInstance);
	return false;
}

void UNiagaraDataInterfaceVectorField::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	/*FNDIVectorField_InstanceData* Inst = (FNDIVectorField_InstanceData*)PerInstanceData;
	Inst->~FNDIVectorField_InstanceData();*/
}

bool UNiagaraDataInterfaceVectorField::PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float InDeltaSeconds)
{
	/*FNDIVectorField_InstanceData* Inst = (FNDIVectorField_InstanceData*)PerInstanceData;
	return Inst->Tick(this, SystemInstance, InDeltaSeconds);*/
	return false;
}

bool UNiagaraDataInterfaceVectorField::GetFunctionHLSL(const FName& DefinitionFunctionName, FString InstanceFunctionName, TArray<FDIGPUBufferParamDescriptor> &Descriptors, FString &HLSLInterfaceID, FString &OutHLSL)
{
	//FString BufferName = Descriptors[0].BufferParamName;
	if (DefinitionFunctionName == SampleVectorFieldName)
	{
		FString BufferName = Descriptors[0].BufferParamName;
		OutHLSL += TEXT("void ") + InstanceFunctionName + TEXT("(float3 In_SamplePoint,  out float3 Out_Value) \n{\n");
		OutHLSL += TEXT("\t int3 IntCoord = int3(0,0,0);\n");
		OutHLSL += TEXT("\t int3 Dimensions = int3(1,1,1);\n");
		OutHLSL += TEXT("\t Out_Value = ") + BufferName + TEXT("[IntCoord.x + IntCoord.y*Dimensions.x + IntCoord.z*Dimensions.x*Dimensions.y].xyz;\n");
		OutHLSL += TEXT("\n}\n");
		return true;
	}
	else if (DefinitionFunctionName == GetVectorDimsName)
	{
		OutHLSL += TEXT("void ") + InstanceFunctionName + TEXT("(out float3 Out_Value) \n{\n");
		OutHLSL += TEXT("\t Out_Value = float3(1.0f, 1.0f, 1.0f);\n");
		OutHLSL += TEXT("\n}\n");
		return true;
	}
	else if (DefinitionFunctionName == GetVectorFieldBoundsName)
	{
		OutHLSL += TEXT("void ") + InstanceFunctionName + TEXT("(out float3 Out_MinBounds, out float3 Out_MaxBounds) \n{\n");
		OutHLSL += TEXT("\t Out_MinBounds = Out_MaxBounds = float3(1.0f, 1.0f, 1.0f);\n");
		OutHLSL += TEXT("\n}\n");
		return true;
	}
	return false;
}

// build buffer definition hlsl
// 1. Choose a buffer name, add the data interface ID (important!)
// 2. add a DIGPUBufferParamDescriptor to the array argument; that'll be passed on to the FNiagaraShader for binding to a shader param, that can
// then later be found by name via FindDIBufferParam for setting; 
// 3. store buffer declaration hlsl in OutHLSL
// multiple buffers can be defined at once here
//
void UNiagaraDataInterfaceVectorField::GetBufferDefinitionHLSL(FString DataInterfaceID, TArray<FDIGPUBufferParamDescriptor> &BufferDescriptors, FString &OutHLSL)
{
	FString BufferName = "VectorFieldLUT" + DataInterfaceID;
	OutHLSL += TEXT("Buffer<float4> ") + BufferName + TEXT(";\n");

	BufferDescriptors.Add(FDIGPUBufferParamDescriptor(BufferName, 0));		// add a descriptor for shader parameter binding
}

// called after translate, to setup buffers matching the buffer descriptors generated during hlsl translation
// need to do this because the script used during translate is a clone, including its DIs
//
void UNiagaraDataInterfaceVectorField::SetupBuffers(FDIBufferDescriptorStore &BufferDescriptors)
{
	GPUBuffers.Empty();
	for (FDIGPUBufferParamDescriptor &Desc : BufferDescriptors.Descriptors)
	{
		FNiagaraDataInterfaceBufferData BufferData(*Desc.BufferParamName);	// store off the data for later use
		GPUBuffers.Add(BufferData);
	}
	bGPUBufferDirty = true;
}

// return the GPU buffer array (called from NiagaraInstanceBatcher to get the buffers for setting to the shader)
// we lazily update the buffer with a new LUT here if necessary
//
TArray<FNiagaraDataInterfaceBufferData> &UNiagaraDataInterfaceVectorField::GetBufferDataArray()
{
	check(IsInRenderingThread());

	if (bGPUBufferDirty)
	{
		UVectorFieldStatic* StaticVectorField = Cast<UVectorFieldStatic>(Field);
		UVectorFieldAnimated* AnimatedVectorField = Cast<UVectorFieldAnimated>(Field);

		uint32 Width = 10;
		uint32 Height = 10;
		uint32 Depth = 10;
		const FFloat16Color* Data = nullptr;

		if (StaticVectorField)
		{
			Width = StaticVectorField->SizeX;
			Height = StaticVectorField->SizeY;
			Depth = StaticVectorField->SizeZ;
		}
		
		check(GPUBuffers.Num() > 0);
		FNiagaraDataInterfaceBufferData &GPUBuffer = GPUBuffers[0];
		GPUBuffer.Buffer.Release();
		uint32 BufferSize = Width * Height * Depth * sizeof(float) * 4;
		TResourceArray<FVector4> TempTable;
		TempTable.AddUninitialized(Width*Height*Depth);
		
		if (StaticVectorField)
		{
			Data = (const FFloat16Color*)StaticVectorField->SourceData.LockReadOnly();

			// TODO sckime - can we just send this down as half4's ???
			for (uint32 z = 0; z < Depth; z++)
			{
				for (uint32 y = 0; y < Height; y++)
				{
					for (uint32 x = 0; x < Width; x++)
					{
						uint32 Index = x + (y* Height) + (z * Width * Height);
						TempTable[Index] = FVector4(Data[Index].R, Data[Index].G, Data[Index].B, 0.0f);
					}
				}
			}
			StaticVectorField->SourceData.Unlock();
		}
		else
		{
			// We don't support the other types for now.
			for (uint32 z = 0; z < Depth; z++)
			{
				for (uint32 y = 0; y < Height; y++)
				{
					for (uint32 x = 0; x < Width; x++)
					{
						uint32 Index = x + (y* Height) + (z * Width * Height);
						TempTable[Index] = FVector4(1.0f, 0.0f, 0.0f, 0.0f);
					}
				}
			}
		}

		GPUBuffer.Buffer.Initialize(sizeof(float) * 4, Width*Height*Depth, EPixelFormat::PF_A32B32G32R32F, BUF_Static, TEXT("CurlnoiseTable"), &TempTable);
		//FPlatformMemory::Memcpy(BufferData, TempTable, BufferSize);
		//RHIUnlockVertexBuffer(GPUBuffer.Buffer.Buffer);
		bGPUBufferDirty = false;
	}
	return GPUBuffers;
}



#undef LOCTEXT_NAMESPACE