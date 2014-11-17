// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	VectorVM.h: Public interface to the vector virtual machine.
==============================================================================*/

#pragma once
#include "Core.h"



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
		MaxRegisters = NumTempRegisters + MaxInputRegisters + MaxOutputRegisters + MaxConstants
	};


	/** List of opcodes supported by the VM. */
	namespace EOp
	{
		enum Type
		{
			done = 0,
			add = 1,
			sub = 3,
			mul = 5,
			mad = 7,
			lerp = 13,
			rcp = 18,
			rsq = 19,
			sqrt = 20,
			neg = 21,
			abs = 22,
			exp = 23,
			exp2 = 24,
			log = 25,
			log2 = 26,
			sin = 27,
			cos = 29,
			tan = 30,
			asin = 31,
			acos = 32,
			atan = 33,
			atan2 = 34,
			ceil = 35,
			floor = 36,
			fmod = 37,
			frac = 38,
			trunc = 39,
			clamp = 40,
			min = 44,
			max = 46,
			pow = 48,
			sign = 50,
			step = 51,
			dot = 54,
			cross = 55,
			normalize = 57,
			random = 58,
			length = 59,
			sin4 = 61, 
			noise = 66,
			NumOpcodes
		};
	} // namespace EOp

	/** Operand sources. */
	namespace EOpSrc
	{
		enum Type
		{
			Invalid,
			Register,
			Const
		};
	}

	namespace EOpFlags
	{
		enum
		{
			None = 0x0,
			Implemented = 0x1,
			Commutative = 0x2
		};
	}

	/** Struct used to give info on particular op codes */
	struct VECTORVM_API FVectorVMOpInfo
	{
		/** Base opcode. */
		EOp::Type	BaseOpcode;
		/** Flags. */
		uint8		Flags;
		/** Source operands. */
		EOpSrc::Type SrcTypes[3];
		/** Friendly name of op-code */
		FString		FriendlyName;

		/** Constructor */
		FVectorVMOpInfo(
			EOp::Type InBaseOpcode = EOp::done,
			uint8 InFlags = EOpFlags::None,
			EOpSrc::Type Src0 = EOpSrc::Type::Invalid,
			EOpSrc::Type Src1 = EOpSrc::Type::Invalid,
			EOpSrc::Type Src2 = EOpSrc::Type::Invalid,
			const FString& InFriendlyName = TEXT("Undefined Opcode")
			)
			: BaseOpcode(InBaseOpcode)
			, Flags(InFlags)
			, FriendlyName(InFriendlyName)
		{
			SrcTypes[0] = Src0;
			SrcTypes[1] = Src1;
			SrcTypes[2] = Src2;
		}

		FORCEINLINE bool IsImplemented() const { return (Flags & EOpFlags::Implemented) != 0; }
		FORCEINLINE bool IsCommutative() const { return (Flags & EOpFlags::Commutative) != 0; }
	};
	
	/** Get info on a particular op-code */
	VECTORVM_API FVectorVMOpInfo const& GetOpCodeInfo(uint8 OpCodeIndex);
	/** Get total number of op-codes */
	VECTORVM_API uint8 GetNumOpCodes();

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
		int32 NumVectors
		);

	VECTORVM_API void Init();


} // namespace VectorVM
