// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma  once

#include "IBlueprintCompilerCppBackendModule.h"

class FBlueprintCompilerCppBackend : public IBlueprintCompilerCppBackend
{
protected:
	FCompilerResultsLog& MessageLog;
	FKismetCompilerContext& CompilerContext;

	struct FFunctionLabelInfo
	{
		TMap<FBlueprintCompiledStatement*, int32> StateMap;
		int32 StateCounter;

		FFunctionLabelInfo()
		{
			StateCounter = 0;
		}

		int32 StatementToStateIndex(FBlueprintCompiledStatement* Statement)
		{
			int32& Index = StateMap.FindOrAdd(Statement);
			if (Index == 0)
			{
				Index = ++StateCounter;
			}

			return Index;
		}
	};

	TArray<FFunctionLabelInfo> StateMapPerFunction;
	TMap<FKismetFunctionContext*, int32> FunctionIndexMap;

	FString CppClassName;

public:
	FStringOutputDevice Header;
	FStringOutputDevice Body;

	const FString& GetBody()	const override { return Body; }
	const FString& GetHeader()	const override { return Header; }

public:

	FBlueprintCompilerCppBackend(FKismetCompilerContext& InContext)
		: MessageLog(InContext.MessageLog)
		, CompilerContext(InContext)
	{
	}

	void Emit(FStringOutputDevice& Target, const TCHAR* Message)
	{
		Target += Message;
	}

	void EmitClassProperties(FStringOutputDevice& Target, UClass* SourceClass);

	void GenerateCodeFromClass(UClass* SourceClass, FString NewClassName, TIndirectArray<FKismetFunctionContext>& Functions, bool bGenerateStubsOnly) override;

	void EmitCallStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitCallDelegateStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitAssignmentStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitCastObjToInterfaceStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitCastBetweenInterfacesStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitCastInterfaceToObjStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitDynamicCastStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitObjectToBoolStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitAddMulticastDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitBindDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitClearMulticastDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitCreateArrayStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitGotoStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitPushStateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitEndOfThreadStatement(FKismetFunctionContext& FunctionContext, const FString& ReturnValueString);
	void EmitReturnStatement(FKismetFunctionContext& FunctionContext, const FString& ReturnValueString);
	void EmitRemoveMulticastDelegateStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitMetaCastStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);

	/** Emits local variable declarations for a function */
	void DeclareLocalVariables(FKismetFunctionContext& FunctionContext, TArray<UProperty*>& LocalVariables);

	/** Emits the internal execution flow state declaration for a function */
	void DeclareStateSwitch(FKismetFunctionContext& FunctionContext);
	void CloseStateSwitch(FKismetFunctionContext& FunctionContext);

	/** Builds both the header declaration and body implementation of a function */
	void ConstructFunction(FKismetFunctionContext& FunctionContext, bool bGenerateStubOnly);

	FString TermToText(const FBPTerminal* Term, const UProperty* SourceProperty = nullptr);

protected:

	FString LatentFunctionInfoTermToText(FBPTerminal* Term, FBlueprintCompiledStatement* TargetLabel);

	int32 StatementToStateIndex(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement* Statement)
	{
		int32 Index = FunctionIndexMap.FindChecked(&FunctionContext);
		return StateMapPerFunction[Index].StatementToStateIndex(Statement);
	}

	FString EmitMethodInputParameterList(FBlueprintCompiledStatement& Statement);

	FString EmitSwitchValueStatmentInner(FBlueprintCompiledStatement& Statement);

	FString EmitCallStatmentInner(FBlueprintCompiledStatement& Statement, bool bInline);
};