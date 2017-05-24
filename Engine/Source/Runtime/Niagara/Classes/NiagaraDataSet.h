// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "NiagaraCommon.h"
#include "RHI.h"
#include "VectorVM.h"

/** Helper class defining the layout and location of an FNiagaraVariable in an FNiagaraDataBuffer. */
struct FNiagaraVariableLayoutInfo
{
	/** Index of this variable in the combined component arrays of FNiagaraDataSet and FNiagaraDataBuffer. */
	uint32 ComponentIdx;
	/** This variable's type layout info. */
	FNiagaraTypeLayoutInfo LayoutInfo;
};

class FNiagaraDataSet;

/** Buffer containing one frame of Niagara simulation data. */
class FNiagaraDataBuffer
{
public:
	void Init(FNiagaraDataSet* InOwner, uint32 NumComponents);
	void Allocate(uint32 NumInstances);
	void KillInstance(uint32 InstanceIdx);

	/** Returns a ptr to the start of the buffer for the passed component idx. */
	uint8* GetComponentPtr(uint32 ComponentIdx);
	/** Returns a ptr to specific instance data in the buffer for the passed component idx. */
	uint8* GetInstancePtr(uint32 ComponentIdx, uint32 InstanceIdx);

	uint32 GetNumInstances()const { return NumInstances; }
	uint32 GetNumInstancesAllocated()const { return NumInstancesAllocated; }

	void SetNumInstances(uint32 InNumInstances) { NumInstances = InNumInstances; }
	
	void Reset()
	{
		OffsetTable.Empty();
		Data.Empty();
		NumInstances = 0;
		NumInstancesAllocated = 0;
	}

	uint32 GetSizeBytes()const
	{
		return Data.Num();
	}

private:

	int32 GetSafeComponentBufferSize(int32 RequiredSize)
	{
		//Round up to VECTOR_WIDTH_BYTES.
		//Both aligns the component buffers to the vector width but also ensures their ops cannot stomp over one another.		
		return RequiredSize + VECTOR_WIDTH_BYTES - (RequiredSize % VECTOR_WIDTH_BYTES);
	}

	/** Back ptr to our owning data set. Used to access layout info for the buffer. */
	FNiagaraDataSet* Owner;
	/** Offset to the beginning of each component in the data array. */
	TArray<uint32> OffsetTable;
	/** Simulation data. */
	TArray<uint8> Data;
	/** Number of instances in data. */
	uint32 NumInstances;
	/** Number of instances the buffer has been allocated for. */
	uint32 NumInstancesAllocated;
};

//////////////////////////////////////////////////////////////////////////

/**
General storage class for all per instance simulation data in Niagara.
*/
class FNiagaraDataSet
{
	friend FNiagaraDataBuffer;
public:
	FNiagaraDataSet()
	{
		Reset();
	}
	
	FNiagaraDataSet(FNiagaraDataSetID InID)
		: ID(InID)
	{
		Reset();
	}

	void Reset()
	{
		Variables.Empty();
		VariableLayoutMap.Empty();
		ComponentSizes.Empty();
		Data[0].Reset();
		Data[1].Reset();
		CurrBuffer = 0;
		bFinalized = false;
	}

	void AddVariable(FNiagaraVariable& Variable)
	{
		check(!bFinalized);
		Variables.AddUnique(Variable);
	}

	void AddVariables(const TArray<FNiagaraVariable>& Vars)
	{
		check(!bFinalized);
		for (const FNiagaraVariable& Var : Vars)
		{
			Variables.AddUnique(Var);
		}
	}

	/** Finalize the addition of variables and other setup before this data set can be used. */
	void Finalize()
	{
		check(!bFinalized);
		bFinalized = true;
		BuildLayout();
	}

	/** Removes a specific instance from the current frame's data buffer. */
	void KillInstance(uint32 InstanceIdx)
	{
		check(bFinalized);
		CurrData().KillInstance(InstanceIdx);
	}

