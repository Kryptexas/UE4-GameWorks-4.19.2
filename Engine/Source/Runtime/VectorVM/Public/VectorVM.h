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
		NumTempRegisters = 8,
		MaxInputRegisters = 32,
		MaxOutputRegisters = MaxInputRegisters,
		FirstInputRegister = NumTempRegisters,
		FirstOutputRegister = FirstInputRegister + MaxInputRegisters,
		MaxRegisters = NumTempRegisters + MaxInputRegisters + MaxOutputRegisters,
		MaxConstants = 256,
	};

	/** List of opcodes supported by the VM. */
	namespace EOp
	{
		enum Type
		{
			done = 0,
			add,
			addi,
			sub,
			subi,
			mul,
			muli,
			mad,
			madrri,
			madrir,
			madrii,
			madiir, // Remove me when a constant expressions can be evaluated at a coarser granularity.
			madiii, // Remove me when a constant expressions can be evaluated at a coarser granularity.
			lerp,
			lerpirr, 
			lerprir, 
			lerprri,
			lerprii,
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
			clampir,
			clampri,
			clampii,
			min,
			mini,
			max,
			maxi,
			pow,
			powi,
			sign,
			step,
			stepi,
			tex1d,
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
			EOp::Type InBaseOpcode,
			uint8 InFlags,
			EOpSrc::Type Src0,
			EOpSrc::Type Src1,
			EOpSrc::Type Src2,
			const FString& InFriendlyName
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
		float const* ConstantTable,
		int32 NumVectors
		);

} // namespace VectorVM
