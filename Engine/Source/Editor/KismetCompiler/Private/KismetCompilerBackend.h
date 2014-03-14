// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __KismetCompilerBackend_h__
#define __KismetCompilerBackend_h__

#pragma once

#include "KismetCompiler.h"

//////////////////////////////////////////////////////////////////////////
// IKismetCompilerBackend

class IKismetCompilerBackend
{
};

//////////////////////////////////////////////////////////////////////////
// FKismetCppBackend

class FKismetCppBackend : public IKismetCompilerBackend
{
protected:
	UEdGraphSchema_K2* Schema;
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

	// Pointers to commonly used structures (found in constructor)
	UScriptStruct* VectorStruct;
	UScriptStruct* RotatorStruct;
	UScriptStruct* TransformStruct;
	UScriptStruct* LatentInfoStruct;
public:
	FStringOutputDevice Header;
	FStringOutputDevice Body;
protected:
	FString TermToText(FBPTerminal* Term, UProperty* SourceProperty = NULL);
	FString LatentFunctionInfoTermToText(FBPTerminal* Term, FBlueprintCompiledStatement* TargetLabel);

	int32 StatementToStateIndex(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement* Statement)
	{
		int32 Index = FunctionIndexMap.FindChecked(&FunctionContext);
		return StateMapPerFunction[Index].StatementToStateIndex(Statement);
	}
public:

	FKismetCppBackend(UEdGraphSchema_K2* InSchema, FKismetCompilerContext& InContext)
		: Schema(InSchema)
		, MessageLog(InContext.MessageLog)
		, CompilerContext(InContext)
	{
		extern UScriptStruct* Z_Construct_UScriptStruct_UObject_FVector();
		VectorStruct = Z_Construct_UScriptStruct_UObject_FVector();
		RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
		TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));
		LatentInfoStruct = FLatentActionInfo::StaticStruct();
	}

	void Emit(FStringOutputDevice& Target, const TCHAR* Message)
	{
		Target += Message;
	}

	void EmitClassProperties(FStringOutputDevice& Target, UClass* SourceClass);

	void GenerateCodeFromClass(UClass* SourceClass, TIndirectArray<FKismetFunctionContext>& Functions, bool bGenerateStubsOnly=false);

	void EmitCallStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitCallDelegateStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitAssignmentStatment(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
	void EmitCastToInterfaceStatement(FKismetFunctionContext& FunctionContext, FBlueprintCompiledStatement& Statement);
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
	
	/** Emits local variable declarations for a function */
	void DeclareLocalVariables(FKismetFunctionContext& FunctionContext, TArray<UProperty*>& LocalVariables);

	/** Emits the internal execution flow state declaration for a function */
	void DeclareStateSwitch(FKismetFunctionContext& FunctionContext);
	void CloseStateSwitch(FKismetFunctionContext& FunctionContext);

	/** Builds both the header declaration and body implementation of a function */
	void ConstructFunction(FKismetFunctionContext& FunctionContext, bool bGenerateStubOnly);
};

//////////////////////////////////////////////////////////////////////////
// FKismetVMBackend

class FKismetCompilerVMBackend : public IKismetCompilerBackend
{
public:
	typedef TMap<FBlueprintCompiledStatement*, CodeSkipSizeType> TStatementToSkipSizeMap;
protected:
	UBlueprint* Blueprint;
	UEdGraphSchema_K2* Schema;
	FCompilerResultsLog& MessageLog;
	FKismetCompilerContext& CompilerContext;

	TStatementToSkipSizeMap UbergraphStatementLabelMap;
public:
	FKismetCompilerVMBackend(UBlueprint* InBlueprint, UEdGraphSchema_K2* InSchema, FKismetCompilerContext& InContext)
		: Blueprint(InBlueprint)
		, Schema(InSchema)
		, MessageLog(InContext.MessageLog)
		, CompilerContext(InContext)
	{
	}

	void GenerateCodeFromClass(UClass* SourceClass, TIndirectArray<FKismetFunctionContext>& Functions, bool bGenerateStubsOnly=false);

protected:
	/** Builds both the header declaration and body implementation of a function */
	void ConstructFunction(FKismetFunctionContext& FunctionContext, bool bIsUbergraph, bool bGenerateStubOnly);
};

//////////////////////////////////////////////////////////////////////////

#endif	// __KismetCompilerBackend_h__
