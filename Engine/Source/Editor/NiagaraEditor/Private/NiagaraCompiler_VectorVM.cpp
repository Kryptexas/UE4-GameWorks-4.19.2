// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorPrivatePCH.h"
#include "NiagaraScript.h"
#include "NiagaraComponent.h"
#include "CompilerResultsLog.h"
#include "EdGraphUtilities.h"
#include "VectorVM.h"
#include "ComponentReregisterContext.h"
#include "NiagaraCompiler_VectorVM.h"

#define LOCTEXT_NAMESPACE "NiagaraCompiler_VectorVM"

DEFINE_LOG_CATEGORY_STATIC(LogNiagaraCompiler_VectorVM, All, All);

/** Expression handling native operations of the VectorVM. */
class FNiagaraExpression_VMOperation : public FNiagaraExpression
{
public:
	VectorVM::EOp OpCode;

	FNiagaraExpression_VMOperation(FNiagaraCompiler* Compiler, const FNiagaraVariableInfo& InResult, TArray<TNiagaraExprPtr>& InputExpressions, VectorVM::EOp Op)
		: FNiagaraExpression(Compiler, InResult)
		, OpCode(Op)
	{
		SourceExpressions = InputExpressions;
	}

	virtual ~FNiagaraExpression_VMOperation()
	{
		if (ResultLocation == ENiagaraExpressionResultLocation::Temporaries && ResultIndex != INDEX_NONE)
		{
			Compiler->FreeTemporary(ResultIndex);
		}
	}

	virtual void Process()override
	{
		FNiagaraCompiler_VectorVM* VMCompiler = (FNiagaraCompiler_VectorVM*)Compiler;

		//Should be invalid at this point.
		check(ResultIndex == INDEX_NONE);
		check(ComponentIndex == INDEX_NONE);

		//Special case where we're splatting but we don't know the component until now. E.g. Getting constants.
		//Choose the component now based on the source expressions component index
		if (OpCode == VectorVM::EOp::splatx)
		{
			check(SourceExpressions.Num() == 1);
			TNiagaraExprPtr Src0 = GetSourceExpression(0);
			if (Src0->ComponentIndex != INDEX_NONE)
			{
				switch (Src0->ComponentIndex)
				{
				case 0: OpCode = VectorVM::EOp::splatx; break;
				case 1: OpCode = VectorVM::EOp::splaty; break;
				case 2: OpCode = VectorVM::EOp::splatz; break;
				case 3: OpCode = VectorVM::EOp::splatw; break;
				}
			}
		}

		ResultIndex = VMCompiler->AquireTemporary();
		ResultLocation = ENiagaraExpressionResultLocation::Temporaries;
		check(ResultIndex < VectorVM::NumTempRegisters);

		VMCompiler->WriteCode((uint8)OpCode);
		VMCompiler->WriteCode(VMCompiler->GetResultVMIndex( this ));

		//Add the bitfield defining whether each source operand comes from a constant or a register.
		int32 NumOperands = SourceExpressions.Num();
		check(NumOperands <= 4);


		uint8 OpTypeMask = VectorVM::CreateSrcOperandMask(NumOperands > 0 ? VMCompiler->GetVMOperandType(GetSourceExpression(0).Get()) : VectorVM::RegisterOperandType,
			NumOperands > 1 ? VMCompiler->GetVMOperandType(GetSourceExpression(1).Get()) : VectorVM::RegisterOperandType,
			NumOperands > 2 ? VMCompiler->GetVMOperandType(GetSourceExpression(2).Get()) : VectorVM::RegisterOperandType,
			NumOperands > 3 ? VMCompiler->GetVMOperandType(GetSourceExpression(3).Get()) : VectorVM::RegisterOperandType);
		VMCompiler->WriteCode(OpTypeMask);


		//Add the locations for each of the source operands.
		for (int32 SrcIdx = 0; SrcIdx < SourceExpressions.Num(); ++SrcIdx)
		{
			VMCompiler->WriteCode(VMCompiler->GetResultVMIndex(GetSourceExpression(SrcIdx).Get()));
		}
	}
};

