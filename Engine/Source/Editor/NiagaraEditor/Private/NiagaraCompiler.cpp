// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorPrivatePCH.h"
#include "Engine/NiagaraScript.h"
#include "Components/NiagaraComponent.h"
#include "CompilerResultsLog.h"
#include "EdGraphUtilities.h"
#include "VectorVM.h"
#include "ComponentReregisterContext.h"
#include "NiagaraCompiler_VectorVM.h"
#include "NiagaraEditorCommon.h"

#define LOCTEXT_NAMESPACE "NiagaraCompiler"

DEFINE_LOG_CATEGORY_STATIC(LogNiagaraCompiler,All,All);

void FNiagaraEditorModule::CompileScript(UNiagaraScript* ScriptToCompile)
{
	check(ScriptToCompile != NULL);
	if (ScriptToCompile->Source == NULL)
	{
		UE_LOG(LogNiagaraCompiler, Error, TEXT("No source for Niagara script: %s"), *ScriptToCompile->GetPathName());
		return;
	}

	TComponentReregisterContext<UNiagaraComponent> ComponentReregisterContext;

	FNiagaraCompiler_VectorVM Compiler;
	Compiler.CompileScript(ScriptToCompile);

	//Compile for compute here too?
}

//////////////////////////////////////////////////////////////////////////

/** Expression that gets an input attribute. */
class FNiagaraExpression_GetAttribute : public FNiagaraExpression
{
public:

	FNiagaraExpression_GetAttribute(class FNiagaraCompiler* InCompiler, const FNiagaraVariableInfo& InAttribute)
		: FNiagaraExpression(InCompiler, InAttribute)
	{
		ResultLocation = ENiagaraExpressionResultLocation::InputData;
	}

	virtual void Process()override
	{
		check(ResultLocation == ENiagaraExpressionResultLocation::InputData);
		ResultIndex = Compiler->GetAttributeIndex(Result);
	}
};

/** Expression that gets a constant. */
class FNiagaraExpression_GetConstant : public FNiagaraExpression
{
public:

	FNiagaraExpression_GetConstant(class FNiagaraCompiler* InCompiler, const FNiagaraVariableInfo& InConstant, bool bIsInternal)
		: FNiagaraExpression(InCompiler, InConstant)
		, bInternal(bIsInternal)
	{
		ResultLocation = ENiagaraExpressionResultLocation::Constants;
		//For matrix constants we must add 4 input expressions that will be filled in as the constant is processed.
		//They must at least exist now so that other expressions can reference them.
		if (Result.Type == ENiagaraDataType::Matrix)
		{
			static const FName ResultName(TEXT("MatrixComponent"));
			SourceExpressions.Add(MakeShareable(new FNiagaraExpression(InCompiler, FNiagaraVariableInfo(ResultName, ENiagaraDataType::Vector))));
			SourceExpressions.Add(MakeShareable(new FNiagaraExpression(InCompiler, FNiagaraVariableInfo(ResultName, ENiagaraDataType::Vector))));
			SourceExpressions.Add(MakeShareable(new FNiagaraExpression(InCompiler, FNiagaraVariableInfo(ResultName, ENiagaraDataType::Vector))));
			SourceExpressions.Add(MakeShareable(new FNiagaraExpression(InCompiler, FNiagaraVariableInfo(ResultName, ENiagaraDataType::Vector))));
		}
// 		else if (Result.Type == ENiagaraDataType::Curve)
// 		{
// 			static const FName ResultName(TEXT("Curve"));
// 			SourceExpressions.Add(MakeShareable(new FNiagaraExpression(InCompiler, FNiagaraVariableInfo(ResultName, ENiagaraDataType::Curve))));
// 		}
	}

	virtual void Process()override
	{
		check(ResultLocation == ENiagaraExpressionResultLocation::Constants);
		//Get the index of the constant. Also gets a component index if this is for a packed scalar constant.
		Compiler->GetConstantResultIndex(Result, bInternal, ResultIndex, ComponentIndex);
		
		if (Result.Type == ENiagaraDataType::Matrix)
		{
			check(SourceExpressions.Num() == 4);
			TNiagaraExprPtr Src0 = GetSourceExpression(0);
			TNiagaraExprPtr Src1 = GetSourceExpression(1);
			TNiagaraExprPtr Src2 = GetSourceExpression(2);
			TNiagaraExprPtr Src3 = GetSourceExpression(3);
			Src0->ResultLocation = ENiagaraExpressionResultLocation::Constants;
			Src0->ResultIndex = ResultIndex;
			Src1->ResultLocation = ENiagaraExpressionResultLocation::Constants;
			Src1->ResultIndex = ResultIndex + 1;
			Src2->ResultLocation = ENiagaraExpressionResultLocation::Constants;
			Src2->ResultIndex = ResultIndex + 2;
			Src3->ResultLocation = ENiagaraExpressionResultLocation::Constants;
			Src3->ResultIndex = ResultIndex + 3;
		}
		else if (Result.Type == ENiagaraDataType::Curve)
		{
			ResultLocation = ENiagaraExpressionResultLocation::BufferConstants;
		}
	}