	/** Appends the passed variable to the set of input and output registers ready for consumption by the VectorVM. */
	bool AppendToRegisterTable(const FNiagaraVariable& VarInfo, uint8** InputRegisters, uint8* InputRegisterSizes, int32& NumInputRegisters, uint8** OutputRegisters, uint8* OutputRegisterSizes, int32& NumOutputRegisters, int32 StartInstance, bool bNoOutputRegisters=false)
	{
		check(bFinalized);
		if (const FNiagaraVariableLayoutInfo* VariableLayout = VariableLayoutMap.Find(VarInfo))
		{
			uint32 StartIdx = VariableLayout->ComponentIdx;
			uint32 EndIdx = StartIdx + VariableLayout->LayoutInfo.GetNumScalarComponents();
			for (uint32 VarComp = StartIdx; VarComp < EndIdx; ++VarComp)
			{
				InputRegisters[NumInputRegisters] = PrevData().GetInstancePtr(VarComp, StartInstance);
				InputRegisterSizes[NumInputRegisters++] = ComponentSizes[VarComp];

				// we won't be writing to the data set
				if (bNoOutputRegisters)
				{
					OutputRegisters[NumOutputRegisters] = nullptr;
					OutputRegisterSizes[NumOutputRegisters++] = 0;
				}
				else
				{
					OutputRegisters[NumOutputRegisters] = CurrData().GetInstancePtr(VarComp, StartInstance);
					OutputRegisterSizes[NumOutputRegisters++] = ComponentSizes[VarComp];
				}
			}
			return true;
		}
		return false;
	}

	bool AppendSingleInstanceToRegisterTable(const FNiagaraVariable& VarInfo, uint8** InputRegisters, uint8* InputRegisterSizes, int32& NumInputRegisters, uint8** OutputRegisters, uint8* OutputRegisterSizes, int32& NumOutputRegisters, int32 InstanceIdx)
	{
		return false;
	}

	void Allocate(uint32 NumInstances)
	{
		check(bFinalized);
		CurrData().Allocate(NumInstances);
	}

	void Tick()
	{
		SwapBuffers();
	}

	void CopyPrevToCur()
	{
		CurrData().Allocate(PrevData().GetNumInstances());
		CurrData().SetNumInstances(PrevData().GetNumInstances());
		FMemory::Memcpy(CurrData().GetInstancePtr(0, 0), PrevData().GetInstancePtr(0, 0), CurrData().GetSizeBytes());
	}

	FNiagaraDataSetID GetID()const { return ID; }
	void SetID(FNiagaraDataSetID InID) { ID = InID; }

	FNiagaraDataBuffer& CurrData() { return Data[CurrBuffer]; }
	FNiagaraDataBuffer& PrevData() { return Data[CurrBuffer ^ 0x1]; }
	const FNiagaraDataBuffer& CurrData()const { return Data[CurrBuffer]; }
	const FNiagaraDataBuffer& PrevData()const { return Data[CurrBuffer ^ 0x1]; }

	uint32 GetNumInstances()const { return CurrData().GetNumInstances(); }
	uint32 GetNumInstancesAllocated()const { return CurrData().GetNumInstancesAllocated(); }
	void SetNumInstances(uint32 InNumInstances) { CurrData().SetNumInstances(InNumInstances); }

	void ResetNumInstances()
	{
		CurrData().SetNumInstances(0);
		PrevData().SetNumInstances(0);
	}

	uint32 GetPrevNumInstances()const { return PrevData().GetNumInstances(); }

	uint32 GetNumVariables()const { return Variables.Num(); }

	uint32 GetSizeBytes()const
	{
		return Data[0].GetSizeBytes() + Data[1].GetSizeBytes();
	}

	bool HasVariable(const FNiagaraVariable& Var)const 
	{
		return Variables.Contains(Var);
	}

	const FNiagaraVariableLayoutInfo* GetVariableLayout(const FNiagaraVariable& Var)const
	{
		return VariableLayoutMap.Find(Var);
	}

	void Dump(bool bCurr);
	const TArray<FNiagaraVariable> &GetVariables() { return Variables; }
private:

	void SwapBuffers()
	{
		CurrBuffer ^= 0x1;
	}
	
