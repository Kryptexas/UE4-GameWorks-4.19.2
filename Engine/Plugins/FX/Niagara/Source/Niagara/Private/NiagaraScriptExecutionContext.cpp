// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraScriptExecutionContext.h"
#include "NiagaraStats.h"
#include "NiagaraEmitterInstanceBatcher.h"
#include "NiagaraDataInterface.h"
#include "NiagaraSystemInstance.h"

DECLARE_CYCLE_STAT(TEXT("Register Setup"), STAT_NiagaraSimRegisterSetup, STATGROUP_Niagara);


//////////////////////////////////////////////////////////////////////////
FNiagaraScriptExecutionParameterStore::FNiagaraScriptExecutionParameterStore()
	: FNiagaraParameterStore()
{

}

FNiagaraScriptExecutionParameterStore::FNiagaraScriptExecutionParameterStore(const FNiagaraParameterStore& Other)
{
	*this = Other;
}

FNiagaraScriptExecutionParameterStore& FNiagaraScriptExecutionParameterStore::operator=(const FNiagaraParameterStore& Other)
{
	FNiagaraParameterStore::operator=(Other);
	return *this;
}

uint32 OffsetAlign(uint32 SrcOffset, uint32 Size)
{
	uint32 OffsetRemaining = UNIFORM_BUFFER_STRUCT_ALIGNMENT - (SrcOffset % UNIFORM_BUFFER_STRUCT_ALIGNMENT);
	if (Size <= OffsetRemaining)
	{
		return SrcOffset;
	}
	else
	{
		return Align(SrcOffset, UNIFORM_BUFFER_STRUCT_ALIGNMENT);
	}
}

