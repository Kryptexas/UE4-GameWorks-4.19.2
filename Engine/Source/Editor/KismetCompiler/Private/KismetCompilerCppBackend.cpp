// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	KismetCompilerBackend.cpp
=============================================================================*/

#include "KismetCompilerPrivatePCH.h"

#include "KismetCompilerBackend.h"

#include "DefaultValueHelper.h"

//////////////////////////////////////////////////////////////////////////
// FKismetCppBackend

FString FKismetCppBackend::TermToText(FBPTerminal* Term, UProperty* CoerceProperty)
{
	if (Term->bIsLiteral)
	{
		//@TODO: Must have a coercion type if it's a literal, because the symbol table isn't plumbed in here and the literals don't carry type information either, yay!
		check(CoerceProperty);

		if (CoerceProperty->IsA(UStrProperty::StaticClass()))
		{
			return FString::Printf(TEXT("TEXT(\"%s\")"), *(Term->Name));
		}
		else if (CoerceProperty->IsA(UFloatProperty::StaticClass()))
		{
			float Value = FCString::Atof(*(Term->Name));
			return FString::Printf(TEXT("%f"), Value);
		}
		else if (CoerceProperty->IsA(UIntProperty::StaticClass()))
		{
			int32 Value = FCString::Atoi(*(Term->Name));
			return FString::Printf(TEXT("%d"), Value);
		}
		else if (UByteProperty* ByteProperty = Cast<UByteProperty>(CoerceProperty))
		{
			// The PinSubCategoryObject check is to allow enum literals communicate with byte properties as literals
			if (ByteProperty->Enum != NULL ||
				(NULL != Cast<UEnum>(Term->Type.PinSubCategoryObject.Get())))
			{
				return Term->Name;
			}
			else
			{
				uint8 Value = FCString::Atoi(*(Term->Name));
				return FString::Printf(TEXT("%u"), Value);
			}
		}
		else if (UBoolProperty* BoolProperty = Cast<UBoolProperty>(CoerceProperty))
		{
			bool bValue = Term->Name.ToBool();
			return *(FName::GetEntry(bValue ? NAME_TRUE : NAME_FALSE)->GetPlainNameString());
		}
		else if (UNameProperty* NameProperty = Cast<UNameProperty>(CoerceProperty))
		{
			FName LiteralName(*(Term->Name));
			return FString::Printf(TEXT("FName(TEXT(\"%s\")"), *(LiteralName.ToString()));
		}
		else if (UStructProperty* StructProperty = Cast<UStructProperty>(CoerceProperty))
		{
			if (StructProperty->Struct == VectorStruct)
			{
				FVector Vect = FVector::ZeroVector;
				FDefaultValueHelper::ParseVector(Term->Name, /*out*/ Vect);

				return FString::Printf(TEXT("FVector(%f,%f,%f)"), Vect.X, Vect.Y, Vect.Z);
			}
			else if (StructProperty->Struct == RotatorStruct)
			{
				FRotator Rot = FRotator::ZeroRotator;
				FDefaultValueHelper::ParseRotator(Term->Name, /*out*/ Rot);

				return FString::Printf(TEXT("FRotator(%f,%f,%f)"), Rot.Pitch, Rot.Yaw, Rot.Roll);
			}
			else if (StructProperty->Struct == TransformStruct)
			{
				FTransform Trans = FTransform::Identity;
				Trans.InitFromString( Term->Name );

				const FQuat Rot = Trans.GetRotation();
				const FVector Translation = Trans.GetTranslation();
				const FVector Scale = Trans.GetScale3D();

				return FString::Printf(TEXT("FTransform( FQuat(%f,%f,%f,%f), FVector(%f,%f,%f), FVector(%f,%f,%f) )"),
					Rot.X, Rot.Y, Rot.Z, Rot.W, Translation.X, Translation.Y, Translation.Z, Scale.X, Scale.Y, Scale.Z);
			}
			else
			{
				//@todo:  This needs to be more robust, since import text isn't really proper for struct construction.
				return FString(TEXT("F")) + StructProperty->Struct->GetName() + Term->Name;
			}
		}
		else if (UClassProperty* ClassProperty = Cast<UClassProperty>(CoerceProperty))
		{
			return FString::Printf(TEXT("%s::StaticClass()"), *(Term->Name)); //@TODO: Need the correct class prefix
		}
		else if (CoerceProperty->IsA(UDelegateProperty::StaticClass()))
		{
			//@TODO: K2 Delegate Support: Won't compile, there isn't an operator= that does what we want here, it should be a proper call to BindDynamic instead!
			if (Term->Name == TEXT(""))
			{
				return TEXT("NULL");
			}
			else
			{
				return FString::Printf(TEXT("BindDynamic(this, &%s::%s)"), *CppClassName, *(Term->Name));
			}
		}
		else if (CoerceProperty->IsA(UObjectPropertyBase::StaticClass()))
		{
			if (Term->Type.PinSubCategory == Schema->PSC_Self)
			{
				return TEXT("this");
			}
			else if (Term->ObjectLiteral)
			{
				if (UClass* LiteralClass = Cast<UClass>(Term->ObjectLiteral))
				{
					return FString::Printf(TEXT("%s::StaticClass()"), *LiteralClass->GetName()); //@TODO: Need the correct class prefix
				}
				else
				{
					return FString::Printf(TEXT("FindObject<UObject>(ANY_PACKAGE, TEXT(\"%s\"))"), *(Term->ObjectLiteral->GetPathName()));
				}
			}
			else
			{
				return FString(TEXT("NULL"));
			}
		}
		else if (CoerceProperty->IsA(UInterfaceProperty::StaticClass()))
		{
			if (Term->Type.PinSubCategory == Schema->PSC_Self)
			{
				return TEXT("this");
			}
			else 
			{
				ensureMsg(false, TEXT("It is not possible to express this interface property as a literal value!"));
				return Term->Name;
			}
		}
		else
		// else if (CoerceProperty->IsA(UMulticastDelegateProperty::StaticClass()))
		// Cannot assign a literal to a multicast delegate; it should be added instead of assigned
		{
			ensureMsg(false, TEXT("It is not possible to express this type as a literal value!"));
			return Term->Name;
		}
	}
	else
	{
		FString Prefix(TEXT(""));
		if (Term->Context != NULL)
		{
			//@TODO: Badness, could be a self reference wired to another instance!
			check(Term->Context->Name != Schema->PSC_Self);
			Prefix = TermToText(Term->Context);

			if (Term->Context->bIsStructContext)
			{
				Prefix += TEXT(".");
			}
			else
			{
				Prefix += TEXT("->");
			}
		}

		Prefix += Term->Name;
		return Prefix;
	}
}