	void BuildLayout()
	{
		VariableLayoutMap.Empty();
		ComponentSizes.Empty();

		uint32 NumScalarComponents = 0;
		for (FNiagaraVariable& Var : Variables)
		{
			FNiagaraVariableLayoutInfo& VarInfo = VariableLayoutMap.Add(Var);
			VarInfo.ComponentIdx = NumScalarComponents;
			FNiagaraTypeLayoutInfo::GenerateLayoutInfo(VarInfo.LayoutInfo, Var.GetType().GetScriptStruct());

			ComponentSizes.Append(VarInfo.LayoutInfo.ComponentSizes);
			NumScalarComponents += VarInfo.LayoutInfo.GetNumScalarComponents();
		}
		CurrData().Init(this, NumScalarComponents);
		PrevData().Init(this, NumScalarComponents);
	}

	uint32 CurrIdx()const { return CurrBuffer; }
	uint32 PrevIdx()const {	return CurrBuffer ^ 0x1; }
		
	/** Unique ID for this data set. Used to allow referencing from other emitters and effects. */
	FNiagaraDataSetID ID;
	/** Variables in the data set. */
	TArray<FNiagaraVariable> Variables;
	/** Combined array of sizes for every scalar component in the data set.*/
	TArray<uint32> ComponentSizes;
	/** Map from the variable to some extra data describing it's layout in the data set. */
	TMap<FNiagaraVariable, FNiagaraVariableLayoutInfo> VariableLayoutMap;
	/** Index of current state data. */
	uint32 CurrBuffer;
	/** Once finalized, the data layout etc is built and no more variables can be added. */
	bool bFinalized;

	FNiagaraDataBuffer Data[2];
};

/**
General iterator for getting and setting data in and FNiagaraDataSet.
*/
struct FNiagaraDataSetIteratorBase
{
	FNiagaraDataSetIteratorBase(const FNiagaraDataSet& InDataSet, FNiagaraVariable InVar, uint32 StartIdx = 0, bool bCurrBuffer = true)
		: DataSet(const_cast<FNiagaraDataSet&>(InDataSet))//TODO HANDLE CONST ITR
		, DataBuffer(bCurrBuffer ? DataSet.CurrData() : DataSet.PrevData())
		, Variable(InVar)
		, CurrIdx(StartIdx)
	{
		VarLayout = DataSet.GetVariableLayout(Variable);

		if (!VarLayout)
		{
			CurrIdx = INDEX_NONE;
			NumScalarComponents = 0;
		}
		else
		{
			NumScalarComponents = VarLayout->LayoutInfo.GetNumScalarComponents();
		}
	}

	void Advance() { ++CurrIdx; }
	
	bool IsValid()const
	{
		return CurrIdx < DataBuffer.GetNumInstances();
	}

	uint32 GetCurrIndex()const { return CurrIdx; }

protected:

	FNiagaraDataSet& DataSet;
	FNiagaraDataBuffer& DataBuffer;
	FNiagaraVariable Variable;
	const FNiagaraVariableLayoutInfo* VarLayout;

	uint32 NumScalarComponents;
	uint32 CurrIdx;
};

template<typename T>
struct FNiagaraDataSetIterator : public FNiagaraDataSetIteratorBase
{
	FNiagaraDataSetIterator(const FNiagaraDataSet& InDataSet, FNiagaraVariable InVar, uint32 StartIdx = 0, bool bCurrBuffer = true)
		: FNiagaraDataSetIteratorBase(InDataSet, InVar, StartIdx, bCurrBuffer)
	{
		check(sizeof(T) == InVar.GetType().GetSize());		
		checkf(false, TEXT("You must provide a fast runtime specialization for this type."));// Allow this slow generic version?
	}

	T operator*() { return Get(); }
	T Get()const 
	{
		T Ret;	
		GetValue(&Ret);
		return Ret; 	
	}
	void Get(T& OutValue)const	{ GetValue(&OutValue);	}
	//T* operator *(const T& InValue){ Set(InValue);	}

