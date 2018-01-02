// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "NiagaraModule.h"
#include "NiagaraTypes.h"
#include "NiagaraCommon.h"

#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "UObject/ObjectKey.h"

class UNiagaraEmitter;
class UNiagaraScript;
class UNiagaraNodeOutput;
class UNiagaraNodeFunctionCall;
class UNiagaraNodeParameterMapSet;
class UEdGraphNode;
class UEdGraphPin;
class UNiagaraDataInterface;
struct FNiagaraEventScriptProperties;
class UNiagaraRendererProperties;

class FNiagaraStackFunctionMergeAdapter;

class FNiagaraStackFunctionInputOverrideMergeAdapter
{
public:
	FNiagaraStackFunctionInputOverrideMergeAdapter(
		FString InUniqueEmitterName, 
		UNiagaraScript& InOwningScript, 
		UNiagaraNodeFunctionCall& InOwningFunctionCallNode, 
		UEdGraphPin& InOverridePin);

	FNiagaraStackFunctionInputOverrideMergeAdapter(
		FString InUniqueEmitterName,
		UNiagaraScript& InOwningScript,
		UNiagaraNodeFunctionCall& InOwningFunctionCallNode,
		FString InInputName,
		FNiagaraVariable InRapidIterationParameter);

	UNiagaraScript* GetOwningScript() const;
	UNiagaraNodeFunctionCall* GetOwningFunctionCall() const;
	FString GetInputName() const;
	UNiagaraNodeParameterMapSet* GetOverrideNode() const;
	UEdGraphPin* GetOverridePin() const;

	TOptional<FString> GetLocalValueString() const;
	TOptional<FNiagaraVariable> GetLocalValueRapidIterationParameter() const;
	TOptional<FNiagaraParameterHandle> GetLinkedValueHandle() const;
	UNiagaraDataInterface* GetDataValueObject() const;
	TSharedPtr<FNiagaraStackFunctionMergeAdapter> GetDynamicValueFunction() const;

private:
	FString UniqueEmitterName;
	TWeakObjectPtr<UNiagaraScript> OwningScript;
	TWeakObjectPtr<UNiagaraNodeFunctionCall> OwningFunctionCallNode;
	FString InputName;

	TWeakObjectPtr<UNiagaraNodeParameterMapSet> OverrideNode;
	UEdGraphPin* OverridePin;

	TOptional<FString> LocalValueString;
	TOptional<FNiagaraVariable> LocalValueRapidIterationParameter;
	TOptional<FNiagaraParameterHandle> LinkedValueHandle;
	UNiagaraDataInterface* DataValueObject;
	TSharedPtr<FNiagaraStackFunctionMergeAdapter> DynamicValueFunction;
};

class FNiagaraStackFunctionMergeAdapter
{
public:
	FNiagaraStackFunctionMergeAdapter(FString InUniqueEmitterName, UNiagaraScript& InOwningScript, UNiagaraNodeFunctionCall& InFunctionCallNode, int32 InStackIndex);

	UNiagaraNodeFunctionCall* GetFunctionCallNode() const;

	int32 GetStackIndex() const;

	const TArray<TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter>>& GetInputOverrides() const;

	TSharedPtr<FNiagaraStackFunctionInputOverrideMergeAdapter> GetInputOverrideByInputName(FString InputName) const;

private:
	FString UniqueEmitterName;
	TWeakObjectPtr<UNiagaraScript> OwningScript;
	TWeakObjectPtr<UNiagaraNodeFunctionCall> FunctionCallNode;
	int32 StackIndex;

	TArray<TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter>> InputOverrides;
};

class FNiagaraScriptStackMergeAdapter
{
public:
	FNiagaraScriptStackMergeAdapter(UNiagaraNodeOutput& InOutputNode, UNiagaraScript& InScript, FString InUniqueEmitterName);

	UNiagaraNodeOutput* GetOutputNode() const;

	UNiagaraScript* GetScript() const;

	FString GetUniqueEmitterName() const;

	const TArray<TSharedRef<FNiagaraStackFunctionMergeAdapter>>& GetModuleFunctions() const;

	TSharedPtr<FNiagaraStackFunctionMergeAdapter> GetModuleFunctionById(FGuid FunctionCallNodeId) const;

private:
	TWeakObjectPtr<UNiagaraNodeOutput> OutputNode;
	TWeakObjectPtr<UNiagaraScript> Script;
	FString UniqueEmitterName;
	TArray<TSharedRef<FNiagaraStackFunctionMergeAdapter>> ModuleFunctions;
};