	bool bInternal;
};

/** Expression that just collects some other expressions together. E.g for a matrix. */
class FNiagaraExpression_Collection : public FNiagaraExpression
{
public:

	FNiagaraExpression_Collection(class FNiagaraCompiler* InCompiler, const FNiagaraVariableInfo& Result, TArray<TNiagaraExprPtr>& InSourceExpressions)
		: FNiagaraExpression(InCompiler, Result)
	{
		SourceExpressions = InSourceExpressions;
	}
};

//////////////////////////////////////////////////////////////////////////

void FNiagaraCompiler::CheckInputs(FName OpName, TArray<TNiagaraExprPtr>& Inputs)
{
	//check the types of the input expressions.
	const FNiagaraOpInfo* OpInfo = FNiagaraOpInfo::GetOpInfo(OpName);
	check(OpInfo);
	int32 NumInputs = Inputs.Num();
	check(OpInfo->Inputs.Num() == NumInputs);
	for (int32 i = 0; i < NumInputs ; ++i)
	{
		if (OpInfo->Inputs[i].DataType != Inputs[i]->Result.Type)
		{
			UE_LOG(LogNiagaraCompiler, Error, TEXT("Exression %s has incorrect inputs!\nExpected: %d - Actual: %d"), *OpName.ToString(), (int32)OpInfo->Inputs[i].DataType, (int32)((TNiagaraExprPtr)Inputs[i])->Result.Type.GetValue());
		}
	}
}

void FNiagaraCompiler::CheckOutputs(FName OpName, TArray<TNiagaraExprPtr>& Outputs)
{
	//check the types of the input expressions.
	const FNiagaraOpInfo* OpInfo = FNiagaraOpInfo::GetOpInfo(OpName);
	check(OpInfo);
	int32 NumOutputs = Outputs.Num();
	check(OpInfo->Outputs.Num() == NumOutputs);
	for (int32 i = 0; i < NumOutputs; ++i)
	{
		if (OpInfo->Outputs[i].DataType != Outputs[i]->Result.Type)
		{
			UE_LOG(LogNiagaraCompiler, Error, TEXT("Exression %s has incorrect outputs!\nExpected: %d - Actual: %d"), *OpName.ToString(), (int32)OpInfo->Outputs[i].DataType, (int32)((TNiagaraExprPtr)Outputs[i])->Result.Type.GetValue());
		}
	}
}

int32 FNiagaraCompiler::GetAttributeIndex(const FNiagaraVariableInfo& Attr)const
{
	return SourceGraph->GetAttributeIndex(Attr);
}

TNiagaraExprPtr FNiagaraCompiler::CompileOutputPin(UEdGraphPin* Pin)
{
	check(Pin->Direction == EGPD_Output);

	TNiagaraExprPtr Ret;

	TNiagaraExprPtr* Expr = PinToExpression.Find(Pin);
	if (Expr)
	{
		Ret = *Expr; //We've compiled this pin before. Return it's expression.
	}
	else
	{
		//Otherwise we need to compile the node to get its output pins.
		UNiagaraNode* Node = Cast<UNiagaraNode>(Pin->GetOwningNode());
		TArray<FNiagaraNodeResult> Outputs;
		Node->Compile(this, Outputs);
		for (int32 i=0; i<Outputs.Num(); ++i)
		{
			PinToExpression.Add(Outputs[i].OutputPin, Outputs[i].Expression);
			//We also grab the expression for the pin we're currently interested in. Otherwise we'd have to search the map for it.
			Ret = Outputs[i].OutputPin == Pin ? Outputs[i].Expression : Ret;
		}
	}

	return Ret;
}

