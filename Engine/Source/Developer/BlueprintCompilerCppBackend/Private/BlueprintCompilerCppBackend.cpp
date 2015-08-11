// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilerCppBackendModulePrivatePCH.h"
#include "BlueprintCompilerCppBackend.h"
#include "BlueprintCompilerCppBackendUtils.h"
#include "EdGraphSchema_K2.h"

FString FBlueprintCompilerCppBackend::TermToText(const FBPTerminal* Term, bool bUseSafeContext)
{
	const FString PSC_Self(TEXT("self"));
	if (Term->bIsLiteral)
	{
		return FEmitHelper::LiteralTerm(Term->Type, Term->Name, Term->ObjectLiteral);
	}
	else if (Term->InlineGeneratedParameter)
	{
		if (KCST_SwitchValue == Term->InlineGeneratedParameter->Type)
		{
			return EmitSwitchValueStatmentInner(*Term->InlineGeneratedParameter);
		}
		else if (KCST_CallFunction == Term->InlineGeneratedParameter->Type)
		{
			return EmitCallStatmentInner(*Term->InlineGeneratedParameter, true);
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
		if ((Term->Context != NULL) && (Term->Context->Name != PSC_Self))
		{
			ResultPath = TermToText(Term->Context, false);

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
			Conditions = FSafeContextScopedEmmitter::ValidationChain(Term->Context, *this);
		}

		ResultPath += Term->AssociatedVarProperty ? Term->AssociatedVarProperty->GetNameCPP() : Term->Name;
		return Conditions.IsEmpty()
			? ResultPath
			: FString::Printf(TEXT("((%s) ? (%s) : (%s))"), *Conditions, *ResultPath, *FEmitHelper::DefaultValue(Term->Type));
	}
}

FString FBlueprintCompilerCppBackend::LatentFunctionInfoTermToText(FBPTerminal* Term, FBlueprintCompiledStatement* TargetLabel)
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

	return FString(TEXT("F")) + LatentInfoStruct->GetName() + StructValues;
}

void FBlueprintCompilerCppBackend::EmitStructProperties(FStringOutputDevice& Target, UStruct* SourceClass)
{
	// Emit class variables
	for (TFieldIterator<UProperty> It(SourceClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		UProperty* Property = *It;
		check(Property);

		Emit(Header, TEXT("\n\tUPROPERTY("));
		{
			TArray<FString> Tags = FEmitHelper::ProperyFlagsToTags(Property->PropertyFlags);
			Tags.Emplace(FEmitHelper::HandleRepNotifyFunc(Property));
			Tags.Emplace(FEmitHelper::HandleMetaData(Property, false));
			Tags.Remove(FString());

			FString AllTags;
			FEmitHelper::ArrayToString(Tags, AllTags, TEXT(", "));
			Emit(Header, *AllTags);
		}
		Emit(Header, TEXT(")\n"));
		Emit(Header, TEXT("\t"));
		Property->ExportCppDeclaration(Target, EExportedDeclaration::Member, NULL, EPropertyExportCPPFlags::CPPF_CustomTypeName | EPropertyExportCPPFlags::CPPF_BlueprintCppBackend);
		Emit(Header, TEXT(";\n"));
	}
}

void FBlueprintCompilerCppBackend::GenerateCodeFromClass(UClass* SourceClass, TIndirectArray<FKismetFunctionContext>& Functions, bool bGenerateStubsOnly)
{
	auto CleanCppClassName = SourceClass->GetName();
	CppClassName = FString(SourceClass->GetPrefixCPP()) + CleanCppClassName;
	
	EmitFileBeginning(CleanCppClassName, SourceClass);

	// MC DELEGATE DECLARATION
	{
		auto DelegateDeclarations = FEmitHelper::EmitMulticastDelegateDeclarations(SourceClass);
		FString AllDeclarations;
		FEmitHelper::ArrayToString(DelegateDeclarations, AllDeclarations, TEXT(";\n"));
		if (DelegateDeclarations.Num())
		{
			Emit(Header, *AllDeclarations);
			Emit(Header, TEXT(";\n"));
		}
	}

	// GATHER ALL SC DELEGATES
	{
		TArray<UDelegateProperty*> Delegates;
		for (TFieldIterator<UDelegateProperty> It(SourceClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			Delegates.Add(*It);
		}

		for (auto& FuncContext : Functions)
		{
			for (TFieldIterator<UDelegateProperty> It(FuncContext.Function, EFieldIteratorFlags::ExcludeSuper); It; ++It)
			{
				Delegates.Add(*It);
			}
		}

		auto DelegateDeclarations = FEmitHelper::EmitSinglecastDelegateDeclarations(Delegates);
		FString AllDeclarations;
		FEmitHelper::ArrayToString(DelegateDeclarations, AllDeclarations, TEXT(";\n"));
		if (DelegateDeclarations.Num())
		{
			Emit(Header, *AllDeclarations);
			Emit(Header, TEXT(";\n"));
		}
	}

	// Class declaration
	const bool bIsInterface = SourceClass->IsChildOf<UInterface>();
	if (bIsInterface)
	{
		Emit(Header, *FString::Printf(TEXT("UINTERFACE(Blueprintable)\nclass U%s : public UInterface\n{\n\tGENERATED_BODY()\n};\n"), *CleanCppClassName));
		Emit(Header, *FString::Printf(TEXT("\nclass I%s"), *CleanCppClassName));
	}
	else
	{
		Emit(Header, TEXT("UCLASS("));
		if (!SourceClass->IsChildOf<UBlueprintFunctionLibrary>())
		{
			Emit(Header, TEXT("Blueprintable, BlueprintType"));
		}
		Emit(Header, TEXT(")\n"));

		UClass* SuperClass = SourceClass->GetSuperClass();
		Emit(Header, *FString::Printf(TEXT("class %s : public %s%s"), *CppClassName, SuperClass->GetPrefixCPP(), *SuperClass->GetName()));

		for (auto& ImplementedInterface : SourceClass->Interfaces)
		{
			if (ImplementedInterface.Class)
			{
				Emit(Header, *FString::Printf(TEXT(", public I%s"), *ImplementedInterface.Class->GetName()));
			}
		}
	}

	// Begin scope
	Emit(Header,TEXT("\n{\npublic:\n\tGENERATED_BODY()\n"));

	EmitStructProperties(Header, SourceClass);

	// Create the state map
	for (int32 i = 0; i < Functions.Num(); ++i)
	{
		StateMapPerFunction.Add(FFunctionLabelInfo());
		FunctionIndexMap.Add(&Functions[i], i);
	}

	// Emit function declarations and definitions (writes to header and body simultaneously)
	if (Functions.Num() > 0)
	{
		Emit(Header, TEXT("\n"));
	}

	if (!bIsInterface)
	{
		Emit(Header, *FString::Printf(TEXT("\t%s(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());\n\n"), *CppClassName));
		Emit(Body, *FEmitDefaultValueHelper::GenerateConstructor(SourceClass));
	}

	for (int32 i = 0; i < Functions.Num(); ++i)
	{
		if (Functions[i].IsValid())
		{
			ConstructFunction(Functions[i], bGenerateStubsOnly);
		}
	}

	Emit(Header, TEXT("};\n\n"));

	Emit(Body, *FEmitHelper::EmitLifetimeReplicatedPropsImpl(SourceClass, CppClassName, TEXT("")));
}

void FBlueprintCompilerCppBackend::EmitCallDelegateStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.FunctionContext && Statement.FunctionContext->AssociatedVarProperty);
	FSafeContextScopedEmmitter SafeContextScope(Body, Statement.FunctionContext->Context, *this, TEXT("\t\t\t"));

	Emit(Body, TEXT("\t\t\t"));
	Emit(Body, *SafeContextScope.GetAdditionalIndent());
	Emit(Body, *FString::Printf(TEXT("%s.Broadcast(%s);"), *TermToText(Statement.FunctionContext, false), *EmitMethodInputParameterList(Statement)));
}