/** Expression to write attribute data output in the VM. */
class FNiagaraExpression_VMOutput : public FNiagaraExpression
{
public:

	FNiagaraExpression_VMOutput(class FNiagaraCompiler* InCompiler, const FNiagaraVariableInfo& InAttribute, TNiagaraExprPtr& InSourceExpression)
		: FNiagaraExpression(InCompiler, InAttribute)
	{
		ResultLocation = ENiagaraExpressionResultLocation::OutputData;
		SourceExpressions.Add(InSourceExpression);
		check(SourceExpressions.Num() == 1);
	}

	virtual void Process()override
	{
		FNiagaraCompiler_VectorVM* VMCompiler = (FNiagaraCompiler_VectorVM*)Compiler;
		check(ResultLocation == ENiagaraExpressionResultLocation::OutputData);
		ResultIndex = VMCompiler->GetAttributeIndex(Result);
		
		FNiagaraExpression* SrcExpr = GetSourceExpression(0).Get();
		check(SourceExpressions.Num() == 1);

		uint8 DestIndex = VMCompiler->GetResultVMIndex(this);
		uint8 SrcIndex = VMCompiler->GetResultVMIndex(SrcExpr);

		VMCompiler->WriteCode((uint8)VectorVM::EOp::output);
		VMCompiler->WriteCode(DestIndex);
		VMCompiler->WriteCode(VectorVM::CreateSrcOperandMask( VMCompiler->GetVMOperandType(SrcExpr) ) );
		VMCompiler->WriteCode(SrcIndex);
	}
};

bool FNiagaraCompiler_VectorVM::ExpressionIsConstant(FNiagaraExpression*  Expression)
{
	return Expression->ResultLocation == ENiagaraExpressionResultLocation::Constants;
}

bool FNiagaraCompiler_VectorVM::ExpressionIsBufferConstant(FNiagaraExpression*  Expression)
{
	return Expression->ResultLocation == ENiagaraExpressionResultLocation::BufferConstants;
}

VectorVM::EOperandType FNiagaraCompiler_VectorVM::GetVMOperandType(FNiagaraExpression *Expression)
{
	return Expression->ResultLocation == ENiagaraExpressionResultLocation::Constants ? VectorVM::ConstantOperandType :
		Expression->ResultLocation == ENiagaraExpressionResultLocation::BufferConstants ? VectorVM::DataObjConstantOperandType :
		VectorVM::RegisterOperandType;
}

uint8 FNiagaraCompiler_VectorVM::GetResultVMIndex(FNiagaraExpression* Expression)
{
	if (Expression->ResultLocation == ENiagaraExpressionResultLocation::InputData)
	{
		return VectorVM::FirstInputRegister + Expression->ResultIndex;
	}
	else if (Expression->ResultLocation == ENiagaraExpressionResultLocation::OutputData)
	{
		return VectorVM::FirstOutputRegister + Expression->ResultIndex;
	}
	else
	{
		if (Expression->ResultLocation == ENiagaraExpressionResultLocation::BufferConstants)
		{
			return Expression->ResultIndex; //TempRegister or Constant.
		}

		return Expression->ResultIndex;//TempRegister or Constant.
	}
}

ENiagaraDataType FNiagaraCompiler_VectorVM::GetConstantResultIndex(const FNiagaraVariableInfo& Constant, bool bInternal, int32& OutResultIndex, int32& OutComponentIndex)
{
	ENiagaraDataType Type;
	ConstantData.GetTableIndex(Constant, bInternal, OutResultIndex, OutComponentIndex, Type);
	return Type;
}

int32 FNiagaraCompiler_VectorVM::AquireTemporary()
{
	int32 Free = TempRegisters.Find(false);
	TempRegisters[Free] = true;
	return Free;
}

void FNiagaraCompiler_VectorVM::FreeTemporary(int32 TempIndex)
{
	check(TempIndex < TempRegisters.Num() && TempIndex >= 0);
	TempRegisters[TempIndex] = false;
}

