// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataInterfaceCollisionQuery.h"
#include "NiagaraTypes.h"
#include "NiagaraCustomVersion.h"

//////////////////////////////////////////////////////////////////////////
//Color Curve


UNiagaraDataInterfaceCollisionQuery::UNiagaraDataInterfaceCollisionQuery(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UNiagaraDataInterfaceCollisionQuery::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* InSystemInstance)
{
	CQDIPerInstanceData *PIData = new (PerInstanceData) CQDIPerInstanceData;
	PIData->SystemInstance = InSystemInstance;
	if (InSystemInstance)
	{
		PIData->CollisionBatch.Init(InSystemInstance->GetIDName(), InSystemInstance->GetComponent()->GetWorld());
	}
	return true;
}


void UNiagaraDataInterfaceCollisionQuery::PostInitProperties()
{
	Super::PostInitProperties();

	//Can we register data interfaces as regular types and fold them into the FNiagaraVariable framework for UI and function calls etc?
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
	}
}

void UNiagaraDataInterfaceCollisionQuery::PostLoad()
{
	Super::PostLoad();

	const int32 NiagaraVer = GetLinkerCustomVersion(FNiagaraCustomVersion::GUID);
}

#if WITH_EDITOR

void UNiagaraDataInterfaceCollisionQuery::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif


bool UNiagaraDataInterfaceCollisionQuery::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}
	return true;
}

bool UNiagaraDataInterfaceCollisionQuery::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}
	return true;
}


void UNiagaraDataInterfaceCollisionQuery::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	FNiagaraFunctionSignature Sig3;
	Sig3.Name = TEXT("PerformCollisionQuery");
	Sig3.bMemberFunction = true;
	Sig3.bRequiresContext = false;
	Sig3.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CollisionQuery")));
	//Sig3.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("PerformQuery")));
	Sig3.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("ReturnQueryID")));
	Sig3.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("ParticlePosition")));
	Sig3.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("ParticleVelocity")));
	Sig3.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("DeltaTime")));
	Sig3.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("CollisionSize")));
	Sig3.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("QueryID")));
	Sig3.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("CollisionValid")));
	Sig3.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("CollisionPos")));
	Sig3.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("CollisionVelocity")));
	Sig3.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("CollisionNormal")));
	Sig3.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Friction")));
	Sig3.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Restitution")));
	OutFunctions.Add(Sig3);


	FNiagaraFunctionSignature Sig;
	Sig.Name = TEXT("SubmitQuery");
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CollisionQuery")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("ParticlePosition")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("ParticleVelocity")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("DeltaTime")));
	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("CollisionID")));
	OutFunctions.Add(Sig);

	FNiagaraFunctionSignature Sig2;
	Sig2.Name = TEXT("ReadQuery");
	Sig2.bMemberFunction = true;
	Sig2.bRequiresContext = false;
	Sig2.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CollisionQuery")));
	Sig2.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("CollisionID")));
	Sig2.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("CollisionValid")));
	Sig2.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("CollisionPos")));
	Sig2.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("CollisionVelocity")));
	Sig2.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("CollisionNormal")));
	OutFunctions.Add(Sig2);
}

// build the shader function HLSL; function name is passed in, as it's defined per-DI; that way, configuration could change
// the HLSL in the spirit of a static switch
// TODO: need a way to identify each specific function here
// 
bool UNiagaraDataInterfaceCollisionQuery::GetFunctionHLSL(const FName& DefinitionFunctionName, FString InstanceFunctionName, TArray<FDIGPUBufferParamDescriptor> &Descriptors, FString &HLSLInterfaceID, FString &OutHLSL)
{
	// a little tricky, since we've got two functions for submitting and retrieving a query; we store submitted queries per thread group,
	// assuming it'll usually be the same thread trying to call ReadQuery for a particular QueryID, that submitted it in the first place.
	if (InstanceFunctionName == TEXT("PerformQuery"))
	{
		FString TextureName = Descriptors[0].BufferParamName;
		OutHLSL += TEXT("groupshared int CollisionQueryID = 0;\n");
		OutHLSL += TEXT("\t\n");
		OutHLSL += TEXT("void ") + InstanceFunctionName + TEXT("(in int InQueryID, in float3 In_ParticlePos, in float3 In_ParticleVel, in float In_Deltatime, out int OutQueryID, out bool OutCollisionValid, out float3 Out_CollisionPos, out float3 Out_CollisionVel, out float3 Out_CollisionNormal) \n{\n");
		OutHLSL += TEXT("\tInterlockedAdd(CollisionQueryID, 1);\n");
		// get the screen position
		OutHLSL += TEXT("float3 CollisionPosition = InPosition + CollisionOffset;\
							float4 SamplePosition = float4(CollisionPosition + View.PreViewTranslation, 1);\
							float4 ClipPosition = mul(SamplePosition, View.TranslatedWorldToClip);\
							float2 ScreenPosition = ClipPosition.xy / ClipPosition.w;\
							// Don't try to collide if the particle falls outside the view.\
							if (all(abs(ScreenPosition.xy) <= float2(1, 1)))\
							{\
								// Sample the depth buffer to get a world position near the particle.\
								float2 ScreenUV = ScreenPosition * View.ScreenPositionScaleBias.xy + View.ScreenPositionScaleBias.wz;\
								float SceneDepth = CalcSceneDepth(ScreenUV);\
								if (abs(ClipPosition.w - SceneDepth) < CollisionDepthBounds)");
		OutHLSL += TEXT("\n}\n");
	}


	return true;
}

