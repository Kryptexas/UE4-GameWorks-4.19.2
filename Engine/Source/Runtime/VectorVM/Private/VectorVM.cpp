// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VectorVMPrivate.h"
#include "CurveVector.h"
#include "VectorVMDataObject.h"
#include "ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, VectorVM);

DEFINE_LOG_CATEGORY_STATIC(LogVectorVM, All, All);

//#define VM_FORCEINLINE
#define VM_FORCEINLINE FORCEINLINE

#define OP_REGISTER (0)
#define OP0_CONST (1 << 0)
#define OP1_CONST (1 << 1)
#define OP2_CONST (1 << 2)
#define OP3_CONST (1 << 3)
#define OP0_DATAOBJ (1 << 4)
#define OP1_DATAOBJ (1 << 5)
#define OP2_DATAOBJ (1 << 6)
#define OP3_DATAOBJ (1 << 7)

#define SRCOP_RRRR (OP_REGISTER | OP_REGISTER | OP_REGISTER | OP_REGISTER)
#define SRCOP_RRRC (OP_REGISTER | OP_REGISTER | OP_REGISTER | OP0_CONST)
#define SRCOP_RRCR (OP_REGISTER | OP_REGISTER | OP1_CONST | OP_REGISTER)
#define SRCOP_RRCC (OP_REGISTER | OP_REGISTER | OP1_CONST | OP0_CONST)
#define SRCOP_RCRR (OP_REGISTER | OP2_CONST | OP_REGISTER | OP_REGISTER)
#define SRCOP_RCRC (OP_REGISTER | OP2_CONST | OP_REGISTER | OP0_CONST)
#define SRCOP_RCCR (OP_REGISTER | OP2_CONST | OP1_CONST | OP_REGISTER)
#define SRCOP_RCCC (OP_REGISTER | OP2_CONST | OP1_CONST | OP0_CONST)
#define SRCOP_CRRR (OP3_CONST | OP_REGISTER | OP_REGISTER | OP_REGISTER)
#define SRCOP_CRRC (OP3_CONST | OP_REGISTER | OP_REGISTER | OP0_CONST)
#define SRCOP_CRCR (OP3_CONST | OP_REGISTER | OP1_CONST | OP_REGISTER)
#define SRCOP_CRCC (OP3_CONST | OP_REGISTER | OP1_CONST | OP0_CONST)
#define SRCOP_CCRR (OP3_CONST | OP2_CONST | OP_REGISTER | OP_REGISTER)
#define SRCOP_CCRC (OP3_CONST | OP2_CONST | OP_REGISTER | OP0_CONST)
#define SRCOP_CCCR (OP3_CONST | OP2_CONST | OP1_CONST | OP_REGISTER)
#define SRCOP_CCCC (OP3_CONST | OP2_CONST | OP1_CONST | OP0_CONST)

#define SRCOP_RRRB (OP_REGISTER | OP_REGISTER | OP_REGISTER | OP0_DATAOBJ)
#define SRCOP_RRBR (OP_REGISTER | OP_REGISTER | OP1_DATAOBJ | OP_REGISTER)
#define SRCOP_RRBB (OP_REGISTER | OP_REGISTER | OP1_DATAOBJ | OP0_DATAOBJ)

#define SRCOP_RRCB (OP_REGISTER | OP_REGISTER | OP1_CONST | OP0_DATAOBJ)

uint8 VectorVM::CreateSrcOperandMask(VectorVM::EOperandType Type1, VectorVM::EOperandType Type2, VectorVM::EOperandType Type3, VectorVM::EOperandType Type4)
{
	return	(Type1 == VectorVM::ConstantOperandType ? OP0_CONST : OP_REGISTER) |
		(Type2 == VectorVM::ConstantOperandType ? OP1_CONST : OP_REGISTER) |
		(Type3 == VectorVM::ConstantOperandType ? OP2_CONST : OP_REGISTER) |
		(Type4 == VectorVM::ConstantOperandType ? OP3_CONST : OP_REGISTER) |
		(Type1 == VectorVM::DataObjConstantOperandType ? OP0_DATAOBJ : OP_REGISTER) |
		(Type2 == VectorVM::DataObjConstantOperandType ? OP1_DATAOBJ : OP_REGISTER) |
		(Type3 == VectorVM::DataObjConstantOperandType ? OP2_DATAOBJ : OP_REGISTER) |
		(Type4 == VectorVM::DataObjConstantOperandType ? OP3_DATAOBJ : OP_REGISTER);
}