void FNiagaraCompiler_VectorVM::WriteCode(uint8 InCode)
{
	check(Script);
	Script->ByteCode.Add(InCode);
}

TNiagaraExprPtr FNiagaraCompiler_VectorVM::Output(const FNiagaraVariableInfo& Attr, TNiagaraExprPtr& SourceExpression)
{
	int32 Index = Expressions.Add(MakeShareable(new FNiagaraExpression_VMOutput(this, Attr, SourceExpression)));
	return Expressions[Index];
}

bool FNiagaraCompiler_VectorVM::CompileScript(UNiagaraScript* InScript)
{
	//TODO - Most of this function can be refactored up into FNiagaraCompiler once we know how the compute script and VM script will coexist in the UNiagaraScript.

	check(InScript);

	Script = InScript;
	Source = CastChecked<UNiagaraScriptSource>(InScript->Source);

	//Clear previous graph errors.
	if (InScript->ByteCode.Num() == 0)
	{
		bool bChanged = false;
		for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
		{
			if (Node->bHasCompilerMessage)
			{
				Node->ErrorMsg.Empty();
				Node->ErrorType = EMessageSeverity::Info;
				Node->bHasCompilerMessage = false;
				Node->Modify(true);
				bChanged = true;
			}
		}
		if (bChanged)
		{
			Source->NodeGraph->NotifyGraphChanged();
		}
	}
	
	//Should we roll our own message/error log and put it in a window somewhere?
	MessageLog.SetSourceName(InScript->GetPathName());

	// Clone the source graph so we can modify it as needed; merging in the child graphs
	SourceGraph = CastChecked<UNiagaraGraph>(FEdGraphUtilities::CloneGraph(Source->NodeGraph, NULL, &MessageLog));
	FEdGraphUtilities::MergeChildrenGraphsIn(SourceGraph, SourceGraph, /*bRequireSchemaMatch=*/ true);

	if (MergeInFunctionNodes())
	{
		TempRegisters.AddZeroed(VectorVM::NumTempRegisters);

		// Find the output node.
		UNiagaraNodeOutput* OutputNode = SourceGraph->FindOutputNode();

		TArray<FNiagaraNodeResult> OutputExpressions;
		OutputNode->Compile(this, OutputExpressions);

		//Todo - track usage of temp registers so we can allocate and free and reuse from a smaller set.
		int32 NextTempRegister = 0;
		Script->ByteCode.Empty();

		//Now we have a set of expressions from which we can generate the bytecode.
		for (int32 i = 0; i < Expressions.Num(); ++i)
		{
			TNiagaraExprPtr Expr = Expressions[i];
			if (Expr.IsValid())
			{
				Expr->Process();
				Expr->PostProcess();
			}
			else
			{
				check(0);//Replace with graceful bailout?
			}

			Expressions[i].Reset();
		}

		PinToExpression.Empty();//TODO - Replace this with expression caching at a lower level. Will be more useful and nicer.

		if (MessageLog.NumErrors == 0)
		{
			// Terminate with the 'done' opcode.
			Script->ByteCode.Add((int8)VectorVM::EOp::done);
			Script->ConstantData = ConstantData;
			Script->Attributes = OutputNode->Outputs;
			return true;
		}
	}

	//Compilation has failed.
	InScript->Attributes.Empty();
	InScript->ByteCode.Empty();
	InScript->ConstantData.Empty();
	//Tell the graph to show the errors.
	Source->NodeGraph->NotifyGraphChanged();
		
	return false;
}

TNiagaraExprPtr FNiagaraCompiler_VectorVM::Expression_VMNative(VectorVM::EOp Op, TArray<TNiagaraExprPtr>& InputExpressions)
{
	static const FName OpName(TEXT("VMOp"));
	int32 Index = Expressions.Add(MakeShareable(new FNiagaraExpression_VMOperation(this, FNiagaraVariableInfo(OpName, ENiagaraDataType::Vector), InputExpressions, Op)));
	return Expressions[Index];
}

