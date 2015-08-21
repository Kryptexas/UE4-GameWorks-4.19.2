// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilerCppBackendModulePrivatePCH.h"
#include "BlueprintCompilerCppBackend.h"
#include "BlueprintCompilerCppBackendUtils.h"
#include "EdGraphSchema_K2.h"

// Generates single "if" scope. Its condition checks context of given term.
struct FSafeContextScopedEmmitter
{
private:
	FEmitterLocalContext& EmitterContext;
	bool bSafeContextUsed;
public:

	bool IsSafeContextUsed() const
	{
		return bSafeContextUsed;
	}

	static FString ValidationChain(FEmitterLocalContext& EmitterContext, const FBPTerminal* Term, FBlueprintCompilerCppBackend& CppBackend)
	{
		TArray<FString> SafetyConditions;
		for (; Term; Term = Term->Context)
		{
			if (!Term->IsStructContextType() && (Term->Type.PinSubCategory != TEXT("self")))
			{
				SafetyConditions.Add(CppBackend.TermToText(EmitterContext, Term, false));
			}
		}

		FString Result;
		for (int32 Iter = SafetyConditions.Num() - 1; Iter >= 0; --Iter)
		{
			Result += FString(TEXT("IsValid("));
			Result += SafetyConditions[Iter];
			Result += FString(TEXT(")"));
			if (Iter)
			{
				Result += FString(TEXT(" && "));
			}
		}

		return Result;
	}

	FSafeContextScopedEmmitter(FEmitterLocalContext& InEmitterContext, const FBPTerminal* Term, FBlueprintCompilerCppBackend& CppBackend)
		: EmitterContext(InEmitterContext)
		, bSafeContextUsed(false)
	{
		const FString Conditions = ValidationChain(EmitterContext, Term, CppBackend);

		if (!Conditions.IsEmpty())
		{
			bSafeContextUsed = true;
			EmitterContext.AddLine(FString::Printf(TEXT("if(%s)"), *Conditions));
			EmitterContext.AddLine(TEXT("{"));
			EmitterContext.IncreaseIndent();
		}
	}

	~FSafeContextScopedEmmitter()
	{
		if (bSafeContextUsed)
		{
			EmitterContext.DecreaseIndent();
			EmitterContext.AddLine(TEXT("}"));
		}
	}

};

void FBlueprintCompilerCppBackend::EmitCallDelegateStatment(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.FunctionContext && Statement.FunctionContext->AssociatedVarProperty);
	FSafeContextScopedEmmitter SafeContextScope(EmitterContext, Statement.FunctionContext->Context, *this);
	EmitterContext.AddLine(FString::Printf(TEXT("%s.Broadcast(%s);"), *TermToText(EmitterContext, Statement.FunctionContext, false), *EmitMethodInputParameterList(EmitterContext, Statement)));
}

void FBlueprintCompilerCppBackend::EmitCallStatment(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	const bool bCallOnDifferentObject = Statement.FunctionContext && (Statement.FunctionContext->Name != TEXT("self"));
	const bool bStaticCall = Statement.FunctionToCall->HasAnyFunctionFlags(FUNC_Static);
	const bool bUseSafeContext = bCallOnDifferentObject && !bStaticCall;
	{
		FSafeContextScopedEmmitter SafeContextScope(EmitterContext, bUseSafeContext ? Statement.FunctionContext : nullptr, *this);
		FString Result = EmitCallStatmentInner(EmitterContext, Statement, false);
		EmitterContext.AddLine(Result);
	}
}

void FBlueprintCompilerCppBackend::EmitAssignmentStatment(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.LHS && Statement.RHS[0]);
	FString DestinationExpression = TermToText(EmitterContext, Statement.LHS, false);
	FString SourceExpression = TermToText(EmitterContext, Statement.RHS[0]);

	FSafeContextScopedEmmitter SafeContextScope(EmitterContext, Statement.LHS->Context, *this);

	FString BeginCast;
	FString EndCast;
	FEmitHelper::GenerateAutomaticCast(Statement.LHS->Type, Statement.RHS[0]->Type, BeginCast, EndCast);
	EmitterContext.AddLine(FString::Printf(TEXT("%s = %s%s%s;"), *DestinationExpression, *BeginCast, *SourceExpression, *EndCast));
}