UNiagaraDataObject::UNiagaraDataObject(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

UNiagaraCurveDataObject::UNiagaraCurveDataObject(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

FVector4 UNiagaraCurveDataObject::Sample(const FVector4& InCoords) const
{
	FVector Vec = CurveObj->GetVectorValue(InCoords.X);
	return FVector4(Vec, 0.0f);
}




UNiagaraSparseVolumeDataObject::UNiagaraSparseVolumeDataObject(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	Size = 64;
	NumBuckets = Size*Size*Size;
	//Data.AddZeroed(NumBuckets);
	Data.Init(FVector4(0.1f, 0.1f, 0.1f, 0.1f), NumBuckets);
}




/**
 * Context information passed around during VM execution.
 */
struct FVectorVMContext
{
	/** Pointer to the next element in the byte code. */
	uint8 const* RESTRICT Code;
	/** Pointer to the table of vector register arrays. */
	VectorRegister* RESTRICT * RESTRICT RegisterTable;
	/** Pointer to the constant table. */
	FVector4 const* RESTRICT ConstantTable;
	/** Pointer to the data object constant table. */
	UNiagaraDataObject * RESTRICT *DataObjConstantTable;

	/** The number of vectors to process. */
	int32 NumVectors;

	/** Initialization constructor. */
	FVectorVMContext(
		const uint8* InCode,
		VectorRegister** InRegisterTable,
		const FVector4* InConstantTable,
		UNiagaraDataObject** InDataObjTable,
		int32 InNumVectors
		)
		: Code(InCode)
		, RegisterTable(InRegisterTable)
		, ConstantTable(InConstantTable)
		, DataObjConstantTable(InDataObjTable)
		, NumVectors(InNumVectors)
	{
	}
};




/** Decode the next operation contained in the bytecode. */
static VM_FORCEINLINE VectorVM::EOp DecodeOp(FVectorVMContext& Context)
{
	return static_cast<VectorVM::EOp>(*Context.Code++);
}

/** Decode a register from the bytecode. */
static VM_FORCEINLINE VectorRegister* DecodeRegister(FVectorVMContext& Context)
{
	return Context.RegisterTable[*Context.Code++];
}

/** Decode a constant from the bytecode. */
static VM_FORCEINLINE VectorRegister DecodeConstant(FVectorVMContext& Context)
{
	const FVector4* vec = &Context.ConstantTable[*Context.Code++];
	return VectorLoad(vec);
}

/** Decode a constant from the bytecode. */
static /*VM_FORCEINLINE*/ const UNiagaraDataObject *DecodeDataObjConstant(FVectorVMContext& Context)
{
	const UNiagaraDataObject* Obj = Context.DataObjConstantTable[*Context.Code++];
	return Obj;
}

/** Decode a constant from the bytecode. */
static /*VM_FORCEINLINE*/ UNiagaraDataObject *DecodeWritableDataObjConstant(FVectorVMContext& Context)
{
	UNiagaraDataObject* Obj = Context.DataObjConstantTable[*Context.Code++];
	return Obj;
}


static VM_FORCEINLINE uint8 DecodeSrcOperandTypes(FVectorVMContext& Context)
{
	return *Context.Code++;
}

static VM_FORCEINLINE uint8 DecodeByte(FVectorVMContext& Context)
{
	return *Context.Code++;
}



/** Handles reading of a constant. */
struct FConstantHandler
{
	VectorRegister Constant;
	FConstantHandler(FVectorVMContext& Context)
		: Constant(DecodeConstant(Context))
	{}
	VM_FORCEINLINE const VectorRegister& Get(){ return Constant; }
};

/** Handles reading of a data object constant. */
struct FDataObjectConstantHandler
{
	const UNiagaraDataObject *Constant;
	FDataObjectConstantHandler(FVectorVMContext& Context)
		: Constant(DecodeDataObjConstant(Context))
	{}
	VM_FORCEINLINE const UNiagaraDataObject *Get(){ return Constant; }
};

/** Handles reading of a data object constant. */
struct FWritableDataObjectConstantHandler
{
	UNiagaraDataObject *Constant;
	FWritableDataObjectConstantHandler(FVectorVMContext& Context)
		: Constant(DecodeWritableDataObjConstant(Context))
	{}
	VM_FORCEINLINE UNiagaraDataObject *Get(){ return Constant; }
};


/** Handles reading of a register, advancing the pointer with each read. */
struct FRegisterHandler
{
	VectorRegister* Register;
	VectorRegister RegisterValue;
	FRegisterHandler(FVectorVMContext& Context)
		: Register(DecodeRegister(Context))
	{}
	VM_FORCEINLINE const VectorRegister& Get(){ RegisterValue = *Register++;  return RegisterValue; }
};

template<typename Kernel, typename Arg0Handler>
VM_FORCEINLINE void VectorUnaryLoop(FVectorVMContext& Context, VectorRegister* RESTRICT Dst, int32 NumVectors)
{
	Arg0Handler Arg0(Context);
	for (int32 i = 0; i < NumVectors; ++i)
	{
		Kernel::DoKernel(Dst++, Arg0.Get());
	}
}

template<typename Kernel, typename Arg0Handler, typename Arg1Handler>
VM_FORCEINLINE void VectorBinaryLoop(FVectorVMContext& Context, VectorRegister* RESTRICT Dst, int32 NumVectors)
{
	Arg0Handler Arg0(Context);
	Arg1Handler Arg1(Context);
	for (int32 i = 0; i < NumVectors; ++i)
	{
		Kernel::DoKernel(Dst++, Arg0.Get(), Arg1.Get());
	}
}


template<typename Kernel, typename Arg0Handler, typename Arg1Handler, typename Arg2Handler>
VM_FORCEINLINE void VectorTrinaryLoop(FVectorVMContext& Context, VectorRegister* RESTRICT Dst, int32 NumVectors)
{
	Arg0Handler Arg0(Context);
	Arg1Handler Arg1(Context);
	Arg2Handler Arg2(Context);
	for (int32 i = 0; i < NumVectors; ++i)
	{
		Kernel::DoKernel(Dst++, Arg0.Get(), Arg1.Get(), Arg2.Get());
	}
}

template<typename Kernel, typename Arg0Handler, typename Arg1Handler, typename Arg2Handler, typename Arg3Handler>
VM_FORCEINLINE void VectorQuatenaryLoop(FVectorVMContext& Context, VectorRegister* RESTRICT Dst, int32 NumVectors)
{
	Arg0Handler Arg0(Context);
	Arg1Handler Arg1(Context);
	Arg2Handler Arg2(Context);
	Arg3Handler Arg3(Context);
	for (int32 i = 0; i < NumVectors; ++i)
	{
		Kernel::DoKernel(Dst++, Arg0.Get(), Arg1.Get(), Arg2.Get(), Arg3.Get());
	}
}

/** Base class of vector kernels with a single operand. */
template <typename Kernel>
struct TUnaryVectorKernel
{
	static void Exec(FVectorVMContext& Context)
	{
		VectorRegister* RESTRICT Dst = DecodeRegister(Context);
		uint32 SrcOpTypes = DecodeSrcOperandTypes(Context);
		int32 NumVectors = Context.NumVectors;
		switch (SrcOpTypes)
		{
		case SRCOP_RRRR: 	VectorUnaryLoop<Kernel, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRRC:	VectorUnaryLoop<Kernel, FConstantHandler>(Context, Dst, NumVectors); break;
		default: check(0); break;
		};
	}
};

/** Base class of Vector kernels with 2 operands. */
template <typename Kernel>
struct TBinaryVectorKernel
{
	static void Exec(FVectorVMContext& Context)
	{
		VectorRegister* RESTRICT Dst = DecodeRegister(Context);
		uint32 SrcOpTypes = DecodeSrcOperandTypes(Context);
		int32 NumVectors = Context.NumVectors;
		switch (SrcOpTypes)
		{
		case SRCOP_RRRR: 	VectorBinaryLoop<Kernel, FRegisterHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRRC:	VectorBinaryLoop<Kernel, FConstantHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRCR: 	VectorBinaryLoop<Kernel, FRegisterHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRCC:	VectorBinaryLoop<Kernel, FConstantHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		default: check(0); break;
		};
	}
};




/** Base class of Vector kernels with 2 operands, one of which can be a data object. */
template <typename Kernel>
struct TBinaryVectorKernelData
{
	static void Exec(FVectorVMContext& Context)
	{
		VectorRegister* RESTRICT Dst = DecodeRegister(Context);
		uint32 SrcOpTypes = DecodeSrcOperandTypes(Context);
		int32 NumVectors = Context.NumVectors;
		switch (SrcOpTypes)
		{
		case SRCOP_RRRB:	VectorBinaryLoop<Kernel, FDataObjectConstantHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRCB:	VectorBinaryLoop<Kernel, FDataObjectConstantHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		default: check(0); break;
		};
	}
};



/** Base class of Vector kernels with 2 operands, one of which can be a data object. */
template <typename Kernel>
struct TTrinaryVectorKernelData
{
	static void Exec(FVectorVMContext& Context)
	{
		VectorRegister* RESTRICT Dst = DecodeRegister(Context);
		uint32 SrcOpTypes = DecodeSrcOperandTypes(Context);
		int32 NumVectors = Context.NumVectors;
		switch (SrcOpTypes)
		{
		case SRCOP_RRRB:	VectorTrinaryLoop<Kernel, FWritableDataObjectConstantHandler, FRegisterHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		default: check(0); break;
		};
	}
};




/** Base class of Vector kernels with 3 operands. */
template <typename Kernel>
struct TTrinaryVectorKernel
{
	static void Exec(FVectorVMContext& Context)
	{
		VectorRegister* RESTRICT Dst = DecodeRegister(Context);
		uint32 SrcOpTypes = DecodeSrcOperandTypes(Context);
		int32 NumVectors = Context.NumVectors;
		switch (SrcOpTypes)
		{
		case SRCOP_RRRR: 	VectorTrinaryLoop<Kernel, FRegisterHandler, FRegisterHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRRC:	VectorTrinaryLoop<Kernel, FConstantHandler, FRegisterHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRCR: 	VectorTrinaryLoop<Kernel, FRegisterHandler, FConstantHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRCC:	VectorTrinaryLoop<Kernel, FConstantHandler, FConstantHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RCRR: 	VectorTrinaryLoop<Kernel, FRegisterHandler, FRegisterHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RCRC:	VectorTrinaryLoop<Kernel, FConstantHandler, FRegisterHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RCCR: 	VectorTrinaryLoop<Kernel, FRegisterHandler, FConstantHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RCCC:	VectorTrinaryLoop<Kernel, FConstantHandler, FConstantHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		default: check(0); break;
		};
	}
};

/** Base class of Vector kernels with 4 operands. */
template <typename Kernel>
struct TQuatenaryVectorKernel
{
	static void Exec(FVectorVMContext& Context)
	{
		VectorRegister* RESTRICT Dst = DecodeRegister(Context);
		uint32 SrcOpTypes = DecodeSrcOperandTypes(Context);
		int32 NumVectors = Context.NumVectors;
		switch (SrcOpTypes)
		{
		case SRCOP_RRRR: 	VectorQuatenaryLoop<Kernel, FRegisterHandler, FRegisterHandler, FRegisterHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRRC:	VectorQuatenaryLoop<Kernel, FConstantHandler, FRegisterHandler, FRegisterHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRCR: 	VectorQuatenaryLoop<Kernel, FRegisterHandler, FConstantHandler, FRegisterHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RRCC:	VectorQuatenaryLoop<Kernel, FConstantHandler, FConstantHandler, FRegisterHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RCRR: 	VectorQuatenaryLoop<Kernel, FRegisterHandler, FRegisterHandler, FConstantHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RCRC:	VectorQuatenaryLoop<Kernel, FConstantHandler, FRegisterHandler, FConstantHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RCCR: 	VectorQuatenaryLoop<Kernel, FRegisterHandler, FConstantHandler, FConstantHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_RCCC:	VectorQuatenaryLoop<Kernel, FConstantHandler, FConstantHandler, FConstantHandler, FRegisterHandler>(Context, Dst, NumVectors); break;
		case SRCOP_CRRR: 	VectorQuatenaryLoop<Kernel, FRegisterHandler, FRegisterHandler, FRegisterHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_CRRC:	VectorQuatenaryLoop<Kernel, FConstantHandler, FRegisterHandler, FRegisterHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_CRCR: 	VectorQuatenaryLoop<Kernel, FRegisterHandler, FConstantHandler, FRegisterHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_CRCC:	VectorQuatenaryLoop<Kernel, FConstantHandler, FConstantHandler, FRegisterHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_CCRR: 	VectorQuatenaryLoop<Kernel, FRegisterHandler, FRegisterHandler, FConstantHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_CCRC:	VectorQuatenaryLoop<Kernel, FConstantHandler, FRegisterHandler, FConstantHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_CCCR: 	VectorQuatenaryLoop<Kernel, FRegisterHandler, FConstantHandler, FConstantHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		case SRCOP_CCCC:	VectorQuatenaryLoop<Kernel, FConstantHandler, FConstantHandler, FConstantHandler, FConstantHandler>(Context, Dst, NumVectors); break;
		default: check(0); break;
		};
	}
};

/*------------------------------------------------------------------------------
	Implementation of all kernel operations.
------------------------------------------------------------------------------*/

struct FVectorKernelAdd : public TBinaryVectorKernel<FVectorKernelAdd>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorAdd(Src0, Src1);
	}
};

