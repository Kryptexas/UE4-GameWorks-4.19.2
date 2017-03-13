// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraCommon.h"
#include "INiagaraCompiler.h"
#include "Kismet2/CompilerResultsLog.h"
#include "NiagaraScript.h"

class Error;
class UNiagaraGraph;
class UNiagaraNode;
class UNiagaraNodeFunctionCall;
class UNiagaraScriptSource;

enum class ENiagaraCodeChunkMode : uint8
{
	Uniform,
	Source,
	Body,
	Num,
};

FORCEINLINE uint32 GetTypeHash(const FNiagaraFunctionSignature& Sig)
{
	uint32 Hash = GetTypeHash(Sig.Name);
	for (const FNiagaraVariable& Var : Sig.Inputs)
	{
		Hash = HashCombine(Hash, GetTypeHash(Var));
	}
	for (const FNiagaraVariable& Var : Sig.Outputs)
	{
		Hash = HashCombine(Hash, GetTypeHash(Var));
	}
	Hash = HashCombine(Hash, GetTypeHash(Sig.Owner));
	return Hash;
}

struct FNiagaraCodeChunk
{
	/** Symbol name for the chunk. Cam be empty for some types of chunk. */
	FString SymbolName;
	/** Format definition for incorporating SourceChunks into the final code for this chunk. */
	FString Definition;
	/** The returned data type of this chunk. */
	FNiagaraTypeDefinition Type;
	/** If this chunk should declare it's symbol name. */
	bool bDecl;
	/** Chunks used as input for this chunk. */
	TArray<int32> SourceChunks;

	ENiagaraCodeChunkMode Mode;

	FNiagaraCodeChunk()
		: bDecl(true)
		, Mode(ENiagaraCodeChunkMode::Num)
	{
		Type = FNiagaraTypeDefinition::GetFloatDef();
	}

	void AddSourceChunk(int32 ChunkIdx)
	{
		SourceChunks.Add(ChunkIdx);
	}

	int32 GetSourceChunk(int32 i)
	{
		return SourceChunks[i];
	}

	void ReplaceSourceIndex(int32 SourceIdx, int32 NewIdx)
	{
		SourceChunks[SourceIdx] = NewIdx;
	}

	bool operator==(const FNiagaraCodeChunk& Other)
	{
		return SymbolName == Other.SymbolName &&
			Definition == Other.Definition &&
			Mode == Other.Mode &&
			Type == Other.Type &&
			bDecl == Other.bDecl &&
			SourceChunks == Other.SourceChunks;
	}
};

class NIAGARAEDITOR_API FHlslNiagaraCompiler : public INiagaraCompiler
{
public:

	struct FDataSetAccessInfo
	{
		//Variables accessed.
		TArray<FNiagaraVariable> Variables;
		/** Code chunks relating to this access. */
		TArray<int32> CodeChunks;
	};

protected:
	/** The script we are compiling. */
	UNiagaraScript* Script;

	const class UEdGraphSchema_Niagara* Schema;

	/** The set of all generated code chunks for this script. */
	TArray<FNiagaraCodeChunk> CodeChunks;

	/** Array of code chunks of each different type. */
	TArray<int32> ChunksByMode[(int32)ENiagaraCodeChunkMode::Num];

	/** Map of Pins to compiled code chunks. Allows easy reuse of previously compiled pins. */
	TMap<UEdGraphPin*, int32> PinToCodeChunks;

	/** The combined output of the compilation of this script. This is temporary and will be reworked soon. */
	FNiagaraCompilationOutput CompilationOutput;

	/** Message log. Automatically handles marking the NodeGraph with errors. */
	FCompilerResultsLog MessageLog;

	/** Captures information about a script compile. */
	FNiagaraCompileResults CompileResults;

	TMap<FName, uint32> GeneratedSymbolCounts;

	FDataSetAccessInfo InstanceRead;
	FDataSetAccessInfo InstanceWrite;

	TMap<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>> DataSetReadInfo[(int32)ENiagaraDataSetAccessMode::Num];
	TMap<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>> DataSetWriteInfo[(int32)ENiagaraDataSetAccessMode::Num];