FString FBlueprintCompilerCppBackend::EmitMethodInputParameterList(FBlueprintCompiledStatement& Statement)
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
			ensure(Term != NULL);

			if ((Statement.TargetLabel != NULL) && (Statement.UbergraphCallIndex == NumParams))
			{
				// The target label will only ever be set on a call function when calling into the Ubergraph or
				// on a latent function that will later call into the ubergraph, either of which requires a patchup
				UStructProperty* StructProp = Cast<UStructProperty>(FuncParamProperty);
				if (StructProp && (StructProp->Struct == FLatentActionInfo::StaticStruct()))
				{
					// Latent function info case
					VarName = LatentFunctionInfoTermToText(Term, Statement.TargetLabel);
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
				VarName = TermToText(Term);
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

FString FBlueprintCompilerCppBackend::EmitCallStatmentInner(FBlueprintCompiledStatement& Statement, bool bInline)
{
	FString Result;

	const bool bCallOnDifferentObject = Statement.FunctionContext && (Statement.FunctionContext->Name != TEXT("self"));
	const bool bStaticCall = Statement.FunctionToCall->HasAnyFunctionFlags(FUNC_Static);
	const bool bUseSafeContext = bCallOnDifferentObject && !bStaticCall;
	const bool bInterfaceCall = bCallOnDifferentObject && Statement.FunctionContext && (UEdGraphSchema_K2::PC_Interface == Statement.FunctionContext->Type.PinCategory);
	{
		FSafeContextScopedEmmitter SafeContextScope(Result, bUseSafeContext ? Statement.FunctionContext : nullptr, *this, TEXT("\t\t\t"));
		ensure(!bInline || !SafeContextScope.IsSafeContextUsed());

		bool bCloseCast = false;
		if (!bInline)
		{
			Result += TEXT("\t\t\t");
			Result += SafeContextScope.GetAdditionalIndent();

			// Handle the return value of the function being called
			UProperty* FuncToCallReturnProperty = Statement.FunctionToCall->GetReturnProperty();
			if (FuncToCallReturnProperty && ensure(Statement.LHS))
			{
				FString BeginCast;
				if (auto ObjectProperty = Cast<UObjectProperty>(FuncToCallReturnProperty))
				{
					UClass* LClass = Statement.LHS ? Cast<UClass>(Statement.LHS->Type.PinSubCategoryObject.Get()) : nullptr;
					if (LClass && LClass->IsChildOf(ObjectProperty->PropertyClass) && !ObjectProperty->PropertyClass->IsChildOf(LClass))
					{
						BeginCast = FString::Printf(TEXT("CastChecked<%s%s>("), LClass->GetPrefixCPP(), *LClass->GetName());
						bCloseCast = true;
					}
				}

				Result += FString::Printf(TEXT("%s = %s"), *TermToText(Statement.LHS), *BeginCast);
				
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
				, *TermToText(Statement.FunctionContext, false));
		}
		else
		{
			if (bStaticCall)
			{
				const bool bIsCustomThunk = Statement.FunctionToCall->HasMetaData(TEXT("CustomStructureParam")) || Statement.FunctionToCall->HasMetaData(TEXT("ArrayParm"));
				auto OwnerClass = Statement.FunctionToCall->GetOuterUClass();
				Result += bIsCustomThunk ? TEXT("FCustomThunkTemplates::") : FString::Printf(TEXT("%s%s::"), OwnerClass->GetPrefixCPP(), *OwnerClass->GetName());
			}
			else if (bCallOnDifferentObject) //@TODO: Badness, could be a self reference wired to another instance!
			{
				Result += FString::Printf(TEXT("%s->"), *TermToText(Statement.FunctionContext, false));
			}

			if (Statement.bIsParentContext)
			{
				Result += TEXT("Super::");
			}
			Result += Statement.FunctionToCall->GetName();
			if (Statement.bIsParentContext && FEmitHelper::ShoulsHandleAsNativeEvent(Statement.FunctionToCall))
			{
				ensure(!bCallOnDifferentObject);
				Result += TEXT("_Implementation");
			}

			// Emit method parameter list
			Result += TEXT("(");
		}
		Result += EmitMethodInputParameterList(Statement);
		Result += TEXT(")");

		if (bCloseCast)
		{
			Result += TEXT(", ECastCheckedType::NullAllowed)");
		}

		if (!bInline)
		{
			Result += TEXT(";\n");
		}
	}
	return Result;
}

void FBlueprintCompilerCppBackend::EmitCallStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	Emit(Body, *EmitCallStatmentInner(Statement, false));
}

FString FBlueprintCompilerCppBackend::EmitSwitchValueStatmentInner(FBlueprintCompiledStatement& Statement)
{
	check(Statement.RHS.Num() >= 2);
	const int32 TermsBeforeCases = 1;
	const int32 TermsPerCase = 2;
	const int32 NumCases = ((Statement.RHS.Num() - 2) / TermsPerCase);
	auto IndexTerm = Statement.RHS[0];
	auto DefaultValueTerm = Statement.RHS.Last();

	FString Result = FString::Printf(TEXT("TSwitchValue(%s, %s, %d")
		, *TermToText(IndexTerm) //index
		, *TermToText(DefaultValueTerm) // default
		, NumCases);
	
	const uint32 CppTemplateTypeFlags = EPropertyExportCPPFlags::CPPF_CustomTypeName 
		| EPropertyExportCPPFlags::CPPF_NoConst | EPropertyExportCPPFlags::CPPF_NoRef 
		| EPropertyExportCPPFlags::CPPF_BlueprintCppBackend;

	FStringOutputDevice IndexDeclaration;
	check(IndexTerm && IndexTerm->AssociatedVarProperty);
	IndexTerm->AssociatedVarProperty->ExportCppDeclaration(IndexDeclaration, EExportedDeclaration::Parameter, NULL, CppTemplateTypeFlags, true);

	check(DefaultValueTerm && DefaultValueTerm->AssociatedVarProperty);
	FStringOutputDevice ValueDeclaration;
	DefaultValueTerm->AssociatedVarProperty->ExportCppDeclaration(ValueDeclaration, EExportedDeclaration::Parameter, NULL, CppTemplateTypeFlags, true);

	for (int32 TermIndex = TermsBeforeCases; TermIndex < (NumCases * TermsPerCase); TermIndex += TermsPerCase)
	{
		Result += FString::Printf(TEXT(", TSwitchPair<%s, %s>(%s, %s)")
			, *IndexDeclaration
			, *ValueDeclaration
			, *TermToText(Statement.RHS[TermIndex])
			, *TermToText(Statement.RHS[TermIndex + 1]));
	}

	Result += TEXT(")");

	return Result;
}

void FBlueprintCompilerCppBackend::EmitAssignmentStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.LHS && Statement.RHS[0]);
	FString DestinationExpression = TermToText(Statement.LHS, false);
	FString SourceExpression = TermToText(Statement.RHS[0]);

	FSafeContextScopedEmmitter SafeContextScope(Body, Statement.LHS->Context, *this, TEXT("\t\t\t"));

	FString BeginCast;
	FString EndCast;
	FEmitHelper::GenerateAssignmentCast(Statement.LHS->Type, Statement.RHS[0]->Type, BeginCast, EndCast);
	// Emit the assignment statement
	Emit(Body, TEXT("\t\t\t"));
	Emit(Body, *SafeContextScope.GetAdditionalIndent());
	Emit(Body, *FString::Printf(TEXT("%s = %s%s%s;\n"), *DestinationExpression, *BeginCast, *SourceExpression, *EndCast));
}