struct FVectorKernelSub : public TBinaryVectorKernel<FVectorKernelSub>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorSubtract(Src0, Src1);
	}
};

struct FVectorKernelMul : public TBinaryVectorKernel<FVectorKernelMul>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorMultiply(Src0, Src1);
	}
};

struct FVectorKernelDiv : public TBinaryVectorKernel<FVectorKernelDiv>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0, VectorRegister Src1)
	{
		*Dst = VectorDivide(Src0, Src1);
	}
};


struct FVectorKernelMad : public TTrinaryVectorKernel<FVectorKernelMad>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1,VectorRegister Src2)
	{
		*Dst = VectorMultiplyAdd(Src0, Src1, Src2);
	}
};

struct FVectorKernelLerp : public TTrinaryVectorKernel<FVectorKernelLerp>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1,VectorRegister Src2)
	{
		const VectorRegister One = MakeVectorRegister(1.0f, 1.0f, 1.0f, 1.0f);
		const VectorRegister OneMinusAlpha = VectorSubtract(One, Src2);
		const VectorRegister Tmp = VectorMultiply(Src0, OneMinusAlpha);
		*Dst = VectorMultiplyAdd(Src1, Src2, Tmp);
	}
};

struct FVectorKernelRcp : public TUnaryVectorKernel<FVectorKernelRcp>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		*Dst = VectorReciprocal(Src0);
	}
};

