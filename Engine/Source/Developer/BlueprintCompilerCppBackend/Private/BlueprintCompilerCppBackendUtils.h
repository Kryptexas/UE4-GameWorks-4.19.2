// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma  once

#include "BlueprintCompilerCppBackendGatherDependencies.h"

struct FEmitterLocalContext
{
private:
	FString Indent;
	FString Result;
	int32 LocalNameIndexMax;
	TArray<FString> HighPriorityLines;
	TArray<FString> LowPriorityLines;
	UClass* ActualClass;

	//ConstructorOnly
	TMap<UObject*, FString> NativeObjectNamesInConstructor;
	TArray<UObject*> ObjectsCreatedPerClass;

	const FGatherConvertedClassDependencies& Dependencies;

public:

	bool bCreatingObjectsPerClass;
	bool bInsideConstructor;

	FEmitterLocalContext(UClass* InClass, const FGatherConvertedClassDependencies& InDependencies)
		: LocalNameIndexMax(0)
		, ActualClass(InClass)
		, Dependencies(InDependencies)
		, bCreatingObjectsPerClass(false)
		, bInsideConstructor(false)
	{}

	// Functions to use in constructor only
	bool FindLocalObject_InConstructor(UObject* Object, FString& OutNamePath) const
	{
		ensure(bInsideConstructor);
		if (Object == ActualClass)
		{
			OutNamePath = TEXT("GetClass()");
			return true;
		}

		if (const FString* NamePtr = NativeObjectNamesInConstructor.Find(Object))
		{
			OutNamePath = *NamePtr;
			return true;
		}

		return false;
	}

	void AddObjectFromLocalProperty_InConstructor(UObject* Object, const FString& NativeName)
	{
		ensure(bInsideConstructor);
		NativeObjectNamesInConstructor.Add(Object, NativeName);
	}

	FString AddNewObject_InConstructor(UObject* Object, bool bAddToObjectsCreatedPerClass)
	{
		ensure(bInsideConstructor);
		const FString UniqueName = GenerateUniqueLocalName();
		NativeObjectNamesInConstructor.Add(Object, UniqueName);
		if (bAddToObjectsCreatedPerClass)
		{
			ensure(bCreatingObjectsPerClass);
			check(!ObjectsCreatedPerClass.Contains(Object));
			ObjectsCreatedPerClass.Add(Object);
		}

		return UniqueName;
	}

	const TArray<UObject*>& GetObjectsCreatedPerClass_InConstructor()
	{
		ensure(bInsideConstructor);
		return ObjectsCreatedPerClass;
	}

	// Universal functions
	FString GenerateGetProperty(const UProperty* Property) const
	{
		check(Property);
		const UStruct* OwnerStruct = Property->GetOwnerStruct();
		const FString OwnerName = FString(OwnerStruct->GetPrefixCPP()) + OwnerStruct->GetName();
		const FString OwnerPath = OwnerName + (OwnerStruct->IsA<UClass>() ? TEXT("::StaticClass()") : TEXT("::StaticStruct()"));
		//return FString::Printf(TEXT("%s->FindPropertyByName(GET_MEMBER_NAME_CHECKED(%s, %s))"), *OwnerPath, *OwnerName, *Property->GetNameCPP());

		return FString::Printf(TEXT("%s->FindPropertyByName(FName(TEXT(\"%s\")))"), *OwnerPath, *Property->GetNameCPP());
	}

	FString GenerateUniqueLocalName()
	{
		const FString UniqueNameBase = TEXT("__Local__");
		const FString UniqueName = FString::Printf(TEXT("%s%d"), *UniqueNameBase, LocalNameIndexMax);
		++LocalNameIndexMax;
		return UniqueName;
	}

	UClass* GetCurrentlyGeneratedClass() const
	{
		return ActualClass;
	}

	/** All objects (that can be referenced from other package) that will have a different path in cooked build 
	(due to the native code generation), should be handled by this function */
	FString FindGloballyMappedObject(UObject* Object, bool bLoadIfNotFound = false)
	{
		// TODO: check if not excluded

		if (ActualClass && (Object == ActualClass))
		{
			return TEXT("GetClass()");
		}

		if (auto ObjClass = Cast<UClass>(Object))
		{
			auto BPGC = Cast<UBlueprintGeneratedClass>(ObjClass);
			if (ObjClass->HasAnyClassFlags(CLASS_Native) || (BPGC && Dependencies.WillClassBeConverted(BPGC)))
			{
				return FString::Printf(TEXT("%s%s::StaticClass()"), ObjClass->GetPrefixCPP(), *ObjClass->GetName());
			}
		}

		// TODO Handle native structires, and special cases..

		if (auto UDS = Cast<UScriptStruct>(Object))
		{
			// Check if  
			// TODO: check if supported 
			return FString::Printf(TEXT("%s%s::StaticStruct()"), UDS->GetPrefixCPP(), *UDS->GetName());
		}

		if (auto UDE = Cast<UEnum>(Object))
		{
			// TODO:
			// TODO: check if supported 
			//return FString::Printf(TEXT("%s_StaticEnum()"), *UDE->GetName());
			return FString::Printf(TEXT("FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT(\"%s\"))"), *UDE->GetName());
		}

		int32 ObjectsCreatedPerClassIdx = INDEX_NONE;
		if (ActualClass && Object && ObjectsCreatedPerClass.Find(Object, ObjectsCreatedPerClassIdx))
		{
			return FString::Printf(TEXT("CastChecked<%s%s>(%s%s::StaticClass()->ConvertedSubobjectsFromBPGC[%d])")
				, Object->GetClass()->GetPrefixCPP()
				, *Object->GetClass()->GetName()
				, ActualClass->GetPrefixCPP()
				, *ActualClass->GetName()
				, ObjectsCreatedPerClassIdx);
		}

		// TODO: handle subobjects

		if (bLoadIfNotFound && ensure(Object))
		{
			UClass* FoundClass = Object->GetClass();
			const FString ClassString = FString(FoundClass->GetPrefixCPP()) + FoundClass->GetName();
			return FString::Printf(TEXT("LoadObject<%s>(nullptr, TEXT(\"%s\"))"), *ClassString, *(Object->GetPathName().ReplaceCharWithEscapedChar()));
		}

		return FString{};
	}