FString FKismetCppBackend::LatentFunctionInfoTermToText(FBPTerminal* Term, FBlueprintCompiledStatement* TargetLabel)
{
	check(LatentInfoStruct);

	// Find the term name we need to fixup
	FString FixupTermName;
	for (UProperty* Prop = LatentInfoStruct->PropertyLink; Prop; Prop = Prop->PropertyLinkNext)
	{
		if (Prop->GetBoolMetaData(FBlueprintMetadata::MD_NeedsLatentFixup))
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

void FKismetCppBackend::EmitClassProperties(FStringOutputDevice& Target, UClass* SourceClass)
{
	//@TODO: There is a lot of complexity when emitting properties, this code should be copied from FNativeClassHeaderGenerator::ExportProperties (or that code made commonly available in editor mode)

	// Emit class variables
	for (TFieldIterator<UProperty> It(SourceClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		UProperty* Property = *It;

		Emit(Header, TEXT("\n\tUPROPERTY()\n"));
		Emit(Header, TEXT("\t"));
		Property->ExportCppDeclaration(Target, EExportedDeclaration::Member);
		Emit(Header, TEXT(";\n"));
	}
}

void FKismetCppBackend::GenerateCodeFromClass(UClass* SourceClass, TIndirectArray<FKismetFunctionContext>& Functions, bool bGenerateStubsOnly)
{
	CppClassName = FString(SourceClass->GetPrefixCPP()) + SourceClass->GetName();

	UClass* SuperClass = SourceClass->GetSuperClass();
	Emit(Header,
		*FString::Printf(TEXT("class %s : public %s%s\n"), *CppClassName, SuperClass->GetPrefixCPP(), *SuperClass->GetName()));
	Emit(Header,
		TEXT("{\n")
		TEXT("public:\n"));
	Emit(Header, *FString::Printf(TEXT("\tGENERATED_UCLASS_BODY();\n")));

	EmitClassProperties(Header, SourceClass);

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

	for (int32 i = 0; i < Functions.Num(); ++i)
	{
		if (Functions[i].IsValid())
		{
			ConstructFunction(Functions[i], bGenerateStubsOnly);
		}
	}

	Emit(Header, TEXT("};\n\n"));
}

void FKismetCppBackend::EmitCallDelegateStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	Emit(Body, *FString::Printf(TEXT("\t\t\t%s.Broadcast("), *TermToText(Statement.FunctionContext)));
	int32 NumParams = 0;
	for (TFieldIterator<UProperty> PropIt(Statement.FunctionToCall); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* FuncParamProperty = *PropIt;

		if (!FuncParamProperty->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			if (NumParams > 0)
			{
				Emit(Body, TEXT(", "));
			}

			FString VarName;

			FBPTerminal* Term = Statement.RHS[NumParams];
			ensure(Term != NULL);

			// See if this is a hidden array param term, which needs to be fixed up with the final generated UArrayProperty
			if( FBPTerminal** ArrayParmTerm = Statement.ArrayCoersionTermMap.Find(Term) )
			{
				Term->ObjectLiteral = (*ArrayParmTerm)->AssociatedVarProperty;
			}

			if( (Statement.TargetLabel != NULL) && (Statement.UbergraphCallIndex == NumParams) )
			{
				// The target label will only ever be set on a call function when calling into the Ubergraph or
				// on a latent function that will later call into the ubergraph, either of which requires a patchup
				UStructProperty* StructProp = Cast<UStructProperty>(FuncParamProperty);
				if( StructProp && StructProp->Struct == LatentInfoStruct )
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
				VarName = TermToText(Term, FuncParamProperty);
			}

			if (FuncParamProperty->HasAnyPropertyFlags(CPF_OutParm))
			{
				Emit(Body, TEXT("/*out*/ "));
			}
			Emit(Body, *VarName);

			NumParams++;
		}
	}
	Emit(Body, TEXT(");\n"));
}

void FKismetCppBackend::EmitCallStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	Emit(Body, TEXT("\t\t\t"));

	// Handle the return value of the function being called
	UProperty* FuncToCallReturnProperty = Statement.FunctionToCall->GetReturnProperty();
	if (FuncToCallReturnProperty != NULL)
	{
		FBPTerminal* Term = Statement.LHS;
		ensure(Term != NULL);

		FString ReturnVarName = TermToText(Term);
		Emit(Body, *FString::Printf(TEXT("%s = "), *ReturnVarName));
	}

	// Emit object to call the method on
	if ((Statement.FunctionContext != NULL) && (Statement.FunctionContext->Name != Schema->PSC_Self)) //@TODO: Badness, could be a self reference wired to another instance!
	{
		if (Statement.FunctionToCall->HasAnyFunctionFlags(FUNC_Static))
		{
			Emit(Body, *FString::Printf(TEXT("U%s::"), *Statement.FunctionToCall->GetOuter()->GetName())); //@TODO: Assuming U prefix but could be A
		}
		else
		{
			Emit(Body, *FString::Printf(TEXT("%s->"), *TermToText(Statement.FunctionContext, (UProperty*)(GetDefault<UObjectProperty>()))));
		}
	}

	// Emit method name
	FString FunctionNameToCall;
	Statement.FunctionToCall->GetName(FunctionNameToCall);
	if( Statement.bIsParentContext )
	{
		const FString ContextString = Statement.FunctionToCall->GetOuter()->GetName();
		FunctionNameToCall = ContextString + TEXT("::") + FunctionNameToCall;
	}
	Emit(Body, *FString::Printf(TEXT("%s"), *FunctionNameToCall));

	// Emit method parameter list
	Emit(Body, TEXT("("));
	{
		int32 NumParams = 0;

		for (TFieldIterator<UProperty> PropIt(Statement.FunctionToCall); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			UProperty* FuncParamProperty = *PropIt;

			if (!FuncParamProperty->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				if (NumParams > 0)
				{
					Emit(Body, TEXT(", "));
				}

				FString VarName;

				FBPTerminal* Term = Statement.RHS[NumParams];
				ensure(Term != NULL);

				// See if this is a hidden array param term, which needs to be fixed up with the final generated UArrayProperty
				if( FBPTerminal** ArrayParmTerm = Statement.ArrayCoersionTermMap.Find(Term) )
				{
					Term->ObjectLiteral = (*ArrayParmTerm)->AssociatedVarProperty;
				}

				if( (Statement.TargetLabel != NULL) && (Statement.UbergraphCallIndex == NumParams) )
				{
					// The target label will only ever be set on a call function when calling into the Ubergraph or
					// on a latent function that will later call into the ubergraph, either of which requires a patchup
					UStructProperty* StructProp = Cast<UStructProperty>(FuncParamProperty);
					if( StructProp && StructProp->Struct == LatentInfoStruct )
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
					VarName = TermToText(Term, FuncParamProperty);
				}

				if (FuncParamProperty->HasAnyPropertyFlags(CPF_OutParm))
				{
					Emit(Body, TEXT("/*out*/ "));
				}
				Emit(Body, *VarName);

				NumParams++;
			}
		}
	}
	Emit(Body, TEXT(");\n"));
}

void FKismetCppBackend::EmitAssignmentStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString DestinationExpression;
	DestinationExpression = TermToText(Statement.LHS);

	FString SourceExpression;
	SourceExpression = TermToText(Statement.RHS[0], Statement.LHS->AssociatedVarProperty);

	// Emit the assignment statement
	Emit(Body, *FString::Printf(TEXT("\t\t\t%s = %s;\n"), *DestinationExpression, *SourceExpression));
}