struct FVectorKernelRsq : public TUnaryVectorKernel<FVectorKernelRsq>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		*Dst = VectorReciprocalSqrt(Src0);
	}
};

struct FVectorKernelSqrt : public TUnaryVectorKernel<FVectorKernelSqrt>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		// TODO: Need a SIMD sqrt!
		*Dst = VectorReciprocal(VectorReciprocalSqrt(Src0));
	}
};

struct FVectorKernelNeg : public TUnaryVectorKernel<FVectorKernelNeg>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		*Dst = VectorNegate(Src0);
	}
};

struct FVectorKernelAbs : public TUnaryVectorKernel<FVectorKernelAbs>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		*Dst = VectorAbs(Src0);
	}
};

struct FVectorKernelExp : public TUnaryVectorKernel<FVectorKernelExp>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorExp(Src0);
	}
}; 

struct FVectorKernelExp2 : public TUnaryVectorKernel<FVectorKernelExp2>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorExp2(Src0);
	}
};

struct FVectorKernelLog : public TUnaryVectorKernel<FVectorKernelLog>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorLog(Src0);
	}
};

struct FVectorKernelLog2 : public TUnaryVectorKernel<FVectorKernelLog2>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorLog2(Src0);
	}
};

struct FVectorKernelClamp : public TTrinaryVectorKernel<FVectorKernelClamp>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1,VectorRegister Src2)
	{
		const VectorRegister Tmp = VectorMax(Src0, Src1);
		*Dst = VectorMin(Tmp, Src2);
	}
};

struct FVectorKernelSin : public TUnaryVectorKernel<FVectorKernelSin>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorSin( VectorMultiply(Src0, GlobalVectorConstants::PiByTwo));
	}
};

struct FVectorKernelCos : public TUnaryVectorKernel<FVectorKernelCos>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
 	{
		*Dst = VectorCos(VectorMultiply(Src0, GlobalVectorConstants::PiByTwo));
	}
};

struct FVectorKernelTan : public TUnaryVectorKernel<FVectorKernelTan>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorTan(VectorMultiply(Src0, GlobalVectorConstants::PiByTwo));
	}
};

struct FVectorKernelASin : public TUnaryVectorKernel<FVectorKernelASin>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorASin(Src0);
	}
};

struct FVectorKernelACos : public TUnaryVectorKernel<FVectorKernelACos>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorACos(Src0);
	}
};

struct FVectorKernelATan : public TUnaryVectorKernel<FVectorKernelATan>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorATan(Src0);
	}
};

struct FVectorKernelATan2 : public TBinaryVectorKernel<FVectorKernelATan2>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0, VectorRegister Src1)
	{
		*Dst = VectorATan2(Src0, Src1);
	}
};

struct FVectorKernelCeil : public TUnaryVectorKernel<FVectorKernelCeil>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorCeil(Src0);
	}
};

struct FVectorKernelFloor : public TUnaryVectorKernel<FVectorKernelFloor>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorFloor(Src0);
	}
};

struct FVectorKernelMod : public TBinaryVectorKernel<FVectorKernelMod>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0, VectorRegister Src1)
	{
		*Dst = VectorMod(Src0, Src1);
	}
};

struct FVectorKernelFrac : public TUnaryVectorKernel<FVectorKernelFrac>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorFractional(Src0);
	}
};

struct FVectorKernelTrunc : public TUnaryVectorKernel<FVectorKernelTrunc>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorTruncate(Src0);
	}
};

struct FVectorKernelLessThan : public TBinaryVectorKernel<FVectorKernelLessThan>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0, VectorRegister Src1)
	{
		const VectorRegister Large = MakeVectorRegister(BIG_NUMBER, BIG_NUMBER, BIG_NUMBER, BIG_NUMBER);
		const VectorRegister One = MakeVectorRegister(1.0f, 1.0f, 1.0f, 1.0f);
		const VectorRegister Zero = MakeVectorRegister(0.0f, 0.0f, 0.0f, 0.0f);
		VectorRegister Tmp = VectorSubtract(Src1, Src0);
		Tmp = VectorMultiply(Tmp, Large);
		Tmp = VectorMin(Tmp, One);
		*Dst = VectorMax(Tmp, Zero);
	}
};



struct FVectorKernelSample : public TBinaryVectorKernelData<FVectorKernelSample>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, const UNiagaraDataObject *Src0, VectorRegister Src1)
	{
		if (Src0)
		{
			float const* FloatSrc1 = reinterpret_cast<float const*>(&Src1);
			FVector4 Tmp = Src0->Sample(FVector4(FloatSrc1[0], FloatSrc1[1], FloatSrc1[2], FloatSrc1[3]));
			*Dst = VectorLoad(&Tmp);
		}
	}
};

struct FVectorKernelBufferWrite : public TTrinaryVectorKernelData<FVectorKernelBufferWrite>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, UNiagaraDataObject *Src0, VectorRegister Src1, VectorRegister Src2)
	{
		if (Src0)
		{
			float const* FloatSrc1 = reinterpret_cast<float const*>(&Src1);	// Coords
			float const* FloatSrc2 = reinterpret_cast<float const*>(&Src2);	// Value
			FVector4 Tmp = Src0->Write(FVector4(FloatSrc1[0], FloatSrc1[1], FloatSrc1[2], FloatSrc1[3]), FVector4(FloatSrc2[0], FloatSrc2[1], FloatSrc2[2], FloatSrc2[3]));
			*Dst = VectorLoad(&Tmp);
		}
	}
};