	FString GetDataSetAccessSymbol(FNiagaraDataSetID DataSet, int32 IndexChunk, bool bRead);
	FORCEINLINE FNiagaraDataSetID GetInstanceDatSetID()const { return FNiagaraDataSetID(TEXT("Instance"), ENiagaraDataSetType::ParticleData); }

	/** All functions called in the script. */
	TMap<FNiagaraFunctionSignature, FString> Functions;
	/** Map of function graphs we've seen before and already pre-processed. */
	TMap<const UNiagaraGraph*, UNiagaraGraph*> PreprocessedFunctions;

	void RegisterFunctionCall(UNiagaraNodeFunctionCall* FunctionNode, TArray<int32>& Inputs, FNiagaraFunctionSignature& OutSignature);
	void GenerateFunctionCall(FNiagaraFunctionSignature& FunctionSignature, TArray<int32>& Inputs, TArray<int32>& Outputs);
	FString GetFunctionSignature(FNiagaraFunctionSignature& Sig);

	/** Compiles an output Pin on a graph node. Caches the result for any future inputs connected to it. */
	int32 CompileOutputPin(UEdGraphPin* Pin);

	void WriteDataSetContextVars(TMap<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>>& DataSetAccessInfo, bool bRead, FString &HlslOutput);
	void WriteDataSetStructDeclarations(TMap<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>>& DataSetAccessInfo, bool bRead, FString &HlslOutput);
	void DecomposeVariableAccess(const UStruct* Struct, bool bRead, FString IndexSymbol, FString HLSLString);

	FString GetUniqueSymbolName(FName BaseName);

	/** Stack of all function params. */
	struct FFunctionContext
	{
		UNiagaraScript* CallerScript;
		UNiagaraScript* FunctionScript;	
		FNiagaraFunctionSignature& Signature;
		TArray<int32>& Inputs;
		FFunctionContext(UNiagaraScript* InCallerScript, UNiagaraScript* InFunctionScript, FNiagaraFunctionSignature& InSig, TArray<int32>& InInputs) 
			: CallerScript(InCallerScript)
			, FunctionScript(InFunctionScript)
			, Signature(InSig)
			, Inputs(InInputs)
		{}
	};
	TArray<FFunctionContext> FunctionContextStack;
	const FFunctionContext* FunctionCtx()const { return FunctionContextStack.Num() > 0 ? &FunctionContextStack.Last() : nullptr; }
	void EnterFunction(UNiagaraScript* InFunctionScript, FNiagaraFunctionSignature& Signature, TArray<int32>& Inputs);
	void ExitFunction();
	FString GetCallstack();

	FString GeneratedConstantString(float Constant);
	FString GeneratedConstantString(FVector4 Constant);

	/* Add a chunk that is not written to the source, only used as a source chunk for others. */
	int32 AddSourceChunk(FString SymbolName, const FNiagaraTypeDefinition& Type);

	/** Add a chunk defining a uniform value. */
	int32 AddUniformChunk(FString SymbolName, const FNiagaraTypeDefinition& Type);

	/* Add a chunk that is written to the body of the shader code. */
	int32 AddBodyChunk(FString SymbolName, FString Definition, const FNiagaraTypeDefinition& Type, TArray<int32>& SourceChunks, bool bDecl = true);
	int32 AddBodyChunk(FString SymbolName, FString Definition, const FNiagaraTypeDefinition& Type, int32 SourceChunk, bool bDecl = true);
	int32 AddBodyChunk(FString SymbolName, FString Definition, const FNiagaraTypeDefinition& Type, bool bDecl = true);

	FString GetFunctionDefinitions();
public:

	FHlslNiagaraCompiler();

	//Begin INiagaraCompiler Interface
	virtual const FNiagaraCompileResults& CompileScript(UNiagaraScript* InScript)override;
	virtual int32 CompilePin(UEdGraphPin* Pin)override;

	virtual int32 RegisterDataInterface(FNiagaraVariable& Var, UNiagaraDataInterface* DataInterface)override;

	virtual void Operation(class UNiagaraNodeOp* Operation, TArray<int32>& Inputs, TArray<int32>& Outputs)override;
	virtual void Output(const TArray<FNiagaraVariable>& Attribute, const TArray<int32>& Inputs)override;