class FNiagaraEventHandlerMergeAdapter
{
public:
	FNiagaraEventHandlerMergeAdapter(const UNiagaraEmitter& InEmitter, const FNiagaraEventScriptProperties* InEventScriptProperties, UNiagaraNodeOutput* InOutputNode);
	FNiagaraEventHandlerMergeAdapter(const UNiagaraEmitter& InEmitter, FNiagaraEventScriptProperties* InEventScriptProperties, UNiagaraNodeOutput* InOutputNode);
	FNiagaraEventHandlerMergeAdapter(const UNiagaraEmitter& InEmitter, UNiagaraNodeOutput* InOutputNode);

	FGuid GetUsageId() const;
	const UNiagaraEmitter* GetEmitter() const;
	const FNiagaraEventScriptProperties* GetEventScriptProperties() const;
	FNiagaraEventScriptProperties* GetEditableEventScriptProperties() const;
	UNiagaraNodeOutput* GetOutputNode() const;
	TSharedPtr<FNiagaraScriptStackMergeAdapter> GetEventStack() const;

private:
	void Initialize(const UNiagaraEmitter& InEmitter, const FNiagaraEventScriptProperties* InEventScriptProperties, FNiagaraEventScriptProperties* InEditableEventScriptProperties, UNiagaraNodeOutput* InOutputNode);

private:
	TWeakObjectPtr<UNiagaraEmitter> Emitter;
	const FNiagaraEventScriptProperties* EventScriptProperties;
	FNiagaraEventScriptProperties* EditableEventScriptProperties;
	TWeakObjectPtr<UNiagaraNodeOutput> OutputNode;

	TSharedPtr<FNiagaraScriptStackMergeAdapter> EventStack;
};

class FNiagaraRendererMergeAdapter
{
public:
	FNiagaraRendererMergeAdapter(UNiagaraRendererProperties& InRenderer);

	UNiagaraRendererProperties* GetRenderer();

private:
	TWeakObjectPtr<UNiagaraRendererProperties> Renderer;
};

class FNiagaraEmitterMergeAdapter
{
public:
	FNiagaraEmitterMergeAdapter(const UNiagaraEmitter& InEmitter);
	FNiagaraEmitterMergeAdapter(UNiagaraEmitter& InEmitter);

	UNiagaraEmitter* GetEditableEmitter() const;

	TSharedPtr<FNiagaraScriptStackMergeAdapter> GetEmitterSpawnStack() const;
	TSharedPtr<FNiagaraScriptStackMergeAdapter> GetEmitterUpdateStack() const;
	TSharedPtr<FNiagaraScriptStackMergeAdapter> GetParticleSpawnStack() const;
	TSharedPtr<FNiagaraScriptStackMergeAdapter> GetParticleUpdateStack() const;
	const TArray<TSharedRef<FNiagaraEventHandlerMergeAdapter>> GetEventHandlers() const;
	const TArray<TSharedRef<FNiagaraRendererMergeAdapter>> GetRenderers() const;

	TSharedPtr<FNiagaraScriptStackMergeAdapter> GetScriptStack(ENiagaraScriptUsage Usage, FGuid UsageId);

	TSharedPtr<FNiagaraEventHandlerMergeAdapter> GetEventHandler(FGuid EventScriptUsageId);

	TSharedPtr<FNiagaraRendererMergeAdapter> GetRenderer(FGuid RendererMergeId);

private:
	void Initialize(const UNiagaraEmitter& InEmitter, UNiagaraEmitter* InEditableEmitter);

private:
	TWeakObjectPtr<const UNiagaraEmitter> Emitter;
	TWeakObjectPtr<UNiagaraEmitter> EditableEmitter;
	TSharedPtr<FNiagaraScriptStackMergeAdapter> EmitterSpawnStack;
	TSharedPtr<FNiagaraScriptStackMergeAdapter> EmitterUpdateStack;
	TSharedPtr<FNiagaraScriptStackMergeAdapter> ParticleSpawnStack;
	TSharedPtr<FNiagaraScriptStackMergeAdapter> ParticleUpdateStack;
	TArray<TSharedRef<FNiagaraEventHandlerMergeAdapter>> EventHandlers;
	TArray<TSharedRef<FNiagaraRendererMergeAdapter>> Renderers;
};

struct FNiagaraScriptStackDiffResults
{
	FNiagaraScriptStackDiffResults();

	bool IsEmpty() const;

	bool IsValid() const;

	void AddError(FText ErrorMessage);

	const TArray<FText>& GetErrorMessages() const;

