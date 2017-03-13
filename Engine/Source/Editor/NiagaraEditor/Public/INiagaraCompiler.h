// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraEditorCommon.h"
#include "NiagaraParameters.h"

class Error;
class UEdGraphPin;
class UNiagaraNode;
class UNiagaraGraph;
class UEdGraphPin;
class FCompilerResultsLog;
class UNiagaraDataInterface;




/**
* Enumerates states a Niagara script can be in.
*/
UENUM()
enum ENiagaraScriptCompileStatus
{
	/** Niagara script is in an unknown state. */
	NCS_Unknown,
	/** Niagara script has been modified but not recompiled. */
	NCS_Dirty,
	/** Niagara script tried but failed to be compiled. */
	NCS_Error,
	/** Niagara script has been compiled since it was last modified. */
	NCS_UpToDate,
	/** Niagara script is in the process of being created for the first time. */
	NCS_BeingCreated,
	/** Niagara script has been compiled since it was last modified. There are warnings. */
	NCS_UpToDateWithWarnings,
	NCS_MAX,
};

/** Defines information about the results of a Niagara script compile. */
struct FNiagaraCompileResults
{
	/** Whether or not the script compiled successfully. */
	bool bSuceeded;

	/** A results log with messages, warnings, and errors which occurred during the compile. */
	FCompilerResultsLog* MessageLog;

	/** A text representation of the compilation output. */
	FText OutputAsText;

	FNiagaraCompileResults(FCompilerResultsLog* InMessageLog)
		: MessageLog(InMessageLog)
	{
	}

	static ENiagaraScriptCompileStatus CompileResultsToSummary(const FNiagaraCompileResults* CompileResults);
};

//Interface for Niagara compilers.
class INiagaraCompiler
{
public:

	/** Compiles a script. */
	virtual const FNiagaraCompileResults& CompileScript(class UNiagaraScript* InScript) = 0;

	/** Traverses the pins input, recursively compiling them and then adds any expressions needed for the owner node.*/
	virtual int32 CompilePin(UEdGraphPin* Pin) = 0;
	
	virtual int32 RegisterDataInterface(FNiagaraVariable& Var, UNiagaraDataInterface* DataInterface) = 0;

	virtual void Operation(class UNiagaraNodeOp* Operation, TArray<int32>& Inputs, TArray<int32>& Outputs) = 0;
	virtual void Output(const TArray<FNiagaraVariable>& Attribute, const TArray<int32>& Inputs) = 0;

	virtual int32 GetParameter(const FNiagaraVariable& Parameter) = 0;
	
	virtual int32 GetAttribute(const FNiagaraVariable& Attribute) = 0;

	virtual int32 GetConstant(const FNiagaraVariable& Constant) = 0;

	virtual void ReadDataSet(const FNiagaraDataSetID DataSet, const TArray<FNiagaraVariable>& Variable, ENiagaraDataSetAccessMode AccessMode, int32 InputChunk, TArray<int32>& Outputs) = 0;
	virtual void WriteDataSet(const FNiagaraDataSetID DataSet, const TArray<FNiagaraVariable>& Variable, ENiagaraDataSetAccessMode AccessMode, const TArray<int32>& Inputs) = 0;

	virtual void FunctionCall(class UNiagaraNodeFunctionCall* FunctionNode, TArray<int32>& Inputs, TArray<int32>& Outputs) = 0;

	virtual void Convert(class UNiagaraNodeConvert* Convert, TArray <int32>& Inputs, TArray<int32>& Outputs) = 0;

	virtual void If(TArray<FNiagaraVariable>& Vars, int32 Condition, TArray<int32>& PathA, TArray<int32>& PathB, TArray<int32>& Outputs) = 0;

	/** Adds an error to be reported to the user. Any error will lead to compilation failure. */
	virtual void Error(FText ErrorText, UNiagaraNode* Node, UEdGraphPin* Pin) = 0 ;

	/** Adds a warning to be reported to the user. Warnings will not cause a compilation failure. */
	virtual void Warning(FText WarningText, UNiagaraNode* Node, UEdGraphPin* Pin) = 0;
	
	/** Returns the function script we are compiling, if we are compiling a function. */
	virtual UNiagaraScript* GetFunctionScript() = 0;

	/** If we are compiling a function it will return true and fill in the result of OutParam if the passed Parameter is found. Returns false if we're not compiling a function. */
	virtual bool GetFunctionParameter(const FNiagaraVariable& Parameter, int32& OutParam)const = 0;
};

/** Data which is generated from the hlsl by the VectorVMBackend and fed back into the */
struct FNiagaraCompilationOutput
{
	TArray<uint8> ByteCode;

	/** All external parameters used in the graph. */
	FNiagaraParameters Parameters;

	/** All internal constants used in the graph. */
	FNiagaraParameters InternalParameters;
		
	/** Data sets this script reads. */
	TArray<FNiagaraDataSetProperties> DataSetReads;

	/** Data sets this script writes. */
	TArray<FNiagaraDataSetProperties> DataSetWrites;

	/* Per instance attributes */
	TArray<FNiagaraVariable> Attributes;

	FNiagaraScriptDataUsageInfo DataUsage;

	TArray<FNiagaraScriptDataInterfaceInfo> DataInterfaceInfo;

	FString Errors;
};
