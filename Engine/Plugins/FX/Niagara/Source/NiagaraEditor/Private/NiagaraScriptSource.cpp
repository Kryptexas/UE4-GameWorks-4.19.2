// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraScriptSource.h"
#include "Modules/ModuleManager.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorModule.h"
#include "NiagaraScript.h"
#include "NiagaraComponent.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "ComponentReregisterContext.h"
#include "NiagaraGraph.h"
#include "NiagaraConstants.h"
#include "NiagaraSystem.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeWriteDataSet.h"
#include "NiagaraNodeReadDataSet.h"
#include "NiagaraScript.h"
#include "NiagaraDataInterface.h"
#include "GraphEditAction.h"
#include "NiagaraNodeParameterMapBase.h"
#include "NiagaraNodeOutput.h"
#include "EdGraphUtilities.h"
#include "EdGraphSchema_Niagara.h"
#include "TokenizedMessage.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraCustomVersion.h"
#include "NiagaraStackGraphUtilities.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeParameterMapSet.h"
#include "INiagaraEditorTypeUtilities.h"

DECLARE_CYCLE_STAT(TEXT("Niagara - ScriptSource - Compile"), STAT_NiagaraEditor_ScriptSource_Compile, STATGROUP_NiagaraEditor);
DECLARE_CYCLE_STAT(TEXT("Niagara - ScriptSource - PreCompile"), STAT_NiagaraEditor_ScriptSource_PreCompile, STATGROUP_NiagaraEditor);

UNiagaraScriptSource::UNiagaraScriptSource(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bIsPrecompiled(false)
{
}

void UNiagaraScriptSource::PostLoad()
{
	Super::PostLoad();

	if (NodeGraph)
	{
		// We need to make sure that the node-graph is already resolved b/c we may be asked IsSyncrhonized later...
		NodeGraph->ConditionalPostLoad();

		// Hook up event handlers so the on changed handler can be called correctly.
		NodeGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateUObject(this, &UNiagaraScriptSource::OnGraphChanged));
		NodeGraph->OnDataInterfaceChanged().AddUObject(this, &UNiagaraScriptSource::OnGraphDataInterfaceChanged);
	}

	const int32 NiagaraVer = GetLinkerCustomVersion(FNiagaraCustomVersion::GUID);
	if (NiagaraVer < FNiagaraCustomVersion::AddedScriptRapidIterationVariables)
	{
		UpgradeForScriptRapidIterationVariables();
	}
}

