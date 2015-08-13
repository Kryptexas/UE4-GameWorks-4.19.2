// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma  once

class FBlueprintCompilerCppBackend;

// Generates single if scope. It's condition checks context of given term.
struct FSafeContextScopedEmmitter
{
private:
	FString& Body;
	bool bSafeContextUsed;
	FString CurrentIndent;
public:
	FString GetAdditionalIndent() const;
	bool IsSafeContextUsed() const;

	static FString ValidationChain(const FBPTerminal* Term, FBlueprintCompilerCppBackend& CppBackend);

	FSafeContextScopedEmmitter(FString& InBody, const FBPTerminal* Term, FBlueprintCompilerCppBackend& CppBackend, const TCHAR* InCurrentIndent);
	~FSafeContextScopedEmmitter();
};

struct FEmitHelper
{
	static void ArrayToString(const TArray<FString>& Array, FString& String, const TCHAR* Separator);

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

	static FString GatherNativeHeadersToInclude(UField* SourceItem, const TArray<FString>& PersistentHeaders);

	static FString LiteralTerm(const FEdGraphPinType& Type, const FString& CustomValue, UObject* LiteralObject);

	static FString DefaultValue(const FEdGraphPinType& Type);

	static UFunction* GetOriginalFunction(UFunction* Function);

	static bool ShoulsHandleAsNativeEvent(UFunction* Function);

	static bool ShoulsHandleAsImplementableEvent(UFunction* Function);

	static bool GenerateAssignmentCast(const FEdGraphPinType& LType, const FEdGraphPinType& RType, FString& OutCastBegin, FString& OutCastEnd);
};

struct FDefaultValueHelperContext
{
private:
	FString Indent;
	FString Result;
	int32 LocalNameIndexMax;
	TArray<FString> HighPriorityLines;
	TArray<FString> LowPriorityLines;
	UClass* ActualClass;

	TMap<UObject*, FString> NativeObjectNamesInConstructor;
	TArray<UObject*> ObjectsCreatedPerClass;


public:

	bool bCreatingObjectsPerClass;

	FDefaultValueHelperContext()
		: ActualClass(nullptr)
		, LocalNameIndexMax(0)
		, bCreatingObjectsPerClass(false)
	{}

	// Functions to use in constructor only
	bool FindLocalObject_InConstructor(UObject* Object, FString& OutNamePath) const
	{
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
		NativeObjectNamesInConstructor.Add(Object, NativeName);
	}

	FString AddNewObject_InConstructor(UObject* Object)
	{
		const FString UniqueName = GenerateUniqueLocalName();
		NativeObjectNamesInConstructor.Add(Object, UniqueName);
		if (bCreatingObjectsPerClass)
		{
			check(!ObjectsCreatedPerClass.Contains(Object));
			ObjectsCreatedPerClass.Add(Object);
		}

		return UniqueName;
	}

	const TArray<UObject*>& GetObjectsCreatedPerClass_InConstructor()
	{
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

	void SetCurrentlyGeneratedClass(UClass* InClass)
	{
		ActualClass = InClass;
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

	FString FindGlobalObject(UObject* Object)
	{
		// TODO: check if not excluded

		if (ActualClass && (Object == ActualClass))
		{
			return TEXT("GetClass()");
		}

		if (auto ObjClass = Cast<UClass>(Object))
		{
			if (ObjClass->HasAnyClassFlags(CLASS_Native))
			{
				return FString::Printf(TEXT("%s%s::StaticClass()"), ObjClass->GetPrefixCPP(), *ObjClass->GetName());
			}
		}

		// TODO Handle native structires, and special cases..

		if (auto BPGC = ExactCast<UBlueprintGeneratedClass>(Object))
		{
			// TODO: check if supported 
			return FString::Printf(TEXT("%s%s::StaticClass()"), BPGC->GetPrefixCPP(), *BPGC->GetName());
		}

		if (auto UDS = Cast<UScriptStruct>(Object))
		{
			// Check if  
			// TODO: check if supported 
			return FString::Printf(TEXT("%s%s::StaticStruct()"), UDS->GetPrefixCPP(), *UDS->GetName());
		}

		if (auto UDE = Cast<UEnum>(Object))
		{
			// TODO: check if supported 
			return FString::Printf(TEXT("%s_StaticEnum()"), *UDE->GetName());
		}

		// TODO: handle subobjects

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

struct FEmitDefaultValueHelper
{
	static FString GenerateGetDefaultValue(const UUserDefinedStruct* Struct);

	static FString GenerateConstructor(UClass* BPGC);
private:

	enum class EPropertyAccessOperator
	{
		None, // for self scope, this
		Pointer,
		Dot,
	};

	// OuterPath ends context/outer name (or empty, if the scope is "this")
	static void OuterGenerate(FDefaultValueHelperContext& Context, const UProperty* Property, const FString& OuterPath, const uint8* DataContainer, const uint8* OptionalDefaultDataContainer, EPropertyAccessOperator AccessOperator, bool bAllowProtected = false);
	
	// PathToMember ends with variable name
	static void InnerGenerate(FDefaultValueHelperContext& Context, const UProperty* Property, const FString& PathToMember, const uint8* ValuePtr, const uint8* DefaultValuePtr, bool bWithoutFirstConstructionLine = false);
	
	// Returns native term, 
	// returns empty string if cannot handle
	static FString HandleSpecialTypes(FDefaultValueHelperContext& Context, const UProperty* Property, const uint8* ValuePtr);

	static UActorComponent* HandleNonNativeComponent(FDefaultValueHelperContext& Context, UBlueprintGeneratedClass* BPGC, FName ObjectName, bool bNew, const FString& NativeName);
	
	// Creates the subobject (of class) returns it's native local name, 
	// returns empty string if cannot handle
	static FString HandleClassSubobject(FDefaultValueHelperContext& Context, UObject* Object);

	static FString HandleInstancedSubobject(FDefaultValueHelperContext& Context, UObject* Object);
};