	TArray<TSharedRef<FNiagaraStackFunctionMergeAdapter>> RemovedBaseModules;
	TArray<TSharedRef<FNiagaraStackFunctionMergeAdapter>> AddedOtherModules;

	TArray<TSharedRef<FNiagaraStackFunctionMergeAdapter>> MovedBaseModules;
	TArray<TSharedRef<FNiagaraStackFunctionMergeAdapter>> MovedOtherModules;

	TArray<TSharedRef<FNiagaraStackFunctionMergeAdapter>> EnabledChangedBaseModules;
	TArray<TSharedRef<FNiagaraStackFunctionMergeAdapter>> EnabledChangedOtherModules;

	TArray<TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter>> RemovedBaseInputOverrides;
	TArray<TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter>> AddedOtherInputOverrides;
	TArray<TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter>> ModifiedBaseInputOverrides;
	TArray<TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter>> ModifiedOtherInputOverrides;

private:
	bool bIsValid;
	TArray<FText> ErrorMessages;
};

struct FNiagaraModifiedEventHandlerDiffResults
{
	TSharedPtr<FNiagaraEventHandlerMergeAdapter> BaseAdapter;
	TSharedPtr<FNiagaraEventHandlerMergeAdapter> OtherAdapter;
	TArray<UProperty*> ChangedProperties;
	FNiagaraScriptStackDiffResults ScriptDiffResults;
};

struct FNiagaraEmitterDiffResults
{
	FNiagaraEmitterDiffResults();

	bool IsValid() const;

	bool IsEmpty() const;

	void AddError(FText ErrorMessage);

	const TArray<FText>& GetErrorMessages() const;

	FString GetErrorMessagesString() const;

	TArray<UProperty*> DifferentEmitterProperties;

	FNiagaraScriptStackDiffResults EmitterSpawnDiffResults;
	FNiagaraScriptStackDiffResults EmitterUpdateDiffResults;
	FNiagaraScriptStackDiffResults ParticleSpawnDiffResults;
	FNiagaraScriptStackDiffResults ParticleUpdateDiffResults;

	TArray<TSharedRef<FNiagaraEventHandlerMergeAdapter>> RemovedBaseEventHandlers;
	TArray<TSharedRef<FNiagaraEventHandlerMergeAdapter>> AddedOtherEventHandlers;
	TArray<FNiagaraModifiedEventHandlerDiffResults> ModifiedEventHandlers;

	TArray<TSharedRef<FNiagaraRendererMergeAdapter>> RemovedBaseRenderers;
	TArray<TSharedRef<FNiagaraRendererMergeAdapter>> AddedOtherRenderers;
	TArray<TSharedRef<FNiagaraRendererMergeAdapter>> ModifiedBaseRenderers;
	TArray<TSharedRef<FNiagaraRendererMergeAdapter>> ModifiedOtherRenderers;

private:
	bool bIsValid;
	TArray<FText> ErrorMessages;
};

class FNiagaraScriptMergeManager
{
public:
	struct FApplyDiffResults
	{
		FApplyDiffResults()
			: bSucceeded(true)
			, bModifiedGraph(false)
		{
		}

		bool bSucceeded;
		bool bModifiedGraph;
		TArray<FText> ErrorMessages;
	};

public:
	INiagaraModule::FMergeEmitterResults MergeEmitter(UNiagaraEmitter& Source, UNiagaraEmitter& LastMergedSource, UNiagaraEmitter& Instance);

	static TSharedRef<FNiagaraScriptMergeManager> Get();

	FNiagaraEmitterDiffResults DiffEmitters(UNiagaraEmitter& BaseEmitter, UNiagaraEmitter& OtherEmitter);

	bool IsMergeableScriptUsage(ENiagaraScriptUsage ScriptUsage) const;

	bool HasBaseModule(const UNiagaraEmitter& BaseEmitter, ENiagaraScriptUsage ScriptUsage, FGuid ScriptUsageId, FGuid ModuleId);

	bool IsModuleInputDifferentFromBase(UNiagaraEmitter& Emitter, const UNiagaraEmitter& BaseEmitter, ENiagaraScriptUsage ScriptUsage, FGuid ScriptUsageId, FGuid ModuleId, FString InputName);

	FApplyDiffResults ResetModuleInputToBase(UNiagaraEmitter& Emitter, const UNiagaraEmitter& BaseEmitter, ENiagaraScriptUsage ScriptUsage, FGuid ScriptUsageId, FGuid ModuleId, FString InputName);

	bool HasBaseEventHandler(const UNiagaraEmitter& BaseEmitter, FGuid EventScriptUsageId);

