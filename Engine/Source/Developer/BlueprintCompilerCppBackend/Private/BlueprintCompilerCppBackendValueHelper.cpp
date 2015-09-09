// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilerCppBackendModulePrivatePCH.h"
#include "BlueprintCompilerCppBackendUtils.h"
#include "Editor/UnrealEd/Public/Kismet2/StructureEditorUtils.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/DynamicBlueprintBinding.h"

void FEmitDefaultValueHelper::OuterGenerate(FEmitterLocalContext& Context
	, const UProperty* Property
	, const FString& OuterPath
	, const uint8* DataContainer
	, const uint8* OptionalDefaultDataContainer
	, EPropertyAccessOperator AccessOperator
	, bool bAllowProtected)
{
	// Determine if the given property contains an instanced default subobject reference. We only get here if the values are not identical.
	auto IsInstancedSubobjectLambda = [&](int32 ArrayIndex) -> bool
	{
		if (auto ObjectProperty = Cast<UObjectProperty>(Property))
		{
			check(DataContainer);
			check(OptionalDefaultDataContainer);

			auto ObjectPropertyValue = ObjectProperty->GetObjectPropertyValue_InContainer(DataContainer, ArrayIndex);
			auto DefaultObjectPropertyValue = ObjectProperty->GetObjectPropertyValue_InContainer(OptionalDefaultDataContainer, ArrayIndex);
			if (ObjectPropertyValue && ObjectPropertyValue->IsDefaultSubobject() && DefaultObjectPropertyValue && DefaultObjectPropertyValue->IsDefaultSubobject() && ObjectPropertyValue->GetFName() == DefaultObjectPropertyValue->GetFName())
			{
				return true;
			}
		}

		return false;
	};

	if (Property->HasAnyPropertyFlags(CPF_EditorOnly | CPF_Transient))
	{
		UE_LOG(LogK2Compiler, Log, TEXT("FEmitDefaultValueHelper Skip EditorOnly or Transient property: %s"), *Property->GetPathName());
		return;
	}

	for (int32 ArrayIndex = 0; ArrayIndex < Property->ArrayDim; ++ArrayIndex)
	{
		if (!OptionalDefaultDataContainer
			|| (!Property->Identical_InContainer(DataContainer, OptionalDefaultDataContainer, ArrayIndex) && !IsInstancedSubobjectLambda(ArrayIndex)))
		{
			FString PathToMember;
			if (Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPrivate) || (!bAllowProtected && Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierProtected)))
			{
				ensure(EPropertyAccessOperator::None != AccessOperator);
				
				const FString PropertyObject = Context.GenerateGetProperty(Property); //X::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(X, X))

				FStringOutputDevice TypeDeclaration;
				const uint32 CppTemplateTypeFlags = EPropertyExportCPPFlags::CPPF_CustomTypeName
					| EPropertyExportCPPFlags::CPPF_NoConst | EPropertyExportCPPFlags::CPPF_NoRef | EPropertyExportCPPFlags::CPPF_NoStaticArray
					| EPropertyExportCPPFlags::CPPF_BlueprintCppBackend;
				Property->ExportCppDeclaration(TypeDeclaration, EExportedDeclaration::Parameter, nullptr, CppTemplateTypeFlags, true);

				const FString OperatorStr = (EPropertyAccessOperator::Dot == AccessOperator) ? TEXT("&") : TEXT("");
				const FString ContainerStr = (EPropertyAccessOperator::None == AccessOperator) ? TEXT("this") : FString::Printf(TEXT("%s(%s)"), *OperatorStr, *OuterPath);
				PathToMember = Context.GenerateUniqueLocalName();
				Context.AddLine(FString::Printf(TEXT("auto& %s = *(%s->ContainerPtrToValuePtr<%s>(%s, %d));")
					, *PathToMember, *PropertyObject, *TypeDeclaration, *ContainerStr, ArrayIndex));
			}
			else
			{
				const FString AccessOperatorStr = (EPropertyAccessOperator::None == AccessOperator) ? TEXT("")
					: ((EPropertyAccessOperator::Pointer == AccessOperator) ? TEXT("->") : TEXT("."));
				const bool bStaticArray = (Property->ArrayDim > 1);
				const FString ArrayPost = bStaticArray ? FString::Printf(TEXT("[%d]"), ArrayIndex) : TEXT("");
				PathToMember = FString::Printf(TEXT("%s%s%s%s"), *OuterPath, *AccessOperatorStr, *FEmitHelper::GetCppName(Property), *ArrayPost);
			}
			const uint8* ValuePtr = Property->ContainerPtrToValuePtr<uint8>(DataContainer, ArrayIndex);
			const uint8* DefaultValuePtr = OptionalDefaultDataContainer ? Property->ContainerPtrToValuePtr<uint8>(OptionalDefaultDataContainer, ArrayIndex) : nullptr;
			InnerGenerate(Context, Property, PathToMember, ValuePtr, DefaultValuePtr);
		}
	}
}