// build buffer definition hlsl
// 1. Choose a buffer name, add the data interface ID (important!)
// 2. add a DIGPUBufferParamDescriptor to the array argument; that'll be passed on to the FNiagaraShader for binding to a shader param, that can
// then later be found by name via FindDIBufferParam for setting; 
// 3. store buffer declaration hlsl in OutHLSL
// multiple buffers can be defined at once here
//
void UNiagaraDataInterfaceCollisionQuery::GetBufferDefinitionHLSL(FString DataInterfaceID, TArray<FDIGPUBufferParamDescriptor> &BufferDescriptors, FString &OutHLSL)
{
	FString TextureName = "DepthBuffer" + DataInterfaceID;
	OutHLSL += TEXT("Texture2D ") + TextureName + TEXT(";\n");
	BufferDescriptors.Add(FDIGPUBufferParamDescriptor(TextureName, 0));		// add a descriptor for shader parameter binding

	FString CounterName = "DepthBuffer" + DataInterfaceID;
	OutHLSL += TEXT("Buffer<uint> ") + TextureName + TEXT(";\n");
	BufferDescriptors.Add(FDIGPUBufferParamDescriptor(TextureName, 0));		// add a descriptor for shader parameter binding
}

// called after translate, to setup buffers matching the buffer descriptors generated during hlsl translation
// need to do this because the script used during translate is a clone, including its DIs
//
void UNiagaraDataInterfaceCollisionQuery::SetupBuffers(FDIBufferDescriptorStore &BufferDescriptors)
{
	GPUBuffers.Empty();
	/*
	for (FDIGPUBufferParamDescriptor &Desc : BufferDescriptors.Descriptors)
	{
		FNiagaraDataInterfaceBufferData BufferData(*Desc.BufferParamName);	// store off the data for later use
		GPUBuffers.Add(BufferData);
	}
	GPUBufferDirty = true;
	*/
}

// return the GPU buffer array (called from NiagaraInstanceBatcher to get the buffers for setting to the shader)
// we lazily update the buffer with a new LUT here if necessary
//
TArray<FNiagaraDataInterfaceBufferData> &UNiagaraDataInterfaceCollisionQuery::GetBufferDataArray()
{
	check(IsInRenderingThread());
	return GPUBuffers;
}


DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceCollisionQuery, SubmitQuery);
DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceCollisionQuery, ReadQuery);
DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceCollisionQuery, PerformQuery);

FVMExternalFunction UNiagaraDataInterfaceCollisionQuery::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData)
{
	CQDIPerInstanceData *InstData = (CQDIPerInstanceData *)InstanceData;
	if (BindingInfo.Name == TEXT("SubmitQuery") /*&& BindingInfo.GetNumInputs() == 3 && BindingInfo.GetNumOutputs() == 1*/)
	{
		return TNDIParamBinder<0, float, TNDIParamBinder<1, float, TNDIParamBinder<2, float, TNDIParamBinder<3, float, TNDIParamBinder<4, float, TNDIParamBinder<5, float, TNDIParamBinder<6, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceCollisionQuery, SubmitQuery)>>>>>>>::Bind(this, BindingInfo, InstData);
	}
	else if (BindingInfo.Name == TEXT("ReadQuery") /*&& BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 4*/)
	{
		return TNDIParamBinder<0, int32, NDI_FUNC_BINDER(UNiagaraDataInterfaceCollisionQuery, ReadQuery)>::Bind(this, BindingInfo, InstData);
	}
	else if (BindingInfo.Name == TEXT("PerformCollisionQuery") /*&& BindingInfo.GetNumInputs() == 1 && BindingInfo.GetNumOutputs() == 4*/)
	{
		return TNDIParamBinder<0, int, TNDIParamBinder<1, float, TNDIParamBinder<2, float, TNDIParamBinder<3, float, TNDIParamBinder<4, float, TNDIParamBinder<5, float, TNDIParamBinder<6, float, TNDIParamBinder<7, float, TNDIParamBinder<8, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceCollisionQuery, PerformQuery)>>>>>>>>>::Bind(this, BindingInfo, InstData);
	}
	else
	{
		UE_LOG(LogNiagara, Error, TEXT("Could not find data interface external function. %s\n"),
			*BindingInfo.Name.ToString());
		return FVMExternalFunction();
	}
}