void FNiagaraScriptExecutionParameterStore::GenerateLayoutInfoInternal(TArray<FNiagaraScriptExecutionParameterStore::FPaddingInfo>& Members, uint32& NextMemberOffset, const UStruct* InSrcStruct, uint32 InSrcOffset)
{
	// Now insert an appropriate data member into the mix...
	if (InSrcStruct == FNiagaraTypeDefinition::GetBoolStruct() || InSrcStruct == FNiagaraTypeDefinition::GetIntStruct())
	{
		//Members.Emplace(*Property->GetName(), TEXT(""), InSrcOffset, (EUniformBufferBaseType)TUniformBufferTypeInfo<uint32>::BaseType, EShaderPrecisionModifier::Float, TUniformBufferTypeInfo<uint32>::NumRows, TUniformBufferTypeInfo<uint32>::NumColumns, TUniformBufferTypeInfo<uint32>::NumElements, TUniformBufferTypeInfo<uint32>::GetStruct());
		Members.Emplace(InSrcOffset, Align(NextMemberOffset, TUniformBufferTypeInfo<uint32>::Alignment), (TUniformBufferTypeInfo<uint32>::NumRows * TUniformBufferTypeInfo<uint32>::NumColumns) * sizeof(uint32));
		InSrcOffset += sizeof(uint32);
		NextMemberOffset = Members[Members.Num() - 1].DestOffset + Members[Members.Num() - 1].Size;
	}
	else if (InSrcStruct == FNiagaraTypeDefinition::GetFloatStruct())
	{
		//Members.Emplace(*Property->GetName(), TEXT(""), InSrcOffset, (EUniformBufferBaseType)TUniformBufferTypeInfo < float > ::BaseType, EShaderPrecisionModifier::Float, TUniformBufferTypeInfo<float>::NumRows, TUniformBufferTypeInfo<float>::NumColumns, TUniformBufferTypeInfo<float>::NumElements, TUniformBufferTypeInfo<float>::GetStruct());
		Members.Emplace(InSrcOffset, Align(NextMemberOffset, TUniformBufferTypeInfo<float>::Alignment), (TUniformBufferTypeInfo<float>::NumRows * TUniformBufferTypeInfo<float>::NumColumns) * sizeof(float));
		InSrcOffset += sizeof(float);
		NextMemberOffset = Members[Members.Num() - 1].DestOffset + Members[Members.Num() - 1].Size;
	}
	else if (InSrcStruct == FNiagaraTypeDefinition::GetVec2Struct())
	{
		//Members.Emplace(*Property->GetName(), TEXT(""), InSrcOffset, (EUniformBufferBaseType)TUniformBufferTypeInfo < FVector2D > ::BaseType, EShaderPrecisionModifier::Float, TUniformBufferTypeInfo<FVector2D>::NumRows, TUniformBufferTypeInfo<FVector2D>::NumColumns, TUniformBufferTypeInfo<FVector2D>::NumElements, TUniformBufferTypeInfo<FVector2D>::GetStruct());
		uint32 StructFinalSize = (TUniformBufferTypeInfo<FVector2D>::NumRows * TUniformBufferTypeInfo<FVector2D>::NumColumns) * sizeof(float);
		Members.Emplace(InSrcOffset, OffsetAlign(NextMemberOffset, StructFinalSize),StructFinalSize);
		InSrcOffset += sizeof(FVector2D);
		NextMemberOffset = Members[Members.Num() - 1].DestOffset + Members[Members.Num() - 1].Size;
	}
	else if (InSrcStruct == FNiagaraTypeDefinition::GetVec3Struct())
	{
		//Members.Emplace(*Property->GetName(), TEXT(""), InSrcOffset, (EUniformBufferBaseType)TUniformBufferTypeInfo < FVector > ::BaseType, EShaderPrecisionModifier::Float, TUniformBufferTypeInfo<FVector>::NumRows, TUniformBufferTypeInfo<FVector>::NumColumns, TUniformBufferTypeInfo<FVector>::NumElements, TUniformBufferTypeInfo<FVector>::GetStruct());
		uint32 StructFinalSize = (TUniformBufferTypeInfo<FVector>::NumRows * TUniformBufferTypeInfo<FVector>::NumColumns) * sizeof(float);
		Members.Emplace(InSrcOffset, OffsetAlign(NextMemberOffset, StructFinalSize), StructFinalSize);
		InSrcOffset += sizeof(FVector);
		NextMemberOffset = Members[Members.Num() - 1].DestOffset + Members[Members.Num() - 1].Size;
	}
	else if (InSrcStruct == FNiagaraTypeDefinition::GetVec4Struct() || InSrcStruct == FNiagaraTypeDefinition::GetColorStruct())
	{
		//Members.Emplace(*Property->GetName(), TEXT(""), InSrcOffset, (EUniformBufferBaseType)TUniformBufferTypeInfo < FVector4 > ::BaseType, EShaderPrecisionModifier::Float, TUniformBufferTypeInfo<FVector4>::NumRows, TUniformBufferTypeInfo<FVector4>::NumColumns, TUniformBufferTypeInfo<FVector4>::NumElements, TUniformBufferTypeInfo<FVector4>::GetStruct());
		Members.Emplace(InSrcOffset, Align(NextMemberOffset, TUniformBufferTypeInfo<FVector4>::Alignment), (TUniformBufferTypeInfo<FVector4>::NumRows * TUniformBufferTypeInfo<FVector4>::NumColumns) * sizeof(float));
		InSrcOffset += sizeof(FVector4);
		NextMemberOffset = Members[Members.Num() - 1].DestOffset + Members[Members.Num() - 1].Size;
	}
	else if (InSrcStruct == FNiagaraTypeDefinition::GetMatrix4Struct())
	{
		//Members.Emplace(*Property->GetName(), TEXT(""), InSrcOffset, (EUniformBufferBaseType)TUniformBufferTypeInfo < FMatrix > ::BaseType, EShaderPrecisionModifier::Float, TUniformBufferTypeInfo<FMatrix>::NumRows, TUniformBufferTypeInfo<FMatrix>::NumColumns, TUniformBufferTypeInfo<FMatrix>::NumElements, TUniformBufferTypeInfo<FMatrix>::GetStruct());
		Members.Emplace(InSrcOffset, Align(NextMemberOffset, TUniformBufferTypeInfo<FMatrix>::Alignment), (TUniformBufferTypeInfo<FMatrix>::NumRows * TUniformBufferTypeInfo<FMatrix>::NumColumns) * sizeof(float));
		InSrcOffset += sizeof(FMatrix);
		NextMemberOffset = Members[Members.Num() - 1].DestOffset + Members[Members.Num() - 1].Size;
	}
	else
	{
		NextMemberOffset = Align(NextMemberOffset, UNIFORM_BUFFER_STRUCT_ALIGNMENT); // New structs should be aligned to the head..

		for (TFieldIterator<UProperty> PropertyIt(InSrcStruct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			const UStruct* Struct = nullptr;

			// First determine what struct type we're dealing with...
			if (Property->IsA(UFloatProperty::StaticClass()))
			{
				Struct = FNiagaraTypeDefinition::GetFloatStruct();
			}
			else if (Property->IsA(UIntProperty::StaticClass()))
			{
				Struct = FNiagaraTypeDefinition::GetIntStruct();
			}
			else if (Property->IsA(UBoolProperty::StaticClass()))
			{
				Struct = FNiagaraTypeDefinition::GetBoolStruct();
			}
			//Should be able to support double easily enough
			else if (UStructProperty* StructProp = CastChecked<UStructProperty>(Property))
			{
				Struct = StructProp->Struct;
			}
			else
			{
				check(false);
			}

			GenerateLayoutInfoInternal(Members, NextMemberOffset, Struct, InSrcOffset);
		}
	}

}

void FNiagaraScriptExecutionParameterStore::AddPaddedParamSize(const FNiagaraTypeDefinition& InParamType, uint32 InOffset)
{
	if (InParamType.IsDataInterface())
	{
		return;
	}

	uint32 NextMemberOffset = 0;
	if (PaddingInfo.Num() != 0)
	{
		NextMemberOffset = PaddingInfo[PaddingInfo.Num() - 1].DestOffset + PaddingInfo[PaddingInfo.Num() - 1].Size;
	}
	GenerateLayoutInfoInternal(PaddingInfo, NextMemberOffset, InParamType.GetScriptStruct(), InOffset);

	if (PaddingInfo.Num() != 0)
	{
		NextMemberOffset = PaddingInfo[PaddingInfo.Num() - 1].DestOffset + PaddingInfo[PaddingInfo.Num() - 1].Size;
		PaddedParameterSize = Align(NextMemberOffset, UNIFORM_BUFFER_STRUCT_ALIGNMENT);
	}
	else
	{
		PaddedParameterSize = 0;
	}
}


void FNiagaraScriptExecutionParameterStore::Init(UNiagaraScript* Script, ENiagaraSimTarget SimTarget)
{
	//TEMPORARTY:
	//We should replace the storage on the script with an FNiagaraParameterStore also so we can just copy that over here. Though that is an even bigger refactor job so this is a convenient place to break that work up.

	Empty();
	PaddedParameterSize = 0;
	AddScriptParams(Script, SimTarget);
}

void FNiagaraScriptExecutionParameterStore::AddScriptParams(UNiagaraScript* Script, ENiagaraSimTarget SimTarget)
{
	if (Script != nullptr)
	{
		//Here we add the current frame parameters.
		for (FNiagaraVariable& Param : Script->Parameters.Parameters)
		{
			AddParameter(Param, false);
		}

#if WITH_EDITORONLY_DATA
		Name = *FString::Printf(TEXT("ScriptExecParamStore %s %p"), *Script->GetFullName(), this);
#endif

		//Add previous frame values if we're interpolated spawn.
		bool bIsInterpolatedSpawn = Script->IsInterpolatedParticleSpawnScript();

		if (bIsInterpolatedSpawn)
		{
			for (FNiagaraVariable& Param : Script->Parameters.Parameters)
			{
				FNiagaraVariable PrevParam(Param.GetType(), FName(*(TEXT("PREV__") + Param.GetName().ToString())));
				AddParameter(PrevParam, false);
			}
		}

		ParameterSize = GetParameterDataArray().Num();
		if (bIsInterpolatedSpawn)
		{
			CopyCurrToPrev();
		}

		//Internal constants - only needed for non-GPU sim
		if (SimTarget != ENiagaraSimTarget::GPUComputeSim)
		{
			for (FNiagaraVariable& InternalVar : Script->InternalParameters.Parameters)
			{
				AddParameter(InternalVar, false);
			}
		}

		for (FNiagaraScriptDataInterfaceInfo& Info : Script->DataInterfaceInfo)
		{
			FNiagaraVariable Var(Info.Type, Info.Name);
			AddParameter(Var, false);
			SetDataInterface(Info.DataInterface, IndexOf(Var));
		}
	}
}

void FNiagaraScriptExecutionParameterStore::CopyCurrToPrev()
{
	int32 ParamStart = ParameterSize / 2;
	checkSlow(FMath::Frac((float)ParameterSize / 2) == 0.0f);

	FMemory::Memcpy(GetParameterData_Internal(ParamStart), GetParameterData(0), ParamStart);
}


void FNiagaraScriptExecutionParameterStore::CopyParameterDataToPaddedBuffer(uint8* InTargetBuffer, uint32 InTargetBufferSizeInBytes)
{
	check(InTargetBufferSizeInBytes >= PaddedParameterSize);
	FMemory::Memzero(InTargetBuffer, InTargetBufferSizeInBytes);
	const uint8* SrcData = GetParameterDataArray().GetData();
	for (int32 i = 0; i < PaddingInfo.Num(); i++)
	{
		check((PaddingInfo[i].DestOffset + PaddingInfo[i].Size) <= InTargetBufferSizeInBytes);
		check((PaddingInfo[i].SrcOffset + PaddingInfo[i].Size) <= (uint32)GetParameterDataArray().Num());
		FMemory::Memcpy(InTargetBuffer + PaddingInfo[i].DestOffset, SrcData + PaddingInfo[i].SrcOffset, PaddingInfo[i].Size);
	}
}

uint32 FNiagaraScriptExecutionContext::TickCounter = 0;

FNiagaraScriptExecutionContext::FNiagaraScriptExecutionContext()
	: Script(nullptr)
{

}

FNiagaraScriptExecutionContext::~FNiagaraScriptExecutionContext()
{
}

bool FNiagaraScriptExecutionContext::Init(UNiagaraScript* InScript, ENiagaraSimTarget InTarget)
{
	Script = InScript;

	Parameters.Init(Script, InTarget);
	

	return true;//TODO: Error cases?
}

bool FNiagaraScriptExecutionContext::Tick(FNiagaraSystemInstance* ParentSystemInstance, ENiagaraSimTarget SimTarget)
{
	if (Script)//TODO: Remove. Script can only be null for system instances that currently don't have their script exec context set up correctly.
	{
		//Bind data interfaces if needed.
		if (Parameters.GetInterfacesDirty())
		{
			// UE_LOG(LogNiagara, Log, TEXT("Updating data interfaces for script %s"), *Script->GetFullName());

			FunctionTable.Empty();
			const TArray<UNiagaraDataInterface*>& DataInterfaces = GetDataInterfaces();
			// We must make sure that the data interfaces match up between the original script values and our overrides...
			if (Script->DataInterfaceInfo.Num() != DataInterfaces.Num())
			{
				UE_LOG(LogNiagara, Warning, TEXT("Mismatch between Niagara Exectuion Context data interfaces and those in it's script!"));
				return false;
			}

			//Fill the instance data table.
			if (ParentSystemInstance)
			{
				DataInterfaceInstDataTable.SetNumZeroed(Script->NumUserPtrs);
				for (int32 i = 0; i < Script->DataInterfaceInfo.Num(); i++)
				{
					UNiagaraDataInterface* Interface = DataInterfaces[i];
					int32 UserPtrIdx = Script->DataInterfaceInfo[i].UserPtrIdx;
					if (UserPtrIdx != INDEX_NONE)
					{
						void* InstData = ParentSystemInstance->FindDataInterfaceInstanceData(Interface);
						DataInterfaceInstDataTable[UserPtrIdx] = InstData;
					}
				}
			}
			else
			{
				check(Script->NumUserPtrs == 0);//Can't have user ptrs if we have no parent instance.
			}

			bool bSuccessfullyMapped = true;
			for (FVMExternalFunctionBindingInfo& BindingInfo : Script->CalledVMExternalFunctions)
			{
				for (int32 i = 0; i < Script->DataInterfaceInfo.Num(); i++)
				{
					FNiagaraScriptDataInterfaceInfo& ScriptInfo = Script->DataInterfaceInfo[i];
					UNiagaraDataInterface* ExternalInterface = DataInterfaces[i];

					if (ScriptInfo.Name == BindingInfo.OwnerName)
					{
						void* InstData = ScriptInfo.UserPtrIdx == INDEX_NONE ? nullptr : DataInterfaceInstDataTable[ScriptInfo.UserPtrIdx];
						FVMExternalFunction Func = ExternalInterface->GetVMExternalFunction(BindingInfo, InstData);
						if (Func.IsBound())
						{
							FunctionTable.Add(Func);
						}
						else
						{
							UE_LOG(LogNiagara, Error, TEXT("Could not Get VMExternalFunction '%s'.. emitter will not run!"), *BindingInfo.Name.ToString());
							bSuccessfullyMapped = false;
						}
					}
				}
			}

			if (!bSuccessfullyMapped)
			{
				UE_LOG(LogNiagara, Warning, TEXT("Error building data interface function table!"));
				FunctionTable.Empty();
				return false;
			}
		}
	}

	Parameters.Tick();

	return true;
}

void FNiagaraScriptExecutionContext::PostTick()
{
	//If we're for interpolated spawn, copy over the previous frame's parameters into the Prev parameters.
	if (Script && Script->IsInterpolatedParticleSpawnScript())
	{
		Parameters.CopyCurrToPrev();
	}
}

bool FNiagaraScriptExecutionContext::Execute(uint32 NumInstances, TArray<FNiagaraDataSetExecutionInfo>& DataSetInfos)
{
	if (NumInstances == 0)
	{
		return true;
	}

	++TickCounter;//Should this be per execution?

	int32 NumInputRegisters = 0;
	int32 NumOutputRegisters = 0;
	uint8* InputRegisters[VectorVM::MaxInputRegisters];
	uint8* OutputRegisters[VectorVM::MaxOutputRegisters];

	DataSetMetaTable.Reset();

	bool bRegisterSetupCompleted = true;
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSimRegisterSetup);
		for (FNiagaraDataSetExecutionInfo& DataSetInfo : DataSetInfos)
		{
			check(DataSetInfo.DataSet);
			FDataSetMeta SetMeta(DataSetInfo.DataSet->GetSizeBytes(), &InputRegisters[NumInputRegisters], NumInputRegisters);
			DataSetMetaTable.Add(SetMeta);
			if (DataSetInfo.bAllocate)
			{
				DataSetInfo.DataSet->Allocate(NumInstances);
				DataSetInfo.DataSet->SetNumInstances(NumInstances);
			}
			
			bRegisterSetupCompleted &= DataSetInfo.DataSet->AppendToRegisterTable(InputRegisters, NumInputRegisters, OutputRegisters, NumOutputRegisters, DataSetInfo.StartInstance);
		}
	}

	if (bRegisterSetupCompleted)
	{
		VectorVM::Exec(
			Script->GetByteCode().GetData(),
			InputRegisters,
			NumInputRegisters,
			OutputRegisters,
			NumOutputRegisters,
			Parameters.GetParameterDataArray().GetData(),
			DataSetMetaTable,
			FunctionTable.GetData(),
			DataInterfaceInstDataTable.GetData(),
			NumInstances
#if STATS
			, Script->GetStatScopeIDs()
#endif
		);
	}

	// Tell the datasets we wrote how many instances were actually written.
	for (int Idx = 0; Idx < DataSetInfos.Num(); Idx++)
	{
		FNiagaraDataSetExecutionInfo& Info = DataSetInfos[Idx];
		if (Info.bUpdateInstanceCount)
		{
			Info.DataSet->SetNumInstances(Info.StartInstance + DataSetMetaTable[Idx].DataSetAccessIndex);
		}
	}

#if WITH_EDITORONLY_DATA
	if (Script->GetDebuggerInfo().bRequestDebugFrame)
	{
		DataSetInfos[0].DataSet->Dump(Script->GetDebuggerInfo().DebugFrame, true);
		Script->GetDebuggerInfo().bRequestDebugFrame = false;
		Script->GetDebuggerInfo().DebugFrameLastWriteId = TickCounter;
		Script->GetDebuggerInfo().DebugParameters = Parameters;
	}
#endif
	return true;//TODO: Error cases?
}

void FNiagaraScriptExecutionContext::DirtyDataInterfaces()
{
	Parameters.MarkInterfacesDirty();
}

bool FNiagaraScriptExecutionContext::CanExecute()const
{
	return Script && Script->GetByteCode().Num() > 0;
}