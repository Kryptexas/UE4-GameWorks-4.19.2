// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilerCppBackendModulePrivatePCH.h"
#include "BlueprintCompilerCppBackendBase.h"
#include "BlueprintCompilerCppBackendUtils.h"

void FBlueprintCompilerCppBackendBase::EmitStructProperties(FStringOutputDevice& Target, UStruct* SourceClass)
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

void FBlueprintCompilerCppBackendBase::DeclareDelegates(UClass* SourceClass, TIndirectArray<FKismetFunctionContext>& Functions)
{
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
}

void FBlueprintCompilerCppBackendBase::GenerateCodeFromClass(UClass* SourceClass, TIndirectArray<FKismetFunctionContext>& Functions, bool bGenerateStubsOnly)
{
	auto CleanCppClassName = SourceClass->GetName();
	CppClassName = FString(SourceClass->GetPrefixCPP()) + CleanCppClassName;
	
	EmitFileBeginning(CleanCppClassName, SourceClass);

	// Class declaration
	const bool bIsInterface = SourceClass->IsChildOf<UInterface>();
	if (bIsInterface)
	{
		Emit(Header, TEXT("UINTERFACE(Blueprintable"));
		EmitReplaceConvertedMetaData(SourceClass);
		Emit(Header, *FString::Printf(TEXT(")\nclass U%s : public UInterface\n{\n\tGENERATED_BODY()\n};\n"), *CleanCppClassName));
		Emit(Header, *FString::Printf(TEXT("\nclass I%s"), *CleanCppClassName));
	}
	else
	{
		Emit(Header, TEXT("UCLASS("));
		if (!SourceClass->IsChildOf<UBlueprintFunctionLibrary>())
		{
			Emit(Header, TEXT("Blueprintable, BlueprintType"));
		}
		EmitReplaceConvertedMetaData(SourceClass);
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

	DeclareDelegates(SourceClass, Functions);

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

	Emit(Header, TEXT("\n\tstatic void __StaticDependenciesConvertedClasses(TArray<FName>& OutNames);\n"));
	Emit(Header, TEXT("\n\tstatic void __StaticDependenciesAssets(TArray<FName>& OutNames);\n"));

	Emit(Header, *FBackendHelperUMG::WidgetFunctionsInHeader(SourceClass));

	Emit(Header, TEXT("};\n\n"));

	Emit(Body, *FEmitHelper::EmitLifetimeReplicatedPropsImpl(SourceClass, CppClassName, TEXT("")));
}

void FBlueprintCompilerCppBackendBase::DeclareLocalVariables(FKismetFunctionContext& FunctionContext, TArray<UProperty*>& LocalVariables)
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

void FBlueprintCompilerCppBackendBase::ConstructFunction(FKismetFunctionContext& FunctionContext, bool bGenerateStubOnly)
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
		if (FEmitHelper::ShouldHandleAsNativeEvent(Function)) // BlueprintNativeEvent
		{
			bGenerateAsNativeEventImplementation = true;
			FunctionName += TEXT("_Implementation");
		}
		else if (FEmitHelper::ShouldHandleAsImplementableEvent(Function)) // BlueprintImplementableEvent
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

	if (!bGenerateStubOnly)
	{
		// Emit local variables
		DeclareLocalVariables(FunctionContext, LocalVariables);

		const bool bUseSwitchState = FunctionContext.MustUseSwitchState(nullptr);

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

		const FString FunctionImplementation = InnerFunctionImplementation(FunctionContext, bUseSwitchState);
		Emit(Body, *FunctionImplementation);
	}

	const FString ReturnValueString = ReturnValue ? (FString(TEXT(" ")) + ReturnValue->GetName()) : TEXT("");
	Emit(Body, *FString::Printf(TEXT("\treturn%s;\n"), *ReturnValueString));
	Emit(Body, TEXT("}\n\n"));
}

