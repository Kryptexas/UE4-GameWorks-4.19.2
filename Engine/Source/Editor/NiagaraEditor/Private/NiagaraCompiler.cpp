// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorPrivatePCH.h"
#include "CompilerResultsLog.h"
#include "EdGraphUtilities.h"
#include "VectorVM.h"

DEFINE_LOG_CATEGORY_STATIC(LogNiagaraCompiler,All,All);

/** Information related to an expression. */
struct FNiagaraExpr
{
	/** VM opcode. */
	uint8 OpIndex;
	/** Source operands. */
	int32 Src[3];

	/** Default constructor. */
	FNiagaraExpr()
	{
		OpIndex = 0;
		for (int32 i = 0; i < 3; i++)
		{
			Src[i] = INDEX_NONE;
		}
	}
};

/** Compiler context, passed around during compilation. */
struct FNiagaraCompilerContext
{
	/** Expression. */
	TArray<FNiagaraExpr> Expressions;
	/** Constant table. */
	TArray<float> Constants;
	/** Constant names. */
	TArray<FName> ConstantNames;
	/** Map from pin to expression to prevent executing subtrees multiple times. */
	TMap<UEdGraphPin*,int32> PinToExpression;
	/** Name of all attributes. */
	TArray<FName> Attributes;
	/** The compilation log. */
	FCompilerResultsLog& Log;

	/** Initialization constructor. */
	explicit FNiagaraCompilerContext(FCompilerResultsLog& InLog)
		: Log(InLog)
	{
		// Setup built-in constants.
		ConstantNames.Add(TEXT("__zero__"));
		Constants.Add(0.0f);
		ConstantNames.Add(TEXT("DeltaTime"));
		Constants.Add(0.0f);
	}
};

/** Convert an expression index to an attribute index. */
static int32 AttrIndexFromExpressionIndex(int32 ExpressionIndex)
{
	return ExpressionIndex & 0x3fffffff;
}

/** Returns true if the expression index refers to an attribute. */
static bool IsAttrIndex(int32 ExpressionIndex)
{
	return ExpressionIndex != INDEX_NONE && (ExpressionIndex & 0x80000000);
}

/** Convert an expression index to a constant index. */
static int32 ConstIndexFromExpressionIndex(int32 ExpressionIndex)
{
	return ExpressionIndex & 0x3fffffff;
}

/** Returns true if the expression index refers to a constant. */
static bool IsConstIndex(int32 ExpressionIndex)
{
	return ExpressionIndex != INDEX_NONE && (ExpressionIndex & 0x40000000);
}

/** Returns true if the expression index is valid. */
static bool IsValidExpressionIndex(FNiagaraCompilerContext& Context, int32 ExpressionIndex)
{
	if (ExpressionIndex == INDEX_NONE)
	{
		return false;
	}
	if (IsAttrIndex(ExpressionIndex))
	{
		return Context.Attributes.IsValidIndex(AttrIndexFromExpressionIndex(ExpressionIndex));
	}
	return Context.Expressions.IsValidIndex(ExpressionIndex);
}

/** Returns true if the op has valid source operands. */
static bool IsValidOp(FNiagaraCompilerContext& Context, FNiagaraExpr& Op)
{
	bool bValid = true;
	for (int32 SrcIndex = 0; bValid && SrcIndex < 3; ++SrcIndex)
	{
		VectorVM::EOpSrc::Type Type = VectorVM::GetOpCodeInfo(Op.OpIndex).SrcTypes[SrcIndex];
		int32 ExpressionIndex = Op.Src[SrcIndex];
		switch (Type)
		{
		case VectorVM::EOpSrc::Register:
			bValid = bValid && IsValidExpressionIndex(Context, ExpressionIndex);
			break;
		case VectorVM::EOpSrc::Const:
			bValid = bValid && IsConstIndex(ExpressionIndex)
				&& Context.Constants.IsValidIndex(ConstIndexFromExpressionIndex(ExpressionIndex));
			break;
		default:
			break;
		}
	}
	return bValid;
}