template<typename InQueryIDType, typename PosTypeX, typename PosTypeY, typename PosTypeZ, typename VelTypeX, typename VelTypeY, typename VelTypeZ, typename DTType, typename SizeType>
void UNiagaraDataInterfaceCollisionQuery::PerformQuery(FVectorVMContext& Context)
{
	//PerformType PerformParam(Context);
	InQueryIDType InIDParam(Context);
	PosTypeX PosParamX(Context);
	PosTypeY PosParamY(Context);
	PosTypeZ PosParamZ(Context);
	VelTypeX VelParamX(Context);
	VelTypeY VelParamY(Context);
	VelTypeZ VelParamZ(Context);
	DTType DTParam(Context);
	SizeType SizeParam(Context);

	FUserPtrHandler<CQDIPerInstanceData> InstanceData(Context);

	FRegisterHandler<int32> OutQueryID(Context);

	FRegisterHandler<int32> OutQueryValid(Context);
	FRegisterHandler<float> OutCollisionPosX(Context);
	FRegisterHandler<float> OutCollisionPosY(Context);
	FRegisterHandler<float> OutCollisionPosZ(Context);
	FRegisterHandler<float> OutCollisionVelX(Context);
	FRegisterHandler<float> OutCollisionVelY(Context);
	FRegisterHandler<float> OutCollisionVelZ(Context);
	FRegisterHandler<float> OutCollisionNormX(Context);
	FRegisterHandler<float> OutCollisionNormY(Context);
	FRegisterHandler<float> OutCollisionNormZ(Context);
	FRegisterHandler<float> Friction(Context);
	FRegisterHandler<float> Restitution(Context);

	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		// submit a query, get query id in return
		//if (PerformParam.Get())
		{
			FVector Pos(PosParamX.Get(), PosParamY.Get(), PosParamZ.Get());
			FVector Vel(VelParamX.Get(), VelParamY.Get(), VelParamZ.Get());
			ensure(!Pos.ContainsNaN());
			ensure(!Vel.ContainsNaN());
			float Dt = DTParam.Get();
			float Size = SizeParam.Get();
			*OutQueryID.GetDest() = InstanceData->CollisionBatch.SubmitQuery(Pos, Vel, Size, Dt);
		}

		// try to retrieve a query with the supplied query ID
		FNiagaraDICollsionQueryResult Res;
		int32 ID = InIDParam.Get();
		bool Valid = InstanceData->CollisionBatch.GetQueryResult(ID, Res);
		if (Valid)
		{
			*OutQueryValid.GetDest() = 0xFFFFFFFF; //->SetValue(true);
			*OutCollisionPosX.GetDest() = Res.CollisionPos.X;
			*OutCollisionPosY.GetDest() = Res.CollisionPos.Y;
			*OutCollisionPosZ.GetDest() = Res.CollisionPos.Z;
			*OutCollisionVelX.GetDest() = Res.CollisionVelocity.X;
			*OutCollisionVelY.GetDest() = Res.CollisionVelocity.Y;
			*OutCollisionVelZ.GetDest() = Res.CollisionVelocity.Z;
			*OutCollisionNormX.GetDest() = Res.CollisionNormal.X;
			*OutCollisionNormY.GetDest() = Res.CollisionNormal.Y;
			*OutCollisionNormZ.GetDest() = Res.CollisionNormal.Z;
			*Friction.GetDest() = Res.Friction;
			*Restitution.GetDest() = Res.Restitution;
		}
		else
		{
			*OutQueryValid.GetDest() = 0;// ->SetValue(false);
		}

		//PerformParam.Advance();
		InIDParam.Advance();
		OutQueryValid.Advance();
		OutCollisionPosX.Advance();
		OutCollisionPosY.Advance();
		OutCollisionPosZ.Advance();
		OutCollisionVelX.Advance();
		OutCollisionVelY.Advance();
		OutCollisionVelZ.Advance();
		OutCollisionNormX.Advance();
		OutCollisionNormY.Advance();
		OutCollisionNormZ.Advance();
		Friction.Advance();
		Restitution.Advance();

		PosParamX.Advance();
		PosParamY.Advance();
		PosParamZ.Advance();
		VelParamX.Advance();
		VelParamY.Advance();
		VelParamZ.Advance();
		DTParam.Advance();
		SizeParam.Advance();
		OutQueryID.Advance();
	}

}