	virtual int32 GetParameter(const FNiagaraVariable& Parameter)override;

	virtual int32 GetAttribute(const FNiagaraVariable& Attribute)override;

	virtual int32 GetConstant(const FNiagaraVariable& Constant)override;

	virtual void ReadDataSet(const FNiagaraDataSetID DataSet, const TArray<FNiagaraVariable>& Variable, ENiagaraDataSetAccessMode AccessMode, int32 InputChunk, TArray<int32>& Outputs)override;
	virtual void WriteDataSet(const FNiagaraDataSetID DataSet, const TArray<FNiagaraVariable>& Variable, ENiagaraDataSetAccessMode AccessMode, const TArray<int32>& Inputs)override;

	void DefineDataSetReadFunction(FString &HlslOutput, TArray<FNiagaraDataSetID> &ReadDataSets);
	void DefineDataSetWriteFunction(FString &HlslOutput, TArray<FNiagaraDataSetProperties> &WriteDataSets);
	void DefineMain(FString &HLSLOutput, TArray<FNiagaraVariable> &InstanceReadVars, TArray<FNiagaraVariable> &InstanceWriteVars);

	void GenerateCodeForProperties(const UScriptStruct* Struct, FString Format, FString VariableSymbol, int32& Counter, int32 DataSetIndex, FString InstanceIdxSymbol, FString &HlslOutput);

	FString CompileDataInterfaceFunction(UNiagaraDataInterface* DataInterface, FNiagaraFunctionSignature& Signature);

	//TODO: Write ability to read and write direcly to a specific index.
	//virtual int32 ReadDataSet(const FNiagaraDataSetID DataSet, const FNiagaraVariable& Variable, int32 ReadIndexChunk = INDEX_NONE)override;
	//virtual int32 WriteDataSet(const FNiagaraDataSetID DataSet, const FNiagaraVariable& Variable, int32 WriteIndexChunk = INDEX_NONE)override;

	virtual void FunctionCall(UNiagaraNodeFunctionCall* FunctionNode, TArray<int32>& Inputs, TArray<int32>& Outputs) override;

	virtual void Convert(class UNiagaraNodeConvert* Convert, TArray <int32>& Inputs, TArray<int32>& Outputs) override;
	virtual void If(TArray<FNiagaraVariable>& Vars, int32 Condition, TArray<int32>& PathA, TArray<int32>& PathB, TArray<int32>& Outputs) override;

	virtual void Error(FText ErrorText, UNiagaraNode* Node, UEdGraphPin* Pin)override;
	virtual void Warning(FText WarningText, UNiagaraNode* Node, UEdGraphPin* Pin)override;

	virtual UNiagaraScript* GetFunctionScript() override;
	virtual bool GetFunctionParameter(const FNiagaraVariable& Parameter, int32& OutParam)const override;
	//End INiagaraCompiler Interface

	static FString GetStructHlslTypeName(FNiagaraTypeDefinition Type);
	static FString GetPropertyHlslTypeName(const UProperty* Property);
	static FString BuildHLSLStructDecl(FNiagaraTypeDefinition Type);
	static FString GetHlslDefaultForType(FNiagaraTypeDefinition Type);
	static bool IsHlslBuiltinVector(FNiagaraTypeDefinition Type);
	static TArray<FName> ConditionPropertyPath(const FNiagaraTypeDefinition& Type, const TArray<FName>& InPath);

	static FString GetSanitizedSymbolName(FString SymbolName);
private:
	//Generates the code for the passed chunk.
	FString GetCode(int32 ChunkIdx);
	//Retreives the code for this chunk being used as a source for another chunk
	FString GetCodeAsSource(int32 ChunkIdx);

	bool ValidateTypePins(UNiagaraNode* NodeToValidate);
	void GenerateFunctionSignature(UNiagaraScript* FunctionScript, UNiagaraGraph* FuncGraph, TArray<int32>& Inputs, FNiagaraFunctionSignature& OutSig)const;

	/** Map of symbol names to count of times it's been used. Used for gernerating unique symbol names. */
	TMap<FName, uint32> SymbolCounts;

	// read and write data set indices
	int32 ReadIdx;
	int32 WriteIdx;


};
