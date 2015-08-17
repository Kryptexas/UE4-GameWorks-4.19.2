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
	if (Property->HasAnyPropertyFlags(CPF_EditorOnly | CPF_Transient))
	{
		UE_LOG(LogK2Compiler, Log, TEXT("FEmitDefaultValueHelper Skip EditorOnly or Transient property: %s"), *Property->GetPathName());
		return;
	}

	for (int32 ArrayIndex = 0; ArrayIndex < Property->ArrayDim; ++ArrayIndex)
	{
		if (!OptionalDefaultDataContainer || !Property->Identical_InContainer(DataContainer, OptionalDefaultDataContainer, ArrayIndex))
		{
			FString PathToMember;
			if (Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPrivate) || (!bAllowProtected && Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierProtected)))
			{
				ensure(EPropertyAccessOperator::None != AccessOperator);
				
				const FString PropertyObject = Context.GenerateGetProperty(Property); //X::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(X, X))

				FStringOutputDevice TypeDeclaration;
				const uint32 CppTemplateTypeFlags = EPropertyExportCPPFlags::CPPF_CustomTypeName
					| EPropertyExportCPPFlags::CPPF_NoConst | EPropertyExportCPPFlags::CPPF_NoRef
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
				PathToMember = FString::Printf(TEXT("%s%s%s%s"), *OuterPath, *AccessOperatorStr, *Property->GetNameCPP(), *ArrayPost);
			}
			const uint8* ValuePtr = Property->ContainerPtrToValuePtr<uint8>(DataContainer, ArrayIndex);
			const uint8* DefaultValuePtr = OptionalDefaultDataContainer ? Property->ContainerPtrToValuePtr<uint8>(OptionalDefaultDataContainer, ArrayIndex) : nullptr;
			InnerGenerate(Context, Property, PathToMember, ValuePtr, DefaultValuePtr);
		}
	}
}