void UNiagaraScriptSource::UpgradeForScriptRapidIterationVariables()
{
	// In this version we added the ability to rapidly iterate on top-level module parameters. This means that there are 
	// loose set nodes that were just setting constants that need to be removed from the graph as well as needing to build the script
	// rapid iteration parameters list.
	TArray<UNiagaraNodeOutput*> OutputNodes;
	NodeGraph->GetNodesOfClass(OutputNodes);
	UNiagaraEmitter* ParentEmitter = Cast<UNiagaraEmitter>(GetOuter());
	UNiagaraSystem* ParentSystem = Cast<UNiagaraSystem>(GetOuter());
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::GetModuleChecked<FNiagaraEditorModule>("NiagaraEditor");

	if (ParentEmitter != nullptr || ParentSystem != nullptr)
	{
		TArray<UNiagaraScript*> Scripts;
		if (ParentEmitter != nullptr)
		{
			ParentEmitter->ConditionalPostLoad();
			ParentEmitter->GetScripts(Scripts, false);
		}
		else if (ParentSystem != nullptr)
		{
			ParentSystem->ConditionalPostLoad();
			Scripts.Add(ParentSystem->GetSystemSpawnScript(true));
			Scripts.Add(ParentSystem->GetSystemUpdateScript(true));
			Scripts.Add(ParentSystem->GetSystemSpawnScript(false));
			Scripts.Add(ParentSystem->GetSystemUpdateScript(false));
		}

		for (UNiagaraScript* Script : Scripts)
		{
			ensure(Script->RapidIterationParameters.GetParameterDataArray().Num() == 0);
		}

		bool bNeedsRecompile = false;

		TArray<UEdGraphPin*> PinsToRemove;

		for (UNiagaraNodeOutput* Output : OutputNodes)
		{
			FNiagaraParameterMapHistoryBuilder Builder;
			Builder.BuildParameterMaps(Output);
				
			TArray<UNiagaraNodeFunctionCall*> FunctionCallNodes;
			TArray<UNiagaraNode*> Nodes;
			NodeGraph->BuildTraversal(Nodes, Output);
			for (UNiagaraNode* Node : Nodes)
			{
				UNiagaraNodeFunctionCall* FuncCallNode = Cast<UNiagaraNodeFunctionCall>(Node);
				if (FuncCallNode != nullptr)
				{
					FunctionCallNodes.Add(FuncCallNode);
				}
			}

			TArray<const UEdGraphPin*> CandidateInputPins;
			TArray<FNiagaraVariable> CandidateVariables;
			for (UNiagaraNodeFunctionCall* FunctionCallNode : FunctionCallNodes)
			{
				TArray<const UEdGraphPin*> InputPins;
				FNiagaraStackGraphUtilities::GetStackFunctionInputPins(*FunctionCallNode, InputPins, FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly);

				for (const UEdGraphPin* InputPin : InputPins)
				{
					CandidateInputPins.Add(InputPin);
					const UEdGraphSchema_Niagara* NiagaraSchema = CastChecked<UEdGraphSchema_Niagara>(InputPin->GetSchema());
					FNiagaraVariable FunctionVar = NiagaraSchema->PinToNiagaraVariable(InputPin);
					TMap<FString, FString> AliasMap = TMap<FString, FString>();
					AliasMap.Add(TEXT("Module"), FunctionCallNode->GetFunctionName());
					if (ParentEmitter)
					{
						AliasMap.Add(TEXT("Emitter"), ParentEmitter->GetUniqueEmitterName());
					}
					FunctionVar = FNiagaraParameterMapHistory::ResolveAliases(FunctionVar, AliasMap, TEXT("."));
					CandidateVariables.Add(FunctionVar);
				}
			}

			UE_LOG(LogNiagaraEditor, Log, TEXT("Converting OutputNode: %s"), *Output->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

			// As of this version there aren't multiple instances of Event graphs, so we ignore the occurrence parameter.
			// We do however have the solo and multi versions of the system scripts, so we need to be able to set multiple scripts.
			TArray<UNiagaraScript*> ActiveScripts;
			for (UNiagaraScript* Script : Scripts)
			{
				if (Script->ContainsUsage(Output->GetUsage()))
				{
					ActiveScripts.Add(Script);
				}
			}

			if (ActiveScripts.Num() == 0)
			{
				continue;
			}

			for (int32 i = 0; i < CandidateInputPins.Num(); i++)
			{
				const UEdGraphPin* Pin = CandidateInputPins[i];
				FNiagaraVariable CandidateVar = CandidateVariables[i];

				check(Builder.Histories.Num() == 1);
				const UEdGraphSchema_Niagara* NiagaraSchema = CastChecked<UEdGraphSchema_Niagara>(Pin->GetSchema());
				FNiagaraVariable FunctionVar = NiagaraSchema->PinToNiagaraVariable(Pin);
				FNiagaraTypeDefinition PinDefinition = FunctionVar.GetType();

				// Now check to see if this is one of the appropriate types:
				if (PinDefinition == FNiagaraTypeDefinition::GetBoolDef() || PinDefinition.IsEnum() || PinDefinition == FNiagaraTypeDefinition::GetParameterMapDef())
				{
					continue;
				}
				
				// Is this pin even in the subgraph we care about right now?
				int32 ParamIdx = Builder.Histories[0].FindVariable(CandidateVar.GetName(), CandidateVar.GetType());
				if (ParamIdx == INDEX_NONE)
				{
					continue;
				}

				FString DefaultValue;
				// Now check to see if anyone writes to this variable before reading in the graph.
				if (Builder.Histories[0].PerVariableReadHistory[ParamIdx].Num() != 0)
				{
					const UEdGraphPin* FirstReadPin = Builder.Histories[0].PerVariableReadHistory[ParamIdx][0].Key;
					const UEdGraphPin* FirstWritePin = Builder.Histories[0].PerVariableReadHistory[ParamIdx][0].Value;
					const UEdGraphPin* DefaultValuePin = Builder.Histories[0].GetDefaultValuePin(ParamIdx);

					UNiagaraNodeParameterMapSet* MapSetNode = nullptr;
					if (FirstWritePin != nullptr)
					{
						MapSetNode = Cast<UNiagaraNodeParameterMapSet>(FirstWritePin->GetOwningNode());
					}

					if (FirstWritePin != nullptr && FirstWritePin->LinkedTo.Num() == 0 && nullptr != MapSetNode) // Set by a ParameterMapSet pin linked to a constant...
					{
						DefaultValue = FirstWritePin->DefaultValue;
						PinsToRemove.Add((UEdGraphPin*)FirstWritePin);
						UE_LOG(LogNiagaraEditor, Log, TEXT("Removing Set pin: %s"), *Builder.Histories[0].Variables[ParamIdx].GetName().ToString());
					}
					else if (FirstWritePin != nullptr && FirstWritePin->LinkedTo.Num() > 0) // The Set node is set by something other than a constant, so skip it.
					{
						continue;
					}
					else if (FirstWritePin == nullptr && DefaultValuePin != nullptr) // Get the default value of a pin...
					{
						DefaultValue = DefaultValuePin->DefaultValue;
					}

					FNiagaraVariable FinalVar = Builder.Histories[0].Variables[ParamIdx];
					FinalVar = FNiagaraParameterMapHistory::ConvertVariableToRapidIterationConstantName(FinalVar, ParentEmitter ? *ParentEmitter->GetUniqueEmitterName() : nullptr);

					TSharedPtr<INiagaraEditorTypeUtilities> TypeEditorUtilities = NiagaraEditorModule.GetTypeUtilities(FinalVar.GetType());
					if (TypeEditorUtilities.IsValid() && TypeEditorUtilities->CanHandlePinDefaults())
					{
						bool bValueSet = TypeEditorUtilities->SetValueFromPinDefaultString(DefaultValue, FinalVar);
						if (bValueSet)
						{
							UE_LOG(LogNiagaraEditor, Log, TEXT("Adding Rapid Iteration Variable: %s  Default Value: %s"), *FinalVar.GetName().ToString(), *DefaultValue);
						}
						else
						{
							UE_LOG(LogNiagaraEditor, Warning, TEXT("Adding Rapid Iteration Variable: %s  Default Value '%s' could not be set for type %s"),
								*FinalVar.GetName().ToString(), *DefaultValue, *FinalVar.GetType().GetName());
						}
					}

					for (UNiagaraScript* ActiveScript : ActiveScripts)
					{
						ActiveScript->RapidIterationParameters.AddParameter(FinalVar);
					}
				}					
			}
		}

		for (UEdGraphPin* PinToRemove : PinsToRemove)
		{
			if (PinToRemove->IsPendingKill())
			{
				continue;
			}
			UNiagaraNodeParameterMapSet* SetNode = Cast<UNiagaraNodeParameterMapSet>(PinToRemove->GetOwningNode());
			if (SetNode)
			{
				SetNode->RemovePin(PinToRemove);
				bNeedsRecompile = true;
			}
		}

		if (bNeedsRecompile)
		{
			NodeGraph->NotifyGraphNeedsRecompile();
		}
	}
}

UNiagaraScriptSourceBase* UNiagaraScriptSource::MakeRecursiveDeepCopy(UObject* DestOuter, TMap<const UObject*, UObject*>& ExistingConversions) const
{
	check(GetOuter() != DestOuter);
	EObjectFlags Flags = RF_AllFlags & ~RF_Standalone & ~RF_Public; // Remove Standalone and Public flags.
	ResetLoaders(GetTransientPackage()); // Make sure that we're not going to get invalid version number linkers into the package we are going into. 
	GetTransientPackage()->LinkerCustomVersion.Empty();

	UNiagaraScriptSource*	ScriptSource = CastChecked<UNiagaraScriptSource>(StaticDuplicateObject(this, GetTransientPackage(), NAME_None, Flags));
	check(ScriptSource->HasAnyFlags(RF_Standalone) == false);
	check(ScriptSource->HasAnyFlags(RF_Public) == false);

	ScriptSource->Rename(nullptr, DestOuter, REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
	UE_LOG(LogNiagaraEditor, Warning, TEXT("MakeRecursiveDeepCopy %s"), *ScriptSource->GetFullName());
	ExistingConversions.Add(this, ScriptSource);

	ScriptSource->SubsumeExternalDependencies(ExistingConversions);
	return ScriptSource;
}

void UNiagaraScriptSource::SubsumeExternalDependencies(TMap<const UObject*, UObject*>& ExistingConversions)
{
	if (NodeGraph)
	{
		NodeGraph->SubsumeExternalDependencies(ExistingConversions);
	}
}

bool UNiagaraScriptSource::IsSynchronized(const FGuid& InChangeId)
{
	if (NodeGraph)
	{
		return NodeGraph->IsOtherSynchronized(InChangeId);
	}
	else
	{
		return false;
	}
}

void UNiagaraScriptSource::MarkNotSynchronized()
{
	if (NodeGraph)
	{
		NodeGraph->MarkGraphRequiresSynchronization();
	}
}

bool UNiagaraScriptSource::IsPreCompiled() const
{
	return bIsPrecompiled;
}

void UNiagaraScriptSource::PreCompile(UNiagaraEmitter* InEmitter, bool bClearErrors)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraEditor_ScriptSource_PreCompile);
	if (!bIsPrecompiled)
	{
		bIsPrecompiled = true;

		if (bClearErrors)
		{
			//Clear previous graph errors.
			bool bHasClearedGraphErrors = false;
			for (UEdGraphNode* Node : NodeGraph->Nodes)
			{
				if (Node->bHasCompilerMessage)
				{
					Node->ErrorMsg.Empty();
					Node->ErrorType = EMessageSeverity::Info;
					Node->bHasCompilerMessage = false;
					Node->Modify(true);
					bHasClearedGraphErrors = true;
				}
			}
			if (bHasClearedGraphErrors)
			{
				NodeGraph->NotifyGraphChanged();
			}
		}

		// Clone the source graph so we can modify it as needed; merging in the child graphs
		NodeGraphDeepCopy = CastChecked<UNiagaraGraph>(FEdGraphUtilities::CloneGraph(NodeGraph, this, 0));
		FEdGraphUtilities::MergeChildrenGraphsIn(NodeGraphDeepCopy, NodeGraphDeepCopy, /*bRequireSchemaMatch=*/ true);
		
		PrecompiledHistories.Empty();

		TArray<UNiagaraNodeOutput*> OutputNodes;
		NodeGraphDeepCopy->FindOutputNodes(OutputNodes);
		PrecompiledHistories.Empty();
		for (UNiagaraNodeOutput* FoundOutputNode : OutputNodes)
		{
			// Map all for this output node
			FNiagaraParameterMapHistoryBuilder Builder;
			Builder.BeginTranslation(InEmitter);
			Builder.EnableScriptWhitelist(true, FoundOutputNode->GetUsage());
			Builder.BuildParameterMaps(FoundOutputNode, true);
			TArray<FNiagaraParameterMapHistory> Histories = Builder.Histories;
			ensure(Histories.Num() <= 1);
			PrecompiledHistories.Append(Histories);
			Builder.EndTranslation(InEmitter);
		}
	}
}