void FBlueprintCompilerCppBackend::EmitCastObjToInterfaceStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString InterfaceClass = TermToText(Statement.RHS[0]);
	FString ObjectValue = TermToText(Statement.RHS[1]);
	FString InterfaceValue = TermToText(Statement.LHS);

	Emit(Body, *FString::Printf(TEXT("\t\t\tif ( IsValid(%s) && %s->GetClass()->ImplementsInterface(%s) )\n"), *ObjectValue, *ObjectValue, *InterfaceClass));
	Emit(Body, *FString::Printf(TEXT("\t\t\t{\n")));
	Emit(Body, *FString::Printf(TEXT("\t\t\t\t%s.SetObject(%s);\n"), *InterfaceValue, *ObjectValue));
	Emit(Body, *FString::Printf(TEXT("\t\t\t\tvoid* IAddress = %s->GetInterfaceAddress(%s);\n"), *ObjectValue, *InterfaceClass));
	Emit(Body, *FString::Printf(TEXT("\t\t\t\t%s.SetInterface(IAddress);\n"), *InterfaceValue));
	Emit(Body, *FString::Printf(TEXT("\t\t\t}\n")));
	Emit(Body, *FString::Printf(TEXT("\t\t\telse\n")));
	Emit(Body, *FString::Printf(TEXT("\t\t\t{\n")));
	Emit(Body, *FString::Printf(TEXT("\t\t\t\t%s.SetObject(NULL);\n"), *InterfaceValue));
	Emit(Body, *FString::Printf(TEXT("\t\t\t}\n")));
}