void FBlueprintCompilerCppBackend::EmitCastObjToInterfaceStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString InterfaceClass = TermToText(EmitterContext, Statement.RHS[0]);
	FString ObjectValue = TermToText(EmitterContext, Statement.RHS[1]);
	FString InterfaceValue = TermToText(EmitterContext, Statement.LHS);

	EmitterContext.AddLine(FString::Printf(TEXT("if ( IsValid(%s) && %s->GetClass()->ImplementsInterface(%s) )"), *ObjectValue, *ObjectValue, *InterfaceClass));
	EmitterContext.AddLine(FString::Printf(TEXT("{")));
	EmitterContext.AddLine(FString::Printf(TEXT("\t%s.SetObject(%s);"), *InterfaceValue, *ObjectValue));
	EmitterContext.AddLine(FString::Printf(TEXT("\tvoid* IAddress = %s->GetInterfaceAddress(%s);"), *ObjectValue, *InterfaceClass));
	EmitterContext.AddLine(FString::Printf(TEXT("\t%s.SetInterface(IAddress);"), *InterfaceValue));
	EmitterContext.AddLine(FString::Printf(TEXT("}")));
	EmitterContext.AddLine(FString::Printf(TEXT("else")));
	EmitterContext.AddLine(FString::Printf(TEXT("{")));
	EmitterContext.AddLine(FString::Printf(TEXT("\t%s.SetObject(nullptr);"), *InterfaceValue));
	EmitterContext.AddLine(FString::Printf(TEXT("}")));
}

void FBlueprintCompilerCppBackend::EmitCastBetweenInterfacesStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString ClassToCastTo = TermToText(EmitterContext, Statement.RHS[0]);
	FString InputInterface = TermToText(EmitterContext, Statement.RHS[1]);
	FString ResultInterface = TermToText(EmitterContext, Statement.LHS);

	FString InputObject = FString::Printf(TEXT("%s.GetObjectRef()"), *InputInterface);

	EmitterContext.AddLine(FString::Printf(TEXT("if ( %s && %s->GetClass()->IsChildOf(%s) )"), *InputObject, *InputObject, *ClassToCastTo));
	EmitterContext.AddLine(FString::Printf(TEXT("{")));
	EmitterContext.AddLine(FString::Printf(TEXT("\t%s.SetObject(%s);"), *ResultInterface, *InputObject));
	EmitterContext.AddLine(FString::Printf(TEXT("\tvoid* IAddress = %s->GetInterfaceAddress(%s);"), *InputObject, *ClassToCastTo));
	EmitterContext.AddLine(FString::Printf(TEXT("\t%s.SetInterface(IAddress);"), *ResultInterface));
	EmitterContext.AddLine(FString::Printf(TEXT("}")));
	EmitterContext.AddLine(FString::Printf(TEXT("else")));
	EmitterContext.AddLine(FString::Printf(TEXT("{")));
	EmitterContext.AddLine(FString::Printf(TEXT("\t%s.SetObject(nullptr);"), *ResultInterface));
	EmitterContext.AddLine(FString::Printf(TEXT("}")));
}

void FBlueprintCompilerCppBackend::EmitCastInterfaceToObjStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString ClassToCastTo = TermToText(EmitterContext, Statement.RHS[0]);
	FString InputInterface = TermToText(EmitterContext, Statement.RHS[1]);
	FString ResultObject = TermToText(EmitterContext, Statement.LHS);
	FString InputObject = FString::Printf(TEXT("%s.GetObjectRef()"), *InputInterface);
	EmitterContext.AddLine(FString::Printf(TEXT("%s = Cast<%s>(%s);"),*ResultObject, *ClassToCastTo, *InputObject));
}

void FBlueprintCompilerCppBackend::EmitDynamicCastStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString TargetClass = TermToText(EmitterContext, Statement.RHS[0]);
	FString ObjectValue = TermToText(EmitterContext, Statement.RHS[1]);
	FString CastedValue = TermToText(EmitterContext, Statement.LHS);
	EmitterContext.AddLine(FString::Printf(TEXT("%s = Cast<%s>(%s);"),*CastedValue, *TargetClass, *ObjectValue));
}