void FKismetCppBackend::EmitCastToInterfaceStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString InterfaceClass = TermToText(Statement.RHS[0], (UProperty*)(GetDefault<UClassProperty>()));
	FString ObjectValue = TermToText(Statement.RHS[1], (UProperty*)(GetDefault<UObjectProperty>()));
	FString InterfaceValue = TermToText(Statement.LHS, (UProperty*)(GetDefault<UObjectProperty>()));

	Emit(Body, *FString::Printf(TEXT("\t\t\tif ( %s && %s->GetClass()->ImplementsInterface(%s) )\n"), *ObjectValue, *ObjectValue, *InterfaceClass));
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

void FKismetCppBackend::EmitDynamicCastStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString TargetClass = TermToText(Statement.RHS[0], (UProperty*)(GetDefault<UClassProperty>()));
	FString ObjectValue = TermToText(Statement.RHS[1], (UProperty*)(GetDefault<UObjectProperty>()));
	FString CastedValue = TermToText(Statement.LHS, (UProperty*)(GetDefault<UObjectProperty>()));

	Emit(Body, *FString::Printf(TEXT("\t\t\t %s = Cast<%s>(%s);\n"),
		*CastedValue, *TargetClass, *ObjectValue));
}

void FKismetCppBackend::EmitObjectToBoolStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FString ObjectTarget = TermToText(Statement.RHS[0]);
	FString DestinationExpression = TermToText(Statement.LHS);

	Emit(Body, *FString::Printf(TEXT("\t\t\t%s = (%s != NULL);\n"), *DestinationExpression, *ObjectTarget));
}