TNiagaraExprPtr FNiagaraCompiler::CompilePin(UEdGraphPin* Pin)
{
	check(Pin);
	TNiagaraExprPtr Ret;
	if (Pin->Direction == EGPD_Input)
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			// > 1 ???
			Ret = CompileOutputPin(Pin->LinkedTo[0]);
		}
		else if (!Pin->bDefaultValueIsIgnored)
		{
			//No connections to this input so add the default as a const expression.
			const UEdGraphSchema_Niagara* Schema = Cast<const UEdGraphSchema_Niagara>(Pin->GetSchema());
			ENiagaraDataType PinType = Schema->GetPinType(Pin);

			FString ConstString = Pin->GetDefaultAsString();
			FName ConstName(*ConstString);

			FNiagaraVariableInfo Var(ConstName, PinType);

			switch (PinType)
			{
			case ENiagaraDataType::Scalar:
			{
				float Default;
				Schema->GetPinDefaultValue(Pin, Default);
				ConstantData.SetOrAddInternal(Var, Default);
			}
				break;
			case ENiagaraDataType::Vector:
			{
				FVector4 Default;
				Schema->GetPinDefaultValue(Pin, Default);
				ConstantData.SetOrAddInternal(Var, Default);
			}
				break;
			case ENiagaraDataType::Matrix:
			{
				FMatrix Default;
				Schema->GetPinDefaultValue(Pin, Default);
				ConstantData.SetOrAddInternal(Var, Default);
			}
				break;
			case ENiagaraDataType::Curve:
			{
				FNiagaraDataObject *Default = nullptr;
				ConstantData.SetOrAddInternal(Var, Default);
			}
				break;
			}
			Ret = Expression_GetInternalConstant(Var);
		}
	}
	else
	{
		Ret = CompileOutputPin(Pin);
	}

	return Ret;
}

void FNiagaraCompiler::GetParticleAttributes(TArray<FNiagaraVariableInfo>& OutAttributes)const
{
	check(SourceGraph);
	SourceGraph->GetAttributes(OutAttributes);
}

TNiagaraExprPtr FNiagaraCompiler::Expression_GetAttribute(const FNiagaraVariableInfo& Attribute)
{
	int32 Index = Expressions.Add(MakeShareable(new FNiagaraExpression_GetAttribute(this, Attribute)));
	return Expressions[Index];
}

TNiagaraExprPtr FNiagaraCompiler::Expression_GetExternalConstant(const FNiagaraVariableInfo& Constant)
{
	int32 Index = Expressions.Add(MakeShareable(new FNiagaraExpression_GetConstant(this, Constant, false)));
	return Expressions[Index];
}

TNiagaraExprPtr FNiagaraCompiler::Expression_GetInternalConstant(const FNiagaraVariableInfo& Constant)
{
	int32 Index = Expressions.Add(MakeShareable(new FNiagaraExpression_GetConstant(this, Constant, true)));
	return Expressions[Index];
}

TNiagaraExprPtr FNiagaraCompiler::Expression_Collection(TArray<TNiagaraExprPtr>& SourceExpressions)
{
	//Todo - Collection expressions are just to collect parts of a matrix currently. Its a crappy way to do things that should be replaced.
	//Definitely don't start using it for collections of other things.
	int32 Index = Expressions.Add(MakeShareable(new FNiagaraExpression_Collection(this, FNiagaraVariableInfo(NAME_None, ENiagaraDataType::Vector), SourceExpressions)));
	return Expressions[Index];
}

TNiagaraExprPtr FNiagaraCompiler::GetAttribute(const FNiagaraVariableInfo& Attribute)
{
	return Expression_GetAttribute(Attribute);
}

TNiagaraExprPtr FNiagaraCompiler::GetExternalConstant(const FNiagaraVariableInfo& Constant, float Default)
{
	SetOrAddConstant(false, Constant, Default);
	return Expression_GetExternalConstant(Constant);
}
TNiagaraExprPtr FNiagaraCompiler::GetExternalConstant(const FNiagaraVariableInfo& Constant, const FVector4& Default)
{
	SetOrAddConstant(false, Constant, Default);
	return Expression_GetExternalConstant(Constant);
}
TNiagaraExprPtr FNiagaraCompiler::GetExternalConstant(const FNiagaraVariableInfo& Constant, const FMatrix& Default)
{
	SetOrAddConstant(false, Constant, Default);
	return Expression_GetExternalConstant(Constant);
}

TNiagaraExprPtr FNiagaraCompiler::GetExternalCurveConstant(const FNiagaraVariableInfo& Constant)
{
	SetOrAddConstant<FNiagaraDataObject*>(false, Constant, nullptr);
	return Expression_GetExternalConstant(Constant);
}

#undef LOCTEXT_NAMESPACE