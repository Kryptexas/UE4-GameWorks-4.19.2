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

struct FEmitDefaultValueHelper
{
	static FString GenerateGetDefaultValue(const UUserDefinedStruct* Struct);

	static FString GenerateConstructor(UClass* BPGC);
private:

	static void OuterGenerate(const UProperty* Property, const uint8* DataContainer, const FString OuterPath, const uint8* OptionalDefaultDataContainer, FString& OutResult, bool bAllowProtected = false);
	static void InnerGenerate(const UProperty* Property, const uint8* ValuePtr, const FString& PathToMember, FString& OutResult);
	
	//Return if handled
	static bool HandleSpecialTypes(const UProperty* Property, const uint8* ValuePtr, FString& OutResult);
	static bool HandleNonNativeComponent(UBlueprintGeneratedClass* BPGC, FName Name, bool bNew, const FString MemberPath, FString& OutResult);
};