FString FEmitDefaultValueHelper::GenerateGetDefaultValue(const UUserDefinedStruct* Struct, const FGatherConvertedClassDependencies& Dependencies)
{
	check(Struct);
	const FString StructName = FEmitHelper::GetCppName(Struct);
	FString Result = FString::Printf(TEXT("\tstatic %s GetDefaultValue()\n\t{\n\t\t%s DefaultData__;\n"), *StructName, *StructName);

	FStructOnScope StructData(Struct);
	FStructureEditorUtils::Fill_MakeStructureDefaultValue(Struct, StructData.GetStructMemory());

	FEmitterLocalContext Context(nullptr, Dependencies);
	Context.IncreaseIndent();
	Context.IncreaseIndent();
	for (auto Property : TFieldRange<const UProperty>(Struct))
	{
		OuterGenerate(Context, Property, TEXT("DefaultData__"), StructData.GetStructMemory(), nullptr, EPropertyAccessOperator::Dot);
	}
	Result += Context.GetResult();
	Result += TEXT("\n\t\treturn DefaultData__;\n\t}\n");
	return Result;
}

void FEmitDefaultValueHelper::InnerGenerate(FEmitterLocalContext& Context, const UProperty* Property, const FString& PathToMember, const uint8* ValuePtr, const uint8* DefaultValuePtr, bool bWithoutFirstConstructionLine)
{
	auto OneLineConstruction = [](FEmitterLocalContext& LocalContext, const UProperty* LocalProperty, const uint8* LocalValuePtr, FString& OutSingleLine, bool bGenerateEmptyStructConstructor) -> bool
	{
		bool bComplete = true;
		FString ValueStr = HandleSpecialTypes(LocalContext, LocalProperty, LocalValuePtr);
		if (ValueStr.IsEmpty())
		{
			auto StructProperty = Cast<const UStructProperty>(LocalProperty);
			LocalProperty->ExportTextItem(ValueStr, LocalValuePtr, LocalValuePtr, nullptr, EPropertyPortFlags::PPF_ExportCpp);
			if (ValueStr.IsEmpty() && StructProperty)
			{
				check(StructProperty->Struct);
				if (bGenerateEmptyStructConstructor)
				{
					ValueStr = FString::Printf(TEXT("%s{}"), *FEmitHelper::GetCppName(StructProperty->Struct)); //don;t override existing values
				}
				bComplete = false;
			}
			else if (ValueStr.IsEmpty())
			{
				UE_LOG(LogK2Compiler, Error, TEXT("FEmitDefaultValueHelper Cannot generate initilization: %s"), *LocalProperty->GetPathName());
			}
		}
		OutSingleLine += ValueStr;
		return bComplete;
	};

	auto StructProperty = Cast<const UStructProperty>(Property);
	check(!StructProperty || StructProperty->Struct);
	auto ArrayProperty = Cast<const UArrayProperty>(Property);
	check(!ArrayProperty || ArrayProperty->Inner);

	if (!bWithoutFirstConstructionLine)
	{
		FString ValueStr;
		const bool bComplete = OneLineConstruction(Context, Property, ValuePtr, ValueStr, false);
		if (!ValueStr.IsEmpty())
		{
			Context.AddLine(FString::Printf(TEXT("%s = %s;"), *PathToMember, *ValueStr));
		}
		// array initialization "array_var = TArray<..>()" is complete, but it still needs items.
		if (bComplete && !ArrayProperty)
		{
			return;
		}
	}

	if (StructProperty)
	{
		for (auto LocalProperty : TFieldRange<const UProperty>(StructProperty->Struct))
		{
			OuterGenerate(Context, LocalProperty, PathToMember, ValuePtr, DefaultValuePtr, EPropertyAccessOperator::Dot);
		}
	}
	
	if (ArrayProperty)
	{
		FScriptArrayHelper ScriptArrayHelper(ArrayProperty, ValuePtr);
		for (int32 Index = 0; Index < ScriptArrayHelper.Num(); ++Index)
		{
			const uint8* LocalValuePtr = ScriptArrayHelper.GetRawPtr(Index);

			FString ValueStr;
			const bool bComplete = OneLineConstruction(Context, ArrayProperty->Inner, LocalValuePtr, ValueStr, true);
			Context.AddLine(FString::Printf(TEXT("%s.Add(%s);"), *PathToMember, *ValueStr));
			if (!bComplete)
			{
				const FString LocalPathToMember = FString::Printf(TEXT("%s[%d]"), *PathToMember, Index);
				InnerGenerate(Context, ArrayProperty->Inner, LocalPathToMember, LocalValuePtr, nullptr, true);
			}
		}
	}
}

