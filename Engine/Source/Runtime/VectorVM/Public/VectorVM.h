// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"


/* Vector VM data object; encapsulates buffers, curves and other data in its derivatives
*  for access by VectorVM kernels;
*/
class FNiagaraDataObject
{
public:
	virtual FVector4 Sample(const FVector4& InCoords) const = 0;
	virtual FVector4 Write(const FVector4& InCoords, const FVector4 & Value) = 0;
};

/* Curve object; encapsulates a curve for the VectorVM
*/
class FNiagaraCurveDataObject : public FNiagaraDataObject
{
private:
	class UCurveVector *CurveObj;
public:
	FNiagaraCurveDataObject(class UCurveVector *InCurve) : CurveObj(InCurve)
	{
	}

	virtual FVector4 Sample(const FVector4& InCoords) const override
	{
		// commented out for the time being; until Niagara is in its own module, we can't depend on curve objects here
		// because they're part of engine
		//FVector Vec = CurveObj->GetVectorValue(InTime);	
		//return FVector4(Vec, 0.0f);
		return FVector4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	virtual FVector4 Write(const FVector4& InCoords, const FVector4& Value) override
	{
		return FVector4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	UCurveVector *GetCurveObject()	{ return CurveObj; }
	void SetCurveObject(UCurveVector *InCurve)	{ CurveObj = InCurve; }
};


/* Curve object; encapsulates sparse volumetric data for the VectorVM
*/
class FNiagaraSparseVolumeDataObject : public FNiagaraDataObject
{
private:
	TArray<FVector4> Data;
public:
	FNiagaraSparseVolumeDataObject()
	{
		Data.AddZeroed(32768);
	}

	uint32 MakeHash(const FVector4& InCoords) const
	{
		FVector4 Coords = InCoords;
		const uint32 P1 = 73856093; // some large primes
		const uint32 P2 = 19349663;
		const uint32 P3 = 83492791;
		Coords += FVector4(500.0f, 500.0f, 500.0f, 500.0f);
		Coords *= 0.02f;
		Coords = FVector4(FMath::Max(Coords.X, 0.0f), FMath::Max(Coords.Y, 0.0f), FMath::Max(Coords.Z, 0.0f), FMath::Max(Coords.W, 0.0f));
		int N = (P1*static_cast<uint32>(Coords.X)) ^ (P2*static_cast<uint32>(Coords.Y)) ^ (P3*static_cast<uint32>(Coords.Z));
		N %= 32768;
		return FMath::Clamp(N, 0, 32768);
	}

	virtual FVector4 Sample(const FVector4& Coords) const override
	{
		int32 Index = MakeHash(Coords);
		Index = FMath::Clamp(Index, 0, Data.Num() - 1);
		return Data[Index];
	}

	virtual FVector4 Write(const FVector4& InCoords, const FVector4& Value) override
	{
		Data[MakeHash(InCoords)] = Value;
		return Value;
	}
};


namespace VectorVM
{
	/** Constants. */
	enum
	{
		NumTempRegisters = 100,
		MaxInputRegisters = 100,
		MaxOutputRegisters = MaxInputRegisters,
		MaxConstants = 256,
		FirstInputRegister = NumTempRegisters,
		FirstOutputRegister = FirstInputRegister + MaxInputRegisters,
		FirstConstantRegister = FirstOutputRegister + MaxOutputRegisters,
		MaxRegisters = NumTempRegisters + MaxInputRegisters + MaxOutputRegisters + MaxConstants,
	};

	enum EOperandType
	{
		UndefinedOperandType = 0,
		RegisterOperandType,
		ConstantOperandType,
		DataObjConstantOperandType,
		TemporaryOperandType
	};

	/** List of opcodes supported by the VM. */
	enum class EOp
	{
			done,
			add,
			sub,
			mul,
			mad,
			lerp,
			rcp,
			rsq,
			sqrt,
			neg,
			abs,
			exp,
			exp2,
			log,
			log2,
			sin,
			cos,
			tan,
			asin,
			acos,
			atan,
			atan2,
			ceil,
			floor,
			fmod,
			frac,
			trunc,
			clamp,
			min,
			max,
			pow,
			sign,
			step,
			dot,
			cross,
			normalize,
			random,
			length,
			noise,
			splatx,
			splaty,
			splatz,
			splatw,
			compose,
			composex,
			composey,
			composez,
			composew,
			output,
			lessthan,
			sample,
			bufferwrite,
			eventbroadcast,
			NumOpcodes
	};

	/** Get total number of op-codes */
	VECTORVM_API uint8 GetNumOpCodes();

	VECTORVM_API uint8 CreateSrcOperandMask(bool bIsOp0Constant, bool bIsOp1Constant = false, bool bIsOp2Constant = false, bool bIsOp3Constant = false);
	VECTORVM_API uint8 CreateSrcOperandMask(VectorVM::EOperandType Type1, VectorVM::EOperandType Type2 = VectorVM::RegisterOperandType, VectorVM::EOperandType Type3 = VectorVM::RegisterOperandType, VectorVM::EOperandType Type4 = VectorVM::RegisterOperandType);

	/**
	 * Execute VectorVM bytecode.
	 */
	VECTORVM_API void Exec(
		uint8 const* Code,
		VectorRegister** InputRegisters,
		int32 NumInputRegisters,
		VectorRegister** OutputRegisters,
		int32 NumOutputRegisters,
		FVector4 const* ConstantTable,
		FNiagaraDataObject* *DataObjTable,
		int32 NumVectors
		);

	VECTORVM_API void Init();


} // namespace VectorVM