void FBlueprintCompilerCppBackend::EmitCastBetweenInterfacesStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString ClassToCastTo = TermToText(Statement.RHS[0]);
	FString InputInterface = TermToText(Statement.RHS[1]);
	FString ResultInterface = TermToText(Statement.LHS);

	FString InputObject = FString::Printf(TEXT("%s.GetObjectRef()"), *InputInterface);

	Emit(Body, *FString::Printf(TEXT("\t\t\tif ( %s && %s->GetClass()->IsChildOf(%s) )\n"), *InputObject, *InputObject, *ClassToCastTo));
	Emit(Body, *FString::Printf(TEXT("\t\t\t{\n")));
	Emit(Body, *FString::Printf(TEXT("\t\t\t\t%s.SetObject(%s);\n"), *ResultInterface, *InputObject));
	Emit(Body, *FString::Printf(TEXT("\t\t\t\tvoid* IAddress = %s->GetInterfaceAddress(%s);\n"), *InputObject, *ClassToCastTo));
	Emit(Body, *FString::Printf(TEXT("\t\t\t\t%s.SetInterface(IAddress);\n"), *ResultInterface));
	Emit(Body, *FString::Printf(TEXT("\t\t\t}\n")));
	Emit(Body, *FString::Printf(TEXT("\t\t\telse\n")));
	Emit(Body, *FString::Printf(TEXT("\t\t\t{\n")));
	Emit(Body, *FString::Printf(TEXT("\t\t\t\t%s.SetObject(NULL);\n"), *ResultInterface));
	Emit(Body, *FString::Printf(TEXT("\t\t\t}\n")));
}

void FBlueprintCompilerCppBackend::EmitCastInterfaceToObjStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString ClassToCastTo = TermToText(Statement.RHS[0]);
	FString InputInterface = TermToText(Statement.RHS[1]);
	FString ResultObject = TermToText(Statement.LHS);

	FString InputObject = FString::Printf(TEXT("%s.GetObjectRef()"), *InputInterface);

	Emit(Body, *FString::Printf(TEXT("\t\t\t%s = Cast<%s>(%s);\n"),
		*ResultObject, *ClassToCastTo, *InputObject));
}

void FBlueprintCompilerCppBackend::EmitDynamicCastStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString TargetClass = TermToText(Statement.RHS[0]);
	FString ObjectValue = TermToText(Statement.RHS[1]);
	FString CastedValue = TermToText(Statement.LHS);

	Emit(Body, *FString::Printf(TEXT("\t\t\t%s = Cast<%s>(%s);\n"),
		*CastedValue, *TargetClass, *ObjectValue));
}

void FBlueprintCompilerCppBackend::EmitMetaCastStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString DesiredClass = TermToText(Statement.RHS[0]);
	FString SourceClass = TermToText(Statement.RHS[1]);
	FString Destination = TermToText(Statement.LHS);

	Emit(Body, *FString::Printf(TEXT("\t\t\t%s = DynamicMetaCast(%s, %s);\n"),
		*Destination, *DesiredClass, *SourceClass));
}

void FBlueprintCompilerCppBackend::EmitObjectToBoolStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString ObjectTarget = TermToText(Statement.RHS[0]);
	FString DestinationExpression = TermToText(Statement.LHS);

	Emit(Body, *FString::Printf(TEXT("\t\t\t%s = (%s != NULL);\n"), *DestinationExpression, *ObjectTarget));
}

void FBlueprintCompilerCppBackend::EmitAddMulticastDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.LHS && Statement.LHS->AssociatedVarProperty);
	FSafeContextScopedEmmitter SafeContextScope(Body, Statement.LHS->Context, *this, TEXT("\t\t\t"));

	Emit(Body, TEXT("\t\t\t"));
	Emit(Body, *SafeContextScope.GetAdditionalIndent());

	const FString Delegate = TermToText(Statement.LHS, false);
	const FString DelegateToAdd = TermToText(Statement.RHS[0]);

	Emit(Body, *FString::Printf(TEXT("%s.Add(%s);\n"), *Delegate, *DelegateToAdd));
}

void FBlueprintCompilerCppBackend::EmitRemoveMulticastDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.LHS && Statement.LHS->AssociatedVarProperty);
	FSafeContextScopedEmmitter SafeContextScope(Body, Statement.LHS->Context, *this, TEXT("\t\t\t"));

	Emit(Body, TEXT("\t\t\t"));
	Emit(Body, *SafeContextScope.GetAdditionalIndent());

	const FString Delegate = TermToText(Statement.LHS, false);
	const FString DelegateToAdd = TermToText(Statement.RHS[0]);

	Emit(Body, *FString::Printf(TEXT("%s.Remove(%s);\n"), *Delegate, *DelegateToAdd));
}

void FBlueprintCompilerCppBackend::EmitBindDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(2 == Statement.RHS.Num());
	check(Statement.LHS);
	FSafeContextScopedEmmitter SafeContextScope(Body, Statement.LHS->Context, *this, TEXT("\t\t\t"));

	Emit(Body, TEXT("\t\t\t"));
	Emit(Body, *SafeContextScope.GetAdditionalIndent());


	const FString Delegate = TermToText(Statement.LHS, false);
	const FString NameTerm = TermToText(Statement.RHS[0]);
	const FString ObjectTerm = TermToText(Statement.RHS[1]);

	Emit(Body, *FString::Printf(TEXT("%s.BindUFunction(%s,%s);\n"), *Delegate, *ObjectTerm, *NameTerm));
}

void FBlueprintCompilerCppBackend::EmitClearMulticastDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(Statement.LHS);
	FSafeContextScopedEmmitter SafeContextScope(Body, Statement.LHS->Context, *this, TEXT("\t\t\t"));

	Emit(Body, TEXT("\t\t\t"));
	Emit(Body, *SafeContextScope.GetAdditionalIndent());

	const FString Delegate = TermToText(Statement.LHS, false);

	Emit(Body, *FString::Printf(TEXT("%s.Clear();\n"), *Delegate));
}