	// TEXT
	void IncreaseIndent()
	{
		Indent += TEXT("\t");
	}

	void DecreaseIndent()
	{
		Indent.RemoveFromEnd(TEXT("\t"));
	}

	void AddHighPriorityLine(const FString& Line)
	{
		HighPriorityLines.Emplace(FString::Printf(TEXT("%s%s\n"), *Indent, *Line));
	}

	void AddLine(const FString& Line)
	{
		LowPriorityLines.Emplace(FString::Printf(TEXT("%s%s\n"), *Indent, *Line));
	}

	void FlushLines()
	{
		for (auto& Line : HighPriorityLines)
		{
			Result += Line;
		}
		HighPriorityLines.Reset();

		for (auto& Line : LowPriorityLines)
		{
			Result += Line;
		}
		LowPriorityLines.Reset();
	}

	FString GetResult()
	{
		FlushLines();
		return Result;
	}

	void ClearResult()
	{
		FlushLines();
		Result.Reset();
	}
};

struct FEmitHelper
{
	static void ArrayToString(const TArray<FString>& Array, FString& OutString, const TCHAR* Separator);

	static bool HasAllFlags(uint64 Flags, uint64 FlagsToCheck);

	static FString HandleRepNotifyFunc(const UProperty* Property);

	static bool MetaDataCanBeNative(const FName MetaDataName);

	static FString HandleMetaData(const UField* Field, bool AddCategory = true, TArray<FString>* AdditinalMetaData = nullptr);

	static TArray<FString> ProperyFlagsToTags(uint64 Flags);

	static TArray<FString> FunctionFlagsToTags(uint64 Flags);

	static bool IsBlueprintNativeEvent(uint64 FunctionFlags);

	static bool IsBlueprintImplementableEvent(uint64 FunctionFlags);

	static FString EmitUFuntion(UFunction* Function, TArray<FString>* AdditinalMetaData = nullptr);

	static int32 ParseDelegateDetails(UFunction* Signature, FString& OutParametersMacro, FString& OutParamNumberStr);

	static TArray<FString> EmitSinglecastDelegateDeclarations(const TArray<UDelegateProperty*>& Delegates);

	static TArray<FString> EmitMulticastDelegateDeclarations(UClass* SourceClass);

	static FString EmitLifetimeReplicatedPropsImpl(UClass* SourceClass, const FString& CppClassName, const TCHAR* InCurrentIndent);

	static FString LiteralTerm(FEmitterLocalContext& EmitterContext, const FEdGraphPinType& Type, const FString& CustomValue, UObject* LiteralObject);

	static FString DefaultValue(FEmitterLocalContext& EmitterContext, const FEdGraphPinType& Type);

	static UFunction* GetOriginalFunction(UFunction* Function);

	static bool ShouldHandleAsNativeEvent(UFunction* Function);

	static bool ShouldHandleAsImplementableEvent(UFunction* Function);

	static bool GenerateAutomaticCast(const FEdGraphPinType& LType, const FEdGraphPinType& RType, FString& OutCastBegin, FString& OutCastEnd);

	static FString GenerateReplaceConvertedMD(UObject* Obj);
};

struct FEmitDefaultValueHelper
{
	static FString GenerateGetDefaultValue(const UUserDefinedStruct* Struct, const FGatherConvertedClassDependencies& Dependencies);

	static FString GenerateConstructor(UClass* BPGC, const FGatherConvertedClassDependencies& Dependencies);

	enum class EPropertyAccessOperator
	{
		None, // for self scope, this
		Pointer,
		Dot,
	};

	// OuterPath ends context/outer name (or empty, if the scope is "this")
	static void OuterGenerate(FEmitterLocalContext& Context, const UProperty* Property, const FString& OuterPath, const uint8* DataContainer, const uint8* OptionalDefaultDataContainer, EPropertyAccessOperator AccessOperator, bool bAllowProtected = false);


	// PathToMember ends with variable name
	static void InnerGenerate(FEmitterLocalContext& Context, const UProperty* Property, const FString& PathToMember, const uint8* ValuePtr, const uint8* DefaultValuePtr, bool bWithoutFirstConstructionLine = false);

	// Creates the subobject (of class) returns it's native local name, 
	// returns empty string if cannot handle
	static FString HandleClassSubobject(FEmitterLocalContext& Context, UObject* Object);

private:
	// Returns native term, 
	// returns empty string if cannot handle
	static FString HandleSpecialTypes(FEmitterLocalContext& Context, const UProperty* Property, const uint8* ValuePtr);

	static FString HandleNonNativeComponent(FEmitterLocalContext& Context, const USCS_Node* Node, TSet<const UProperty*>& OutHandledProperties, const USCS_Node* ParentNode = nullptr);

	static FString HandleInstancedSubobject(FEmitterLocalContext& Context, UObject* Object, bool bSkipEditorOnlyCheck = false);
};

struct FBackendHelperUMG
{
	static FString WidgetFunctionsInHeader(UClass* SourceClass);

	// these function should use the same context as Constructor
	static void CreateClassSubobjects(FEmitterLocalContext& Context);
	static void EmitWidgetInitializationFunctions(FEmitterLocalContext& Context);

};