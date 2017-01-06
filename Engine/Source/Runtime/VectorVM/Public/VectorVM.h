// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

//TODO: move to a per platform header and have VM scale vectorization according to vector width.
#define VECTOR_WIDTH (128)
#define VECTOR_WIDTH_BYTES (16)
#define VECTOR_WIDTH_FLOATS (4)

class FVectorVMSharedDataView;

struct FVMExternalFunctionInput
{
	TArray<const uint8*, TInlineAllocator<64>> ComponentPtrs;
	bool bIsConstant;
};

struct FVMExternalFunctionOutput
{
	TArray<uint8*, TInlineAllocator<64>> ComponentPtrs;
};

typedef TArray<FVMExternalFunctionInput, TInlineAllocator<64>> FVMExternalFuncInputs;
typedef TArray<FVMExternalFunctionOutput, TInlineAllocator<64>> FVMExternalFuncOutputs;

DECLARE_DELEGATE_ThreeParams(FVMExternalFunction, FVMExternalFuncInputs& /*Inputs*/, FVMExternalFuncOutputs& /*Outputs*/, uint32 /*NumInstances*/);

UENUM()
enum class EVectorVMBaseTypes : uint8
{
	Float,
	Int,
	Bool,
	Num UMETA(Hidden),
};

UENUM()
enum class EVectorVMOperandLocation : uint8
{
	Register,
	Constant,
	Num
};

UENUM()
enum class EVectorVMOp : uint8
{
	done,
	add,
	sub,
	mul,
	div,
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
	random,
	noise,
	output,

	//Comparison ops.
	cmplt,
	cmple,
	cmpgt,
	cmpge,
	cmpeq,
	cmpneq,
	select,

// 	easein,  Pretty sure these can be replaced with just a single smoothstep implementation.
// 	easeinout,

	//Integer ops
	addi,
	subi,
	muli,
	//divi,//SSE Integer division is not implemented as an intrinsic. Will have to do some manual implementation.
	clampi,
	mini,
	maxi,
	absi,
	negi,
	signi,
	cmplti,
	cmplei,
	cmpgti,
	cmpgei,
	cmpeqi,
	cmpneqi,
	bit_and,
	bit_or,
	bit_xor,
	bit_not,

	//"Boolean" ops. Currently handling bools as integers.
	logic_and,
	logic_or,
	logic_xor,
	logic_not,

	//conversions
	f2i,
	i2f,
	f2b,
	b2f,
	i2b,
	b2i,

	// data read/write
	inputdata_32bit,
	inputdata_noadvance_32bit,
	outputdata_32bit,
	acquireindex,

	external_func_call,

	/** Returns the index of each instance in the current execution context. */
	exec_index,

	noise2D,
	noise3D,

	NumOpcodes
};


struct FDataSetMeta
{
	uint8 **InputRegisters;
	uint8 NumVariables;
	uint32 DataSetSizeInBytes;
	int32 DataSetAccessIndex;	// index for individual elements of this set
	int32 DataSetOffset;		// offset in the register table

	FDataSetMeta(uint32 DataSetSize, uint8 **Data = nullptr, uint8 InNumVariables = 0)
		: InputRegisters(Data), NumVariables(InNumVariables), DataSetSizeInBytes(DataSetSize)
	{}

	FDataSetMeta()
		: InputRegisters(nullptr), NumVariables(0), DataSetSizeInBytes(0), DataSetAccessIndex(0), DataSetOffset(0)
	{}
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
		FirstTempRegister = 0,
		FirstInputRegister = NumTempRegisters,
		FirstOutputRegister = FirstInputRegister + MaxInputRegisters,
		MaxRegisters = NumTempRegisters + MaxInputRegisters + MaxOutputRegisters + MaxConstants,
	};

	/** Get total number of op-codes */
	VECTORVM_API uint8 GetNumOpCodes();

#if WITH_EDITOR
	VECTORVM_API FString GetOpName(EVectorVMOp Op);
	VECTORVM_API FString GetOperandLocationName(EVectorVMOperandLocation Location);
#endif

	VECTORVM_API uint8 CreateSrcOperandMask(EVectorVMOperandLocation Type0, EVectorVMOperandLocation Type1 = EVectorVMOperandLocation::Register, EVectorVMOperandLocation Type2 = EVectorVMOperandLocation::Register);

	/**
	 * Execute VectorVM bytecode.
	 */
	VECTORVM_API void Exec(
		uint8 const* Code,
		uint8** InputRegisters,
		uint8* InputRegisterSizes,
		int32 NumInputRegisters,
		uint8** OutputRegisters,
		uint8* OutputRegisterSizes,
		int32 NumOutputRegisters,
		uint8 const* ConstantTable,
		TArray<FDataSetMeta> &DataSetMetaTable,
		FVMExternalFunction* ExternalFunctionTable,
		int32 NumInstances
		);

	VECTORVM_API void Init();
} // namespace VectorVM