void FBlueprintCompilerCppBackend::EmitMetaCastStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString DesiredClass = TermToText(EmitterContext, Statement.RHS[0]);
	FString SourceClass = TermToText(EmitterContext, Statement.RHS[1]);
	FString Destination = TermToText(EmitterContext, Statement.LHS);
	EmitterContext.AddLine(FString::Printf(TEXT("%s = DynamicMetaCast(%s, %s);"),*Destination, *DesiredClass, *SourceClass));
}

void FBlueprintCompilerCppBackend::EmitObjectToBoolStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString ObjectTarget = TermToText(EmitterContext, Statement.RHS[0]);
	FString DestinationExpression = TermToText(EmitterContext, Statement.LHS);
	EmitterContext.AddLine(FString::Printf(TEXT("%s = (nullptr != %s);"), *DestinationExpression, *ObjectTarget));
}

void FBlueprintCompilerCppBackend::EmitAddMulticastDelegateStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.LHS && Statement.LHS->AssociatedVarProperty);
	FSafeContextScopedEmmitter SafeContextScope(EmitterContext, Statement.LHS->Context, *this);

	const FString Delegate = TermToText(EmitterContext, Statement.LHS, false);
	const FString DelegateToAdd = TermToText(EmitterContext, Statement.RHS[0]);

	EmitterContext.AddLine(FString::Printf(TEXT("%s.Add(%s);"), *Delegate, *DelegateToAdd));
}

void FBlueprintCompilerCppBackend::EmitRemoveMulticastDelegateStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.LHS && Statement.LHS->AssociatedVarProperty);
	FSafeContextScopedEmmitter SafeContextScope(EmitterContext, Statement.LHS->Context, *this);

	const FString Delegate = TermToText(EmitterContext, Statement.LHS, false);
	const FString DelegateToAdd = TermToText(EmitterContext, Statement.RHS[0]);

	EmitterContext.AddLine(FString::Printf(TEXT("%s.Remove(%s);"), *Delegate, *DelegateToAdd));
}

void FBlueprintCompilerCppBackend::EmitBindDelegateStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(2 == Statement.RHS.Num());
	check(Statement.LHS);
	FSafeContextScopedEmmitter SafeContextScope(EmitterContext, Statement.LHS->Context, *this);

	const FString Delegate = TermToText(EmitterContext, Statement.LHS, false);
	const FString NameTerm = TermToText(EmitterContext, Statement.RHS[0]);
	const FString ObjectTerm = TermToText(EmitterContext, Statement.RHS[1]);

	EmitterContext.AddLine(FString::Printf(TEXT("%s.BindUFunction(%s,%s);"), *Delegate, *ObjectTerm, *NameTerm));
}

void FBlueprintCompilerCppBackend::EmitClearMulticastDelegateStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.LHS);
	FSafeContextScopedEmmitter SafeContextScope(EmitterContext, Statement.LHS->Context, *this);

	const FString Delegate = TermToText(EmitterContext, Statement.LHS, false);

	EmitterContext.AddLine(FString::Printf(TEXT("%s.Clear();"), *Delegate));
}

void FBlueprintCompilerCppBackend::EmitCreateArrayStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FBPTerminal* ArrayTerm = Statement.LHS;
	const FString Array = TermToText(EmitterContext, ArrayTerm);

	EmitterContext.AddLine(FString::Printf(TEXT("%s.SetNum(%d, true);"), *Array, Statement.RHS.Num()));

	for (int32 i = 0; i < Statement.RHS.Num(); ++i)
	{
		FBPTerminal* CurrentTerminal = Statement.RHS[i];
		EmitterContext.AddLine(FString::Printf(TEXT("%s[%d] = %s;"), *Array, i, *TermToText(EmitterContext, CurrentTerminal)));
	}
}

