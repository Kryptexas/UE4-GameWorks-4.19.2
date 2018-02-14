// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraTypes.h"
#include "NiagaraCommon.h"
#include "NiagaraParameterHandle.h"
#include "AssetData.h"

class UEdGraph;
class UEdGraphPin;
class UNiagaraGraph;
class UNiagaraNode;
class UNiagaraNodeInput;
class UNiagaraNodeOutput;
class UNiagaraNodeFunctionCall;
class UNiagaraNodeAssignment;
class UNiagaraMaterialParameterNode;
class UNiagaraNodeParameterMapSet;
class UNiagaraStackEditorData;
class FNiagaraSystemViewModel;
class FNiagaraEmitterViewModel;

namespace FNiagaraStackGraphUtilities
{
	void RelayoutGraph(UEdGraph& Graph);

	void GetWrittenVariablesForGraph(UEdGraph& Graph, TArray<FNiagaraVariable>& OutWrittenVariables);

	void ConnectPinToInputNode(UEdGraphPin& Pin, UNiagaraNodeInput& InputNode);

	UEdGraphPin* GetParameterMapInputPin(UNiagaraNode& Node);

	UEdGraphPin* GetParameterMapOutputPin(UNiagaraNode& Node);

	void GetOrderedModuleNodes(UNiagaraNodeOutput& OutputNode, TArray<UNiagaraNodeFunctionCall*>& ModuleNodes);

	UNiagaraNodeFunctionCall* GetPreviousModuleNode(UNiagaraNodeFunctionCall& CurrentNode);

	UNiagaraNodeFunctionCall* GetNextModuleNode(UNiagaraNodeFunctionCall& CurrentNode);

	UNiagaraNodeOutput* GetEmitterOutputNodeForStackNode(UNiagaraNode& StackNode);

	UNiagaraNodeInput* GetEmitterInputNodeForStackNode(UNiagaraNode& StackNode);

	struct FStackNodeGroup
	{
		TArray<UNiagaraNode*> StartNodes;
		UNiagaraNode* EndNode;
		void GetAllNodesInGroup(TArray<UNiagaraNode*>& OutAllNodes) const;
	};

	void GetStackNodeGroups(UNiagaraNode& StackNode, TArray<FStackNodeGroup>& OutStackNodeGroups);

	void DisconnectStackNodeGroup(const FStackNodeGroup& DisconnectGroup, const FStackNodeGroup& PreviousGroup, const FStackNodeGroup& NextGroup);

	void ConnectStackNodeGroup(const FStackNodeGroup& ConnectGroup, const FStackNodeGroup& NewPreviousGroup, const FStackNodeGroup& NewNextGroup);

	void InitializeDataInterfaceInputs(TSharedRef<FNiagaraSystemViewModel> SystemViewModel, TSharedRef<FNiagaraEmitterViewModel> EmitterViewModel, UNiagaraStackEditorData& StackEditorData, UNiagaraNodeFunctionCall& ModuleNode, UNiagaraNodeFunctionCall& InputFunctionCallNode);

	FString GenerateStackFunctionInputEditorDataKey(UNiagaraNodeFunctionCall& FunctionCallNode, FNiagaraParameterHandle InputParameterHandle);

	FString GenerateStackModuleEditorDataKey(UNiagaraNodeFunctionCall& ModuleNode);

	enum class ENiagaraGetStackFunctionInputPinsOptions
	{
		AllInputs,
		ModuleInputsOnly
	};

	void GetStackFunctionInputPins(UNiagaraNodeFunctionCall& FunctionCallNode, TArray<const UEdGraphPin*>& OutInputPins, ENiagaraGetStackFunctionInputPinsOptions Options = ENiagaraGetStackFunctionInputPinsOptions::AllInputs, bool bIgnoreDisabled = false);

	UNiagaraNodeParameterMapSet* GetStackFunctionOverrideNode(UNiagaraNodeFunctionCall& FunctionCallNode);

	UNiagaraNodeParameterMapSet& GetOrCreateStackFunctionOverrideNode(UNiagaraNodeFunctionCall& FunctionCallNode);

	UEdGraphPin* GetStackFunctionInputOverridePin(UNiagaraNodeFunctionCall& StackFunctionCall, FNiagaraParameterHandle AliasedInputParameterHandle);

	UEdGraphPin& GetOrCreateStackFunctionInputOverridePin(UNiagaraNodeFunctionCall& StackFunctionCall, FNiagaraParameterHandle AliasedInputParameterHandle, FNiagaraTypeDefinition InputType);

	void RemoveNodesForStackFunctionInputOverridePin(UEdGraphPin& StackFunctionInputOverridePin);

	void RemoveNodesForStackFunctionInputOverridePin(UEdGraphPin& StackFunctinoInputOverridePin, TArray<TWeakObjectPtr<UNiagaraDataInterface>>& OutRemovedDataObjects);

	void SetLinkedValueHandleForFunctionInput(UEdGraphPin& OverridePin, FNiagaraParameterHandle LinkedParameterHandle);

	void SetDataValueObjectForFunctionInput(UEdGraphPin& OverridePin, UClass* DataObjectType, FString DataObjectName, UNiagaraDataInterface*& OutDataObject);

	void SetDynamicInputForFunctionInput(UEdGraphPin& OverridePin, UNiagaraScript* DynamicInput, UNiagaraNodeFunctionCall*& OutDynamicInputFunctionCall);

	bool RemoveModuleFromStack(UNiagaraNodeFunctionCall& ModuleNode);

	UNiagaraNodeFunctionCall* AddScriptModuleToStack(FAssetData ModuleScriptAsset, UNiagaraNodeOutput& TargetOutputNode, int32 TargetIndex = INDEX_NONE);
	
	bool FindScriptModulesInStack(FAssetData ModuleScriptAsset, UNiagaraNodeOutput& TargetOutputNode, TArray<UNiagaraNodeFunctionCall*> OutFunctionCalls);

	UNiagaraNodeAssignment* AddParameterModuleToStack(FNiagaraVariable ParameterVariable, UNiagaraNodeOutput& TargetOutputNode, int32 TargetIndex = INDEX_NONE, const FString* InDefaultValue = nullptr);
		
	UNiagaraMaterialParameterNode* AddMaterialParameterModuleToStack(TSharedRef<FNiagaraEmitterViewModel>& EmitterViewModel, UNiagaraNodeOutput& TargetOutputNode, int32 TargetIndex = INDEX_NONE);

	TOptional<bool> GetModuleIsEnabled(UNiagaraNodeFunctionCall& FunctionCallNode);

	void SetModuleIsEnabled(UNiagaraNodeFunctionCall& FunctionCallNode, bool bIsEnabled);

	bool ValidateGraphForOutput(UNiagaraGraph& NiagaraGraph, ENiagaraScriptUsage ScriptUsage, FGuid ScriptUsageId, FText& ErrorMessage);

	UNiagaraNodeOutput* ResetGraphForOutput(UNiagaraGraph& NiagaraGraph, ENiagaraScriptUsage ScriptUsage, FGuid ScriptUsageId);

	const UNiagaraEmitter* GetBaseEmitter(UNiagaraEmitter& Emitter, UNiagaraSystem& OwningSystem);

	void CleanUpStaleRapidIterationParameters(UNiagaraScript& Script, UNiagaraEmitter& OwningEmitter);

	void CleanUpStaleRapidIterationParameters(UNiagaraEmitter& Emitter);
}