FString FEmitDefaultValueHelper::HandleSpecialTypes(FEmitterLocalContext& Context, const UProperty* Property, const uint8* ValuePtr)
{
	//TODO: Use Path maps for Objects
	if (auto ObjectProperty = Cast<UObjectProperty>(Property))
	{
		UObject* Object = ObjectProperty->GetPropertyValue(ValuePtr);
		if (Object)
		{
			{
				const FString MappedObject = Context.FindGloballyMappedObject(Object);
				if (!MappedObject.IsEmpty())
				{
					return MappedObject;
				}
			}

			{
				FString LocalMappedObject;
				if (Context.bInsideConstructor && Context.FindLocalObject_InConstructor(Object, LocalMappedObject))
				{
					return LocalMappedObject;
				}
			}

			{
				auto BPGC = Context.GetCurrentlyGeneratedClass();
				auto CDO = BPGC ? BPGC->GetDefaultObject(false) : nullptr;
				if (BPGC && Object && CDO && Object->IsIn(BPGC) && !Object->IsIn(CDO) && Context.bCreatingObjectsPerClass)
				{
					return HandleClassSubobject(Context, Object);
				}
			}

			if (!Context.bCreatingObjectsPerClass && Property->HasAnyPropertyFlags(CPF_InstancedReference))
			{
				const FString CreateAsInstancedSubobject = HandleInstancedSubobject(Context, Object, Object->HasAnyFlags(RF_ArchetypeObject));
				if (!CreateAsInstancedSubobject.IsEmpty())
				{
					return CreateAsInstancedSubobject;
				}
			}
		}
		else if (ObjectProperty->HasMetaData(FBlueprintMetadata::MD_LatentCallbackTarget))
		{
			return TEXT("this");
		}
	}

	if (auto StructProperty = Cast<UStructProperty>(Property))
	{
		if (TBaseStructure<FTransform>::Get() == StructProperty->Struct)
		{
			check(ValuePtr);
			const FTransform* Transform = reinterpret_cast<const FTransform*>(ValuePtr);
			const auto Rotation = Transform->GetRotation();
			const auto Translation = Transform->GetTranslation();
			const auto Scale = Transform->GetScale3D();
			return FString::Printf(TEXT("FTransform(FQuat(%f, %f, %f, %f), FVector(%f, %f, %f), FVector(%f, %f, %f))")
				, Rotation.X, Rotation.Y, Rotation.Z, Rotation.W
				, Translation.X, Translation.Y, Translation.Z
				, Scale.X, Scale.Y, Scale.Z);
		}

		if (TBaseStructure<FVector>::Get() == StructProperty->Struct)
		{
			const FVector* Vector = reinterpret_cast<const FVector*>(ValuePtr);
			return FString::Printf(TEXT("FVector(%f, %f, %f)"), Vector->X, Vector->Y, Vector->Z);
		}

		if (TBaseStructure<FGuid>::Get() == StructProperty->Struct)
		{
			const FGuid* Guid = reinterpret_cast<const FGuid*>(ValuePtr);
			return FString::Printf(TEXT("FGuid(0x%08X, 0x%08X, 0x%08X, 0x%08X)"), Guid->A, Guid->B, Guid->C, Guid->D);
		}
	}
	return FString();
}