	void Set(const T& InValue)
	{
		uint8* ValuePtr = (uint8*)&InValue;
		uint32 StartIdx = VarLayout->ComponentIdx;
		uint32 EndIdx = StartIdx + VarLayout->LayoutInfo.GetNumScalarComponents();
		for (uint32 VarComp = 0; VarComp < VarLayout->LayoutInfo.GetNumScalarComponents(); ++VarComp)
		{
			int32 SetComp = VarLayout->ComponentIdx + VarComp;

			uint8* DestPtr = DataBuffer.GetInstancePtr(SetComp, CurrIdx);
			uint8* SrcPtr = ValuePtr + VarLayout->LayoutInfo.ComponentOffsets[VarComp];
			FMemory::Memcpy(DestPtr, SrcPtr, VarLayout->LayoutInfo.ComponentSizes[VarComp]);
		}
	}

private:

	void GetValue(T* Output)const
	{
		uint8* ValuePtr = (uint8*)Output;
		for (uint32 VarComp = 0; VarComp < NumScalarComponents; ++VarComp)
		{
			uint32 SetComp = VarLayout->ComponentIdx + VarComp;

			uint8* DestPtr = ValuePtr + VarLayout->LayoutInfo.ComponentOffsets[VarComp];
			uint8* SrcPtr = DataBuffer.GetInstancePtr(SetComp, CurrIdx);
			FMemory::Memcpy(DestPtr, SrcPtr, VarLayout->LayoutInfo.ComponentSizes[VarComp]);
		}
	}
};

template<>
struct FNiagaraDataSetIterator<int32> : public FNiagaraDataSetIteratorBase
{
	FNiagaraDataSetIterator<int32>(const FNiagaraDataSet& InDataSet, FNiagaraVariable InVar, uint32 StartIdx = 0, bool bCurrBuffer = true)
		: FNiagaraDataSetIteratorBase(InDataSet, InVar, StartIdx, bCurrBuffer)
	{
		check(sizeof(int32) == InVar.GetType().GetSize());	
		if (VarLayout != nullptr)
		{
			Base = (int32*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx);
		}
		else
		{
			Base = nullptr;
		}
	}


	FORCEINLINE int32 operator*() { return Get(); }
	FORCEINLINE int32 Get()const
	{
		int32 Ret;
		GetValue(&Ret);
		return Ret;
	}
	FORCEINLINE void Get(int32& OutValue)const { GetValue(&OutValue); }

	FORCEINLINE int32 GetAt(int32 Idx)
	{
		int32 Ret;
		GetValueAt(Idx, &Ret);
		return Ret;
	}

	//FORCEINLINE int32* operator *(const int32& InValue) { Set(InValue); }

	FORCEINLINE void Set(const int32 InValue)
	{
		Base[CurrIdx] = InValue;
	}

private:

	FORCEINLINE void GetValue(int32* Output)const
	{
		*Output = Base[CurrIdx];
	}

	FORCEINLINE void GetValueAt(int32 Idx, int32* Output)const
	{
		*Output = Base[Idx];
	}

	int32* Base;
};

template<>
struct FNiagaraDataSetIterator<float> : public FNiagaraDataSetIteratorBase
{
	FNiagaraDataSetIterator<float>(const FNiagaraDataSet& InDataSet, FNiagaraVariable InVar, uint32 StartIdx = 0, bool bCurrBuffer = true)
		: FNiagaraDataSetIteratorBase(InDataSet, InVar, StartIdx, bCurrBuffer)
	{
		check(sizeof(float) == InVar.GetType().GetSize());
		if (VarLayout != nullptr)
		{
			Base = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx);
		}
		else
		{
			Base = nullptr;
		}
	}

	FORCEINLINE float operator*() { return Get(); }
	FORCEINLINE float Get()const
	{
		float Ret;
		GetValue(&Ret);
		return Ret;
	}
	FORCEINLINE void Get(float& OutValue)const { GetValue(&OutValue); }

	FORCEINLINE float GetAt(int32 Idx)
	{
		float Ret;
		GetValueAt(Idx, &Ret);
		return Ret;
	}

	//FORCEINLINE float* operator *(const float& InValue) { Set(InValue); }

	FORCEINLINE void Set(const float& InValue)
	{
		Base[CurrIdx] = InValue;
	}