TNiagaraExprPtr FNiagaraCompiler_VectorVM::Expression_VMNative(VectorVM::EOp Op, TNiagaraExprPtr A)
{
	TArray<TNiagaraExprPtr> In; In.Add(A);
	return Expression_VMNative(Op, In);
}

TNiagaraExprPtr FNiagaraCompiler_VectorVM::Expression_VMNative(VectorVM::EOp Op, TNiagaraExprPtr A, TNiagaraExprPtr B)
{
	TArray<TNiagaraExprPtr> In; In.Add(A); In.Add(B);
	return Expression_VMNative(Op, In);
}

TNiagaraExprPtr FNiagaraCompiler_VectorVM::Expression_VMNative(VectorVM::EOp Op, TNiagaraExprPtr A, TNiagaraExprPtr B, TNiagaraExprPtr C)
{
	TArray<TNiagaraExprPtr> In; In.Add(A); In.Add(B); In.Add(C);
	return Expression_VMNative(Op, In);
}

TNiagaraExprPtr FNiagaraCompiler_VectorVM::Expression_VMNative(VectorVM::EOp Op, TNiagaraExprPtr A, TNiagaraExprPtr B, TNiagaraExprPtr C, TNiagaraExprPtr D)
{
	TArray<TNiagaraExprPtr> In; In.Add(A); In.Add(B); In.Add(C); In.Add(D);
	return Expression_VMNative(Op, In);
}

bool FNiagaraCompiler_VectorVM::Add_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::add, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Subtract_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add((Expression_VMNative(VectorVM::EOp::sub, InputExpressions)));
	return true;
}

bool FNiagaraCompiler_VectorVM::Multiply_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::mul, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Divide_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::div, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::MultiplyAdd_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::mad, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Lerp_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::lerp, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Reciprocal_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::rcp, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::ReciprocalSqrt_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::rsq, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Sqrt_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::sqrt, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Negate_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::neg, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Abs_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::abs, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Exp_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::exp, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Exp2_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::exp2, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Log_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::log, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::LogBase2_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::log2, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Sin_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::sin, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Cos_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::cos, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Tan_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::tan, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::ASin_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::asin, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::ACos_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::acos, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::ATan_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::atan, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::ATan2_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::atan2, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Ceil_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::ceil, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Floor_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::floor, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Modulo_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::fmod, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Fractional_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::frac, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Truncate_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::trunc, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Clamp_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::clamp, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Min_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::min, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Max_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::max, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Pow_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::pow, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Sign_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::sign, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Step_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::step, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Dot_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::dot, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Cross_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::cross, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Normalize_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::normalize, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Random_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::random, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Length_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::length, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Noise_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::noise, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Scalar_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	GetX_Internal(InputExpressions, OutputExpressions);
	return true;
}

bool FNiagaraCompiler_VectorVM::Vector_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(InputExpressions[0]);
	return true;
}

bool FNiagaraCompiler_VectorVM::Matrix_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	//Just collect the matrix rows into an expression for what ever use subsequent expressions have.
	TNiagaraExprPtr MatrixExpr = Expression_Collection(InputExpressions);
	MatrixExpr->Result.Type = ENiagaraDataType::Matrix;
	OutputExpressions.Add(MatrixExpr);
	return true;
}

bool FNiagaraCompiler_VectorVM::Compose_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::compose, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::ComposeX_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::composex, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::ComposeY_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::composey, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::ComposeZ_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::composez, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::ComposeW_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::composew, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Split_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	GetX_Internal(InputExpressions, OutputExpressions);
	GetY_Internal(InputExpressions, OutputExpressions);
	GetZ_Internal(InputExpressions, OutputExpressions);
	GetW_Internal(InputExpressions, OutputExpressions);
	return true;
}