bool UNiagaraScriptSource::GatherPreCompiledVariables(const FString& InNamespaceFilter, TArray<FNiagaraVariable>& OutVars)
{
	if (!bIsPrecompiled || PrecompiledHistories.Num() == 0)
	{
		return false;
	}

	for (const FNiagaraParameterMapHistory& History : PrecompiledHistories)
	{
		for (const FNiagaraVariable& Var : History.Variables)
		{
			if (FNiagaraParameterMapHistory::IsInNamespace(Var, InNamespaceFilter))
			{
				FNiagaraVariable NewVar = Var;
				if (NewVar.IsDataAllocated() == false && !Var.IsDataInterface())
				{
					FNiagaraEditorUtilities::ResetVariableToDefaultValue(NewVar);
				}
				OutVars.AddUnique(NewVar);
			}
		}
	}
	return true;
}

void UNiagaraScriptSource::PostCompile() 
{
	bIsPrecompiled = false;
	PrecompiledHistories.Empty();
	NodeGraphDeepCopy = nullptr;
}

bool UNiagaraScriptSource::PostLoadFromEmitter(UNiagaraEmitter& OwningEmitter)
{
	bool bNeedsCompile = false;
	const int32 NiagaraCustomVersion = GetLinkerCustomVersion(FNiagaraCustomVersion::GUID);
	if (NiagaraCustomVersion < FNiagaraCustomVersion::ScriptsNowUseAGuidForIdentificationInsteadOfAnIndex)
	{
		TArray<UNiagaraNodeOutput*> OutputNodes;
		NodeGraph->GetNodesOfClass<UNiagaraNodeOutput>(OutputNodes);
		for (int32 i = 0; i < OwningEmitter.GetEventHandlers().Num(); i++)
		{
			const FNiagaraEventScriptProperties& EventScriptProperties = OwningEmitter.GetEventHandlers()[i];
			EventScriptProperties.Script->SetUsageId(FGuid::NewGuid());

			auto FindOutputNodeByUsageIndex = [=](UNiagaraNodeOutput* OutputNode) 
			{ 
				return OutputNode->GetUsage() == ENiagaraScriptUsage::ParticleEventScript && OutputNode->ScriptTypeIndex_DEPRECATED == EventScriptProperties.Script->UsageIndex_DEPRECATED; 
			};
			UNiagaraNodeOutput** MatchingOutputNodePtr = OutputNodes.FindByPredicate(FindOutputNodeByUsageIndex);
			if (MatchingOutputNodePtr != nullptr)
			{
				UNiagaraNodeOutput* MatchingOutputNode = *MatchingOutputNodePtr;
				MatchingOutputNode->SetUsageId(EventScriptProperties.Script->GetUsageId());
			}
		}
		bNeedsCompile = true;
	}
	return bNeedsCompile;
}