void FBlueprintCompilerCppBackend::EmitGotoStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	if (Statement.Type == KCST_ComputedGoto)
	{
		FString NextStateExpression;
		NextStateExpression = TermToText(EmitterContext, Statement.LHS);

		EmitterContext.AddLine(FString::Printf(TEXT("CurrentState = %s;"), *NextStateExpression));
		EmitterContext.AddLine(FString::Printf(TEXT("break;\n")));
	}
	else if ((Statement.Type == KCST_GotoIfNot) || (Statement.Type == KCST_EndOfThreadIfNot) || (Statement.Type == KCST_GotoReturnIfNot))
	{
		FString ConditionExpression;
		ConditionExpression = TermToText(EmitterContext, Statement.LHS);

		EmitterContext.AddLine(FString::Printf(TEXT("if (!%s)"), *ConditionExpression));
		EmitterContext.AddLine(FString::Printf(TEXT("{")));
		EmitterContext.IncreaseIndent();
		if (Statement.Type == KCST_EndOfThreadIfNot)
		{
			ensure(FunctionContext.bUseFlowStack);
			EmitterContext.AddLine(TEXT("CurrentState = (StateStack.Num() > 0) ? StateStack.Pop(/*bAllowShrinking=*/ false) : -1;"));
		}
		else if (Statement.Type == KCST_GotoReturnIfNot)
		{
			EmitterContext.AddLine(TEXT("CurrentState = -1;"));
		}
		else
		{
			EmitterContext.AddLine(FString::Printf(TEXT("CurrentState = %d;"), StatementToStateIndex(FunctionContext, Statement.TargetLabel)));
		}

		EmitterContext.AddLine(FString::Printf(TEXT("break;")));
		EmitterContext.DecreaseIndent();
		EmitterContext.AddLine(FString::Printf(TEXT("}")));
	}
	else if (Statement.Type == KCST_GotoReturn)
	{
		EmitterContext.AddLine(TEXT("CurrentState = -1;"));
		EmitterContext.AddLine(FString::Printf(TEXT("break;")));
	}
	else
	{
		EmitterContext.AddLine(FString::Printf(TEXT("CurrentState = %d;"), StatementToStateIndex(FunctionContext, Statement.TargetLabel)));
		EmitterContext.AddLine(FString::Printf(TEXT("break;")));
	}
}

void FBlueprintCompilerCppBackend::EmitPushStateStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	ensure(FunctionContext.bUseFlowStack);
	EmitterContext.AddLine(FString::Printf(TEXT("StateStack.Push(%d);"), StatementToStateIndex(FunctionContext, Statement.TargetLabel)));
}

void FBlueprintCompilerCppBackend::EmitEndOfThreadStatement(FEmitterLocalContext& EmitterContext, FKismetFunctionContext& FunctionContext)
{
	ensure(FunctionContext.bUseFlowStack);
	EmitterContext.AddLine(TEXT("CurrentState = (StateStack.Num() > 0) ? StateStack.Pop(/*bAllowShrinking=*/ false) : -1;"));
	EmitterContext.AddLine(TEXT("break;"));
}

FString FBlueprintCompilerCppBackend::EmitSwitchValueStatmentInner(FEmitterLocalContext& EmitterContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.RHS.Num() >= 2);
	const int32 TermsBeforeCases = 1;
	const int32 TermsPerCase = 2;
	const int32 NumCases = ((Statement.RHS.Num() - 2) / TermsPerCase);
	auto IndexTerm = Statement.RHS[0];
	auto DefaultValueTerm = Statement.RHS.Last();

	FString Result = FString::Printf(TEXT("TSwitchValue(%s, %s, %d")
		, *TermToText(EmitterContext, IndexTerm) //index
		, *TermToText(EmitterContext, DefaultValueTerm) // default
		, NumCases);

	const uint32 CppTemplateTypeFlags = EPropertyExportCPPFlags::CPPF_CustomTypeName
		| EPropertyExportCPPFlags::CPPF_NoConst | EPropertyExportCPPFlags::CPPF_NoRef
		| EPropertyExportCPPFlags::CPPF_BlueprintCppBackend;

	FStringOutputDevice IndexDeclaration;
	check(IndexTerm && IndexTerm->AssociatedVarProperty);
	IndexTerm->AssociatedVarProperty->ExportCppDeclaration(IndexDeclaration, EExportedDeclaration::Parameter, nullptr, CppTemplateTypeFlags, true);

	check(DefaultValueTerm && DefaultValueTerm->AssociatedVarProperty);
	FStringOutputDevice ValueDeclaration;
	DefaultValueTerm->AssociatedVarProperty->ExportCppDeclaration(ValueDeclaration, EExportedDeclaration::Parameter, nullptr, CppTemplateTypeFlags, true);

	for (int32 TermIndex = TermsBeforeCases; TermIndex < (NumCases * TermsPerCase); TermIndex += TermsPerCase)
	{
		Result += FString::Printf(TEXT(", TSwitchPair<%s, %s>(%s, %s)")
			, *IndexDeclaration
			, *ValueDeclaration
			, *TermToText(EmitterContext, Statement.RHS[TermIndex])
			, *TermToText(EmitterContext, Statement.RHS[TermIndex + 1]));
	}

	Result += TEXT(")");

	return Result;
}