	bool IsEventHandlerPropertySetDifferentFromBase(UNiagaraEmitter& Emitter, const UNiagaraEmitter& BaseEmitter, FGuid EventScriptUsageId);

	void ResetEventHandlerPropertySetToBase(UNiagaraEmitter& Emitter, const UNiagaraEmitter& BaseEmitter, FGuid EventScriptUsageId);

	bool HasBaseRenderer(const UNiagaraEmitter& BaseEmitter, FGuid RendererMergeId);

	bool IsRendererDifferentFromBase(UNiagaraEmitter& Emitter, const UNiagaraEmitter& BaseEmitter, FGuid RendererMergeId);

	void ResetRendererToBase(UNiagaraEmitter& Emitter, const UNiagaraEmitter& BaseEmitter, FGuid RendererMergeId);

	bool IsEmitterEditablePropertySetDifferentFromBase(const UNiagaraEmitter& Emitter, const UNiagaraEmitter& BaseEmitter);

	void ResetEmitterEditablePropertySetToBase(UNiagaraEmitter& Emitter, const UNiagaraEmitter& BaseEmitter);

	void DiffEventHandlers(const TArray<TSharedRef<FNiagaraEventHandlerMergeAdapter>>& BaseEventHandlers, const TArray<TSharedRef<FNiagaraEventHandlerMergeAdapter>>& OtherEventHandlers, FNiagaraEmitterDiffResults& DiffResults);

	void DiffRenderers(const TArray<TSharedRef<FNiagaraRendererMergeAdapter>>& BaseRenderers, const TArray<TSharedRef<FNiagaraRendererMergeAdapter>>& OtherRenderers, FNiagaraEmitterDiffResults& DiffResults);

	void DiffScriptStacks(TSharedRef<FNiagaraScriptStackMergeAdapter> BaseScriptStackAdapter, TSharedRef<FNiagaraScriptStackMergeAdapter> OtherScriptStackAdapter, FNiagaraScriptStackDiffResults& DiffResults);

	void DiffFunctionInputs(TSharedRef<FNiagaraStackFunctionMergeAdapter> BaseFunctionAdapter, TSharedRef<FNiagaraStackFunctionMergeAdapter> OtherFunctionAdapter, FNiagaraScriptStackDiffResults& DiffResults);

	void DiffEditableProperties(const void* BaseDataAddress, const void* OtherDataAddress, UStruct& Struct, TArray<UProperty*>& OutDifferentProperties);

private:
	TOptional<bool> DoFunctionInputOverridesMatch(TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter> BaseFunctionInputAdapter, TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter> OtherFunctionInputAdapter);

	FApplyDiffResults ApplyScriptStackDiff(TSharedRef<FNiagaraScriptStackMergeAdapter> BaseScriptStackAdapter, const FNiagaraScriptStackDiffResults& DiffResults);

	FApplyDiffResults ApplyEventHandlerDiff(TSharedRef<FNiagaraEmitterMergeAdapter> BaseEmitterAdapter, const FNiagaraEmitterDiffResults& DiffResults);

	FApplyDiffResults ApplyRendererDiff(UNiagaraEmitter& BaseEmitter, const FNiagaraEmitterDiffResults& DiffResults);

	FApplyDiffResults AddModule(FString UniqueEmitterName, UNiagaraScript& OwningScript, UNiagaraNodeOutput& TargetOutputNode, TSharedRef<FNiagaraStackFunctionMergeAdapter> AddModule);

	FApplyDiffResults RemoveInputOverride(UNiagaraScript& OwningScript, TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter> OverrideToRemove);

	FApplyDiffResults AddInputOverride(FString UniqueEmitterName, UNiagaraScript& OwningScript, UNiagaraNodeFunctionCall& TargetFunctionCall, TSharedRef<FNiagaraStackFunctionInputOverrideMergeAdapter> OverrideToAdd);

	void CopyPropertiesToBase(void* BaseDataAddress, const void* OtherDataAddress, TArray<UProperty*> PropertiesToCopy);

private:
	TSharedRef<FNiagaraEmitterMergeAdapter> GetEmitterMergeAdapterUsingCache(const UNiagaraEmitter& Emitter);

	TSharedRef<FNiagaraEmitterMergeAdapter> GetEmitterMergeAdapterUsingCache(UNiagaraEmitter& Emitter);

private:
	struct FCachedMergeAdapter
	{
		FGuid ChangeId;
		TSharedPtr<FNiagaraEmitterMergeAdapter> EmitterMergeAdapter;
	};

	TMap<FObjectKey, FCachedMergeAdapter> CachedMergeAdapters;
};