FString FEmitDefaultValueHelper::HandleNonNativeComponent(FEmitterLocalContext& Context, const USCS_Node* Node, TSet<const UProperty*>& OutHandledProperties, const USCS_Node* ParentNode)
{
	check(Node);

	FString VariableName;
	UBlueprintGeneratedClass* BPGC = CastChecked<UBlueprintGeneratedClass>(Context.GetCurrentlyGeneratedClass());
	if (UActorComponent* ComponentTemplate = Node->GetActualComponentTemplate(BPGC))
	{
		VariableName = Node->VariableName.ToString();
		Context.AddObjectFromLocalProperty_InConstructor(ComponentTemplate, VariableName);

		const UObjectProperty* VariableProperty = FindField<UObjectProperty>(BPGC, *VariableName);
		if (VariableProperty)
		{
			OutHandledProperties.Add(VariableProperty);
		}

		if (ComponentTemplate->GetOuter() == BPGC)
		{
			UClass* ComponentClass = ComponentTemplate->GetClass();
			check(ComponentClass != nullptr);

			UObject* ObjectToCompare = ComponentClass->GetDefaultObject(false);

			if (ComponentTemplate->HasAnyFlags(RF_InheritableComponentTemplate))
			{
				ObjectToCompare = Node->GetActualComponentTemplate(Cast<UBlueprintGeneratedClass>(BPGC->GetSuperClass()));
			}
			else
			{
				Context.AddHighPriorityLine(FString::Printf(TEXT("%s%s = CreateDefaultSubobject<%s>(TEXT(\"%s\"));")
					, (VariableProperty == nullptr) ? TEXT("auto ") : TEXT("")
					, *VariableName
					, *FEmitHelper::GetCppName(ComponentClass)
					, *ComponentTemplate->GetName()));

				Context.AddLine(FString::Printf(TEXT("%s->CreationMethod = EComponentCreationMethod::SimpleConstructionScript;"), *VariableName));

				FString ParentVariableName;
				if (ParentNode)
				{
					ParentVariableName = ParentNode->VariableName.ToString();
				}
				else if (USceneComponent* ParentComponentTemplate = Node->GetParentComponentTemplate(CastChecked<UBlueprint>(BPGC->ClassGeneratedBy)))
				{
					Context.FindLocalObject_InConstructor(ParentComponentTemplate, ParentVariableName);
				}

				if (!ParentVariableName.IsEmpty())
				{
					Context.AddLine(FString::Printf(TEXT("%s->AttachParent = %s;"), *VariableName, *ParentVariableName));
				}
			}

			for (auto Property : TFieldRange<const UProperty>(ComponentClass))
			{
				OuterGenerate(Context, Property, VariableName
					, reinterpret_cast<const uint8*>(ComponentTemplate)
					, reinterpret_cast<const uint8*>(ObjectToCompare)
					, EPropertyAccessOperator::Pointer);
			}
		}
	}

	// Recursively handle child nodes.
	for (auto ChildNode : Node->ChildNodes)
	{
		HandleNonNativeComponent(Context, ChildNode, OutHandledProperties, Node);
	}

	return VariableName;
}