FString FBlueprintCompilerCppBackend::EmitMethodInputParameterList(FEmitterLocalContext& EmitterContext, FBlueprintCompiledStatement& Statement)
{
	FString Result;
	int32 NumParams = 0;
	for (TFieldIterator<UProperty> PropIt(Statement.FunctionToCall); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* FuncParamProperty = *PropIt;

		if (!FuncParamProperty->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			if (NumParams > 0)
			{
				Result += TEXT(", ");
			}

			FString VarName;

			FBPTerminal* Term = Statement.RHS[NumParams];
			check(Term != nullptr);

			if ((Statement.TargetLabel != nullptr) && (Statement.UbergraphCallIndex == NumParams))
			{
				// The target label will only ever be set on a call function when calling into the Ubergraph or
				// on a latent function that will later call into the ubergraph, either of which requires a patchup
				UStructProperty* StructProp = Cast<UStructProperty>(FuncParamProperty);
				if (StructProp && (StructProp->Struct == FLatentActionInfo::StaticStruct()))
				{
					// Latent function info case
					VarName = LatentFunctionInfoTermToText(EmitterContext, Term, Statement.TargetLabel);
				}
				else
				{
					// Ubergraph entry point case
					VarName = FString::FromInt(StateMapPerFunction[0].StatementToStateIndex(Statement.TargetLabel));
				}
			}
			else
			{
				// Emit a normal parameter term
				FString BeginCast;
				FString CloseCast;
				FEdGraphPinType LType;
				auto Schema = GetDefault<UEdGraphSchema_K2>();
				check(Schema);
				if (Schema->ConvertPropertyToPinType(FuncParamProperty, LType))
				{
					FEmitHelper::GenerateAutomaticCast(LType, Term->Type, BeginCast, CloseCast);
				}
				VarName += BeginCast;
				VarName += TermToText(EmitterContext, Term);
				VarName += CloseCast;
			}

			if (FuncParamProperty->HasAnyPropertyFlags(CPF_OutParm))
			{
				Result += TEXT("/*out*/ ");
			}
			Result += *VarName;

			NumParams++;
		}
	}

	return Result;
}