void FBlueprintCompilerCppBackendBase::GenerateCodeFromEnum(UUserDefinedEnum* SourceEnum)
{
	check(SourceEnum);
	const FString Name = SourceEnum->GetName();
	EmitFileBeginning(Name, nullptr);

	Emit(Header, TEXT("UENUM(BlueprintType"));
	EmitReplaceConvertedMetaData(SourceEnum);
	Emit(Header, TEXT(")\nenum class "));
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

void FBlueprintCompilerCppBackendBase::EmitReplaceConvertedMetaData(UObject* Obj)
{
	const FString ReplaceConvertedMD = FEmitHelper::GenerateReplaceConvertedMD(Obj);
	if (!ReplaceConvertedMD.IsEmpty())
	{
		TArray<FString> AdditionalMD;
		AdditionalMD.Add(ReplaceConvertedMD);
		Emit(Header, TEXT(","));
		Emit(Header, *FEmitHelper::HandleMetaData(nullptr, false, &AdditionalMD));
	}
}

void FBlueprintCompilerCppBackendBase::GenerateCodeFromStruct(UUserDefinedStruct* SourceStruct)
{
	check(SourceStruct);

	EmitFileBeginning(SourceStruct->GetName(), SourceStruct);

	const FString NewName = FString(TEXT("F")) + SourceStruct->GetName();
	Emit(Header, TEXT("USTRUCT(BlueprintType"));
	EmitReplaceConvertedMetaData(SourceStruct);
	Emit(Header, TEXT(")\n"));

	Emit(Header, *FString::Printf(TEXT("struct %s\n{\npublic:\n\tGENERATED_BODY()\n"), *NewName));
	EmitStructProperties(Header, SourceStruct);
	Emit(Header, *FEmitDefaultValueHelper::GenerateGetDefaultValue(SourceStruct));
	Emit(Header, TEXT("};\n"));
}

void FBlueprintCompilerCppBackendBase::EmitFileBeginning(const FString& CleanName, UStruct* SourceStruct)
{
	Emit(Header, TEXT("#pragma once\n\n"));
	if (SourceStruct)
	{ 
		// find objects referenced by functions/script
		TArray<UObject*> IncludeInHeader;
		TArray<const UStruct*> DeclareInHeader;
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
						const UObjectProperty* ObjectProperty = Cast<UObjectProperty>(OwnerProperty);
						if (ObjectProperty)
						{
							DeclareInHeader.AddUnique(ObjectProperty->PropertyClass);
							BodyReferenceFinder.FindReferences(Obj);
						}
						else
						{
							HeaderReferenceFinder.FindReferences(Obj);
						}
					}
					else
					{
						BodyReferenceFinder.FindReferences(Obj);
					}
				}
			}

			if (auto BPGC = Cast<UClass>(SourceStruct))
			{
				BodyReferenceFinder.FindReferences(BPGC->GetDefaultObject(false));
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

		auto EmitIncludeHeader = [&](FStringOutputDevice& Dst, const TCHAR* Message, bool bAddDotH)
		{
			Emit(Dst, *FString::Printf(TEXT("#include \"%s%s\"\n"), Message, bAddDotH ? TEXT(".h") : TEXT("")));
		};
		EmitIncludeHeader(Body, FApp::GetGameName(), true);
		EmitIncludeHeader(Body, *CleanName, true);
		EmitIncludeHeader(Body, TEXT("GeneratedCodeHelpers"), true);

		TSet<FString> AlreadyIncluded;
		AlreadyIncluded.Add(SourceStruct->GetName());
		auto EmitInner = [&](FStringOutputDevice& Dst, const TArray<UObject*>& Src, const TArray<const UStruct*>& Declarations )
		{
			auto EngineSourceDir = FPaths::EngineSourceDir();
			auto GameSourceDir = FPaths::GameSourceDir();
			auto CurrentPackage = SourceStruct->GetTypedOuter<UPackage>();

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

				auto BPGC = (Obj && !bWantedType) ? Cast<UBlueprintGeneratedClass>(Obj->GetClass()) : nullptr;
				if (BPGC)
				{
					bWantedType = true;
					Obj = BPGC;
				}

				// Wanted no-native type, thet will be converted
				if (bWantedType && Obj)
				{
					const FString Name = Obj->GetName();
					bool bAlreadyIncluded = false;
					AlreadyIncluded.Add(Name, &bAlreadyIncluded);
					if (!bAlreadyIncluded)
					{
						EmitIncludeHeader(Dst, *Name, true);
					}
				}
				// headers for native items
				else if (auto Field = Cast<UField>(Obj))
				{
					FString PackPath;
					if ((Field->GetTypedOuter<UPackage>() != CurrentPackage)
						&& FSourceCodeNavigation::FindClassHeaderPath(Field, PackPath))
					{
						if (!PackPath.RemoveFromStart(EngineSourceDir))
						{
							if (!PackPath.RemoveFromStart(GameSourceDir))
							{
								PackPath = FPaths::GetCleanFilename(PackPath);
							}
						}
						bool bAlreadyIncluded = false;
						AlreadyIncluded.Add(PackPath, &bAlreadyIncluded);
						if (!bAlreadyIncluded)
						{
							EmitIncludeHeader(Dst, *PackPath, false);
						}
					}

				}
			}

			Emit(Dst, TEXT("\n"));

			for (const UStruct* Type : Declarations)
			{
				Emit(Dst, *FString::Printf(TEXT("class %s;\n"), *(FString(Type->GetPrefixCPP()) + Type->GetName())));
			}

			Emit(Dst, TEXT("\n"));
		};

		EmitInner(Header, IncludeInHeader, DeclareInHeader);
		EmitInner(Body, IncludeInBody, TArray<const UStruct*>() );
	}
	Emit(Header, *FString::Printf(TEXT("#include \"%s.generated.h\"\n\n"), *CleanName));
}