bool FNiagaraCompiler_VectorVM::GetX_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::splatx, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::GetY_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::splaty, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::GetZ_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::splatz, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::GetW_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::splatw, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::TransformVector_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	TNiagaraExprPtr MatrixExpr = InputExpressions[0];
	check(MatrixExpr->SourceExpressions.Num() == 4);
	TNiagaraExprPtr Row0 = MatrixExpr->GetSourceExpression(0);
	TNiagaraExprPtr Row1 = MatrixExpr->GetSourceExpression(1);
	TNiagaraExprPtr Row2 = MatrixExpr->GetSourceExpression(2);
	TNiagaraExprPtr Row3 = MatrixExpr->GetSourceExpression(3);

	TNiagaraExprPtr VectorExpr = InputExpressions[1];

	TArray<TNiagaraExprPtr> Temp;
	Temp.Add(Expression_VMNative(VectorVM::EOp::splatx, VectorExpr));
	Temp.Add(Expression_VMNative(VectorVM::EOp::splaty, VectorExpr));
	Temp.Add(Expression_VMNative(VectorVM::EOp::splatz, VectorExpr));
	Temp.Add(Expression_VMNative(VectorVM::EOp::splatw, VectorExpr));

	Temp[0] = Expression_VMNative(VectorVM::EOp::mul, MatrixExpr->GetSourceExpression(0), Temp[0]);
	Temp[1] = Expression_VMNative(VectorVM::EOp::mul, MatrixExpr->GetSourceExpression(1), Temp[1]);
	Temp[2] = Expression_VMNative(VectorVM::EOp::mul, MatrixExpr->GetSourceExpression(2), Temp[2]);
	Temp[3] = Expression_VMNative(VectorVM::EOp::mul, MatrixExpr->GetSourceExpression(3), Temp[3]);

	Temp[0] = Expression_VMNative(VectorVM::EOp::add, Temp[0], Temp[1]);
	Temp[1] = Expression_VMNative(VectorVM::EOp::add, Temp[2], Temp[3]);
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::add, Temp[0], Temp[1]));
	return true;
}

bool FNiagaraCompiler_VectorVM::Transpose_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	TNiagaraExprPtr MatrixExpr = InputExpressions[0];
	check(MatrixExpr->SourceExpressions.Num() == 4);
	TNiagaraExprPtr Row0 = MatrixExpr->GetSourceExpression(0);
	TNiagaraExprPtr Row1 = MatrixExpr->GetSourceExpression(1);
	TNiagaraExprPtr Row2 = MatrixExpr->GetSourceExpression(2);
	TNiagaraExprPtr Row3 = MatrixExpr->GetSourceExpression(3);

	//TODO: can probably make this faster if I provide direct shuffle ops in the VM rather than using the flexible, catch-all op compose.
	//Maybe don't want to make that externally usable though? Maybe for power users...
	TArray<TNiagaraExprPtr> Columns;
	Columns.Add(Expression_VMNative(VectorVM::EOp::composex, Row0, Row1, Row2, Row3));
	Columns.Add(Expression_VMNative(VectorVM::EOp::composey, Row0, Row1, Row2, Row3));
	Columns.Add(Expression_VMNative(VectorVM::EOp::composez, Row0, Row1, Row2, Row3));
	Columns.Add(Expression_VMNative(VectorVM::EOp::composew, Row0, Row1, Row2, Row3));

	Matrix_Internal(Columns, OutputExpressions);
	return true;
}


bool FNiagaraCompiler_VectorVM::Inverse_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	Error(LOCTEXT("VMNoImpl", "VM Operation not yet implemented"), nullptr, nullptr);
	return false;
}


bool FNiagaraCompiler_VectorVM::LessThan_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::lessthan, InputExpressions));
	return true;
}


bool FNiagaraCompiler_VectorVM::Sample_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::sample, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::Write_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::bufferwrite, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::EventBroadcast_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::eventbroadcast, InputExpressions));
	return true;
}

bool FNiagaraCompiler_VectorVM::EaseIn_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::easein, InputExpressions));
	return true;
}


bool FNiagaraCompiler_VectorVM::EaseInOut_Internal(TArray<TNiagaraExprPtr>& InputExpressions, TArray<TNiagaraExprPtr>& OutputExpressions)
{
	OutputExpressions.Add(Expression_VMNative(VectorVM::EOp::easeinout, InputExpressions));
	return true;
}


#undef LOCTEXT_NAMESPACE