void FBlueprintCompilerCppBackend::EmitCreateArrayStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FBPTerminal* ArrayTerm = Statement.LHS;
	const FString Array = TermToText(ArrayTerm);

	UArrayProperty* ArrayProperty = CastChecked<UArrayProperty>(ArrayTerm->AssociatedVarProperty);
	UProperty* InnerProperty = ArrayProperty->Inner;

	for (int32 i = 0; i < Statement.RHS.Num(); ++i)
	{
		FBPTerminal* CurrentTerminal = Statement.RHS[i];
		Emit(Body,*FString::Printf(TEXT("\t\t\t%s[%d] = %s;"), *Array, i, *TermToText(CurrentTerminal)));
	}
}

void FBlueprintCompilerCppBackend::EmitGotoStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	if (Statement.Type == KCST_ComputedGoto)
	{
		FString NextStateExpression;
		NextStateExpression = TermToText(Statement.LHS);

		Emit(Body, *FString::Printf(TEXT("\t\t\tCurrentState = %s;\n"), *NextStateExpression));
		Emit(Body, *FString::Printf(TEXT("\t\t\tbreak;\n")));
	}
	else if ((Statement.Type == KCST_GotoIfNot) || (Statement.Type == KCST_EndOfThreadIfNot) || (Statement.Type == KCST_GotoReturnIfNot))
	{
		FString ConditionExpression;
		ConditionExpression = TermToText(Statement.LHS);

		Emit(Body, *FString::Printf(TEXT("\t\t\tif (!%s)\n"), *ConditionExpression));
		Emit(Body, *FString::Printf(TEXT("\t\t\t{\n")));

		if (Statement.Type == KCST_EndOfThreadIfNot)
		{
			ensure(FunctionContext.bUseFlowStack);
			Emit(Body, TEXT("\t\t\t\tCurrentState = (StateStack.Num() > 0) ? StateStack.Pop(/*bAllowShrinking=*/ false) : -1;\n"));
		}
		else if (Statement.Type == KCST_GotoReturnIfNot)
		{
			Emit(Body, TEXT("\t\t\t\tCurrentState = -1;\n"));
		}
		else
		{
			Emit(Body, *FString::Printf(TEXT("\t\t\t\tCurrentState = %d;\n"), StatementToStateIndex(FunctionContext, Statement.TargetLabel)));
		}

		Emit(Body, *FString::Printf(TEXT("\t\t\t\tbreak;\n")));
		Emit(Body, *FString::Printf(TEXT("\t\t\t}\n")));
	}
	else if (Statement.Type == KCST_GotoReturn)
	{
		Emit(Body, TEXT("\t\t\tCurrentState = -1;\n"));
		Emit(Body, *FString::Printf(TEXT("\t\t\tbreak;\n")));
	}
	else
	{
		Emit(Body, *FString::Printf(TEXT("\t\t\tCurrentState = %d;\n"), StatementToStateIndex(FunctionContext, Statement.TargetLabel)));
		Emit(Body, *FString::Printf(TEXT("\t\t\tbreak;\n")));
	}
}

void FBlueprintCompilerCppBackend::EmitPushStateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	ensure(FunctionContext.bUseFlowStack);
	Emit(Body, *FString::Printf(TEXT("\t\t\tStateStack.Push(%d);\n"), StatementToStateIndex(FunctionContext, Statement.TargetLabel)));
}

void FBlueprintCompilerCppBackend::EmitEndOfThreadStatement(FKismetFunctionContext& FunctionContext, const FString& ReturnValueString)
{
	ensure(FunctionContext.bUseFlowStack);
	Emit(Body, TEXT("\t\t\tCurrentState = (StateStack.Num() > 0) ? StateStack.Pop(/*bAllowShrinking=*/ false) : -1;\n"));
	Emit(Body, TEXT("\t\t\tbreak;\n"));
}

void FBlueprintCompilerCppBackend::EmitReturnStatement(FKismetFunctionContext& FunctionContext, const FString& ReturnValueString)
{
	Emit(Body, *FString::Printf(TEXT("\treturn%s;\n"), *ReturnValueString));
}

void FBlueprintCompilerCppBackend::DeclareLocalVariables(FKismetFunctionContext& FunctionContext, TArray<UProperty*>& LocalVariables)
{
	for (int32 i = 0; i < LocalVariables.Num(); ++i)
	{
		UProperty* LocalVariable = LocalVariables[i];

		Emit(Body, TEXT("\t"));
		LocalVariable->ExportCppDeclaration(Body, EExportedDeclaration::Local, NULL, EPropertyExportCPPFlags::CPPF_CustomTypeName | EPropertyExportCPPFlags::CPPF_BlueprintCppBackend);
		Emit(Body, TEXT("{};\n"));
	}

	if (LocalVariables.Num() > 0)
	{
		Emit(Body, TEXT("\n"));
	}
}

void FBlueprintCompilerCppBackend::DeclareStateSwitch(FKismetFunctionContext& FunctionContext)
{
	if (FunctionContext.bUseFlowStack)
	{
		Emit(Body, TEXT("\tTArray< int32, TInlineAllocator<8> > StateStack;\n"));
	}
	Emit(Body, TEXT("\tint32 CurrentState = 0;\n"));
	Emit(Body, TEXT("\tdo\n"));
	Emit(Body, TEXT("\t{\n"));
	Emit(Body, TEXT("\t\tswitch( CurrentState )\n"));
	Emit(Body, TEXT("\t\t{\n"));
	Emit(Body, TEXT("\t\tcase 0:\n"));
}

void FBlueprintCompilerCppBackend::CloseStateSwitch(FKismetFunctionContext& FunctionContext)
{
	// Default error-catching case 
	Emit(Body, TEXT("\t\tdefault:\n"));
	if (FunctionContext.bUseFlowStack)
	{
		Emit(Body, TEXT("\t\t\tcheck(false); // Invalid state\n"));
	}
	Emit(Body, TEXT("\t\t\tbreak;\n"));

	// Close the switch block and do-while loop
	Emit(Body, TEXT("\t\t}\n"));
	Emit(Body, TEXT("\t} while( CurrentState != -1 );\n"));
}