FString FEmitDefaultValueHelper::GenerateGetDefaultValue(const UUserDefinedStruct* Struct)
{
	check(Struct);
	const FString StructName = FString(TEXT("F")) + Struct->GetName();
	FString Result = FString::Printf(TEXT("\tstatic %s GetDefaultValue()\n\t{\n\t\t%s DefaultData__;\n"), *StructName, *StructName);

	FStructOnScope StructData(Struct);
	FStructureEditorUtils::Fill_MakeStructureDefaultValue(Struct, StructData.GetStructMemory());

	FEmitterLocalContext Context;
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
					ValueStr = FString::Printf(TEXT("F%s{}"), *StructProperty->Struct->GetName()); //don;t override existing values
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
				const FString CreateAsInstancedSubobject = HandleInstancedSubobject(Context, Object);
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

UActorComponent* FEmitDefaultValueHelper::HandleNonNativeComponent(FEmitterLocalContext& Context, UBlueprintGeneratedClass* BPGC, FName ObjectName, bool bNew, const FString& NativeName)
{
	USCS_Node* Node = nullptr;
	for (UBlueprintGeneratedClass* NodeOwner = BPGC; NodeOwner && !Node; NodeOwner = Cast<UBlueprintGeneratedClass>(NodeOwner->GetSuperClass()))
	{
		Node = NodeOwner->SimpleConstructionScript ? NodeOwner->SimpleConstructionScript->FindSCSNode(ObjectName) : nullptr;
	}

	if (!Node)
	{
		return nullptr;
	}

	ensure(bNew == (Node->GetOuter() == BPGC->SimpleConstructionScript));
	UActorComponent* ComponentTemplate = bNew
		? Node->ComponentTemplate
		: (BPGC->InheritableComponentHandler ? BPGC->InheritableComponentHandler->GetOverridenComponentTemplate(Node) : nullptr);

	if (!ComponentTemplate)
	{
		return nullptr;
	}

	auto ComponentClass = ComponentTemplate->GetClass();
	if (bNew)
	{
		Context.AddHighPriorityLine(FString::Printf(TEXT("%s = CreateDefaultSubobject<%s%s>(TEXT(\"%s\"));")
			, *NativeName, ComponentClass->GetPrefixCPP(), *ComponentClass->GetName(), *ComponentTemplate->GetName()));
		Context.AddObjectFromLocalProperty_InConstructor(ComponentTemplate, NativeName);
	}

	UObject* ParentTemplateComponent = bNew 
		? nullptr 
		: Node->GetActualComponentTemplate(Cast<UBlueprintGeneratedClass>(BPGC->GetSuperClass()));
	ensure(bNew || ParentTemplateComponent);
	ensure(ParentTemplateComponent != ComponentTemplate);
	ensure(!ParentTemplateComponent || (ParentTemplateComponent->GetClass() == ComponentClass));

	const UObject* ObjectToCompare = bNew
		? ComponentClass->GetDefaultObject(false)
		: ParentTemplateComponent;
	for (auto Property : TFieldRange<const UProperty>(ComponentClass))
	{
		OuterGenerate(Context, Property, NativeName
			, reinterpret_cast<const uint8*>(ComponentTemplate)
			, reinterpret_cast<const uint8*>(ObjectToCompare)
			, EPropertyAccessOperator::Pointer);
	}

	auto FindParentComponentName = [&]() -> FString
	{
		FString ResultName;
		if (auto ParentNode = Node->GetSCS()->FindParentNode(Node))
		{
			const FName ParentVariableName = ParentNode->GetVariableName();
			ResultName = (ParentVariableName != NAME_None) ? ParentVariableName.ToString() : FString();
		}

		if (ResultName.IsEmpty())
		{
			auto ParentTemplate = Node->GetParentComponentTemplate(CastChecked<UBlueprint>(BPGC->ClassGeneratedBy));
			FString ParentComponentNativeName;
			if (ParentTemplate && Context.FindLocalObject_InConstructor(ParentTemplateComponent, ParentComponentNativeName))
			{
				ResultName = ParentComponentNativeName;
			}
		}

		if (ResultName.IsEmpty())
		{
			ResultName = (Node->ParentComponentOrVariableName != NAME_None) ? Node->ParentComponentOrVariableName.ToString() : FString();
		}

		return ResultName;
	};

	const FString ParentNodeName = FindParentComponentName();
	if (!ParentNodeName.IsEmpty())
	{
		Context.AddLine(FString::Printf(TEXT("%s->AttachParent = %s;"), *NativeName, *ParentNodeName));
	}

	// TODO: Set root component
	// TODO: Update local transform, etc..

	return ComponentTemplate;
}

FString FEmitDefaultValueHelper::GenerateConstructor(UClass* InBPGC)
{
	auto BPGC = CastChecked<UBlueprintGeneratedClass>(InBPGC);
	const FString CppClassName = FString(BPGC->GetPrefixCPP()) + BPGC->GetName();

	FEmitterLocalContext Context;
	Context.SetCurrentlyGeneratedClass(BPGC);
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

		Context.bCreatingObjectsPerClass = false;
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
			Context.AddHighPriorityLine(FString::Printf(TEXT("auto %s = CastChecked<%s%s>(GetClass()->ConvertedSubobjectsFromBPGC[%d]);")
				, *NativeName
				, Obj->GetClass()->GetPrefixCPP()
				, *Obj->GetClass()->GetName()
				, SubobjectIndex));
			SubobjectIndex++;
		}
	}

	UObject* CDO = BPGC->GetDefaultObject(false);
	Context.AddObjectFromLocalProperty_InConstructor(CDO, TEXT("this"));

	UObject* ParentCDO = BPGC->GetSuperClass()->GetDefaultObject(false);
	check(CDO && ParentCDO);
	Context.AddLine(TEXT(""));

	// TIMELINES
	TSet<const UProperty*> HandledProperties;
	const bool bIsActor = BPGC->IsChildOf<AActor>();

	// COMPONENTS
	for (auto Property : TFieldRange<const UProperty>(BPGC))
	{
		const bool bNewProperty = Property->GetOwnerStruct() == BPGC;
		const bool bIsAccessible = bNewProperty || !Property->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPrivate);
		if (bIsAccessible && !HandledProperties.Contains(Property))
		{
			auto ObjectProperty = Cast<UObjectProperty>(Property);
			const bool bComponentProp = ObjectProperty && ObjectProperty->PropertyClass && ObjectProperty->PropertyClass->IsChildOf<UActorComponent>();
			const bool bNullValue = ObjectProperty && (nullptr == ObjectProperty->GetPropertyValue_InContainer(CDO));
			const bool bActorComponent = bIsActor && bComponentProp && bNullValue;
			if (bIsActor && bComponentProp && bNullValue)
			{
				auto HandledComponent = HandleNonNativeComponent(Context, BPGC, Property->GetFName(), bNewProperty, Property->GetNameCPP());
				if (HandledComponent)
				{
					ensure(!ActorComponentTempatesOwnedByClass.Contains(HandledComponent));
					continue;
				}
			}

			OuterGenerate(Context, Property, TEXT(""), reinterpret_cast<const uint8*>(CDO), bNewProperty ? nullptr : reinterpret_cast<const uint8*>(ParentCDO), EPropertyAccessOperator::None, true);
		}
	}

	Context.DecreaseIndent();
	Context.AddLine(TEXT("}"));
	Context.DecreaseIndent();
	Context.AddLine(TEXT("}"));
	Context.bInsideConstructor = false;
	return Context.GetResult();
}

