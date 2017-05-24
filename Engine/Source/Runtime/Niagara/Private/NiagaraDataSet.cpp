// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataSet.h"
#include "NiagaraCommon.h"

//////////////////////////////////////////////////////////////////////////


void FNiagaraDataSet::Dump(bool bCurr) 
{
	TArray<FNiagaraVariable> Vars(Variables);

	FNiagaraDataSetVariableIterator Itr(*this, 0, bCurr);
	Itr.AddVariables(Vars);

	TArray<FString> Lines;
	Lines.Reserve(GetNumInstances());
	while (Itr.IsValid())
	{
		Itr.Get();

		FString Line = TEXT("| ");
		for (FNiagaraVariable& Var : Vars)
		{
			Line += Var.ToString() + TEXT(" | ");
		}
		Lines.Add(Line);
		Itr.Advance();
	}

	static FString Sep;
	if (Sep.Len() == 0)
	{
		for (int32 i = 0; i < 50; ++i)
		{
			Sep.AppendChar(TEXT('='));
		}
	}

	UE_LOG(LogNiagara, Log, TEXT("%s"), *Sep);
	UE_LOG(LogNiagara, Log, TEXT(" Buffer: %d"), CurrBuffer);
 	UE_LOG(LogNiagara, Log, TEXT("%s"), *Sep);
// 	UE_LOG(LogNiagara, Log, TEXT("%s"), *HeaderStr);
// 	UE_LOG(LogNiagara, Log, TEXT("%s"), *Sep);
	for (FString& Str : Lines)
	{
		UE_LOG(LogNiagara, Log, TEXT("%s"), *Str);
	}
	UE_LOG(LogNiagara, Log, TEXT("%s"), *Sep);
}

//////////////////////////////////////////////////////////////////////////

void FNiagaraDataBuffer::Init(FNiagaraDataSet* InOwner, uint32 NumComponents)
{
	Owner = InOwner;
	OffsetTable.SetNumZeroed(NumComponents);
}

void FNiagaraDataBuffer::Allocate(uint32 InNumInstances)
{
	check(Owner && Owner->ComponentSizes.Num() == OffsetTable.Num());
	uint32 CurrSizeBytes = 0;
	NumInstancesAllocated = InNumInstances;
	for (int32 CompIdx = 0; CompIdx < Owner->ComponentSizes.Num(); ++CompIdx)
	{
		OffsetTable[CompIdx] = CurrSizeBytes;

		CurrSizeBytes += GetSafeComponentBufferSize(NumInstancesAllocated * Owner->ComponentSizes[CompIdx]);
	}
	Data.SetNum(CurrSizeBytes, false);
}

uint8* FNiagaraDataBuffer::GetComponentPtr(uint32 ComponentIdx)
{
	return Data.Num() > 0 ? &Data[OffsetTable[ComponentIdx]] : nullptr;
}

uint8* FNiagaraDataBuffer::GetInstancePtr(uint32 ComponentIdx, uint32 InstanceIdx)
{
	uint8* CompPtr = GetComponentPtr(ComponentIdx);
	return CompPtr + InstanceIdx * Owner->ComponentSizes[ComponentIdx];
}

void FNiagaraDataBuffer::KillInstance(uint32 InstanceIdx)
{
	check(InstanceIdx < NumInstances);
	--NumInstances;
	for (int32 CompIdx = 0; CompIdx < Owner->ComponentSizes.Num(); ++CompIdx)
	{
		uint32 Offset = OffsetTable[CompIdx];
		uint32 CompSize = Owner->ComponentSizes[CompIdx];
		void* DeadIdx = &Data[Offset + InstanceIdx * CompSize];
		void* SwapIdx = &Data[Offset + NumInstances * CompSize];

		FMemory::Memcpy(DeadIdx, SwapIdx, CompSize);
	}
}