struct FVectorKernelDot : public TBinaryVectorKernel<FVectorKernelDot>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0, VectorRegister Src1)
	{
		*Dst = VectorDot4(Src0, Src1);
	}
};

struct FVectorKernelLength : public TUnaryVectorKernel<FVectorKernelLength>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		VectorRegister Temp = VectorReciprocalLen(Src0);
		*Dst = VectorReciprocal(Temp);
	}
};

struct FVectorKernelCross : public TBinaryVectorKernel<FVectorKernelCross>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0, VectorRegister Src1)
	{
		*Dst = VectorCross(Src0, Src1);
	}
};

struct FVectorKernelNormalize : public TUnaryVectorKernel<FVectorKernelNormalize>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorNormalize(Src0);
	}
};

struct FVectorKernelRandom : public TUnaryVectorKernel<FVectorKernelRandom>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		const float rm = RAND_MAX;
		VectorRegister Result = MakeVectorRegister(static_cast<float>(FMath::Rand()) / rm,
			static_cast<float>(FMath::Rand()) / rm,
			static_cast<float>(FMath::Rand()) / rm,
			static_cast<float>(FMath::Rand()) / rm);
		*Dst = VectorMultiply(Result, Src0);
	}
};


/* gaussian distribution random number (not working yet) */
struct FVectorKernelRandomGauss : public TBinaryVectorKernel<FVectorKernelRandomGauss>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0, VectorRegister Src1)
	{
		const float rm = RAND_MAX;
		VectorRegister Result = MakeVectorRegister(static_cast<float>(FMath::Rand()) / rm,
			static_cast<float>(FMath::Rand()) / rm,
			static_cast<float>(FMath::Rand()) / rm,
			static_cast<float>(FMath::Rand()) / rm);

		Result = VectorSubtract(Result, MakeVectorRegister(0.5f, 0.5f, 0.5f, 0.5f));
		Result = VectorMultiply(MakeVectorRegister(3.0f, 3.0f, 3.0f, 3.0f), Result);

		// taylor series gaussian approximation
		const VectorRegister SPi2 = VectorReciprocal(VectorReciprocalSqrt(MakeVectorRegister(2 * PI, 2 * PI, 2 * PI, 2 * PI)));
		VectorRegister Gauss = VectorReciprocal(SPi2);
		VectorRegister Div = VectorMultiply(MakeVectorRegister(2.0f, 2.0f, 2.0f, 2.0f), SPi2);
		Gauss = VectorSubtract(Gauss, VectorDivide(VectorMultiply(Result, Result), Div));
		Div = VectorMultiply(MakeVectorRegister(8.0f, 8.0f, 8.0f, 8.0f), SPi2);
		Gauss = VectorAdd(Gauss, VectorDivide(VectorPow(MakeVectorRegister(4.0f, 4.0f, 4.0f, 4.0f), Result), Div));
		Div = VectorMultiply(MakeVectorRegister(48.0f, 48.0f, 48.0f, 48.0f), SPi2);
		Gauss = VectorSubtract(Gauss, VectorDivide(VectorPow(MakeVectorRegister(6.0f, 6.0f, 6.0f, 6.0f), Result), Div));

		Gauss = VectorDivide(Gauss, MakeVectorRegister(0.4f, 0.4f, 0.4f, 0.4f));
		Gauss = VectorMultiply(Gauss, Src0);
		*Dst = Gauss;
	}
};




struct FVectorKernelMin : public TBinaryVectorKernel<FVectorKernelMin>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorMin(Src0, Src1);
	}
};

struct FVectorKernelMax : public TBinaryVectorKernel<FVectorKernelMax>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorMax(Src0, Src1);
	}
};

struct FVectorKernelPow : public TBinaryVectorKernel<FVectorKernelPow>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorPow(Src0, Src1);
	}
};

struct FVectorKernelSign : public TUnaryVectorKernel<FVectorKernelSign>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorSign(Src0);
	}
};

struct FVectorKernelStep : public TUnaryVectorKernel<FVectorKernelStep>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorStep(Src0);
	}
};

struct FVectorKernelNoise : public TUnaryVectorKernel<FVectorKernelNoise>
{
	static VectorRegister RandomTable[17][17][17];

	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		const VectorRegister One = MakeVectorRegister(1.0f, 1.0f, 1.0f, 1.0f);
		const VectorRegister VecSize = MakeVectorRegister(16.0f, 16.0f, 16.0f, 16.0f);

		*Dst = MakeVectorRegister(0.0f, 0.0f, 0.0f, 0.0f);
		
		for (uint32 i = 1; i < 2; i++)
		{
			float Di = 0.2f * (1.0f/(1<<i));
			VectorRegister Div = MakeVectorRegister(Di, Di, Di, Di);
			VectorRegister Coords = VectorMod( VectorAbs( VectorMultiply(Src0, Div) ), VecSize );
			const float *CoordPtr = reinterpret_cast<float const*>(&Coords);
			const int32 Cx = CoordPtr[0];
			const int32 Cy = CoordPtr[1];
			const int32 Cz = CoordPtr[2];

			VectorRegister Frac = VectorFractional(Coords);
			VectorRegister Alpha = VectorReplicate(Frac, 0);
			VectorRegister OneMinusAlpha = VectorSubtract(One, Alpha);
			
			VectorRegister XV1 = VectorMultiplyAdd(RandomTable[Cx][Cy][Cz], Alpha, VectorMultiply(RandomTable[Cx+1][Cy][Cz], OneMinusAlpha));
			VectorRegister XV2 = VectorMultiplyAdd(RandomTable[Cx][Cy+1][Cz], Alpha, VectorMultiply(RandomTable[Cx+1][Cy+1][Cz], OneMinusAlpha));
			VectorRegister XV3 = VectorMultiplyAdd(RandomTable[Cx][Cy][Cz+1], Alpha, VectorMultiply(RandomTable[Cx+1][Cy][Cz+1], OneMinusAlpha));
			VectorRegister XV4 = VectorMultiplyAdd(RandomTable[Cx][Cy+1][Cz+1], Alpha, VectorMultiply(RandomTable[Cx+1][Cy+1][Cz+1], OneMinusAlpha));

			Alpha = VectorReplicate(Frac, 1);
			OneMinusAlpha = VectorSubtract(One, Alpha);
			VectorRegister YV1 = VectorMultiplyAdd(XV1, Alpha, VectorMultiply(XV2, OneMinusAlpha));
			VectorRegister YV2 = VectorMultiplyAdd(XV3, Alpha, VectorMultiply(XV4, OneMinusAlpha));

			Alpha = VectorReplicate(Frac, 2);
			OneMinusAlpha = VectorSubtract(One, Alpha);
			VectorRegister ZV = VectorMultiplyAdd(YV1, Alpha, VectorMultiply(YV2, OneMinusAlpha));

			*Dst = VectorAdd(*Dst, ZV);
		}
	}
};