private:

	FORCEINLINE void GetValue(float* Output)const
	{
		*Output = Base[CurrIdx];
	}

	FORCEINLINE void GetValueAt(int32 Idx, float* Output)const
	{
		*Output = Base[Idx];
	}
	float* Base;
};

template<>
struct FNiagaraDataSetIterator<FVector2D> : public FNiagaraDataSetIteratorBase
{
	FNiagaraDataSetIterator<FVector2D>(const FNiagaraDataSet& InDataSet, FNiagaraVariable InVar, uint32 StartIdx = 0, bool bCurrBuffer = true)
		: FNiagaraDataSetIteratorBase(InDataSet, InVar, StartIdx, bCurrBuffer)
	{
		check(sizeof(FVector2D) == InVar.GetType().GetSize());

		if (VarLayout != nullptr)
		{
			XBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx);
			YBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx + 1);
		}
		else
		{
			XBase = nullptr;
			YBase = nullptr;
		}
	}

	FORCEINLINE FVector2D operator*() { return Get(); }
	FORCEINLINE FVector2D Get()const
	{
		FVector2D Ret;
		GetValue(&Ret);
		return Ret;
	}
	FORCEINLINE void Get(FVector2D& OutValue)const { GetValue(&OutValue); }

	FORCEINLINE FVector2D GetAt(int32 Idx)
	{
		FVector2D Ret;
		GetValueAt(Idx, &Ret);
		return Ret;
	}
	//FORCEINLINE FVector2D* operator *(const FVector2D& InValue) { Set(InValue); }

	FORCEINLINE void Set(const FVector2D& InValue)
	{
		XBase[CurrIdx] = InValue.X;
		YBase[CurrIdx] = InValue.Y;
	}

private:

	FORCEINLINE void GetValue(FVector2D* Output)const
	{
		Output->X = XBase[CurrIdx];
		Output->Y = YBase[CurrIdx];
	}

	FORCEINLINE void GetValueAt(int32 Idx, FVector2D* Output)const
	{
		Output->X = XBase[Idx];
		Output->Y = YBase[Idx];
	}
	float* XBase;
	float* YBase;
};

template<>
struct FNiagaraDataSetIterator<FVector> : public FNiagaraDataSetIteratorBase
{
	FNiagaraDataSetIterator<FVector>(const FNiagaraDataSet& InDataSet, FNiagaraVariable InVar, uint32 StartIdx = 0, bool bCurrBuffer = true)
		: FNiagaraDataSetIteratorBase(InDataSet, InVar, StartIdx, bCurrBuffer)
	{
		check(sizeof(FVector) == InVar.GetType().GetSize());
		if (VarLayout != nullptr)
		{
			XBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx);
			YBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx + 1);
			ZBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx + 2);
		}
		else
		{
			XBase = nullptr;
			YBase = nullptr;
			ZBase = nullptr;
		}
	}

	FORCEINLINE FVector operator*() { return Get(); }
	FORCEINLINE FVector Get()const
	{
		FVector Ret;
		GetValue(&Ret);
		return Ret;
	}
	FORCEINLINE void Get(FVector& OutValue)const { GetValue(&OutValue); }

	FORCEINLINE FVector GetAt(int32 Idx)
	{
		FVector Ret;
		GetValueAt(Idx, &Ret);
		return Ret;
	}
	//FORCEINLINE FVector* operator *(const FVector& InValue) { Set(InValue); }

	FORCEINLINE void Set(const FVector& InValue)
	{
		XBase[CurrIdx] = InValue.X;
		YBase[CurrIdx] = InValue.Y;
		ZBase[CurrIdx] = InValue.Z;
	}

private:

	FORCEINLINE void GetValue(FVector* Output)const
	{
		Output->X = XBase[CurrIdx];
		Output->Y = YBase[CurrIdx];
		Output->Z = ZBase[CurrIdx];
	}

	FORCEINLINE void GetValueAt(int32 Idx, FVector* Output)const
	{
		Output->X = XBase[Idx];
		Output->Y = YBase[Idx];
		Output->Z = ZBase[Idx];
	}


	float* XBase;
	float* YBase;
	float* ZBase;
};