struct FDependenciesHelper
{
	static void AddDependenciesInConstructor(FEmitterLocalContext& Context, const FGatherConvertedClassDependencies& GatherDependencies)
	{
		if (GatherDependencies.ConvertedClasses.Num())
		{
			Context.AddLine(TEXT("// List of all referenced converted classes"));
		}
		for (auto LocStruct : GatherDependencies.ConvertedClasses)
		{
			Context.AddLine(FString::Printf(TEXT("GetClass()->ConvertedSubobjectsFromBPGC.Add(%s::StaticClass());")
				, *FEmitHelper::GetCppName(LocStruct)));
		}

		if (GatherDependencies.ConvertedStructs.Num())
		{
			Context.AddLine(TEXT("// List of all referenced converted structures"));
		}
		for (auto LocStruct : GatherDependencies.ConvertedStructs)
		{
			Context.AddLine(FString::Printf(TEXT("GetClass()->ConvertedSubobjectsFromBPGC.Add(%s::StaticStruct());")
				, *FEmitHelper::GetCppName(LocStruct)));
		}

		if (GatherDependencies.Assets.Num())
		{
			Context.AddLine(TEXT("// List of all referenced assets"));
		}
		for (auto LocAsset : GatherDependencies.Assets)
		{
			const FString AssetStr = Context.FindGloballyMappedObject(LocAsset, true);
			Context.AddLine(FString::Printf(TEXT("GetClass()->ConvertedSubobjectsFromBPGC.Add(%s);"), *AssetStr));
		}
	}

	static void AddStaticFunctionsForDependencies(FEmitterLocalContext& Context, const FGatherConvertedClassDependencies& GatherDependencies)
	{
		auto OriginalClass = Context.GetCurrentlyGeneratedClass();
		const FString CppClassName = FEmitHelper::GetCppName(OriginalClass);

		// __StaticDependenciesAssets
		Context.AddLine(FString::Printf(TEXT("void %s::__StaticDependenciesAssets(TArray<FName>& OutPackagePaths)"), *CppClassName));
		Context.AddLine(TEXT("{"));
		Context.IncreaseIndent();

		for (UObject* LocAsset : GatherDependencies.Assets)
		{
			const FString PakagePath = LocAsset->GetOutermost()->GetPathName();
			Context.AddLine(FString::Printf(TEXT("OutPackagePaths.Add(TEXT(\"%s\"));"), *PakagePath));
		}

		Context.DecreaseIndent();
		Context.AddLine(TEXT("}"));

		// Register Helper Struct
		const FString RegisterHelperName = FString::Printf(TEXT("FRegisterHelper__%s"), *CppClassName);
		Context.AddLine(FString::Printf(TEXT("struct %s"), *RegisterHelperName));
		Context.AddLine(TEXT("{"));
		Context.IncreaseIndent();

		Context.AddLine(FString::Printf(TEXT("%s()"), *RegisterHelperName));
		Context.AddLine(TEXT("{"));
		Context.IncreaseIndent();
		Context.AddLine(FString::Printf(
			TEXT("FConvertedBlueprintsDependencies::Get().RegisterClass(TEXT(\"%s\"), &%s::__StaticDependenciesAssets);")
			, *OriginalClass->GetName()
			, *CppClassName
			, *CppClassName));
		Context.DecreaseIndent();
		Context.AddLine(TEXT("}"));

		Context.AddLine(FString::Printf(TEXT("static %s Instance;"), *RegisterHelperName));

		Context.DecreaseIndent();
		Context.AddLine(TEXT("};"));

		Context.AddLine(FString::Printf(TEXT("%s %s::Instance;"), *RegisterHelperName, *RegisterHelperName));
	}
};