FString FBlueprintCompilerCppBackend::EmitCallStatmentInner(FEmitterLocalContext& EmitterContext, FBlueprintCompiledStatement& Statement, bool bInline)
{
	const bool bCallOnDifferentObject = Statement.FunctionContext && (Statement.FunctionContext->Name != TEXT("self"));
	const bool bStaticCall = Statement.FunctionToCall->HasAnyFunctionFlags(FUNC_Static);
	const bool bUseSafeContext = bCallOnDifferentObject && !bStaticCall;
	const bool bInterfaceCall = bCallOnDifferentObject && Statement.FunctionContext && (UEdGraphSchema_K2::PC_Interface == Statement.FunctionContext->Type.PinCategory);

	FString Result;
	FString CloseCast;
	if (!bInline)
	{
		// Handle the return value of the function being called
		UProperty* FuncToCallReturnProperty = Statement.FunctionToCall->GetReturnProperty();
		if (FuncToCallReturnProperty && ensure(Statement.LHS))
		{
			FString BeginCast;
			FEdGraphPinType RType;
			auto Schema = GetDefault<UEdGraphSchema_K2>();
			check(Schema);
			if (Schema->ConvertPropertyToPinType(FuncToCallReturnProperty, RType))
			{
				FEmitHelper::GenerateAutomaticCast(Statement.LHS->Type, RType, BeginCast, CloseCast);
			}
			Result += FString::Printf(TEXT("%s = %s"), *TermToText(EmitterContext, Statement.LHS), *BeginCast);
		}
	}

	// Emit object to call the method on
	if (bInterfaceCall)
	{
		auto ContextInterfaceClass = CastChecked<UClass>(Statement.FunctionContext->Type.PinSubCategoryObject.Get());
		ensure(ContextInterfaceClass->IsChildOf<UInterface>());
		Result += FString::Printf(TEXT("I%s::Execute_%s(%s.GetObject(), ")
			, *ContextInterfaceClass->GetName()
			, *Statement.FunctionToCall->GetName()
			, *TermToText(EmitterContext, Statement.FunctionContext, false));
	}
	else
	{
		if (bStaticCall)
		{
			const bool bIsCustomThunk = Statement.FunctionToCall->GetBoolMetaData(TEXT("CustomThunk")) 
				|| Statement.FunctionToCall->HasMetaData(TEXT("CustomStructureParam")) 
				|| Statement.FunctionToCall->HasMetaData(TEXT("ArrayParm"));
			auto OwnerClass = Statement.FunctionToCall->GetOuterUClass();
			Result += bIsCustomThunk ? TEXT("FCustomThunkTemplates::") : FString::Printf(TEXT("%s%s::"), OwnerClass->GetPrefixCPP(), *OwnerClass->GetName());
		}
		else if (bCallOnDifferentObject) //@TODO: Badness, could be a self reference wired to another instance!
		{
			Result += FString::Printf(TEXT("%s->"), *TermToText(EmitterContext, Statement.FunctionContext, false));
		}

		if (Statement.bIsParentContext)
		{
			Result += TEXT("Super::");
		}
		Result += Statement.FunctionToCall->GetName();
		if (Statement.bIsParentContext && FEmitHelper::ShouldHandleAsNativeEvent(Statement.FunctionToCall))
		{
			ensure(!bCallOnDifferentObject);
			Result += TEXT("_Implementation");
		}

		// Emit method parameter list
		Result += TEXT("(");
	}
	Result += EmitMethodInputParameterList(EmitterContext, Statement);
	Result += TEXT(")");
	Result += CloseCast;
	if (!bInline)
	{
		Result += TEXT(";");
	}

	return Result;
}

FString FBlueprintCompilerCppBackend::TermToText(FEmitterLocalContext& EmitterContext, const FBPTerminal* Term, bool bUseSafeContext)
{
	const FString PSC_Self(TEXT("self"));
	if (Term->bIsLiteral)
	{
		return FEmitHelper::LiteralTerm(EmitterContext, Term->Type, Term->Name, Term->ObjectLiteral);
	}
	else if (Term->InlineGeneratedParameter)
	{
		if (KCST_SwitchValue == Term->InlineGeneratedParameter->Type)
		{
			return EmitSwitchValueStatmentInner(EmitterContext, *Term->InlineGeneratedParameter);
		}
		else if (KCST_CallFunction == Term->InlineGeneratedParameter->Type)
		{
			return EmitCallStatmentInner(EmitterContext, *Term->InlineGeneratedParameter, true);
		}
		else
		{
			ensureMsgf(false, TEXT("KCST %d is not accepted as inline statement."), Term->InlineGeneratedParameter->Type);
			return FString();
		}
	}
	else
	{
		FString ResultPath(TEXT(""));
		if ((Term->Context != nullptr) && (Term->Context->Name != PSC_Self))
		{
			const bool bFromDefaultValue = Term->Context->IsClassContextType();
			if (bFromDefaultValue)
			{
				const UClass* MinimalClass = ensure(Term->AssociatedVarProperty)
					? Term->AssociatedVarProperty->GetOwnerClass()
					: Cast<UClass>(Term->Context->Type.PinSubCategoryObject.Get());
				if (MinimalClass)
				{
					ResultPath += FString::Printf(TEXT("GetDefaultValueSafe<%s%s>(")
						, MinimalClass->GetPrefixCPP()
						, *MinimalClass->GetName());
				}
				else
				{
					UE_LOG(LogK2Compiler, Error, TEXT("C++ backend cannot find specific class"));
				}
			}

			ResultPath += TermToText(EmitterContext, Term->Context, false);
			ResultPath += bFromDefaultValue ? TEXT(")") : TEXT("");
			if (Term->Context->IsStructContextType())
			{
				ResultPath += TEXT(".");
			}
			else
			{
				ResultPath += TEXT("->");
			}
		}

		FString Conditions;
		if (bUseSafeContext)
		{
			Conditions = FSafeContextScopedEmmitter::ValidationChain(EmitterContext, Term->Context, *this);
		}

		ResultPath += Term->AssociatedVarProperty ? Term->AssociatedVarProperty->GetNameCPP() : Term->Name;
		return Conditions.IsEmpty()
			? ResultPath
			: FString::Printf(TEXT("((%s) ? (%s) : (%s))"), *Conditions, *ResultPath, *FEmitHelper::DefaultValue(EmitterContext, Term->Type));
	}
}