bool UNiagaraScriptSource::AddModuleIfMissing(FString ModulePath, ENiagaraScriptUsage Usage, bool& bOutFoundModule)
{
	FStringAssetReference SystemUpdateScriptRef(ModulePath);
	FAssetData ModuleScriptAsset;
	ModuleScriptAsset.ObjectPath = SystemUpdateScriptRef.GetAssetPathName();
	bOutFoundModule = false;

	if (ModuleScriptAsset.IsValid())
	{
		bOutFoundModule = true;
		if (UNiagaraNodeOutput* OutputNode = NodeGraph->FindOutputNode(Usage))
		{
			TArray<UNiagaraNodeFunctionCall*> FoundCalls;
			if (!FNiagaraStackGraphUtilities::FindScriptModulesInStack(ModuleScriptAsset, *OutputNode, FoundCalls))
			{
				FNiagaraStackGraphUtilities::AddScriptModuleToStack(ModuleScriptAsset, *OutputNode);
				return true;
			}
		}
	}

	return false;
}

ENiagaraScriptCompileStatus UNiagaraScriptSource::Compile(UNiagaraScript* ScriptOwner, FString& OutGraphLevelErrorMessages)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraEditor_ScriptSource_Compile);
	bool bDoPostCompile = false;
	if (!bIsPrecompiled)
	{
		PreCompile(nullptr);
		bDoPostCompile = true;
	}

	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::Get().LoadModuleChecked<FNiagaraEditorModule>(TEXT("NiagaraEditor"));
	ENiagaraScriptCompileStatus Status = NiagaraEditorModule.CompileScript(ScriptOwner, OutGraphLevelErrorMessages);
	check(ScriptOwner != nullptr && IsSynchronized(ScriptOwner->GetChangeID()));
	
	if (bDoPostCompile)
	{
		PostCompile();
	}
	return Status;