template<>
struct FNiagaraDataSetIterator<FVector4> : public FNiagaraDataSetIteratorBase
{
	FNiagaraDataSetIterator<FVector4>(const FNiagaraDataSet& InDataSet, FNiagaraVariable InVar, uint32 StartIdx = 0, bool bCurrBuffer = true)
		: FNiagaraDataSetIteratorBase(InDataSet, InVar, StartIdx, bCurrBuffer)
	{
		check(sizeof(FVector4) == InVar.GetType().GetSize());
		if (VarLayout)
		{
			XBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx);
			YBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx + 1);
			ZBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx + 2);
			WBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx + 3);
		}
		else
		{
			XBase = nullptr;
			YBase = nullptr;
			ZBase = nullptr;
			WBase = nullptr;
		}
	}

	FORCEINLINE FVector4 operator*() { return Get(); }
	FORCEINLINE FVector4 Get()const
	{
		FVector4 Ret;
		GetValue(&Ret);
		return Ret;
	}
	FORCEINLINE void Get(FVector4& OutValue)const { GetValue(&OutValue); }

	FORCEINLINE FVector4 GetAt(int32 Idx)
	{
		FVector4 Ret;
		GetValueAt(Idx, &Ret);
		return Ret;
	}
	//FORCEINLINE FVector4* operator *(const FVector4& InValue) { Set(InValue); }

	FORCEINLINE void Set(const FVector4& InValue)
	{
		XBase[CurrIdx] = InValue.X;
		YBase[CurrIdx] = InValue.Y;
		ZBase[CurrIdx] = InValue.Z;
		WBase[CurrIdx] = InValue.W;
	}

private:

	FORCEINLINE void GetValue(FVector4* Output)const
	{
		Output->X = XBase[CurrIdx];
		Output->Y = YBase[CurrIdx];
		Output->Z = ZBase[CurrIdx];
		Output->W = WBase[CurrIdx];
	}

	FORCEINLINE void GetValueAt(int32 Idx, FVector4* Output)const
	{
		Output->X = XBase[Idx];
		Output->Y = YBase[Idx];
		Output->Z = ZBase[Idx];
		Output->W = WBase[Idx];
	}

	float* XBase;
	float* YBase;
	float* ZBase;
	float* WBase;
};

template<>
struct FNiagaraDataSetIterator<FLinearColor> : public FNiagaraDataSetIteratorBase
{
	FNiagaraDataSetIterator<FLinearColor>(const FNiagaraDataSet& InDataSet, FNiagaraVariable InVar, uint32 StartIdx = 0, bool bCurrBuffer = true)
		: FNiagaraDataSetIteratorBase(InDataSet, InVar, StartIdx, bCurrBuffer)
	{
		check(sizeof(FLinearColor) == InVar.GetType().GetSize());
		if (VarLayout)
		{
			RBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx);
			GBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx + 1);
			BBase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx + 2);
			ABase = (float*)DataBuffer.GetComponentPtr(VarLayout->ComponentIdx + 3);
		}
		else
		{
			RBase = nullptr;
			GBase = nullptr;
			BBase = nullptr;
			ABase = nullptr;
		}
	}

	FORCEINLINE FLinearColor operator*() { return Get(); }
	FORCEINLINE FLinearColor Get()const
	{
		FLinearColor Ret;
		GetValue(&Ret);
		return Ret;
	}
	FORCEINLINE void Get(FLinearColor& OutValue)const { GetValue(&OutValue); }

	FORCEINLINE FLinearColor GetAt(int32 Idx)
	{
		FLinearColor Ret;
		GetValueAt(Idx, &Ret);
		return Ret;
	}

	//FORCEINLINE FLinearColor* operator *(const FLinearColor& InValue) { Set(InValue); }

	FORCEINLINE void Set(const FLinearColor& InValue)
	{
		RBase[CurrIdx] = InValue.R;
		GBase[CurrIdx] = InValue.G;
		BBase[CurrIdx] = InValue.B;
		ABase[CurrIdx] = InValue.A;
	}

