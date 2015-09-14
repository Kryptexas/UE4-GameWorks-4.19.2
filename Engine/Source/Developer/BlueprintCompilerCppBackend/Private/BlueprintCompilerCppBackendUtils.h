// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma  once

#include "BlueprintCompilerCppBackendGatherDependencies.h"

struct FEmitterLocalContext
{
	enum class EClassSubobjectList
	{
		ComponentTemplates,
		Timelines,
		DynamicBindingObjects,
		MiscConvertedSubobjects,
	};

	enum class EGeneratedCodeType
	{
		SubobjectsOfClass,
		CommonConstructor,
		Regular,
	};

	EGeneratedCodeType CurrentCodeType;

private:
	// TEXT
	FString Indent;
	FString Result;
	int32 LocalNameIndexMax;
	TArray<FString> HighPriorityLines;
	TArray<FString> LowPriorityLines;
	UClass* ActualClass;

	//ConstructorOnly Local Names
	TMap<UObject*, FString> ClassSubobjectsMap;
	//ConstructorOnly Local Names
	TMap<UObject*, FString> CommonSubobjectsMap;

	TArray<UObject*> MiscConvertedSubobjects;
	TArray<UObject*> DynamicBindingObjects;
	TArray<UObject*> ComponentTemplates;
	TArray<UObject*> Timelines;

public:

	const FGatherConvertedClassDependencies& Dependencies;

	FEmitterLocalContext(UClass* InClass, const FGatherConvertedClassDependencies& InDependencies)
		: CurrentCodeType(EGeneratedCodeType::Regular)
		, LocalNameIndexMax(0)
		, ActualClass(InClass)
		, Dependencies(InDependencies)
	{}

	UClass* GetActualClass() const
	{
		return ActualClass;
	}

	// CONSTRUCTOR FUNCTIONS

	const TCHAR* ClassSubobjectListName(EClassSubobjectList ListType)
		{
		if (ListType == EClassSubobjectList::ComponentTemplates)
		{
			return TEXT("ComponentTemplates");
		}
		if (ListType == EClassSubobjectList::Timelines)
		{
			return TEXT("Timelines");
		}
		if (ListType == EClassSubobjectList::DynamicBindingObjects)
		{
			return TEXT("DynamicBindingObjects");
		}
		return TEXT("MiscConvertedSubobjects");
		}

	void RegisterClassSubobject(UObject* Object, EClassSubobjectList ListType)
	{
		if (ListType == EClassSubobjectList::ComponentTemplates)
		{
			ComponentTemplates.Add(Object);
	}
		else if (ListType == EClassSubobjectList::Timelines)
	{
			Timelines.Add(Object);
	}
		else if(ListType == EClassSubobjectList::DynamicBindingObjects)
	{
			DynamicBindingObjects.Add(Object);
		}
		else
		{
			MiscConvertedSubobjects.Add(Object);
		}
		}

	void AddClassSubObject_InConstructor(UObject* Object, const FString& NativeName)
	{
		ensure(CurrentCodeType == EGeneratedCodeType::SubobjectsOfClass);
		ClassSubobjectsMap.Add(Object, NativeName);
	}

	void AddCommonSubObject_InConstructor(UObject* Object, const FString& NativeName)
	{
		ensure(CurrentCodeType == EGeneratedCodeType::CommonConstructor);
		ClassSubobjectsMap.Add(Object, NativeName);
	}

	// UNIVERSAL FUNCTIONS

	FString GenerateGetProperty(const UProperty* Property) const;

	FString GenerateUniqueLocalName();

	UClass* GetCurrentlyGeneratedClass() const
	{
		return ActualClass;
	}

	/** All objects (that can be referenced from other package) that will have a different path in cooked build
	(due to the native code generation), should be handled by this function */
	FString FindGloballyMappedObject(UObject* Object, bool bLoadIfNotFound = false);

	// TEXT FUNCTIONS

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
	static FString GetCppName(const UField* Field);

	static void ArrayToString(const TArray<FString>& Array, FString& OutString, const TCHAR* Separator);

	static bool HasAllFlags(uint64 Flags, uint64 FlagsToCheck);

	static FString HandleRepNotifyFunc(const UProperty* Property);

	static bool MetaDataCanBeNative(const FName MetaDataName);

	static FString HandleMetaData(const UField* Field, bool AddCategory = true, TArray<FString>* AdditinalMetaData = nullptr);

	static TArray<FString> ProperyFlagsToTags(uint64 Flags, bool bIsClassProperty);

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

	static FString GenerateConstructor(FEmitterLocalContext& Context);

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
	static FString HandleClassSubobject(FEmitterLocalContext& Context, UObject* Object, FEmitterLocalContext::EClassSubobjectList ListOfSubobjectsTyp);

private:
	// Returns native term, 
	// returns empty string if cannot handle
	static FString HandleSpecialTypes(FEmitterLocalContext& Context, const UProperty* Property, const uint8* ValuePtr);

	static FString HandleNonNativeComponent(FEmitterLocalContext& Context, const USCS_Node* Node, TSet<const UProperty*>& OutHandledProperties, TArray<FString>& NativeCreatedComponentProperties, const USCS_Node* ParentNode = nullptr);

	static FString HandleInstancedSubobject(FEmitterLocalContext& Context, UObject* Object, bool bCreateInstance = true, bool bSkipEditorOnlyCheck = false);
};

struct FBackendHelperUMG
{
	static FString WidgetFunctionsInHeader(UClass* SourceClass);

	static FString AdditionalHeaderIncludeForWidget(UClass* SourceClass);

	// these function should use the same context as Constructor
	static void CreateClassSubobjects(FEmitterLocalContext& Context);
	static void EmitWidgetInitializationFunctions(FEmitterLocalContext& Context);

};