void FBlueprintCompilerCppBackend::ConstructFunction(FKismetFunctionContext& FunctionContext, bool bGenerateStubOnly)
{
	if (FunctionContext.IsDelegateSignature())
	{
		return;
	}

	UFunction* Function = FunctionContext.Function;

	UProperty* ReturnValue = NULL;
	TArray<UProperty*> LocalVariables;

	{
		FString FunctionName;
		Function->GetName(FunctionName);

		TArray<UProperty*> ArgumentList;

		// Split the function property list into arguments, a return value (if any), and local variable declarations
		for (TFieldIterator<UProperty> It(Function); It; ++It)
		{
			UProperty* Property = *It;
			if (Property->HasAnyPropertyFlags(CPF_Parm))
			{
				if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					if (ReturnValue == NULL)
					{
						ReturnValue = Property;
						LocalVariables.Add(Property);
					}
					else
					{
						UE_LOG(LogK2Compiler, Error, TEXT("Function %s from graph @@ has more than one return value (named %s and %s)"),
							*FunctionName, *GetPathNameSafe(FunctionContext.SourceGraph), *ReturnValue->GetName(), *Property->GetName());
					}
				}
				else
				{
					ArgumentList.Add(Property);
				}
			}
			else
			{
				LocalVariables.Add(Property);
			}
		}

		bool bAddCppFromBpEventMD = false;
		bool bGenerateAsNativeEventImplementation = false;
		// Get original function declaration
		if (FEmitHelper::ShoulsHandleAsNativeEvent(Function)) // BlueprintNativeEvent
		{
			bGenerateAsNativeEventImplementation = true;
			FunctionName += TEXT("_Implementation");
		}
		else if (FEmitHelper::ShoulsHandleAsImplementableEvent(Function)) // BlueprintImplementableEvent
		{
			bAddCppFromBpEventMD = true;
		}

		// Emit the declaration
		const FString ReturnType = ReturnValue ? ReturnValue->GetCPPType(NULL, EPropertyExportCPPFlags::CPPF_CustomTypeName) : TEXT("void");

		//@TODO: Make the header+body export more uniform
		{
			const FString Start = FString::Printf(TEXT("%s %s%s%s("), *ReturnType, TEXT("%s"), TEXT("%s"), *FunctionName);

			TArray<FString> AdditionalMetaData;
			if (bAddCppFromBpEventMD)
			{
				AdditionalMetaData.Emplace(TEXT("CppFromBpEvent"));
			}

			if (!bGenerateAsNativeEventImplementation)
			{
				Emit(Header, *FString::Printf(TEXT("\t%s\n"), *FEmitHelper::EmitUFuntion(Function, &AdditionalMetaData)));
			}
			Emit(Header, TEXT("\t"));

			if (Function->HasAllFunctionFlags(FUNC_Static))
			{
				Emit(Header, TEXT("static "));
			}

			if (bGenerateAsNativeEventImplementation)
			{
				Emit(Header, TEXT("virtual "));
			}

			Emit(Header, *FString::Printf(*Start, TEXT(""), TEXT("")));
			Emit(Body, *FString::Printf(*Start, *CppClassName, TEXT("::")));

			for (int32 i = 0; i < ArgumentList.Num(); ++i)
			{
				UProperty* ArgProperty = ArgumentList[i];

				if (i > 0)
				{
					Emit(Header, TEXT(", "));
					Emit(Body, TEXT(", "));
				}

				if (ArgProperty->HasAnyPropertyFlags(CPF_OutParm))
				{
					Emit(Header, TEXT("/*out*/ "));
					Emit(Body, TEXT("/*out*/ "));
				}

				ArgProperty->ExportCppDeclaration(Header, EExportedDeclaration::Parameter, NULL, EPropertyExportCPPFlags::CPPF_CustomTypeName | EPropertyExportCPPFlags::CPPF_BlueprintCppBackend);
				ArgProperty->ExportCppDeclaration(Body, EExportedDeclaration::Parameter, NULL, EPropertyExportCPPFlags::CPPF_CustomTypeName | EPropertyExportCPPFlags::CPPF_BlueprintCppBackend);
			}

			Emit(Header, TEXT(")"));
			if (bGenerateAsNativeEventImplementation)
			{
				Emit(Header, TEXT(" override"));
			}
			Emit(Header, TEXT(";\n"));
			Emit(Body, TEXT(")\n"));
		}

		// Start the body of the implementation
		Emit(Body, TEXT("{\n"));
	}

	const FString ReturnValueString = ReturnValue ? (FString(TEXT(" ")) + ReturnValue->GetName()) : TEXT("");

	if (!bGenerateStubOnly)
	{
		// Emit local variables
		DeclareLocalVariables(FunctionContext, LocalVariables);

		bool bUseSwitchState = false;
		for (auto Node : FunctionContext.LinearExecutionList)
		{
			TArray<FBlueprintCompiledStatement*>* StatementList = FunctionContext.StatementsPerNode.Find(Node);
			if (StatementList)
			{
				for (auto Statement : (*StatementList))
				{
					if (Statement && (
						Statement->Type == KCST_UnconditionalGoto ||
						Statement->Type == KCST_PushState ||
						Statement->Type == KCST_GotoIfNot ||
						Statement->Type == KCST_ComputedGoto ||
						Statement->Type == KCST_EndOfThread ||
						Statement->Type == KCST_EndOfThreadIfNot ||
						Statement->Type == KCST_GotoReturn ||
						Statement->Type == KCST_GotoReturnIfNot))
					{
						bUseSwitchState = true;
						break;
					}
				}
			}
			if (bUseSwitchState)
			{
				break;
			}
		}

		if (bUseSwitchState)
		{
			DeclareStateSwitch(FunctionContext);
		}


		// Run thru code looking only at things marked as jump targets, to make sure the jump targets are ordered in order of appearance in the linear execution list
		// Emit code in the order specified by the linear execution list (the first node is always the entry point for the function)
		for (int32 NodeIndex = 0; NodeIndex < FunctionContext.LinearExecutionList.Num(); ++NodeIndex)
		{
			UEdGraphNode* StatementNode = FunctionContext.LinearExecutionList[NodeIndex];
			TArray<FBlueprintCompiledStatement*>* StatementList = FunctionContext.StatementsPerNode.Find(StatementNode);

			if (StatementList != NULL)
			{
				for (int32 StatementIndex = 0; StatementIndex < StatementList->Num(); ++StatementIndex)
				{
					FBlueprintCompiledStatement& Statement = *((*StatementList)[StatementIndex]);

					if (Statement.bIsJumpTarget)
					{
						// Just making sure we number them in order of appearance, so jump statements don't influence the order
						const int32 StateNum = StatementToStateIndex(FunctionContext, &Statement);
					}
				}
			}
		}

		// Emit code in the order specified by the linear execution list (the first node is always the entry point for the function)
		for (int32 NodeIndex = 0; NodeIndex < FunctionContext.LinearExecutionList.Num(); ++NodeIndex)
		{
			UEdGraphNode* StatementNode = FunctionContext.LinearExecutionList[NodeIndex];
			TArray<FBlueprintCompiledStatement*>* StatementList = FunctionContext.StatementsPerNode.Find(StatementNode);

			if (StatementList != NULL)
			{
				for (int32 StatementIndex = 0; StatementIndex < StatementList->Num(); ++StatementIndex)
				{
					FBlueprintCompiledStatement& Statement = *((*StatementList)[StatementIndex]);

					if (Statement.bIsJumpTarget && bUseSwitchState)
					{
						const int32 StateNum = StatementToStateIndex(FunctionContext, &Statement);
						Emit(Body, *FString::Printf(TEXT("\n\t\tcase %d:\n"), StateNum));
					}

					switch (Statement.Type)
					{
					case KCST_Nop:
						Emit(Body, TEXT("\t\t\t//No operation.\n"));
						break;
					case KCST_CallFunction:
						EmitCallStatment(FunctionContext, Statement);
						break;
					case KCST_Assignment:
						EmitAssignmentStatment(FunctionContext, Statement);
						break;
					case KCST_CompileError:
						UE_LOG(LogK2Compiler, Error, TEXT("C++ backend encountered KCST_CompileError"));
						Emit(Body, TEXT("\t\t\tstatic_assert(false); // KCST_CompileError"));
						break;
					case KCST_PushState:
						EmitPushStateStatement(FunctionContext, Statement);
						break;
					case KCST_Return:
						Emit(Body, TEXT("\t\t\t// Return statement.\n"));
						break;
					case KCST_EndOfThread:
						EmitEndOfThreadStatement(FunctionContext, ReturnValueString);
						break;
					case KCST_Comment:
						Emit(Body, *FString::Printf(TEXT("\t\t\t// %s\n"), *Statement.Comment));
						break;
					case KCST_DebugSite:
						Emit(Body, TEXT("\t\t\t// Debug site.\n"));
						break;
					case KCST_CastObjToInterface:
						EmitCastObjToInterfaceStatement(FunctionContext, Statement);
						break;
					case KCST_DynamicCast:
						EmitDynamicCastStatement(FunctionContext, Statement);
						break;
					case KCST_ObjectToBool:
						EmitObjectToBoolStatement(FunctionContext, Statement);
						break;
					case KCST_AddMulticastDelegate:
						EmitAddMulticastDelegateStatement(FunctionContext, Statement);
						break;
					case KCST_ClearMulticastDelegate:
						EmitClearMulticastDelegateStatement(FunctionContext, Statement);
						break;
					case KCST_WireTraceSite:
						Emit(Body, TEXT("\t\t\t// Wire debug site.\n"));
						break;
					case KCST_BindDelegate:
						EmitBindDelegateStatement(FunctionContext, Statement);
						break;
					case KCST_RemoveMulticastDelegate:
						EmitRemoveMulticastDelegateStatement(FunctionContext, Statement);
						break;
					case KCST_CallDelegate:
						EmitCallDelegateStatment(FunctionContext, Statement);
						break;
					case KCST_CreateArray:
						EmitCreateArrayStatement(FunctionContext, Statement);
						break;
					case KCST_CrossInterfaceCast:
						EmitCastBetweenInterfacesStatement(FunctionContext, Statement);
						break;
					case KCST_MetaCast:
						EmitMetaCastStatement(FunctionContext, Statement);
						break;
					case KCST_CastInterfaceToObj:
						EmitCastInterfaceToObjStatement(FunctionContext, Statement);
						break;
					case KCST_ComputedGoto:
					case KCST_UnconditionalGoto:
					case KCST_GotoIfNot:
					case KCST_EndOfThreadIfNot:
					case KCST_GotoReturn:
					case KCST_GotoReturnIfNot:
						EmitGotoStatement(FunctionContext, Statement);
						break;
					case KCST_SwitchValue:
						// Switch Value should be always an "inline" statement, so there is no point to handle it here
						// case: KCST_AssignmentOnPersistentFrame
					default:
						Emit(Body, TEXT("\t// Warning: Ignoring unsupported statement\n"));
						UE_LOG(LogK2Compiler, Error, TEXT("C++ backend encountered unsupported statement type %d"), (int32)Statement.Type);
						break;
					};
				}
			}
		}

		if (bUseSwitchState)
		{
			CloseStateSwitch(FunctionContext);
		}
	}

	EmitReturnStatement(FunctionContext, ReturnValueString);

	Emit(Body, TEXT("}\n\n"));
}