private:

	FORCEINLINE void GetValue(FLinearColor* Output)const
	{
		Output->R = RBase[CurrIdx];
		Output->G = GBase[CurrIdx];
		Output->B = BBase[CurrIdx];
		Output->A = ABase[CurrIdx];
	}

	FORCEINLINE void GetValueAt(int32 Idx, FLinearColor* Output)const
	{
		Output->R = RBase[Idx];
		Output->G = GBase[Idx];
		Output->B = BBase[Idx];
		Output->A = ABase[Idx];
	}
	float* RBase;
	float* GBase;
	float* BBase;
	float* ABase;
};

/** 
Iterator that will pull or push data between a DataSet and some FNiagaraVariables it contains.
Super slow. Don't use at runtime.
*/
struct FNiagaraDataSetVariableIterator
{
	FNiagaraDataSetVariableIterator(FNiagaraDataSet& InDataSet, uint32 StartIdx = 0, bool bCurrBuffer = true)
		: DataSet(InDataSet)
		, DataBuffer(bCurrBuffer ? DataSet.CurrData() : DataSet.PrevData())
		, CurrIdx(StartIdx)
	{
	}

	void Get()
	{
		for (int32 VarIdx = 0; VarIdx < Variables.Num(); ++VarIdx)
		{
			FNiagaraVariable* Var = Variables[VarIdx];
			const FNiagaraVariableLayoutInfo* Layout = VarLayouts[VarIdx];

			check(Var && Layout);

			for (uint32 VarComp = 0; VarComp < Layout->LayoutInfo.GetNumScalarComponents(); ++VarComp)
			{
				int32 SetComp = Layout->ComponentIdx + VarComp;

				uint8* DestPtr = Var->GetData() + Layout->LayoutInfo.ComponentOffsets[VarComp];
				uint8* SrcPtr = DataBuffer.GetInstancePtr(SetComp, CurrIdx);
				FMemory::Memcpy(DestPtr, SrcPtr, Layout->LayoutInfo.ComponentSizes[VarComp]);
			}
		}
	}

	void Set()
	{
		for (int32 VarIdx = 0; VarIdx < Variables.Num(); ++VarIdx)
		{
			FNiagaraVariable* Var = Variables[VarIdx];
			const FNiagaraVariableLayoutInfo* Layout = VarLayouts[VarIdx];

			check(Var && Layout);

			for (uint32 VarComp = 0; VarComp < Layout->LayoutInfo.GetNumScalarComponents(); ++VarComp)
			{
				uint32 SetComp = Layout->ComponentIdx + VarComp;

				uint8* DestPtr = DataBuffer.GetInstancePtr(SetComp, CurrIdx); 
				uint8* SrcPtr = Var->GetData() + Layout->LayoutInfo.ComponentOffsets[VarComp];
				FMemory::Memcpy(DestPtr, SrcPtr, Layout->LayoutInfo.ComponentSizes[VarComp]);
			}
		}
	}

	void Advance() { ++CurrIdx; }

	bool IsValid()const
	{
		return CurrIdx < DataBuffer.GetNumInstances();
	}

	uint32 GetCurrIndex()const { return CurrIdx; }

	void AddVariable(FNiagaraVariable* InVar)
	{
		check(InVar);
		Variables.AddUnique(InVar);
		VarLayouts.AddUnique(DataSet.GetVariableLayout(*InVar));
		InVar->AllocateData();
	}

	void AddVariables(TArray<FNiagaraVariable>& Vars)
	{
		for (FNiagaraVariable& Var : Vars)
		{
			AddVariable(&Var);
		}
	}
private:
	
	FNiagaraDataSet& DataSet;
	FNiagaraDataBuffer& DataBuffer;
	TArray<FNiagaraVariable*> Variables;
	TArray<const FNiagaraVariableLayoutInfo*> VarLayouts;

	uint32 CurrIdx;
};