FString FEmitDefaultValueHelper::GenerateConstructor(UClass* InBPGC, const FGatherConvertedClassDependencies& Dependencies)
{
	auto BPGC = CastChecked<UBlueprintGeneratedClass>(InBPGC);
	const FString CppClassName = FEmitHelper::GetCppName(BPGC);

	FEmitterLocalContext Context(BPGC, Dependencies);
	Context.AddLine(FString::Printf(TEXT("%s::%s(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)"), *CppClassName, *CppClassName));
	Context.AddLine(TEXT("{"));
	Context.IncreaseIndent();
	Context.bInsideConstructor = true;

	// When CDO is created create all subobjects owned by the class
	TArray<UActorComponent*> ActorComponentTempatesOwnedByClass = BPGC->ComponentTemplates;
	{
		// Gather all CT from SCS and IH, the remaining ones are generated for class..
		if (auto SCS = BPGC->SimpleConstructionScript)
		{
			for (auto Node : SCS->GetAllNodes())
			{
				ActorComponentTempatesOwnedByClass.RemoveSwap(Node->ComponentTemplate);
			}
		}
		if (auto IH = BPGC->GetInheritableComponentHandler())
		{
			TArray<UActorComponent*> AllTemplates;
			IH->GetAllTemplates(AllTemplates);
			ActorComponentTempatesOwnedByClass.RemoveAllSwap([&](UActorComponent* Component) -> bool
			{
				return AllTemplates.Contains(Component);
			});
		}

		Context.AddLine(FString::Printf(TEXT("if(HasAnyFlags(RF_ClassDefaultObject) && (%s::StaticClass() == GetClass()))"), *CppClassName));
		Context.AddLine(TEXT("{"));
		Context.IncreaseIndent();
		Context.AddLine(TEXT("ensure(0 == GetClass()->ConvertedSubobjectsFromBPGC.Num());"));
		Context.bCreatingObjectsPerClass = true;
		Context.FlushLines();
		for (auto ComponentTemplate : ActorComponentTempatesOwnedByClass)
		{
			if (ComponentTemplate)
			{
				HandleClassSubobject(Context, ComponentTemplate);
			}
		}

		for (auto TimelineTemplate : BPGC->Timelines)
		{
			if (TimelineTemplate)
			{
				HandleClassSubobject(Context, TimelineTemplate);
			}
		}

		for (auto DynamicBindingObject : BPGC->DynamicBindingObjects)
		{
			if (DynamicBindingObject)
			{
				HandleClassSubobject(Context, DynamicBindingObject);
			}
		}

		FBackendHelperUMG::CreateClassSubobjects(Context);

		Context.bCreatingObjectsPerClass = false;

		FDependenciesHelper::AddDependenciesInConstructor(Context, Dependencies);

		Context.DecreaseIndent();
		Context.AddLine(TEXT("}"));

		// Let's have an easy access to generated class subobjects
		Context.AddLine(TEXT("{")); // no shadow variables
		Context.IncreaseIndent();
		Context.FlushLines();
		int32 SubobjectIndex = 0;
		for (UObject* Obj : Context.GetObjectsCreatedPerClass_InConstructor())
		{
			FString NativeName;
			const bool bFound = Context.FindLocalObject_InConstructor(Obj, NativeName);
			ensure(bFound);
			Context.AddHighPriorityLine(FString::Printf(TEXT("auto %s = CastChecked<%s>(%s::StaticClass()->ConvertedSubobjectsFromBPGC[%d]);")
				, *NativeName
				, *FEmitHelper::GetCppName(Obj->GetClass())
				, *CppClassName
				, SubobjectIndex));
			SubobjectIndex++;
		}
	}

	UObject* CDO = BPGC->GetDefaultObject(false);
	Context.AddObjectFromLocalProperty_InConstructor(CDO, TEXT("this"));

	UObject* ParentCDO = BPGC->GetSuperClass()->GetDefaultObject(false);
	check(CDO && ParentCDO);
	Context.AddLine(TEXT(""));

	FString NativeRootComponentFallback;
	TSet<const UProperty*> HandledProperties;

	// Generate ctor init code for native class default subobjects that are always instanced (e.g. components).
	// @TODO (pkavan) - We can probably make this faster by generating code to index through the DSO array instead (i.e. in place of HandleInstancedSubobject which will generate a lookup call per DSO).
	TArray<UObject*> NativeDefaultObjectSubobjects;
	BPGC->GetDefaultObjectSubobjects(NativeDefaultObjectSubobjects);
	for (auto DSO : NativeDefaultObjectSubobjects)
	{
		if (DSO && DSO->GetClass()->HasAnyClassFlags(CLASS_DefaultToInstanced))
		{
			// Determine if this is an editor-only subobject.
			bool bIsEditorOnlySubobject = false;
			if (const UActorComponent* ActorComponent = Cast<UActorComponent>(DSO))
			{
				bIsEditorOnlySubobject = ActorComponent->IsEditorOnly();
			}

			// Skip ctor code gen for editor-only subobjects, since they won't be used by the runtime. Any dependencies on editor-only subobjects will be handled later (see HandleInstancedSubobject).
			if (!bIsEditorOnlySubobject)
			{
				const FString VariableName = HandleInstancedSubobject(Context, DSO, false, true);

				// Keep track of which component can be used as a root, in case it's not explicitly set.
				if (NativeRootComponentFallback.IsEmpty())
				{
					USceneComponent* SceneComponent = Cast<USceneComponent>(DSO);
					if (SceneComponent && !SceneComponent->AttachParent && SceneComponent->CreationMethod == EComponentCreationMethod::Native)
					{
						NativeRootComponentFallback = VariableName;
					}
				}
			}
		}
	}

	// Check for a valid RootComponent property value; mark it as handled if already set in the defaults.
	bool bNeedsRootComponentAssignment = false;
	static const FName RootComponentPropertyName(TEXT("RootComponent"));
	const UObjectProperty* RootComponentProperty = FindField<UObjectProperty>(BPGC, RootComponentPropertyName);
	if (RootComponentProperty)
	{
		if (RootComponentProperty->GetObjectPropertyValue_InContainer(CDO))
		{
			HandledProperties.Add(RootComponentProperty);
		}
		else if (!NativeRootComponentFallback.IsEmpty())
		{
			Context.AddLine(FString::Printf(TEXT("RootComponent = %s;"), *NativeRootComponentFallback));
			HandledProperties.Add(RootComponentProperty);
		}
		else
		{
			bNeedsRootComponentAssignment = true;
		}
	}

	// Generate ctor init code for the SCS node hierarchy (i.e. non-native components). SCS nodes may have dependencies on native DSOs, but not vice-versa.
	TArray<const UBlueprintGeneratedClass*> BPGCStack;
	const bool bErrorFree = UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(BPGC, BPGCStack);
	if (bErrorFree)
	{
		// Start at the base of the hierarchy so that dependencies are handled first.
		for (int32 i = BPGCStack.Num() - 1; i >= 0; --i)
		{
			if (BPGCStack[i]->SimpleConstructionScript)
			{
				for (auto Node : BPGCStack[i]->SimpleConstructionScript->GetRootNodes())
				{
					if (Node)
					{
						const FString VariableName = HandleNonNativeComponent(Context, Node, HandledProperties);

						if (i == 0 && bNeedsRootComponentAssignment && Node->ComponentTemplate && Node->ComponentTemplate->IsA<USceneComponent>() && !VariableName.IsEmpty())
						{
							Context.AddLine(FString::Printf(TEXT("RootComponent = %s;"), *VariableName));

							bNeedsRootComponentAssignment = false;
							HandledProperties.Add(RootComponentProperty);
						}
					}
				}
			}
		}
	}

	// Generate ctor init code for generated Blueprint class property values that may differ from parent class defaults (or that otherwise belong to the generated Blueprint class).
	for (auto Property : TFieldRange<const UProperty>(BPGC))
	{
		const bool bNewProperty = Property->GetOwnerStruct() == BPGC;
		const bool bIsAccessible = bNewProperty || !Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPrivate);
		if (bIsAccessible && !HandledProperties.Contains(Property))
		{
			OuterGenerate(Context, Property, TEXT(""), reinterpret_cast<const uint8*>(CDO), bNewProperty ? nullptr : reinterpret_cast<const uint8*>(ParentCDO), EPropertyAccessOperator::None, true);
		}
	}

	Context.DecreaseIndent();
	Context.AddLine(TEXT("}"));
	Context.DecreaseIndent();
	Context.AddLine(TEXT("}"));
	Context.bInsideConstructor = false;

	FDependenciesHelper::AddStaticFunctionsForDependencies(Context, Dependencies);

	FBackendHelperUMG::EmitWidgetInitializationFunctions(Context);

	return Context.GetResult();
}