void FBlueprintCompilerCppBackend::GenerateCodeFromEnum(UUserDefinedEnum* SourceEnum)
{
	check(SourceEnum);
	const FString Name = SourceEnum->GetName();
	EmitFileBeginning(Name, nullptr);

	Emit(Header, TEXT("UENUM(BlueprintType)\nenum class "));
	Emit(Header, *Name);
	Emit(Header, TEXT(" : uint8\n{"));

	for (int32 Index = 0; Index < SourceEnum->NumEnums(); ++Index)
	{
		const FString ElemName = SourceEnum->GetEnumName(Index);
		const int32 ElemValue = Index;

		const FString& DisplayNameMD = SourceEnum->GetMetaData(TEXT("DisplayName"), ElemValue);// TODO: value or index?
		const FString Meta = DisplayNameMD.IsEmpty() ? FString() : FString::Printf(TEXT("UMETA(DisplayName = \"%s\")"), *DisplayNameMD);
		Emit(Header, *FString::Printf(TEXT("\n\t%s = %d %s,"), *ElemName, ElemValue, *Meta));
	}
	Emit(Header, TEXT("\n};\n"));
}

void FBlueprintCompilerCppBackend::GenerateCodeFromStruct(UUserDefinedStruct* SourceStruct)
{
	check(SourceStruct);

	EmitFileBeginning(SourceStruct->GetName(), SourceStruct);

	const FString NewName = FString(TEXT("F")) + SourceStruct->GetName();
	Emit(Header, TEXT("USTRUCT(BlueprintType)\n"));

	Emit(Header, *FString::Printf(TEXT("struct %s\n{\npublic:\n\tGENERATED_BODY()\n"), *NewName));
	EmitStructProperties(Header, SourceStruct);
	Emit(Header, *FEmitDefaultValueHelper::GenerateGetDefaultValue(SourceStruct));
	Emit(Header, TEXT("};\n"));
}