/** Evaluates the graph connected to a pin. */
static int32 EvaluateGraph(FNiagaraCompilerContext& Context, UEdGraphPin* Pin)
{
	int32 ExpressionIndex = INDEX_NONE;
	if (Pin)
	{
		if (Pin->LinkedTo.Num() == 1)
		{
			check(Pin->Direction == EGPD_Input);
			Pin = Pin->LinkedTo[0];
			check(Pin->Direction == EGPD_Output);

			int32* CachedExpressionIndex = Context.PinToExpression.Find(Pin);
			if (CachedExpressionIndex)
			{
				ExpressionIndex = *CachedExpressionIndex;
			}
			else
			{
				UEdGraphNode* Node = Pin->GetOwningNode();
				if (Node->IsA(UNiagaraNodeOp::StaticClass()))
				{
					UNiagaraNodeOp* OpNode = (UNiagaraNodeOp*)Node;
					VectorVM::FVectorVMOpInfo const& OpInfo = VectorVM::GetOpCodeInfo(OpNode->OpIndex);

					FNiagaraExpr NewOp;
					NewOp.OpIndex = OpNode->OpIndex;
					for (int32 SrcIndex = 0; SrcIndex < 3; ++SrcIndex)
					{
						if (OpInfo.SrcTypes[SrcIndex] != VectorVM::EOpSrc::Invalid)
						{
							UEdGraphPin* SrcPin = OpNode->FindPinChecked(UNiagaraNodeOp::InPinNames[SrcIndex]);
							NewOp.Src[SrcIndex] = EvaluateGraph(Context,SrcPin);
						}
					}

					while (!IsValidOp(Context, NewOp) && VectorVM::GetOpCodeInfo(NewOp.OpIndex).BaseOpcode == OpInfo.BaseOpcode)
					{
						NewOp.OpIndex++;
					}

					if (OpInfo.IsCommutative() && !IsValidOp(Context, NewOp))
					{
						int32 Temp = NewOp.Src[0];
						NewOp.Src[0] = NewOp.Src[1];
						NewOp.Src[1] = Temp;
						NewOp.OpIndex = OpNode->OpIndex;
						while (!IsValidOp(Context, NewOp) && VectorVM::GetOpCodeInfo(NewOp.OpIndex).BaseOpcode == OpInfo.BaseOpcode)
						{
							NewOp.OpIndex++;
						}
					}

					if (IsValidOp(Context, NewOp))
					{
						ExpressionIndex = Context.Expressions.Add(NewOp);
					}
				}
				else if (Node->IsA(UNiagaraNodeGetAttr::StaticClass()))
				{
					UNiagaraNodeGetAttr* AttrNode = (UNiagaraNodeGetAttr*)Node;
					int32 AttrIndex = INDEX_NONE;
					if (Context.Attributes.Find(AttrNode->AttrName, AttrIndex))
					{
						ExpressionIndex = AttrIndex | 0x80000000;
					}
					else if (Context.ConstantNames.Find(AttrNode->AttrName, AttrIndex))
					{
						ExpressionIndex = AttrIndex | 0x40000000;
					}
				}
				Context.PinToExpression.Add(Pin, ExpressionIndex);
			}
		}
		else if (!Pin->bDefaultValueIsIgnored)
		{
			FString ConstString = Pin->GetDefaultAsString();
			FName ConstName(*ConstString);
			int32 ConstIndex = Context.ConstantNames.Find(ConstName);
			if (ConstIndex == INDEX_NONE)
			{
				float ConstValue = FCString::Atof(*ConstString);
				if (FMath::Abs(ConstValue) < SMALL_NUMBER)
				{
					ConstIndex = 0;
				}
				else
				{
					ConstIndex = Context.Constants.Add(ConstValue);
					check(Context.ConstantNames.Num() == ConstIndex);
					Context.ConstantNames.Add(ConstName);
				}
			}
			check(Context.Constants.IsValidIndex(ConstIndex));
			ExpressionIndex = ConstIndex | 0x40000000;			
		}
	}
	return ExpressionIndex;
}