VectorRegister FVectorKernelNoise::RandomTable[17][17][17];

template<int32 Component>
struct FVectorKernelSplat : public TUnaryVectorKernel<FVectorKernelSplat<Component>>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorReplicate(Src0, Component);
	}
};

template<int32 Cmp0, int32 Cmp1, int32 Cmp2, int32 Cmp3>
struct FVectorKernelCompose : public TQuatenaryVectorKernel<FVectorKernelCompose<Cmp0, Cmp1, Cmp2, Cmp3>>
{
	//Passing as const refs as some compilers cant handle > 3 aligned vectorregister params.
	//inlined so shouldn't impact perf
	//Todo: ^^^^ test this
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, const VectorRegister& Src0, const VectorRegister& Src1, const VectorRegister& Src2, const VectorRegister& Src3)
	{
		//TODO - There's probably a faster way to do this.
		VectorRegister Tmp0 = VectorShuffle(Src0, Src1, Cmp0, Cmp0, Cmp1, Cmp1);
		VectorRegister Tmp1 = VectorShuffle(Src2, Src3, Cmp2, Cmp2, Cmp3, Cmp3);		
		*Dst = VectorShuffle(Tmp0, Tmp1, 0, 2, 0, 2);
	}
};

// Ken Perlin's smootherstep function (zero first and second order derivatives at 0 and 1)
// calculated separately for each channel of Src2
struct FVectorKernelEaseIn : public TTrinaryVectorKernel<FVectorKernelEaseIn>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, const VectorRegister& Src0, const VectorRegister& Src1, const VectorRegister& Src2)
	{
		VectorRegister X = VectorMin( VectorDivide(VectorSubtract(Src2, Src0), VectorSubtract(Src1, Src0)), MakeVectorRegister(1.0f, 1.0f, 1.0f, 1.0f) );
		X = VectorMax(X, MakeVectorRegister(0.0f, 0.0f, 0.0f, 0.0f));

		VectorRegister X3 = VectorMultiply( VectorMultiply(X, X), X);
		VectorRegister N6 = MakeVectorRegister(6.0f, 6.0f, 6.0f, 6.0f);
		VectorRegister N15 = MakeVectorRegister(15.0f, 15.0f, 15.0f, 15.0f);
		VectorRegister T = VectorSubtract( VectorMultiply(X, N6), N15 );
		T = VectorAdd(VectorMultiply(X, T), MakeVectorRegister(10.0f, 10.0f, 10.0f, 10.0f));

		*Dst = VectorMultiply(X3, T);
	}
};

// smoothly runs 0->1->0
struct FVectorKernelEaseInOut : public TUnaryVectorKernel<FVectorKernelEaseInOut>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, const VectorRegister& Src0)
	{
		VectorRegister T = VectorMultiply(Src0, MakeVectorRegister(2.0f, 2.0f, 2.0f, 2.0f));
		T = VectorSubtract(T, MakeVectorRegister(1.0f, 1.0f, 1.0f, 1.0f));
		VectorRegister X2 = VectorMultiply(T, T);
		
		VectorRegister R = VectorMultiply(X2, MakeVectorRegister(0.9604f, 0.9604f, 0.9604f, 0.9604f));
		R = VectorSubtract(R, MakeVectorRegister(1.96f, 1.96f, 1.96f, 1.96f));
		R = VectorMultiply(R, X2);
		*Dst = VectorAdd(R, MakeVectorRegister(1.0f, 1.0f, 1.0f, 1.0f));
	}
};


struct FVectorKernelOutput : public TUnaryVectorKernel<FVectorKernelOutput>
{
	static void VM_FORCEINLINE DoKernel(VectorRegister* Dst, VectorRegister Src0)
	{
		VectorStoreAlignedStreamed(Src0, Dst);
	}
};