void FBlueprintCompilerCppBackend::EmitFileBeginning(const FString& CleanName, UStruct* SourceStruct)
{
	Emit(Header, TEXT("#pragma once\n\n"));
	if (SourceStruct)
	{ 
		{
			TArray<FString> PersistentHeaders;
			PersistentHeaders.Add(FString(FApp::GetGameName()) + TEXT(".h"));
			PersistentHeaders.Add(CleanName + TEXT(".h"));
			PersistentHeaders.Add(TEXT("GeneratedCodeHelpers.h"));
			Emit(Body, *FEmitHelper::GatherNativeHeadersToInclude(SourceStruct, PersistentHeaders));
		}

		// find objects referenced by functions/script
		TArray<UObject*> IncludeInHeader;
		TArray<UObject*> IncludeInBody;
		{
			FReferenceFinder HeaderReferenceFinder(IncludeInHeader, NULL, false, false, true, false);
			FReferenceFinder BodyReferenceFinder(IncludeInBody, NULL, false, false, true, false);
			{
				TArray<UObject*> ObjectsToCheck;
				GetObjectsWithOuter(SourceStruct, ObjectsToCheck, true);
				for (auto Obj : ObjectsToCheck)
				{
					auto Property = Cast<UProperty>(Obj);
					auto OwnerProperty = IsValid(Property) ? Property->GetOwnerProperty() : nullptr;
					const bool bIsParam = OwnerProperty && (0 != (OwnerProperty->PropertyFlags & CPF_Parm));
					const bool bIsMemberVariable = OwnerProperty && (OwnerProperty->GetOuter() == SourceStruct);
					if (bIsParam || bIsMemberVariable)
					{
						HeaderReferenceFinder.FindReferences(Obj);
					}
					else
					{
						BodyReferenceFinder.FindReferences(Obj);
					}
				}
			}

			if (auto SuperStruct = SourceStruct->GetSuperStruct())
			{
				IncludeInHeader.AddUnique(SuperStruct);
			}

			if (auto SourceClass = Cast<UClass>(SourceStruct))
			{
				for (auto& ImplementedInterface : SourceClass->Interfaces)
				{
					IncludeInHeader.AddUnique(ImplementedInterface.Class);
				}
			}
		}

		TSet<FString> AlreadyIncluded;
		AlreadyIncluded.Add(SourceStruct->GetName());

		auto EmitInner = [&](FStringOutputDevice& Dst, const TArray<UObject*>& Src)
		{
			for (UObject* Obj : Src)
			{
				bool bWantedType = Obj && (Obj->IsA<UBlueprintGeneratedClass>() || Obj->IsA<UUserDefinedEnum>() || Obj->IsA<UUserDefinedStruct>());
				if (!bWantedType)
				{
					for (UObject* OuterObj = (Obj ? Obj->GetOuter() : nullptr); nullptr != OuterObj; OuterObj = OuterObj->GetOuter())
					{
						if (OuterObj->IsA<UBlueprintGeneratedClass>() || OuterObj->IsA<UUserDefinedStruct>())
						{
							Obj = OuterObj;
							bWantedType = true;
							break;
						}
					}
				}

				if (!bWantedType)
				{
					Obj = Obj ? Cast<UBlueprintGeneratedClass>(Obj->GetClass()) : nullptr;
					bWantedType = (nullptr != Obj);
				}

				if (bWantedType && Obj)
				{
					const FString Name = Obj->GetName();
					if (!AlreadyIncluded.Contains(Name))
					{
						AlreadyIncluded.Add(Name);
						Emit(Dst, *FString::Printf(TEXT("#include \"%s.h\"\n"), *Name));
					}
				}
			}
			Emit(Dst, TEXT("\n"));
		};

		EmitInner(Header, IncludeInHeader);
		EmitInner(Body, IncludeInBody);
	}
	Emit(Header, *FString::Printf(TEXT("#include \"%s.generated.h\"\n\n"), *CleanName));
}