void FKismetCppBackend::EmitAddMulticastDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	const FString Delegate = TermToText(Statement.LHS);
	const FString DelegateToAdd = TermToText(Statement.RHS[0]);

	Emit(Body, *FString::Printf(TEXT("\t\t\t%s->Add(%s);\n"), *Delegate, *DelegateToAdd));
}

void FKismetCppBackend::EmitRemoveMulticastDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	const FString Delegate = TermToText(Statement.LHS);
	const FString DelegateToAdd = TermToText(Statement.RHS[0]);

	Emit(Body, *FString::Printf(TEXT("\t\t\t%s->Remove(%s);\n"), *Delegate, *DelegateToAdd));
}

void FKismetCppBackend::EmitBindDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	check(2 == Statement.RHS.Num());
	const FString Delegate = TermToText(Statement.LHS);
	const FString NameTerm = TermToText(Statement.RHS[0]);
	const FString ObjectTerm = TermToText(Statement.RHS[1]);

	Emit(Body, 
		*FString::Printf(
			TEXT("\t\t\tif(FScriptDelegate* __Delegate = %s)\n\t\t\t{\n\t\t\t\t__Delegate->SetFunctionName(%s);\n\t\t\t\t__Delegate->SetObject(%s);\n\t\t\t|\n"),
			*Delegate, 
			*NameTerm, 
			*ObjectTerm
		)
	);
}