FString FEmitDefaultValueHelper::HandleClassSubobject(FEmitterLocalContext& Context, UObject* Object)
{
	FString OuterStr;
	if (!Context.FindLocalObject_InConstructor(Object->GetOuter(), OuterStr))
	{
		return FString();
	}

	const bool AddAsSubobjectOfClass = Object->GetOuter() == Context.GetCurrentlyGeneratedClass();
	const FString LocalNativeName = Context.AddNewObject_InConstructor(Object, AddAsSubobjectOfClass);
	UClass* ObjectClass = Object->GetClass();
	Context.AddHighPriorityLine(FString::Printf(
		TEXT("auto %s = NewObject<%s%s>(%s, TEXT(\"%s\"));")
		, *LocalNativeName
		, *FEmitHelper::GetCppName(ObjectClass)
		, *OuterStr
		, *Object->GetName()));
	if (AddAsSubobjectOfClass)
	{
		ensure(Context.bCreatingObjectsPerClass);
		Context.AddHighPriorityLine(FString::Printf(TEXT("GetClass()->ConvertedSubobjectsFromBPGC.Add(%s);"), *LocalNativeName));
	}

	for (auto Property : TFieldRange<const UProperty>(ObjectClass))
	{
		OuterGenerate(Context, Property, LocalNativeName
			, reinterpret_cast<const uint8*>(Object)
			, reinterpret_cast<const uint8*>(ObjectClass->GetDefaultObject(false))
			, EPropertyAccessOperator::Pointer);
	}

	return LocalNativeName;
}