FString FBlueprintCompilerCppBackend::LatentFunctionInfoTermToText(FEmitterLocalContext& EmitterContext, FBPTerminal* Term, FBlueprintCompiledStatement* TargetLabel)
{
	auto LatentInfoStruct = FLatentActionInfo::StaticStruct();

	// Find the term name we need to fixup
	FString FixupTermName;
	for (UProperty* Prop = LatentInfoStruct->PropertyLink; Prop; Prop = Prop->PropertyLinkNext)
	{
		static const FName NeedsLatentFixup(TEXT("NeedsLatentFixup"));
		if (Prop->GetBoolMetaData(NeedsLatentFixup))
		{
			FixupTermName = Prop->GetName();
			break;
		}
	}

	check(!FixupTermName.IsEmpty());

	FString StructValues = Term->Name;

	// Index 0 is always the ubergraph
	const int32 TargetStateIndex = StateMapPerFunction[0].StatementToStateIndex(TargetLabel);
	const int32 LinkageTermStartIdx = StructValues.Find(FixupTermName);
	check(LinkageTermStartIdx != INDEX_NONE);
	StructValues = StructValues.Replace(TEXT("-1"), *FString::FromInt(TargetStateIndex));

	return FEmitHelper::LiteralTerm(EmitterContext, Term->Type, StructValues, nullptr);
}