void VectorVM::Init()
{
	static bool Inited = false;
	if (Inited == false)
	{
		// random noise
		float TempTable[17][17][17];
		for (int z = 0; z < 17; z++)
		{
			for (int y = 0; y < 17; y++)
			{
				for (int x = 0; x < 17; x++)
				{
					float f1 = (float)FMath::FRandRange(-1.0f, 1.0f);
					TempTable[x][y][z] = f1;
				}
			}
		}

		// pad
		for (int i = 0; i < 17; i++)
		{
			for (int j = 0; j < 17; j++)
			{
				TempTable[i][j][16] = TempTable[i][j][0];
				TempTable[i][16][j] = TempTable[i][0][j];
				TempTable[16][j][i] = TempTable[0][j][i];
			}
		}

		// compute gradients
		FVector TempTable2[17][17][17];
		for (int z = 0; z < 16; z++)
		{
			for (int y = 0; y < 16; y++)
			{
				for (int x = 0; x < 16; x++)
				{
					FVector XGrad = FVector(1.0f, 0.0f, TempTable[x][y][z] - TempTable[x+1][y][z]);
					FVector YGrad = FVector(0.0f, 1.0f, TempTable[x][y][z] - TempTable[x][y + 1][z]);
					FVector ZGrad = FVector(0.0f, 1.0f, TempTable[x][y][z] - TempTable[x][y][z+1]);

					FVector Grad = FVector(XGrad.Z, YGrad.Z, ZGrad.Z);
					TempTable2[x][y][z] = Grad;
				}
			}
		}

		// pad
		for (int i = 0; i < 17; i++)
		{
			for (int j = 0; j < 17; j++)
			{
				TempTable2[i][j][16] = TempTable2[i][j][0];
				TempTable2[i][16][j] = TempTable2[i][0][j];
				TempTable2[16][j][i] = TempTable2[0][j][i];
			}
		}


		// compute curl of gradient field
		for (int z = 0; z < 16; z++)
		{
			for (int y = 0; y < 16; y++)
			{
				for (int x = 0; x < 16; x++)
				{
					FVector Dy = TempTable2[x][y][z] - TempTable2[x][y + 1][z];
					FVector Sy = TempTable2[x][y][z] + TempTable2[x][y + 1][z];
					FVector Dx = TempTable2[x][y][z] - TempTable2[x + 1][y][z];
					FVector Sx = TempTable2[x][y][z] + TempTable2[x + 1][y][z];
					FVector Dz = TempTable2[x][y][z] - TempTable2[x][y][z + 1];
					FVector Sz = TempTable2[x][y][z] + TempTable2[x][y][z + 1];
					FVector Dir = FVector(Dy.Z - Sz.Y, Dz.X - Sx.Z, Dx.Y - Sy.X);

					FVectorKernelNoise::RandomTable[x][y][z] = MakeVectorRegister(Dir.X, Dir.Y, Dir.Z, 0.0f);
				}
			}
		}


		Inited = true;
	}
}

void VectorVM::Exec(
	uint8 const* Code,
	VectorRegister** InputRegisters,
	int32 NumInputRegisters,
	VectorRegister** OutputRegisters,
	int32 NumOutputRegisters,
	FVector4 const* ConstantTable,
	UNiagaraDataObject* *DataObjConstTable,
	int32 NumVectors
	)
{
	VectorRegister TempRegisters[NumTempRegisters][VectorsPerChunk];
	VectorRegister* RegisterTable[MaxRegisters] = {0};

	// Map temporary registers.
	for (int32 i = 0; i < NumTempRegisters; ++i)
	{
		RegisterTable[i] = TempRegisters[i];
	}

	// Process one chunk at a time.
	int32 NumChunks = (NumVectors + VectorsPerChunk - 1) / VectorsPerChunk;
	for (int32 ChunkIndex = 0; ChunkIndex < NumChunks; ++ChunkIndex)
	{
		// Map input and output registers.
		for (int32 i = 0; i < NumInputRegisters; ++i)
		{
			RegisterTable[NumTempRegisters + i] = InputRegisters[i] + ChunkIndex * VectorsPerChunk;
		}
		for (int32 i = 0; i < NumOutputRegisters; ++i)
		{
			RegisterTable[NumTempRegisters + MaxInputRegisters + i] = OutputRegisters[i] + ChunkIndex * VectorsPerChunk;
		}

		// Setup execution context.
		int32 VectorsThisChunk = FMath::Min<int32>(NumVectors, VectorsPerChunk);
		FVectorVMContext Context(Code, RegisterTable, ConstantTable, DataObjConstTable, VectorsThisChunk);
		EOp Op = EOp::done;

		// Execute VM on all vectors in this chunk.
		do 
		{
			Op = DecodeOp(Context);
			switch (Op)
			{
			// Dispatch kernel ops.
			case EOp::add: FVectorKernelAdd::Exec(Context); break;
			case EOp::sub: FVectorKernelSub::Exec(Context); break;
			case EOp::mul: FVectorKernelMul::Exec(Context); break;
			case EOp::div: FVectorKernelDiv::Exec(Context); break;
			case EOp::mad: FVectorKernelMad::Exec(Context); break;
			case EOp::lerp: FVectorKernelLerp::Exec(Context); break;
			case EOp::rcp: FVectorKernelRcp::Exec(Context); break;
			case EOp::rsq: FVectorKernelRsq::Exec(Context); break;
			case EOp::sqrt: FVectorKernelSqrt::Exec(Context); break;
			case EOp::neg: FVectorKernelNeg::Exec(Context); break;
			case EOp::abs: FVectorKernelAbs::Exec(Context); break;
			case EOp::exp: FVectorKernelExp::Exec(Context); break;
			case EOp::exp2: FVectorKernelExp2::Exec(Context); break;
			case EOp::log: FVectorKernelLog::Exec(Context); break;
			case EOp::log2: FVectorKernelLog2::Exec(Context); break;
			case EOp::sin: FVectorKernelSin::Exec(Context); break;
			case EOp::cos: FVectorKernelCos::Exec(Context); break;
			case EOp::tan: FVectorKernelTan::Exec(Context); break;
			case EOp::asin: FVectorKernelASin::Exec(Context); break;
			case EOp::acos: FVectorKernelACos::Exec(Context); break;
			case EOp::atan: FVectorKernelATan::Exec(Context); break;
			case EOp::atan2: FVectorKernelATan2::Exec(Context); break;
			case EOp::ceil: FVectorKernelCeil::Exec(Context); break;
			case EOp::floor: FVectorKernelFloor::Exec(Context); break;
			case EOp::fmod: FVectorKernelMod::Exec(Context); break;
			case EOp::frac: FVectorKernelFrac::Exec(Context); break;
			case EOp::trunc: FVectorKernelTrunc::Exec(Context); break;
			case EOp::clamp: FVectorKernelClamp::Exec(Context); break;
			case EOp::min: FVectorKernelMin::Exec(Context); break;
			case EOp::max: FVectorKernelMax::Exec(Context); break;
			case EOp::pow: FVectorKernelPow::Exec(Context); break;
			case EOp::sign: FVectorKernelSign::Exec(Context); break;
			case EOp::step: FVectorKernelStep::Exec(Context); break;
			case EOp::dot: FVectorKernelDot::Exec(Context); break;
			case EOp::cross: FVectorKernelCross::Exec(Context); break;
			case EOp::normalize: FVectorKernelNormalize::Exec(Context); break;
			case EOp::random: FVectorKernelRandom::Exec(Context); break;
			case EOp::length: FVectorKernelLength::Exec(Context); break;
			case EOp::noise: FVectorKernelNoise::Exec(Context); break;
			case EOp::splatx: FVectorKernelSplat<0>::Exec(Context); break;
			case EOp::splaty: FVectorKernelSplat<1>::Exec(Context); break;
			case EOp::splatz: FVectorKernelSplat<2>::Exec(Context); break;
			case EOp::splatw: FVectorKernelSplat<3>::Exec(Context); break;
			case EOp::compose: FVectorKernelCompose<0,1,2,3>::Exec(Context); break;
			case EOp::composex: FVectorKernelCompose<0, 0, 0, 0>::Exec(Context); break;
			case EOp::composey: FVectorKernelCompose<1, 1, 1, 1>::Exec(Context); break;
			case EOp::composez: FVectorKernelCompose<2, 2, 2, 2>::Exec(Context); break;
			case EOp::composew: FVectorKernelCompose<3, 3, 3, 3>::Exec(Context); break;
			case EOp::lessthan: FVectorKernelLessThan::Exec(Context); break;
			case EOp::sample: FVectorKernelSample::Exec(Context); break;
			case EOp::bufferwrite: FVectorKernelBufferWrite::Exec(Context); break;
			case EOp::eventbroadcast: break;
			case EOp::easein: FVectorKernelEaseIn::Exec(Context); break;
			case EOp::easeinout: FVectorKernelEaseInOut::Exec(Context); break;
			case EOp::output: FVectorKernelOutput::Exec(Context); break;
				
			// Execution always terminates with a "done" opcode.
			case EOp::done:
				break;

			// Opcode not recognized / implemented.
			default:
				UE_LOG(LogVectorVM, Error, TEXT("Unknown op code 0x%02x"), (uint32)Op);
				return;//BAIL
			}
		} while (Op != EOp::done);

		NumVectors -= VectorsPerChunk;
	}
}