// 	FNiagaraConstants& ExternalConsts = ScriptOwner->ConstantData.GetExternalConstants();
// 
// 	//Build the constant list. 
// 	//This is mainly just jumping through some hoops for the custom UI. Should be removed and have the UI just read directly from the constants stored in the UScript.
// 	const UEdGraphSchema_Niagara* Schema = CastChecked<UEdGraphSchema_Niagara>(NodeGraph->GetSchema());
// 	ExposedVectorConstants.Empty();
// 	for (int32 ConstIdx = 0; ConstIdx < ExternalConsts.GetNumVectorConstants(); ConstIdx++)
// 	{
// 		FNiagaraVariableInfo Info;
// 		FVector4 Value;
// 		ExternalConsts.GetVectorConstant(ConstIdx, Value, Info);
// 		if (Schema->IsSystemConstant(Info))
// 		{
// 			continue;//System constants are "external" but should not be exposed to the editor.
// 		}
// 			
// 		EditorExposedVectorConstant *Const = new EditorExposedVectorConstant();
// 		Const->ConstName = Info.Name;
// 		Const->Value = Value;
// 		ExposedVectorConstants.Add(MakeShareable(Const));
// 	}

}

void UNiagaraScriptSource::OnGraphChanged(const FEdGraphEditAction &Action)
{
	OnChangedDelegate.Broadcast();
}

void UNiagaraScriptSource::OnGraphDataInterfaceChanged()
{
	OnChangedDelegate.Broadcast();
}

FGuid UNiagaraScriptSource::GetChangeID() 
{ 
	return NodeGraph->GetChangeID(); 
}