void FKismetCppBackend::EmitClearMulticastDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	const FString Delegate = TermToText(Statement.LHS);

	Emit(Body, *FString::Printf(TEXT("\t\t\t%s->Clear();\n"), *Delegate));
}

void FKismetCppBackend::EmitCreateArrayStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	FBPTerminal* ArrayTerm = Statement.LHS;
	const FString Array = TermToText(ArrayTerm);

	UArrayProperty* ArrayProperty = CastChecked<UArrayProperty>(ArrayTerm->AssociatedVarProperty);
	UProperty* InnerProperty = ArrayProperty->Inner;

	for(int32 i = 0; i < Statement.RHS.Num(); ++i)
	{
		FBPTerminal* CurrentTerminal = Statement.RHS[i];
		Emit(Body, 
			*FString::Printf(
				TEXT("\t\t\t%s[%d] = %s;"),
				*Array,
				i,
				*TermToText(CurrentTerminal, (CurrentTerminal->bIsLiteral ? InnerProperty : NULL))));
	}
}

void FKismetCppBackend::EmitGotoStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	if (Statement.Type == KCST_ComputedGoto)
	{
		FString NextStateExpression;
		NextStateExpression = TermToText(Statement.LHS, (UProperty*)(GetDefault<UIntProperty>()));

		Emit(Body, *FString::Printf(TEXT("\t\t\tCurrentState = %s;\n"), *NextStateExpression));
		Emit(Body, *FString::Printf(TEXT("\t\t\tbreak;\n")));
	}
	else if ((Statement.Type == KCST_GotoIfNot) || (Statement.Type == KCST_EndOfThreadIfNot))
	{
		FString ConditionExpression;
		ConditionExpression = TermToText(Statement.LHS, (UProperty*)(GetDefault<UBoolProperty>()));

		Emit(Body, *FString::Printf(TEXT("\t\t\tif (!%s)\n"), *ConditionExpression));
		Emit(Body, *FString::Printf(TEXT("\t\t\t{\n")));

		if (Statement.Type == KCST_EndOfThreadIfNot)
		{
			Emit(Body, TEXT("\t\t\t\tCurrentState = (StateStack.Num() > 0) ? StateStack.Pop() : -1;\n"));
		}
		else
		{
			Emit(Body, *FString::Printf(TEXT("\t\t\t\tCurrentState = %d;\n"), StatementToStateIndex(FunctionContext, Statement.TargetLabel)));
		}

		Emit(Body, *FString::Printf(TEXT("\t\t\t\tbreak;\n")));
		Emit(Body, *FString::Printf(TEXT("\t\t\t}\n")));
	}
	else
	{
		Emit(Body, *FString::Printf(TEXT("\t\t\tCurrentState = %d;\n"), StatementToStateIndex(FunctionContext, Statement.TargetLabel)));
		Emit(Body, *FString::Printf(TEXT("\t\t\tbreak;\n")));
	}
}