FString FEmitDefaultValueHelper::HandleInstancedSubobject(FEmitterLocalContext& Context, UObject* Object, bool bCreateInstance, bool bSkipEditorOnlyCheck)
{
	check(Object);

	// Make sure we don't emit initialization code for the same object more than once.
	FString LocalNativeName;
	if (Context.FindLocalObject_InConstructor(Object, LocalNativeName))
	{
		return LocalNativeName;
	}
	else
	{
		LocalNativeName = Context.AddNewObject_InConstructor(Object, false);
	}

	UClass* ObjectClass = Object->GetClass();

	// Determine if this is an editor-only subobject. When handling as a dependency, we'll create a "dummy" object in its place (below).
	bool bIsEditorOnlySubobject = false;
	if (!bSkipEditorOnlyCheck)
	{
		if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
		{
			bIsEditorOnlySubobject = ActorComponent->IsEditorOnly();
			if (bIsEditorOnlySubobject)
			{
				// Replace the potentially editor-only class with a base actor/scene component class that's available to the runtime. We'll create a "dummy" object of this type to stand in for the editor-only subobject below.
				ObjectClass = ObjectClass->IsChildOf<USceneComponent>() ? USceneComponent::StaticClass() : UActorComponent::StaticClass();
			}
		}
	}

	auto BPGC = Context.GetCurrentlyGeneratedClass();
	auto CDO = BPGC ? BPGC->GetDefaultObject(false) : nullptr;
	if (!bIsEditorOnlySubobject && ensure(CDO) && (CDO == Object->GetOuter()))
	{
		if (bCreateInstance)
		{
			Context.AddHighPriorityLine(FString::Printf(TEXT("auto %s = CreateDefaultSubobject<%s>(TEXT(\"%s\"));")
				, *LocalNativeName, *FEmitHelper::GetCppName(ObjectClass), *Object->GetName()));
		}
		else
		{
			Context.AddHighPriorityLine(FString::Printf(TEXT("auto %s = CastChecked<%s>(GetDefaultSubobjectByName(TEXT(\"%s\")));")
				, *LocalNativeName, *FEmitHelper::GetCppName(ObjectClass), *Object->GetName()));
		}

		const UObject* ObjectArchetype = Object->GetArchetype();
		for (auto Property : TFieldRange<const UProperty>(ObjectClass))
		{
			OuterGenerate(Context, Property, LocalNativeName
				, reinterpret_cast<const uint8*>(Object)
				, reinterpret_cast<const uint8*>(ObjectArchetype)
				, EPropertyAccessOperator::Pointer);
		}
	}
	else
	{
		FString OuterStr;
		if (!Context.FindLocalObject_InConstructor(Object->GetOuter(), OuterStr))
		{
			return FString();
		}
		Context.AddHighPriorityLine(FString::Printf(TEXT("auto %s = NewObject<%s>(%s, TEXT(\"%s\"));")
			, *LocalNativeName
			, *FEmitHelper::GetCppName(ObjectClass)
			, *OuterStr
			, *Object->GetName()));
	}

	return LocalNativeName;
}