template<typename PosTypeX, typename PosTypeY, typename PosTypeZ, typename VelTypeX, typename VelTypeY, typename VelTypeZ, typename DTType>
void UNiagaraDataInterfaceCollisionQuery::SubmitQuery(FVectorVMContext& Context)
{
	PosTypeX PosParamX(Context);
	PosTypeY PosParamY(Context);
	PosTypeZ PosParamZ(Context);
	VelTypeX VelParamX(Context);
	VelTypeY VelParamY(Context);
	VelTypeZ VelParamZ(Context);

	DTType DTParam(Context);
	FUserPtrHandler<CQDIPerInstanceData> InstanceData(Context);
	FRegisterHandler<int32> OutQueryID(Context);
	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		FVector Pos(PosParamX.Get(), PosParamY.Get(), PosParamZ.Get());
		FVector Vel(VelParamX.Get(), VelParamY.Get(), VelParamZ.Get());
		ensure(!Pos.ContainsNaN());
		ensure(!Vel.ContainsNaN());
		float Dt = DTParam.Get();

		*OutQueryID.GetDest() = InstanceData->CollisionBatch.SubmitQuery(Pos, Vel, 0.0f, Dt);

		PosParamX.Advance();
		PosParamY.Advance();
		PosParamZ.Advance();
		VelParamX.Advance();
		VelParamY.Advance();
		VelParamZ.Advance();
		DTParam.Advance();
		OutQueryID.Advance();
	}

}


template<typename IDType>
void UNiagaraDataInterfaceCollisionQuery::ReadQuery(FVectorVMContext& Context)
{	
	IDType IDParam(Context);
	FUserPtrHandler<CQDIPerInstanceData> InstanceData(Context);
	FRegisterHandler<int32> OutQueryValid(Context);
	FRegisterHandler<float> OutCollisionPosX(Context);
	FRegisterHandler<float> OutCollisionPosY(Context);
	FRegisterHandler<float> OutCollisionPosZ(Context);
	FRegisterHandler<float> OutCollisionVelX(Context);
	FRegisterHandler<float> OutCollisionVelY(Context);
	FRegisterHandler<float> OutCollisionVelZ(Context);
	FRegisterHandler<float> OutCollisionNormX(Context);
	FRegisterHandler<float> OutCollisionNormY(Context);
	FRegisterHandler<float> OutCollisionNormZ(Context);

	for (int32 i = 0; i < Context.NumInstances; ++i)
	{
		FNiagaraDICollsionQueryResult Res;
		int32 ID = IDParam.Get();
		bool Valid = InstanceData->CollisionBatch.GetQueryResult(ID, Res);

		if (Valid)
		{
			*OutQueryValid.GetDest() = 0xFFFFFFFF; //->SetValue(true);
			*OutCollisionPosX.GetDest() = Res.CollisionPos.X;
			*OutCollisionPosY.GetDest() = Res.CollisionPos.Y;
			*OutCollisionPosZ.GetDest() = Res.CollisionPos.Z;
			*OutCollisionVelX.GetDest() = Res.CollisionVelocity.X;
			*OutCollisionVelY.GetDest() = Res.CollisionVelocity.Y;
			*OutCollisionVelZ.GetDest() = Res.CollisionVelocity.Z;
			*OutCollisionNormX.GetDest() = Res.CollisionNormal.X;
			*OutCollisionNormY.GetDest() = Res.CollisionNormal.Y;
			*OutCollisionNormZ.GetDest() = Res.CollisionNormal.Z;
		}
		else
		{
			*OutQueryValid.GetDest() = 0;// ->SetValue(false);
		}

		IDParam.Advance();
		OutQueryValid.Advance();
		OutCollisionPosX.Advance();
		OutCollisionPosY.Advance();
		OutCollisionPosZ.Advance();
		OutCollisionVelX.Advance();
		OutCollisionVelY.Advance();
		OutCollisionVelZ.Advance();
		OutCollisionNormX.Advance();
		OutCollisionNormY.Advance();
		OutCollisionNormZ.Advance();
	}
}


bool UNiagaraDataInterfaceCollisionQuery::PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* InSystemInstance, float DeltaSeconds)
{
	return false;
}

bool UNiagaraDataInterfaceCollisionQuery::PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* InSystemInstance, float DeltaSeconds)
{
	CQDIPerInstanceData *PIData = static_cast<CQDIPerInstanceData*>(PerInstanceData);
	PIData->CollisionBatch.Tick(ENiagaraSimTarget::CPUSim);
	PIData->CollisionBatch.ClearWrite();
	return false;
}