void FKismetCppBackend::EmitPushStateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement)
{
	Emit(Body, *FString::Printf(TEXT("\t\t\tStateStack.Push(%d);\n"), StatementToStateIndex(FunctionContext, Statement.TargetLabel)));
}

void FKismetCppBackend::EmitEndOfThreadStatement(FKismetFunctionContext& FunctionContext, const FString& ReturnValueString)
{
	Emit(Body, TEXT("\t\t\tCurrentState = (StateStack.Num() > 0) ? StateStack.Pop() : -1;\n"));
	Emit(Body, TEXT("\t\t\tbreak;\n"));
}

void FKismetCppBackend::EmitReturnStatement(FKismetFunctionContext& FunctionContext, const FString& ReturnValueString)
{
	Emit(Body, *FString::Printf(TEXT("\treturn%s;\n"), *ReturnValueString));
}

void FKismetCppBackend::DeclareLocalVariables(FKismetFunctionContext& FunctionContext, TArray<UProperty*>& LocalVariables)
{
	for (int32 i = 0; i < LocalVariables.Num(); ++i)
	{
		UProperty* LocalVariable = LocalVariables[i];

		Emit(Body, TEXT("\t"));
		LocalVariable->ExportCppDeclaration(Body, EExportedDeclaration::Local);
		Emit(Body, TEXT(";\n"));
	}

	if (LocalVariables.Num() > 0)
	{
		Emit(Body, TEXT("\n"));
	}
}

void FKismetCppBackend::DeclareStateSwitch(FKismetFunctionContext& FunctionContext)
{
	Emit(Body, TEXT("\tTArray< int32, TInlineAllocator<8> > StateStack;\n"));
	Emit(Body, TEXT("\tint32 CurrentState = 0;\n"));
	Emit(Body, TEXT("\tdo\n"));
	Emit(Body, TEXT("\t{\n"));
	Emit(Body, TEXT("\t\tswitch( CurrentState )\n"));
	Emit(Body, TEXT("\t\t{\n"));
	Emit(Body, TEXT("\t\tcase 0:\n"));
}

void FKismetCppBackend::CloseStateSwitch(FKismetFunctionContext& FunctionContext)
{
	// Default error-catching case 
	Emit(Body, TEXT("\t\tdefault:\n"));
	Emit(Body, TEXT("\t\t\tcheck(false); // Invalid state\n"));
	Emit(Body, TEXT("\t\t\tbreak;\n"));

	// Close the switch block and do-while loop
	Emit(Body, TEXT("\t\t}\n"));
	Emit(Body, TEXT("\t} while( CurrentState != -1 );\n"));
}