FString FBlueprintCompilerCppBackend::InnerFunctionImplementation(FKismetFunctionContext& FunctionContext, bool bUseSwitchState)
{
	FEmitterLocalContext EmitterContext;
	EmitterContext.IncreaseIndent();
	if (bUseSwitchState)
	{
		if (FunctionContext.bUseFlowStack)
		{
			EmitterContext.AddLine(TEXT("TArray< int32, TInlineAllocator<8> > StateStack;\n"));
		}

		EmitterContext.AddLine(TEXT("int32 CurrentState = 0;"));
		EmitterContext.AddLine(TEXT("do"));
		EmitterContext.AddLine(TEXT("{"));
		EmitterContext.IncreaseIndent();
		EmitterContext.AddLine(TEXT("switch( CurrentState )"));
		EmitterContext.AddLine(TEXT("{"));
		EmitterContext.AddLine(TEXT("case 0:"));
		EmitterContext.IncreaseIndent();
		EmitterContext.AddLine(TEXT("{"));
		EmitterContext.IncreaseIndent();
	}
	EmitterContext.SetCurrentlyGeneratedClass(FunctionContext.NewClass);

	// Emit code in the order specified by the linear execution list (the first node is always the entry point for the function)
	for (int32 NodeIndex = 0; NodeIndex < FunctionContext.LinearExecutionList.Num(); ++NodeIndex)
	{
		UEdGraphNode* StatementNode = FunctionContext.LinearExecutionList[NodeIndex];
		TArray<FBlueprintCompiledStatement*>* StatementList = FunctionContext.StatementsPerNode.Find(StatementNode);

		if (!StatementList)
		{
			continue;
		}

		for (int32 StatementIndex = 0; StatementIndex < StatementList->Num(); ++StatementIndex)
		{
			FBlueprintCompiledStatement& Statement = *((*StatementList)[StatementIndex]);

			if (Statement.bIsJumpTarget && bUseSwitchState)
			{
				EmitterContext.DecreaseIndent();
				EmitterContext.AddLine(TEXT("}"));
				const int32 StateNum = StatementToStateIndex(FunctionContext, &Statement);
				EmitterContext.DecreaseIndent();
				EmitterContext.AddLine(FString::Printf(TEXT("case %d:"), StateNum));
				EmitterContext.IncreaseIndent();
				EmitterContext.AddLine(TEXT("{"));
				EmitterContext.IncreaseIndent();
			}

			switch (Statement.Type)
			{
			case KCST_Nop:
				EmitterContext.AddLine(TEXT("//No operation."));
				break;
			case KCST_CallFunction:
				EmitCallStatment(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_Assignment:
				EmitAssignmentStatment(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_CompileError:
				UE_LOG(LogK2Compiler, Error, TEXT("C++ backend encountered KCST_CompileError"));
				EmitterContext.AddLine(TEXT("static_assert(false); // KCST_CompileError"));
				break;
			case KCST_PushState:
				EmitPushStateStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_Return:
				UE_LOG(LogK2Compiler, Error, TEXT("C++ backend encountered KCST_Return"));
				EmitterContext.AddLine(TEXT("// Return statement."));
				break;
			case KCST_EndOfThread:
				EmitEndOfThreadStatement(EmitterContext, FunctionContext);
				break;
			case KCST_Comment:
				EmitterContext.AddLine(FString::Printf(TEXT("// %s"), *Statement.Comment));
				break;
			case KCST_DebugSite:
				break;
			case KCST_CastObjToInterface:
				EmitCastObjToInterfaceStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_DynamicCast:
				EmitDynamicCastStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_ObjectToBool:
				EmitObjectToBoolStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_AddMulticastDelegate:
				EmitAddMulticastDelegateStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_ClearMulticastDelegate:
				EmitClearMulticastDelegateStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_WireTraceSite:
				break;
			case KCST_BindDelegate:
				EmitBindDelegateStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_RemoveMulticastDelegate:
				EmitRemoveMulticastDelegateStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_CallDelegate:
				EmitCallDelegateStatment(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_CreateArray:
				EmitCreateArrayStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_CrossInterfaceCast:
				EmitCastBetweenInterfacesStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_MetaCast:
				EmitMetaCastStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_CastInterfaceToObj:
				EmitCastInterfaceToObjStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_ComputedGoto:
			case KCST_UnconditionalGoto:
			case KCST_GotoIfNot:
			case KCST_EndOfThreadIfNot:
			case KCST_GotoReturn:
			case KCST_GotoReturnIfNot:
				EmitGotoStatement(EmitterContext, FunctionContext, Statement);
				break;
			case KCST_SwitchValue:
				// Switch Value should be always an "inline" statement, so there is no point to handle it here
				// case: KCST_AssignmentOnPersistentFrame
			default:
				EmitterContext.AddLine(TEXT("// Warning: Ignoring unsupported statement\n"));
				UE_LOG(LogK2Compiler, Error, TEXT("C++ backend encountered unsupported statement type %d"), (int32)Statement.Type);
				break;
			};
		}
	}

	if (bUseSwitchState)
	{
		EmitterContext.DecreaseIndent();
		// Default error-catching case 
		EmitterContext.AddLine(TEXT("}"));
		EmitterContext.DecreaseIndent();
		EmitterContext.AddLine(TEXT("default:"));
		EmitterContext.IncreaseIndent();
		if (FunctionContext.bUseFlowStack)
		{
			EmitterContext.AddLine(TEXT("check(false); // Invalid state"));
		}
		EmitterContext.AddLine(TEXT("break;"));
		EmitterContext.DecreaseIndent();
		// Close the switch block and do-while loop
		EmitterContext.AddLine(TEXT("}"));
		EmitterContext.DecreaseIndent();
		EmitterContext.AddLine(TEXT("} while( CurrentState != -1 );"));
	}


	return EmitterContext.GetResult();
}