uint8 VectorVM::GetNumOpCodes()
{
	return (uint8)EOp::NumOpcodes;
}

/*------------------------------------------------------------------------------
	Automation test for the VM.
------------------------------------------------------------------------------*/

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVectorVMTest, "System.Core.Math.Vector VM", EAutomationTestFlags::ATF_SmokeTest)

bool FVectorVMTest::RunTest(const FString& Parameters)
{
	uint8 TestCode[] =
	{
		(uint8)VectorVM::EOp::mul, 0x00, SRCOP_RRRR, 0x0 + VectorVM::NumTempRegisters, 0x0 + VectorVM::NumTempRegisters,       // mul r0, r8, r8
		(uint8)VectorVM::EOp::mad, 0x01, SRCOP_RRRR, 0x01 + VectorVM::NumTempRegisters, 0x01 + VectorVM::NumTempRegisters, 0x00, // mad r1, r9, r9, r0
		(uint8)VectorVM::EOp::mad, 0x00, SRCOP_RRRR, 0x02 + VectorVM::NumTempRegisters, 0x02 + VectorVM::NumTempRegisters, 0x01, // mad r0, r10, r10, r1
		(uint8)VectorVM::EOp::add, 0x01, SRCOP_RRCR, 0x00, 0x01,       // addi r1, r0, c1
		(uint8)VectorVM::EOp::neg, 0x00, SRCOP_RRRR, 0x01,             // neg r0, r1
		(uint8)VectorVM::EOp::clamp, VectorVM::FirstOutputRegister, SRCOP_RCCR, 0x00, 0x02, 0x03, // clampii r40, r0, c2, c3
		0x00 // terminator
	};

	VectorRegister TestRegisters[4][VectorVM::VectorsPerChunk];
	VectorRegister* InputRegisters[3] = { TestRegisters[0], TestRegisters[1], TestRegisters[2] };
	VectorRegister* OutputRegisters[1] = { TestRegisters[3] };

	VectorRegister Inputs[3][VectorVM::VectorsPerChunk];
	for (int32 i = 0; i < VectorVM::ChunkSize; i++)
	{
		reinterpret_cast<float*>(&Inputs[0])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(&Inputs[1])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(&Inputs[2])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(InputRegisters[0])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(InputRegisters[1])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(InputRegisters[2])[i] = static_cast<float>(i);
	}

	FVector4 ConstantTable[VectorVM::MaxConstants];
	ConstantTable[0] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	ConstantTable[1] = FVector4(5.0f, 5.0f, 5.0f, 5.0f);
	ConstantTable[2] = FVector4(-20.0f, -20.0f, -20.0f, -20.0f);
	ConstantTable[3] = FVector4(20.0f, 20.0f, 20.0f, 20.0f);

	VectorVM::Exec(
		TestCode,
		InputRegisters, 3,
		OutputRegisters, 1,
		ConstantTable,
		nullptr,
		VectorVM::VectorsPerChunk
		);

	for (int32 i = 0; i < VectorVM::ChunkSize; i++)
	{
		float Ins[3];

		// Verify that the input registers were not overwritten.
		for (int32 InputIndex = 0; InputIndex < 3; ++InputIndex)
		{
			float In = Ins[InputIndex] = reinterpret_cast<float*>(&Inputs[InputIndex])[i];
			float R = reinterpret_cast<float*>(InputRegisters[InputIndex])[i];
			if (In != R)
			{
				UE_LOG(LogVectorVM,Error,TEXT("Input register %d vector %d element %d overwritten. Has %f expected %f"),
					InputIndex,
					i / VectorVM::ElementsPerVector,
					i % VectorVM::ElementsPerVector,
					R,
					In
					);
				return false;
			}
		}

		// Verify that outputs match what we expect.
		float Out = reinterpret_cast<float*>(OutputRegisters[0])[i];
		float Expected = FMath::Clamp<float>(-(Ins[0] * Ins[0] + Ins[1] * Ins[1] + Ins[2] * Ins[2] + 5.0f), -20.0f, 20.0f);
		if (Out != Expected)
		{
			UE_LOG(LogVectorVM,Error,TEXT("Output register %d vector %d element %d is wrong. Has %f expected %f"),
				0,
				i / VectorVM::ElementsPerVector,
				i % VectorVM::ElementsPerVector,
				Out,
				Expected
				);
			return false;
		}
	}

	return true;
}

#undef VM_FORCEINLINE