void FNiagaraEditorModule::CompileScript(UNiagaraScript* ScriptToCompile)
{
	check(ScriptToCompile != NULL);
	if (ScriptToCompile->Source == NULL)
	{
		UE_LOG(LogNiagaraCompiler, Error, TEXT("No source for Niagara script: %s"), *ScriptToCompile->GetPathName());
		return;
	}

	TComponentReregisterContext<UNiagaraComponent> ComponentReregisterContext;

	UNiagaraScriptSource* Source = CastChecked<UNiagaraScriptSource>(ScriptToCompile->Source);
	check(Source->UpdateGraph);

	// Results log.
	FCompilerResultsLog MessageLog;

	// Clone the source graph so we can modify it as needed; merging in the child graphs
	UEdGraph* UpdateGraph = FEdGraphUtilities::CloneGraph(Source->UpdateGraph, Source, &MessageLog, true); 
	FEdGraphUtilities::MergeChildrenGraphsIn(UpdateGraph, UpdateGraph, /*bRequireSchemaMatch=*/ true);

	// Find the output node.
	UNiagaraNodeOutputUpdate* OutputNode = NULL;
	{
		TArray<UNiagaraNodeOutputUpdate*> OutputNodes;
		UpdateGraph->GetNodesOfClass(OutputNodes);
		if (OutputNodes.Num() != 1)
		{
			UE_LOG(LogNiagaraCompiler, Error, TEXT("Script contains %s output nodes: %s"),
				OutputNodes.Num() == 0 ? TEXT("no") : TEXT("too many"),
				*ScriptToCompile->GetPathName()
				);
			return;
		}
		OutputNode = OutputNodes[0];
	}
	check(OutputNode);

	// Traverse the node graph for each output and generate expressions as we go.
	FNiagaraCompilerContext Context(MessageLog);
	Source->GetUpdateOutputs(Context.Attributes);
	TArray<int32> OutputExpressions;
	OutputExpressions.AddUninitialized(Context.Attributes.Num());
	FMemory::Memset(OutputExpressions.GetData(), 0xff, Context.Attributes.Num() * sizeof(int32));

	for (int32 PinIndex = 0; PinIndex < OutputNode->Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = OutputNode->Pins[PinIndex];
		int32 AttrIndex = Context.Attributes.Find(FName(*Pin->PinName));
		if (AttrIndex != INDEX_NONE)
		{
			int32 ExpressionIndex = EvaluateGraph(Context, Pin);
			OutputExpressions[AttrIndex] = ExpressionIndex;
		}
	}

	// Generate pass-thru ops for any outputs that are not connected.
	for (int32 AttrIndex = 0; AttrIndex < OutputExpressions.Num(); ++AttrIndex)
	{
		if (OutputExpressions[AttrIndex] == INDEX_NONE)
		{
			// Generate a pass-thru op.
			FNiagaraExpr PassThru;
			PassThru.OpIndex = VectorVM::EOp::addi;
			PassThru.Src[0] = AttrIndex | 0x80000000;
			PassThru.Src[1] = 0 | 0x40000000;
			PassThru.Src[2] = INDEX_NONE;
			OutputExpressions[AttrIndex] = Context.Expressions.Add(PassThru);
		}
	}
	
	// Figure out the lifetime of each expression.
	TArray<int32> ExpressionLifetimes;
	ExpressionLifetimes.AddUninitialized(Context.Expressions.Num());
	for (int32 i = 0; i < Context.Expressions.Num(); ++i)
	{
		ExpressionLifetimes[i] = i;
		FNiagaraExpr& Expr = Context.Expressions[i];
		VectorVM::FVectorVMOpInfo const& OpInfo = VectorVM::GetOpCodeInfo(Expr.OpIndex);
		for (int32 k = 0; k < 3; ++k)
		{
			if (OpInfo.SrcTypes[k] == VectorVM::EOpSrc::Register
				&& !IsAttrIndex(Expr.Src[k]))
			{
				check(Context.Expressions.IsValidIndex(Expr.Src[k]));
				check(Expr.Src[k] < i);
				ExpressionLifetimes[Expr.Src[k]] = i;
			}
		}
	}

	// Allocate temporary registers for the output of each expression.
	int32 Registers[VectorVM::NumTempRegisters];
	FMemory::Memset(Registers, 0xff, sizeof(Registers));
	TArray<int32> RegisterAssignments;
	RegisterAssignments.AddUninitialized(Context.Expressions.Num());
	for (int32 i = 0; i < Context.Expressions.Num(); ++i)
	{
		FNiagaraExpr& Expr = Context.Expressions[i];
		VectorVM::FVectorVMOpInfo const& OpInfo = VectorVM::GetOpCodeInfo(Expr.OpIndex);

		for (int32 j = 0; j < 3; ++j)
		{
			if (OpInfo.SrcTypes[j] == VectorVM::EOpSrc::Register)
			{
				if (IsAttrIndex(Expr.Src[j]))
				{
					Expr.Src[j] = VectorVM::FirstInputRegister + AttrIndexFromExpressionIndex(Expr.Src[j]);
				}
				else
				{
					Expr.Src[j] = RegisterAssignments[Expr.Src[j]];
				}
			}
			else if (OpInfo.SrcTypes[j] == VectorVM::EOpSrc::Const)
			{
				// VectorVM::EOpSrc::Const.
				Expr.Src[j] = ConstIndexFromExpressionIndex(Expr.Src[j]);
			}
		}

		int32 AttrIndex = INDEX_NONE;
		if (OutputExpressions.Find(i, AttrIndex))
		{
			// Assign to an output register.
			RegisterAssignments[i] = VectorVM::FirstOutputRegister + AttrIndex;
		}
		else
		{
			int32 AssignedRegister = INDEX_NONE;
			for (int32 j = 0; j < VectorVM::NumTempRegisters; ++j)
			{
				if (Registers[j] == INDEX_NONE
					|| ExpressionLifetimes[Registers[j]] < i)
				{
					AssignedRegister = j;
					break;
				}
			}
			check(AssignedRegister != INDEX_NONE && AssignedRegister < VectorVM::NumTempRegisters);

			Registers[AssignedRegister] = i;
			RegisterAssignments[i] = AssignedRegister;
		}
	}

	// Generate bytecode!
	TArray<uint8>& Code = ScriptToCompile->ByteCode;
	Code.Empty();
	for (int32 i = 0; i < Context.Expressions.Num(); ++i)
	{
		FNiagaraExpr& Expr = Context.Expressions[i];
		VectorVM::FVectorVMOpInfo const& OpInfo = VectorVM::GetOpCodeInfo(Expr.OpIndex);

		int32 DestRegister = RegisterAssignments[i];
		check(DestRegister < VectorVM::MaxRegisters);

		Code.Add(Expr.OpIndex);
		Code.Add(DestRegister);
		for (int32 j = 0; j < 3; ++j)
		{
			if (OpInfo.SrcTypes[j] == VectorVM::EOpSrc::Invalid)
				break;
			Code.Add(Expr.Src[j]);
		}

		// If the expression is output to multiple attributes, copy them here.
		if (DestRegister >= VectorVM::FirstOutputRegister)
		{
			int32 AttrIndex = DestRegister - VectorVM::FirstOutputRegister;
			for (int32 j = AttrIndex+1; j < OutputExpressions.Num(); ++j)
			{
				if (OutputExpressions[j] == i)
				{
					Code.Add(VectorVM::EOp::addi);
					Code.Add(VectorVM::FirstOutputRegister + j);
					Code.Add(DestRegister);
					Code.Add(0);
				}
			}
		}
	}

	// Terminate with the 'done' opcode.
	Code.Add(VectorVM::EOp::done);

	// And copy the constant table and attributes.
	ScriptToCompile->ConstantTable = Context.Constants;
	ScriptToCompile->Attributes = Context.Attributes;
}