FString FEmitDefaultValueHelper::HandleClassSubobject(FEmitterLocalContext& Context, UObject* Object)
{
	FString OuterStr;
	if (!Context.FindLocalObject_InConstructor(Object->GetOuter(), OuterStr))
	{
		return FString();
	}

	const FString LocalNativeName = Context.AddNewObject_InConstructor(Object);
	UClass* ObjectClass = Object->GetClass();
	Context.AddHighPriorityLine(FString::Printf(
		TEXT("auto %s = NewObject<%s%s>(%s, TEXT(\"%s\"));")
		, *LocalNativeName
		, ObjectClass->GetPrefixCPP()
		, *ObjectClass->GetName()
		, *OuterStr, *Object->GetName()));
	if (Object->GetOuter() == Context.GetCurrentlyGeneratedClass())
	{
		Context.AddLine(FString::Printf(TEXT("GetClass()->ConvertedSubobjectsFromBPGC.Add(%s);"), *LocalNativeName));
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

FString FEmitDefaultValueHelper::HandleInstancedSubobject(FEmitterLocalContext& Context, UObject* Object)
{
	check(Object);

	const FString LocalNativeName = Context.AddNewObject_InConstructor(Object);
	UClass* ObjectClass = Object->GetClass();

	auto BPGC = Context.GetCurrentlyGeneratedClass();
	auto CDO = BPGC ? BPGC->GetDefaultObject(false) : nullptr;
	if (ensure(CDO) && (CDO == Object->GetOuter()))
	{
		Context.AddHighPriorityLine(FString::Printf(TEXT("auto %s = CreateDefaultSubobject<%s%s>(TEXT(\"%s\"));")
			, *LocalNativeName, ObjectClass->GetPrefixCPP(), *ObjectClass->GetName(), *Object->GetName()));
	}
	else
	{
		FString OuterStr;
		if (!Context.FindLocalObject_InConstructor(Object->GetOuter(), OuterStr))
		{
			return FString();
		}
		Context.AddHighPriorityLine(FString::Printf(TEXT("auto %s = NewObject<%s%s>(%s, TEXT(\"%s\"));")
			, *LocalNativeName, ObjectClass->GetPrefixCPP(), *ObjectClass->GetName()
			, *OuterStr, *Object->GetName()));
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