void FKismetCppBackend::ConstructFunction(FKismetFunctionContext& FunctionContext, bool bGenerateStubOnly)
{
	FString FunctionName;
	UFunction* Function = FunctionContext.Function;
	UClass* Class = FunctionContext.NewClass;

	Function->GetName(FunctionName);

	// Split the function property list into arguments, a return value (if any), and local variable declarations
	TArray<UProperty*> ArgumentList;
	UProperty* ReturnValue = NULL;
	TArray<UProperty*> LocalVariables;
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
					MessageLog.Error(*FString::Printf(TEXT("Function %s from graph @@ has more than one return value (named %s and %s)"),
						*FunctionName, *ReturnValue->GetName(), *Property->GetName()), FunctionContext.SourceGraph);
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

	// Emit the declaration
	FString ReturnValueString;
	FString ReturnType;
	if (ReturnValue != NULL)
	{
		ReturnType = ReturnValue->GetCPPType(NULL);
		ReturnValueString = FString(TEXT(" ")) + ReturnValue->GetName();
	}
	else
	{
		ReturnType = TEXT("void");
		ReturnValueString = TEXT("");
	}

	//@TODO: Make the header+body export more uniform
	{
		FString Start = FString::Printf(TEXT("%s %s%s%s("), *ReturnType, TEXT("%s"), TEXT("%s"), *FunctionName);

		Emit(Header, TEXT("\t"));
		Emit(Header, *FString::Printf(*Start, TEXT(""), TEXT("")));
		Emit(Body, *FString::Printf(*Start, *Class->GetName(), TEXT("::")));

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

			ArgProperty->ExportCppDeclaration(Header, EExportedDeclaration::Parameter);
			ArgProperty->ExportCppDeclaration(Body,   EExportedDeclaration::Parameter);
		}

		Emit(Header, TEXT(");\n"));
		Emit(Body, TEXT(")\n"));
	}

	// Start the body of the implementation
	Emit(Body, TEXT("{\n"));

	if( !bGenerateStubOnly )
	{
		// Emit local variables
		DeclareLocalVariables(FunctionContext, LocalVariables);

		DeclareStateSwitch(FunctionContext);


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

					if (Statement.bIsJumpTarget)
					{
						const int32 StateNum = StatementToStateIndex(FunctionContext, &Statement);
						Emit(Body, *FString::Printf(TEXT("\n\t\tcase %d:\n"), StateNum));
					}

					switch (Statement.Type)
					{
					case KCST_Nop:
						Emit(Body, TEXT("\t\t\t//No operation.\n"));
						break;
					case KCST_WireTraceSite:
						Emit(Body, TEXT("\t\t\t// Wire debug site.\n"));
						break;
					case KCST_DebugSite:
						Emit(Body, TEXT("\t\t\t// Debug site.\n"));
						break;
					case KCST_CallFunction:
						EmitCallStatment(FunctionContext, Statement);
						break;
					case KCST_CallDelegate:
						EmitCallDelegateStatment(FunctionContext, Statement);
						break;
					case KCST_Assignment:
						EmitAssignmentStatment(FunctionContext, Statement);
						break;
					case KCST_CastToInterface:
						EmitCastToInterfaceStatement(FunctionContext, Statement);
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
					case KCST_RemoveMulticastDelegate:
						EmitRemoveMulticastDelegateStatement(FunctionContext, Statement);
						break;
					case KCST_BindDelegate:
						EmitBindDelegateStatement(FunctionContext, Statement);
						break;
					case KCST_ClearMulticastDelegate:
						EmitClearMulticastDelegateStatement(FunctionContext, Statement);
						break;
					case KCST_CreateArray:
						EmitCreateArrayStatement(FunctionContext, Statement);
						break;
					case KCST_ComputedGoto:
					case KCST_UnconditionalGoto:
					case KCST_GotoIfNot:
					case KCST_EndOfThreadIfNot:
						EmitGotoStatement(FunctionContext, Statement);
						break;
					case KCST_PushState:
						EmitPushStateStatement(FunctionContext, Statement);
						break;
					case KCST_EndOfThread:
						EmitEndOfThreadStatement(FunctionContext, ReturnValueString);
						break;
					case KCST_Comment:
						Emit(Body, *FString::Printf(TEXT("\t\t\t// %s\n"), *Statement.Comment));
						break;
					default:
						Emit(Body, TEXT("\t// Warning: Ignoring unsupported statement\n"));
						UE_LOG(LogK2Compiler, Warning, TEXT("C++ backend encountered unsupported statement type %d"), (int32)Statement.Type);
						break;
					};
				}
			}
		}

		CloseStateSwitch(FunctionContext);
	}

	EmitReturnStatement(FunctionContext, ReturnValueString);
	
	Emit(Body, TEXT("}\n\n"));
}
