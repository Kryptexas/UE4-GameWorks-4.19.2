// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "EdGraphSchema_Niagara.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorCommon.h"
#include "INiagaraCompiler.h"
#include "NiagaraCompiler.h"
#include "NiagaraComponent.h"
#include "ScopedTransaction.h"
#include "NiagaraGraph.h"
#include "GraphEditorSettings.h"
#include "GraphEditorActions.h"

#include "NiagaraScript.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeReadDataSet.h"
#include "NiagaraNodeWriteDataSet.h"
#include "NiagaraNodeOp.h"
#include "NiagaraNodeConvert.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraDataInterface.h"
#include "NiagaraNodeIf.h"
#include "MessageDialog.h"
#include "NiagaraScriptSource.h"
#include "NiagaraEmitterProperties.h"

#include "ModuleManager.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "NiagaraSchema"

#define SNAP_GRID (16) // @todo ensure this is the same as SNodePanel::GetSnapGridSize()

const FLinearColor UEdGraphSchema_Niagara::NodeTitleColor_Attribute = FLinearColor::Green;
const FLinearColor UEdGraphSchema_Niagara::NodeTitleColor_Constant = FLinearColor::Red;
const FLinearColor UEdGraphSchema_Niagara::NodeTitleColor_SystemConstant = FLinearColor::White;
const FLinearColor UEdGraphSchema_Niagara::NodeTitleColor_FunctionCall = FLinearColor::Blue;
const FLinearColor UEdGraphSchema_Niagara::NodeTitleColor_Event = FLinearColor::Red;

const FString UEdGraphSchema_Niagara::PinCategoryType("Type");
const FString UEdGraphSchema_Niagara::PinCategoryMisc("Misc");
const FString UEdGraphSchema_Niagara::PinCategoryClass("Class");

namespace 
{
	// Maximum distance a drag can be off a node edge to require 'push off' from node
	const int32 NodeDistance = 60;
}

UEdGraphNode* FNiagaraSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	UEdGraphNode* ResultNode = NULL;

	// If there is a template, we actually use it
	if (NodeTemplate != NULL)
	{
		FString OutErrorMsg;
		if (!NodeTemplate->CanAddToGraph(CastChecked<UNiagaraGraph>(ParentGraph), OutErrorMsg))
		{
			if (OutErrorMsg.Len() > 0)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(OutErrorMsg));
			}
			return ResultNode;
		}

		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "NiagaraEditorNewNode", "Niagara Editor: New Node"));
		ParentGraph->Modify();

		NodeTemplate->SetFlags(RF_Transactional);

		// set outer to be the graph so it doesn't go away
		NodeTemplate->Rename(NULL, ParentGraph, REN_NonTransactional);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		// For input pins, new node will generally overlap node being dragged off
		// Work out if we want to visually push away from connected node
		int32 XLocation = Location.X;
		if (FromPin && FromPin->Direction == EGPD_Input)
		{
			UEdGraphNode* PinNode = FromPin->GetOwningNode();
			const float XDelta = FMath::Abs(PinNode->NodePosX - Location.X);

			if (XDelta < NodeDistance)
			{
				// Set location to edge of current node minus the max move distance
				// to force node to push off from connect node enough to give selection handle
				XLocation = PinNode->NodePosX - NodeDistance;
			}
		}

		NodeTemplate->NodePosX = XLocation;
		NodeTemplate->NodePosY = Location.Y;
		NodeTemplate->SnapToGrid(SNAP_GRID);

		ResultNode = NodeTemplate;

		ParentGraph->NotifyGraphChanged();
	}

	return ResultNode;
}

UEdGraphNode* FNiagaraSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode/* = true*/) 
{
	UEdGraphNode* ResultNode = NULL;

	if (FromPins.Num() > 0)
	{
		ResultNode = PerformAction(ParentGraph, FromPins[0], Location, bSelectNewNode);

		if (ResultNode)
		{
			// Try autowiring the rest of the pins
			for (int32 Index = 1; Index < FromPins.Num(); ++Index)
			{
				ResultNode->AutowireNewNode(FromPins[Index]);
			}
		}
	}
	else
	{
		ResultNode = PerformAction(ParentGraph, NULL, Location, bSelectNewNode);
	}

	return ResultNode;
}

void FNiagaraSchemaAction_NewNode::AddReferencedObjects( FReferenceCollector& Collector )
{
	FEdGraphSchemaAction::AddReferencedObjects( Collector );

	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around
	Collector.AddReferencedObject( NodeTemplate );
}

//////////////////////////////////////////////////////////////////////////

static int32 GbAllowAllNiagaraNodesInEmitterGraphs = 1;
static FAutoConsoleVariableRef CVarAllowAllNiagaraNodesInEmitterGraphs(
	TEXT("niagara.AllowAllNiagaraNodesInEmitterGraphs"),
	GbAllowAllNiagaraNodesInEmitterGraphs,
	TEXT("If true, all nodes will be allowed in the Niagara emitter graphs. \n"),
	ECVF_Default
);

UEdGraphSchema_Niagara::UEdGraphSchema_Niagara(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<FNiagaraSchemaAction_NewNode> AddNewNodeAction(FGraphContextMenuBuilder& ContextMenuBuilder, const FText& Category, const FText& MenuDesc, const FText& Tooltip)
{
	TSharedPtr<FNiagaraSchemaAction_NewNode> NewAction = TSharedPtr<FNiagaraSchemaAction_NewNode>(new FNiagaraSchemaAction_NewNode(Category, MenuDesc, Tooltip, 0));
	ContextMenuBuilder.AddAction( NewAction );
	return NewAction;
}

const UNiagaraGraph* GetAlternateGraph(const UNiagaraGraph* NiagaraGraph)
{
	UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(NiagaraGraph->GetOuter());
	if (ScriptSource != nullptr)
	{
		UNiagaraScript* Script = Cast<UNiagaraScript>(ScriptSource->GetOuter());
		if (Script != nullptr)
		{
			UNiagaraEmitterProperties* EmitterProperties = Cast<UNiagaraEmitterProperties>(Script->GetOuter());
			if (EmitterProperties != nullptr)
			{
				if (EmitterProperties->SpawnScriptProps.Script == Script)
				{
					return CastChecked<UNiagaraScriptSource>(EmitterProperties->UpdateScriptProps.Script->Source)->NodeGraph;
				}
				else if (EmitterProperties->UpdateScriptProps.Script == Script)
				{
					return CastChecked<UNiagaraScriptSource>(EmitterProperties->SpawnScriptProps.Script->Source)->NodeGraph;
				}
			}
		}
	}
	return nullptr;
}

FText GetGraphTypeTitle(const UNiagaraGraph* NiagaraGraph)
{
	UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(NiagaraGraph->GetOuter());
	if (ScriptSource != nullptr)
	{
		UNiagaraScript* Script = Cast<UNiagaraScript>(ScriptSource->GetOuter());
		if (Script != nullptr)
		{
			if (Script->IsSpawnScript())
			{
				return LOCTEXT("Parameter Menu Title Spawn", "Spawn Parameters");
			}
			else if (Script->IsUpdateScript())
			{
				return LOCTEXT("Parameter Menu Title Update", "Update Parameters");
			}
			else if (Script->IsEffectScript())
			{
				return LOCTEXT("Parameter Menu Title Effect", "Effect Parameters");
			}
		}
	}
	return LOCTEXT("Parameter Menu Title Generic", "Script Parameters");
}

void AddParametersForGraph(FGraphContextMenuBuilder& ContextMenuBuilder, const UNiagaraGraph* NiagaraGraph)
{
	FText GraphParameterCategory = GetGraphTypeTitle(NiagaraGraph);
	TArray<UNiagaraNodeInput*> InputNodes;
	NiagaraGraph->GetNodesOfClass(InputNodes);

	TArray<FNiagaraVariable> SeenParams;
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		if (InputNode->Usage == ENiagaraInputNodeUsage::Parameter && !SeenParams.Contains(InputNode->Input))
		{
			SeenParams.Add(InputNode->Input);
			FName Name = InputNode->Input.GetName();
			FText MenuDesc = FText::FromName(Name);
			if (NiagaraGraph != ContextMenuBuilder.CurrentGraph)
			{
				Name = UNiagaraNodeInput::GenerateUniqueName(CastChecked<UNiagaraGraph>(ContextMenuBuilder.CurrentGraph), Name, InputNode->Usage);
				MenuDesc = FText::Format(LOCTEXT("Parameter Menu Copy Param","Copy \"{0}\" to this Graph"), FText::FromName(Name));
			}

			TSharedPtr<FNiagaraSchemaAction_NewNode> ExistingInputAction = AddNewNodeAction(ContextMenuBuilder, GraphParameterCategory, MenuDesc, FText::GetEmpty());

			UNiagaraNodeInput* InputNodeTemplate = NewObject<UNiagaraNodeInput>(ContextMenuBuilder.OwnerOfTemporaries);
			InputNodeTemplate->Input = InputNode->Input;
			InputNodeTemplate->Usage = InputNode->Usage;
			InputNodeTemplate->ExposureOptions = InputNode->ExposureOptions;
			InputNodeTemplate->DataInterface = nullptr;

			// We also support parameters from an alternate graph. If that was used, then we need to take special care
			// to make the parameter unique to that graph.
			if (NiagaraGraph != ContextMenuBuilder.CurrentGraph)
			{
				InputNodeTemplate->Input.SetName(Name);
				InputNodeTemplate->Input.SetId(FGuid::NewGuid());

				if (InputNode->DataInterface)
				{
					InputNodeTemplate->DataInterface = Cast<UNiagaraDataInterface>(StaticDuplicateObject(InputNode->DataInterface, InputNodeTemplate, NAME_None, ~RF_Transient));
				}
			}

			ExistingInputAction->NodeTemplate = InputNodeTemplate;
		}
	}
}

void AddParameterMenuOptions(FGraphContextMenuBuilder& ContextMenuBuilder, const UNiagaraGraph* NiagaraGraph)
{
	AddParametersForGraph(ContextMenuBuilder, NiagaraGraph);

	const UNiagaraGraph* AltGraph = GetAlternateGraph(NiagaraGraph);
	if (AltGraph != nullptr)
	{
		AddParametersForGraph(ContextMenuBuilder, AltGraph);
	}
}

void UEdGraphSchema_Niagara::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const UNiagaraGraph* NiagaraGraph = CastChecked<UNiagaraGraph>(ContextMenuBuilder.CurrentGraph);
	ENiagaraScriptUsage GraphUsage = NiagaraGraph->GetUsage();
	
	// Don't allow adding any other nodes to the effect script for now since it isn't compiled.
	if (GraphUsage == ENiagaraScriptUsage::EffectScript)
	{
		// Add existing parameters...
		AddParameterMenuOptions(ContextMenuBuilder, NiagaraGraph);
		
		// Add in ability to create parameters by type.
		{		
			const FText MenuDescFmt = LOCTEXT("Add ParameterFmt", "Add {0} Parameter");
			const TArray<FNiagaraTypeDefinition>& RegisteredTypes = FNiagaraTypeRegistry::GetRegisteredParameterTypes();
			for (FNiagaraTypeDefinition Type : RegisteredTypes)
			{
				FText MenuCat;
				if (const UClass* Class = Type.GetClass())
				{
					MenuCat = Class->GetMetaDataText(TEXT("Category"), TEXT("UObjectCategory"), Class->GetFullGroupName(false));
				}
				else
				{
					MenuCat = LOCTEXT("AddParameterCat", "Add Parameter");
				}

				const FText MenuDesc = FText::Format(MenuDescFmt, Type.GetStruct()->GetDisplayNameText());
				TSharedPtr<FNiagaraSchemaAction_NewNode> InputAction = AddNewNodeAction(ContextMenuBuilder, MenuCat, MenuDesc, FText::GetEmpty());
				UNiagaraNodeInput* InputNode = NewObject<UNiagaraNodeInput>(ContextMenuBuilder.OwnerOfTemporaries);
				FNiagaraEditorUtilities::InitializeParameterInputNode(*InputNode, Type, NiagaraGraph);
				InputAction->NodeTemplate = InputNode;
			}
		}

		// Create top-level type, matching pin you are dragging off of..
		if (ContextMenuBuilder.FromPin)
		{
			FNiagaraTypeDefinition PinType = PinToTypeDefinition(ContextMenuBuilder.FromPin);

			//Add pin specific menu options.
			if (PinType != FNiagaraTypeDefinition::GetGenericNumericDef())
			{
				const FText MenuDescFmt = LOCTEXT("Add ParameterFmt", "Add {0} Parameter");
				//For correctly typed pins, offer the correct type at the top level.				
				const FText MenuDesc = FText::Format(MenuDescFmt, PinType.GetStruct()->GetDisplayNameText());
				TSharedPtr<FNiagaraSchemaAction_NewNode> InputAction = AddNewNodeAction(ContextMenuBuilder, FText::GetEmpty(), MenuDesc, FText::GetEmpty());
				UNiagaraNodeInput* InputNode = NewObject<UNiagaraNodeInput>(ContextMenuBuilder.OwnerOfTemporaries);
				FNiagaraEditorUtilities::InitializeParameterInputNode(*InputNode, PinType, NiagaraGraph);
				InputAction->NodeTemplate = InputNode;
			}
		}
		return;
	}

	if (GbAllowAllNiagaraNodesInEmitterGraphs || GraphUsage == ENiagaraScriptUsage::Function || GraphUsage == ENiagaraScriptUsage::Module)
	{
		const TArray<FNiagaraOpInfo>& OpInfos = FNiagaraOpInfo::GetOpInfoArray();
		for (const FNiagaraOpInfo& OpInfo : OpInfos)
		{
			TSharedPtr<FNiagaraSchemaAction_NewNode> AddOpAction = AddNewNodeAction(ContextMenuBuilder, OpInfo.Category, OpInfo.FriendlyName, FText::GetEmpty());
			UNiagaraNodeOp* OpNode = NewObject<UNiagaraNodeOp>(ContextMenuBuilder.OwnerOfTemporaries);
			OpNode->OpName = OpInfo.Name;
			AddOpAction->NodeTemplate = OpNode;
		}
	}

	//Add functions
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> ScriptAssets;
	AssetRegistryModule.Get().GetAssetsByClass(UNiagaraScript::StaticClass()->GetFName(), ScriptAssets);
	UEnum* NiagaraScriptUsageEnum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("ENiagaraScriptUsage"), true);
	if (GbAllowAllNiagaraNodesInEmitterGraphs || GraphUsage == ENiagaraScriptUsage::Function || GraphUsage == ENiagaraScriptUsage::Module)
	{
		for (const FAssetData& ScriptAsset : ScriptAssets)
		{
			FName UsageName;
			ScriptAsset.GetTagValue(GET_MEMBER_NAME_CHECKED(UNiagaraScript, Usage), UsageName);
			FString QualifiedUsageName = "ENiagaraScriptUsage::" + UsageName.ToString();
			int32 UsageIndex = NiagaraScriptUsageEnum->GetIndexByNameString(QualifiedUsageName);
			if (UsageIndex != INDEX_NONE)
			{
				ENiagaraScriptUsage Usage = static_cast<ENiagaraScriptUsage>(NiagaraScriptUsageEnum->GetValueByIndex(UsageIndex));
				if (Usage == ENiagaraScriptUsage::Function)
				{
					FString DisplayNameString = FName::NameToDisplayString(ScriptAsset.AssetName.ToString(), false);
					const FText MenuDesc = FText::FromString(DisplayNameString);

					TSharedPtr<FNiagaraSchemaAction_NewNode> FunctionCallAction = AddNewNodeAction(ContextMenuBuilder, LOCTEXT("Function Menu Title", "Functions"), MenuDesc, FText::FromString(ScriptAsset.ObjectPath.ToString()));

					UNiagaraNodeFunctionCall* FunctionCallNode = NewObject<UNiagaraNodeFunctionCall>(ContextMenuBuilder.OwnerOfTemporaries);
					FunctionCallNode->FunctionScriptAssetObjectPath = ScriptAsset.ObjectPath;
					FunctionCallAction->NodeTemplate = FunctionCallNode;
				}
			}
		}
	}

	//Add modules
	//if (GraphUsage == ENiagaraScriptUsage::SpawnScript || GraphUsage == ENiagaraScriptUsage::UpdateScript) No reason not to allow modules as part of other modules?
	{
		for (const FAssetData& ScriptAsset : ScriptAssets)
		{
			FName UsageName;
			ScriptAsset.GetTagValue(GET_MEMBER_NAME_CHECKED(UNiagaraScript, Usage), UsageName);
			FString QualifiedUsageName = "ENiagaraScriptUsage::" + UsageName.ToString();
			int32 UsageIndex = NiagaraScriptUsageEnum->GetIndexByNameString(QualifiedUsageName);
			if (UsageIndex != INDEX_NONE)
			{
				ENiagaraScriptUsage Usage = static_cast<ENiagaraScriptUsage>(NiagaraScriptUsageEnum->GetValueByIndex(UsageIndex));
				if (Usage == ENiagaraScriptUsage::Module)
				{
					FString DisplayNameString = FName::NameToDisplayString(ScriptAsset.AssetName.ToString(), false);
					const FText MenuDesc = FText::FromString(DisplayNameString);

					TSharedPtr<FNiagaraSchemaAction_NewNode> FunctionCallAction = AddNewNodeAction(ContextMenuBuilder, LOCTEXT("Module Menu Title", "Modules"), MenuDesc, FText::FromString(ScriptAsset.ObjectPath.ToString()));

					UNiagaraNodeFunctionCall* FunctionCallNode = NewObject<UNiagaraNodeFunctionCall>(ContextMenuBuilder.OwnerOfTemporaries);
					FunctionCallNode->FunctionScriptAssetObjectPath = ScriptAsset.ObjectPath;
					FunctionCallAction->NodeTemplate = FunctionCallNode;
				}
			}
		}
	}

	//Add event read and writes nodes
	{
		const FText MenuCat = LOCTEXT("NiagaraEventMenuCat", "Events");
		const TArray<FNiagaraTypeDefinition>& RegisteredTypes = FNiagaraTypeRegistry::GetRegisteredPayloadTypes();
		for (FNiagaraTypeDefinition Type : RegisteredTypes)
		{
			if (Type.GetStruct() && !Type.GetStruct()->IsA(UNiagaraDataInterface::StaticClass()))
			{
				{
					const FText MenuDescFmt = LOCTEXT("AddEventReadFmt", "Add {0} Event Read");
					const FText MenuDesc = FText::Format(MenuDescFmt, Type.GetStruct()->GetDisplayNameText());

					TSharedPtr<FNiagaraSchemaAction_NewNode> Action = AddNewNodeAction(ContextMenuBuilder, MenuCat, MenuDesc, FText::GetEmpty());

					UNiagaraNodeReadDataSet* EventReadNode = NewObject<UNiagaraNodeReadDataSet>(ContextMenuBuilder.OwnerOfTemporaries);
					EventReadNode->InitializeFromStruct(Type.GetStruct());
					Action->NodeTemplate = EventReadNode;
				}
				{
					const FText MenuDescFmt = LOCTEXT("AddEventWriteFmt", "Add {0} Event Write");
					const FText MenuDesc = FText::Format(MenuDescFmt, Type.GetStruct()->GetDisplayNameText());

					TSharedPtr<FNiagaraSchemaAction_NewNode> Action = AddNewNodeAction(ContextMenuBuilder, MenuCat, MenuDesc, FText::GetEmpty());

					UNiagaraNodeWriteDataSet* EventWriteNode = NewObject<UNiagaraNodeWriteDataSet>(ContextMenuBuilder.OwnerOfTemporaries);
					EventWriteNode->InitializeFromStruct(Type.GetStruct());
					Action->NodeTemplate = EventWriteNode;
				}
			}
		}
	}

	// Add Convert Nodes
	{
		FNiagaraTypeDefinition PinType = FNiagaraTypeDefinition::GetGenericNumericDef();
		bool bAddMakes = true;
		bool bAddBreaks = true;
		if (ContextMenuBuilder.FromPin)
		{
			PinType = PinToTypeDefinition(ContextMenuBuilder.FromPin);
			if (ContextMenuBuilder.FromPin->Direction == EGPD_Input)
			{
				bAddBreaks = false;
			}
			else
			{
				bAddMakes = false;
			}
		}

		if (PinType.GetScriptStruct())
		{
			FText MakeCat = LOCTEXT("NiagaraMake", "Make");
			FText BreakCat = LOCTEXT("NiagaraBreak", "Break");

			FText DescFmt = LOCTEXT("NiagaraMakeBreakFmt", "{0}");
			auto MakeBreakType = [&](FNiagaraTypeDefinition Type, bool bMake)
			{
				FText Desc = FText::Format(DescFmt, Type.GetStruct()->GetDisplayNameText());
				TSharedPtr<FNiagaraSchemaAction_NewNode> Action = AddNewNodeAction(ContextMenuBuilder, bMake ? MakeCat : BreakCat, Desc, FText::GetEmpty());
				UNiagaraNodeConvert* ConvertNode = NewObject<UNiagaraNodeConvert>(ContextMenuBuilder.OwnerOfTemporaries);
				if (bMake)
				{
					ConvertNode->InitAsMake(Type);
				}
				else
				{
					ConvertNode->InitAsBreak(Type);
				}
				Action->NodeTemplate = ConvertNode;
			};

			if (PinType == FNiagaraTypeDefinition::GetGenericNumericDef())
			{
				if (bAddMakes)
				{
					const TArray<FNiagaraTypeDefinition>& RegisteredTypes = FNiagaraTypeRegistry::GetRegisteredTypes();
					for (FNiagaraTypeDefinition Type : RegisteredTypes)
					{
						// Data interfaces can't be made.
						if (!UNiagaraDataInterface::IsDataInterfaceType(Type))
						{
							MakeBreakType(Type, true);
						}
					}
				}

				if (bAddBreaks)
				{
					const TArray<FNiagaraTypeDefinition>& RegisteredTypes = FNiagaraTypeRegistry::GetRegisteredTypes();
					for (FNiagaraTypeDefinition Type : RegisteredTypes)
					{
						//Don't break scalars. Allow makes for now as a convenient method of getting internal script constants when dealing with numeric pins.
						// Data interfaces can't be broken.
						if (!FNiagaraTypeDefinition::IsScalarDefinition(Type) && !UNiagaraDataInterface::IsDataInterfaceType(Type))
						{
							MakeBreakType(Type, false);
						}
					}
				}
			}
			else
			{
				//If we have a valid type then add it as a convenience at the top level.
				FText TypedMakeBreakFmt = LOCTEXT("NiagaraTypedMakeBreakFmt", "{0} {1}");
				FText Desc = FText::Format(TypedMakeBreakFmt, bAddMakes ? MakeCat : BreakCat, PinType.GetStruct()->GetDisplayNameText());
				TSharedPtr<FNiagaraSchemaAction_NewNode> Action = AddNewNodeAction(ContextMenuBuilder, FText::GetEmpty(), Desc, FText::GetEmpty());
				UNiagaraNodeConvert* ConvertNode = NewObject<UNiagaraNodeConvert>(ContextMenuBuilder.OwnerOfTemporaries);
				if (bAddMakes)
				{
					ConvertNode->InitAsMake(PinType);
				}
				else
				{
					ConvertNode->InitAsBreak(PinType);
				}
				Action->NodeTemplate = ConvertNode;
			}

			//Always add generic convert as an option.
			FText Desc = LOCTEXT("NiagaraConvert", "Convert");
			TSharedPtr<FNiagaraSchemaAction_NewNode> Action = AddNewNodeAction(ContextMenuBuilder, FText::GetEmpty(), Desc, FText::GetEmpty());
			UNiagaraNodeConvert* ConvertNode = NewObject<UNiagaraNodeConvert>(ContextMenuBuilder.OwnerOfTemporaries);
			Action->NodeTemplate = ConvertNode;
		}
	}

	if (ContextMenuBuilder.FromPin)
	{
		//Add pin specific menu options.
		FNiagaraTypeDefinition PinType = PinToTypeDefinition(ContextMenuBuilder.FromPin);

		if (UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(ContextMenuBuilder.FromPin->GetOwningNode()))
		{
			if (UNiagaraDataInterface* DataInterface = InputNode->DataInterface)
			{
				const UClass* Class = InputNode->Input.GetType().GetClass();
				FText MenuCat = Class->GetDisplayNameText();
				TArray<FNiagaraFunctionSignature> Functions;
				DataInterface->GetFunctions(Functions);
				for (FNiagaraFunctionSignature& Sig : Functions)
				{
					TSharedPtr<FNiagaraSchemaAction_NewNode> Action = AddNewNodeAction(ContextMenuBuilder, MenuCat, FText::FromString(Sig.GetName()), FText::GetEmpty());
					UNiagaraNodeFunctionCall* FuncNode = NewObject<UNiagaraNodeFunctionCall>(ContextMenuBuilder.OwnerOfTemporaries);
					Action->NodeTemplate = FuncNode;
					FuncNode->Signature = Sig;
				}
			}
		}

		if (ContextMenuBuilder.FromPin->Direction == EGPD_Output)
		{
			//Add all swizzles for this type if it's a vector.
			if (FHlslNiagaraCompiler::IsHlslBuiltinVector(PinType))
			{
				TArray<FString> Components;
				for (TFieldIterator<UProperty> PropertyIt(PinType.GetStruct(), EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
				{
					UProperty* Property = *PropertyIt;
					Components.Add(Property->GetName().ToLower());
				}

				TArray<FString> Swizzles;
				TFunction<void(FString)> GenSwizzles = [&](FString CurrStr)
				{
					if (CurrStr.Len() == 4) return;//Only generate down to float4
					for (FString& CompStr : Components)
					{
						Swizzles.Add(CurrStr + CompStr);
						GenSwizzles(CurrStr + CompStr);
					}
				};

				GenSwizzles(FString());

				for (FString Swiz : Swizzles)
				{
					const FText Category = LOCTEXT("NiagaraSwizzles", "Swizzles");

					TSharedPtr<FNiagaraSchemaAction_NewNode> Action = AddNewNodeAction(ContextMenuBuilder, Category, FText::FromString(Swiz), FText::GetEmpty());

					UNiagaraNodeConvert* ConvertNode = NewObject<UNiagaraNodeConvert>(ContextMenuBuilder.OwnerOfTemporaries);
					Action->NodeTemplate = ConvertNode;
					ConvertNode->InitAsSwizzle(Swiz);
				}
			}
		}
	}

	//Add all input node options for input pins or no pin.
	if (ContextMenuBuilder.FromPin == nullptr || ContextMenuBuilder.FromPin->Direction == EGPD_Input)
	{
		TArray<UNiagaraNodeInput*> InputNodes;
		NiagaraGraph->GetNodesOfClass(InputNodes);

		//Emitter constants managed by the system.
		const TArray<FNiagaraVariable>& SystemConstants = UNiagaraComponent::GetSystemConstants();
		for (const FNiagaraVariable& SysConst : SystemConstants)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Constant"), FText::FromName(SysConst.GetName()));
			const FText MenuDesc = FText::Format(LOCTEXT("GetSystemConstant", "Get {Constant}"), Args);

			TSharedPtr<FNiagaraSchemaAction_NewNode> GetConstAction = AddNewNodeAction(ContextMenuBuilder, LOCTEXT("System Parameters Menu Title", "System Parameters"), MenuDesc, FText::GetEmpty());

			UNiagaraNodeInput* InputNode = NewObject<UNiagaraNodeInput>(ContextMenuBuilder.OwnerOfTemporaries);
			InputNode->Usage = ENiagaraInputNodeUsage::SystemConstant;
			InputNode->Input = SysConst;
			GetConstAction->NodeTemplate = InputNode;
		}

		//Add menu options for Attributes currently defined by the output node.
		{
			const UNiagaraNodeOutput* OutNode = NiagaraGraph->FindOutputNode();
			if (OutNode)
			{
				for (int32 i = 0; i < OutNode->Outputs.Num(); i++)
				{
					const FNiagaraVariable& Attr = OutNode->Outputs[i];

					FFormatNamedArguments Args;
					Args.Add(TEXT("Attribute"), FText::FromName(Attr.GetName()));
					const FText MenuDesc = FText::Format(LOCTEXT("GetAttribute", "Get {Attribute}"), Args);

					TSharedPtr<FNiagaraSchemaAction_NewNode> GetAttrAction = AddNewNodeAction(ContextMenuBuilder, LOCTEXT("Attributes Menu Title", "Attributes"), MenuDesc, FText::GetEmpty());

					UNiagaraNodeInput* InputNode = NewObject<UNiagaraNodeInput>(ContextMenuBuilder.OwnerOfTemporaries);
					InputNode->Usage = ENiagaraInputNodeUsage::Attribute;
					InputNode->Input = Attr;
					GetAttrAction->NodeTemplate = InputNode;
				}
			}
		}

		AddParameterMenuOptions(ContextMenuBuilder, NiagaraGraph);

		//Add a generic Parameter node to allow easy creation of parameters.
		{
			FNiagaraTypeDefinition PinType = FNiagaraTypeDefinition::GetGenericNumericDef();
			if (ContextMenuBuilder.FromPin)
			{
				PinType = PinToTypeDefinition(ContextMenuBuilder.FromPin);
			}

			if (PinType.GetStruct())
			{
				const FText MenuDescFmt = LOCTEXT("Add ParameterFmt", "Add {0} Parameter");
				const TArray<FNiagaraTypeDefinition>& RegisteredTypes = FNiagaraTypeRegistry::GetRegisteredParameterTypes();
				for (FNiagaraTypeDefinition Type : RegisteredTypes)
				{
					FText MenuCat;
					if (const UClass* Class = Type.GetClass())
					{						
						MenuCat = Class->GetMetaDataText(TEXT("Category"), TEXT("UObjectCategory"), Class->GetFullGroupName(false));
					}
					else
					{
						MenuCat = LOCTEXT("AddParameterCat", "Add Parameter");
					}
						
					const FText MenuDesc = FText::Format(MenuDescFmt, Type.GetStruct()->GetDisplayNameText());
					TSharedPtr<FNiagaraSchemaAction_NewNode> InputAction = AddNewNodeAction(ContextMenuBuilder, MenuCat, MenuDesc, FText::GetEmpty());
					UNiagaraNodeInput* InputNode = NewObject<UNiagaraNodeInput>(ContextMenuBuilder.OwnerOfTemporaries);
					FNiagaraEditorUtilities::InitializeParameterInputNode(*InputNode, Type, NiagaraGraph);
					InputAction->NodeTemplate = InputNode;
				}

				if (PinType != FNiagaraTypeDefinition::GetGenericNumericDef())
				{
					//For correctly typed pins, offer the correct type at the top level.				
					const FText MenuDesc = FText::Format(MenuDescFmt, PinType.GetStruct()->GetDisplayNameText());
					TSharedPtr<FNiagaraSchemaAction_NewNode> InputAction = AddNewNodeAction(ContextMenuBuilder, FText::GetEmpty(), MenuDesc, FText::GetEmpty());
					UNiagaraNodeInput* InputNode = NewObject<UNiagaraNodeInput>(ContextMenuBuilder.OwnerOfTemporaries);
					FNiagaraEditorUtilities::InitializeParameterInputNode(*InputNode, PinType, NiagaraGraph);
					InputAction->NodeTemplate = InputNode;
				}
			}
		}
	}

	const FText MenuCat = LOCTEXT("NiagaraLogicMenuCat", "Logic");
	{
		const FText MenuDesc = LOCTEXT("If", "If");

		TSharedPtr<FNiagaraSchemaAction_NewNode> Action = AddNewNodeAction(ContextMenuBuilder, MenuCat, MenuDesc, FText::GetEmpty());

		UNiagaraNodeIf* IfNode = NewObject<UNiagaraNodeIf>(ContextMenuBuilder.OwnerOfTemporaries);
		Action->NodeTemplate = IfNode;
	}

	//TODO: Add quick commands for certain UNiagaraStructs and UNiagaraScripts to be added as functions
}

const FPinConnectionResponse UEdGraphSchema_Niagara::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are on the same node"));
	}

	// Check both pins support connections
	if(PinA->bNotConnectable || PinB->bNotConnectable)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Pin doesn't support connections."));
	}

	// Compare the directions
	const UEdGraphPin* InputPin = NULL;
	const UEdGraphPin* OutputPin = NULL;

	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Directions are not compatible"));
	}

	// Check for compatible type pins.
	if (PinA->PinType.PinCategory == PinCategoryType && 
		PinB->PinType.PinCategory == PinCategoryType && 
		PinA->PinType != PinB->PinType)
	{
		FNiagaraTypeDefinition PinTypeA = PinToTypeDefinition(PinA);
		FNiagaraTypeDefinition PinTypeB = PinToTypeDefinition(PinB);
		if (FNiagaraTypeDefinition::TypesAreAssignable(PinTypeA, PinTypeB) == false)
		{
			//Do some limiting on auto conversions here?
			if (PinTypeA.GetClass())
			{
				return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Types are not compatible"));
			}
			else
			{
				return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, FString::Printf(TEXT("Convert %s to %s"), *(PinToTypeDefinition(PinA).GetNameText().ToString()), *(PinToTypeDefinition(PinB).GetNameText().ToString())));
			}
		}
	}

	// Check for compatible misc pins
	if (PinA->PinType.PinCategory == PinCategoryMisc ||
		PinB->PinType.PinCategory == PinCategoryMisc) 
	{
		// TODO: This shouldn't be handled explicitly here.
		bool PinAIsConvertAddAndPinBIsNonGenericType =
			PinA->PinType.PinCategory == PinCategoryMisc && PinA->PinType.PinSubCategory == UNiagaraNodeWithDynamicPins::AddPinSubCategory &&
			PinB->PinType.PinCategory == PinCategoryType && PinToTypeDefinition(PinB) != FNiagaraTypeDefinition::GetGenericNumericDef();

		bool PinBIsConvertAddAndPinAIsNonGenericType =
			PinB->PinType.PinCategory == PinCategoryMisc && PinB->PinType.PinSubCategory == UNiagaraNodeWithDynamicPins::AddPinSubCategory &&
			PinA->PinType.PinCategory == PinCategoryType && PinToTypeDefinition(PinA) != FNiagaraTypeDefinition::GetGenericNumericDef();

		if(PinAIsConvertAddAndPinBIsNonGenericType == false && PinBIsConvertAddAndPinAIsNonGenericType == false)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Types are not compatible"));
		}
	}

	if (PinA->PinType.PinCategory == PinCategoryClass || PinB->PinType.PinCategory == PinCategoryClass)
	{
		FNiagaraTypeDefinition AType = PinToTypeDefinition(PinA);
		FNiagaraTypeDefinition BType = PinToTypeDefinition(PinB);
		if (AType != BType)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Types are not compatible"));
		}
	}

	// See if we want to break existing connections (if its an input with an existing connection)
	const bool bBreakExistingDueToDataInput = (InputPin->LinkedTo.Num() > 0);
	if (bBreakExistingDueToDataInput)
	{
		const ECanCreateConnectionResponse ReplyBreakInputs = (PinA == InputPin) ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B;
		return FPinConnectionResponse(ReplyBreakInputs, TEXT("Replace existing input connections"));
	}
	else
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FString());
	}
}

void UEdGraphSchema_Niagara::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) 
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "NiagaraEditorBreakConnection", "Niagara Editor: Break Connection"));

	Super::BreakSinglePinLink(SourcePin, TargetPin);
}

bool UEdGraphSchema_Niagara::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "NiagaraEditorCreateConnection", "Niagara Editor: Create Connection"));

	const FPinConnectionResponse Response = CanCreateConnection(PinA, PinB);
	bool bModified = false;

	switch (Response.Response)
	{
	case CONNECT_RESPONSE_MAKE:
		PinA->Modify();
		PinB->Modify();
		PinA->MakeLinkTo(PinB);
		bModified = true;
		break;

	case CONNECT_RESPONSE_BREAK_OTHERS_A:
		PinA->Modify();
		PinB->Modify();
		PinA->BreakAllPinLinks();
		PinA->MakeLinkTo(PinB);
		bModified = true;
		break;

	case CONNECT_RESPONSE_BREAK_OTHERS_B:
		PinA->Modify();
		PinB->Modify();
		PinB->BreakAllPinLinks();
		PinA->MakeLinkTo(PinB);
		bModified = true;
		break;

	case CONNECT_RESPONSE_BREAK_OTHERS_AB:
		PinA->Modify();
		PinB->Modify();
		PinA->BreakAllPinLinks();
		PinB->BreakAllPinLinks();
		PinA->MakeLinkTo(PinB);
		bModified = true;
		break;

	case CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE:
	{
		if (PinA->Direction == EGPD_Input)
		{
			//Swap so that A is the from pin and B is the to pin.
			UEdGraphPin* Temp = PinA;
			PinA = PinB;
			PinB = Temp;
		}

		FNiagaraTypeDefinition AType = PinToTypeDefinition(PinA);
		FNiagaraTypeDefinition BType = PinToTypeDefinition(PinB);
		if (AType != BType && AType.GetClass() == nullptr && BType.GetClass() == nullptr)
		{
			UEdGraphNode* ANode = PinA->GetOwningNode();
			UEdGraphNode* BNode = PinB->GetOwningNode();
			UEdGraph* Graph = ANode->GetTypedOuter<UEdGraph>();
			
			// Since we'll be adding a node, make sure to modify the graph itself.
			Graph->Modify();
			FGraphNodeCreator<UNiagaraNodeConvert> NodeCreator(*Graph);
			UNiagaraNodeConvert* AutoConvertNode = NodeCreator.CreateNode(false);
			AutoConvertNode->AllocateDefaultPins();
			AutoConvertNode->NodePosX = (ANode->NodePosX + BNode->NodePosX) >> 1;
			AutoConvertNode->NodePosY = (ANode->NodePosY + BNode->NodePosY) >> 1;
			NodeCreator.Finalize();

			if (AutoConvertNode->InitConversion(PinA, PinB))
			{
				PinA->Modify();
				PinB->Modify();
				bModified = true;
			}
			else
			{
				Graph->RemoveNode(AutoConvertNode);
			}
		}
	}
	break;

	case CONNECT_RESPONSE_DISALLOW:
	default:
		break;
	}

#if WITH_EDITOR
	if (bModified)
	{
		PinA->GetOwningNode()->PinConnectionListChanged(PinA);
		PinB->GetOwningNode()->PinConnectionListChanged(PinB);
	}
#endif	//#if WITH_EDITOR

	return bModified;
}

FLinearColor UEdGraphSchema_Niagara::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	const UGraphEditorSettings* Settings = GetDefault<UGraphEditorSettings>();
	if (PinType.PinCategory == PinCategoryType)
	{
		FNiagaraTypeDefinition Type(CastChecked<UScriptStruct>(PinType.PinSubCategoryObject.Get()));

		if (Type == FNiagaraTypeDefinition::GetFloatDef())
		{
			return Settings->FloatPinTypeColor;
		}
		else if (Type == FNiagaraTypeDefinition::GetIntDef())
		{
			return Settings->IntPinTypeColor;
		}
		else if (Type == FNiagaraTypeDefinition::GetBoolDef())
		{
			return Settings->BooleanPinTypeColor;
		}
		else if (Type == FNiagaraTypeDefinition::GetVec2Def()
			|| Type == FNiagaraTypeDefinition::GetVec3Def()
			|| Type == FNiagaraTypeDefinition::GetVec4Def())
		{
			return Settings->VectorPinTypeColor;
		}
		else
		{
			return Settings->StructPinTypeColor;
		}
	}
	else
	{
		return Settings->WildcardPinTypeColor;
	}
}

bool UEdGraphSchema_Niagara::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	check(Pin != NULL);

	if (Pin->bDefaultValueIsIgnored)
	{
		return true;
	}

	return false;
}

FNiagaraVariable UEdGraphSchema_Niagara::PinToNiagaraVariable(const UEdGraphPin* Pin, bool bNeedsValue)const
{
	FNiagaraVariable Var = FNiagaraVariable(PinToTypeDefinition(Pin), *Pin->PinName);
	if (bNeedsValue)
	{
		FString Default;
		if (!Pin->DefaultValue.IsEmpty())
		{
			//Having to do some very hacky and fragile messing with the default value. TODO: Our own pin type in which we can control the default value formatting so that we can just shove it right into hlsl.
			// The subsequent logic for alphanumeric constant testing won't work for bools, as we explicitly look for the string "true" below.
			if (Var.GetType() == FNiagaraTypeDefinition::GetBoolDef())
			{
				if (Pin->DefaultValue.Equals(TEXT("true")))
				{
					Default = Pin->DefaultValue;
				}
			}
			else
			{
				Default.Reserve(Pin->DefaultValue.Len());
				for (int Pos = 0; Pos < Pin->DefaultValue.Len(); ++Pos)
				{
					if ((FChar::IsAlnum(Pin->DefaultValue[Pos]) && !FChar::IsAlpha(Pin->DefaultValue[Pos])) || Pin->DefaultValue[Pos] == TEXT(',') || Pin->DefaultValue[Pos] == TEXT('.') || Pin->DefaultValue[Pos] == TEXT('-'))
					{
						Default.AppendChar(Pin->DefaultValue[Pos]);
					}
				}
			}
		}

		Var.AllocateData();
		if (Default.IsEmpty())
		{
			FMemory::Memset(Var.GetData(), 0, Var.GetType().GetSize());
		}
		else
		{
			FNiagaraTypeDefinition Type = Var.GetType();
			//Here we should have a, delimited array of values to use as a default.
			TArray<FString> SplitValues;
			Default.ParseIntoArray(SplitValues, TEXT(","));
			if (Type == FNiagaraTypeDefinition::GetFloatDef())
			{
				check(SplitValues.Num() == 1);
				float* ValuePtr = (float*)Var.GetData();
				LexicalConversion::FromString(ValuePtr[0], *SplitValues[0]);
			}
			else if (Type == FNiagaraTypeDefinition::GetVec2Def())
			{
				check(SplitValues.Num() == 2);
				float* ValuePtr = (float*)Var.GetData();
				LexicalConversion::FromString(ValuePtr[0], *SplitValues[0]);
				LexicalConversion::FromString(ValuePtr[1], *SplitValues[1]);
			}
			else if (Type == FNiagaraTypeDefinition::GetVec3Def())
			{
				check(SplitValues.Num() == 3);
				float* ValuePtr = (float*)Var.GetData();
				LexicalConversion::FromString(ValuePtr[0], *SplitValues[0]);
				LexicalConversion::FromString(ValuePtr[1], *SplitValues[1]);
				LexicalConversion::FromString(ValuePtr[2], *SplitValues[2]);
			}
			else if (Type == FNiagaraTypeDefinition::GetVec4Def())
			{
				check(SplitValues.Num() == 4);
				float* ValuePtr = (float*)Var.GetData();
				LexicalConversion::FromString(ValuePtr[0], *SplitValues[0]);
				LexicalConversion::FromString(ValuePtr[1], *SplitValues[1]);
				LexicalConversion::FromString(ValuePtr[2], *SplitValues[2]);
				LexicalConversion::FromString(ValuePtr[3], *SplitValues[3]);
			}
			else if (Type == FNiagaraTypeDefinition::GetColorDef())
			{
				check(SplitValues.Num() == 4);
				float* ValuePtr = (float*)Var.GetData();
				LexicalConversion::FromString(ValuePtr[0], *SplitValues[0]);
				LexicalConversion::FromString(ValuePtr[1], *SplitValues[1]);
				LexicalConversion::FromString(ValuePtr[2], *SplitValues[2]);
				LexicalConversion::FromString(ValuePtr[3], *SplitValues[3]);
			}
			else if (Type == FNiagaraTypeDefinition::GetIntDef())
			{
				check(SplitValues.Num() == 1);
				int32* ValuePtr = (int32*)Var.GetData();
				LexicalConversion::FromString(ValuePtr[0], *SplitValues[0]);
			}
			else if (Type == FNiagaraTypeDefinition::GetBoolDef())
			{
				check(SplitValues.Num() == 1);
				int32* ValuePtr = (int32*)Var.GetData();
				ValuePtr[0] = SplitValues[0] == TEXT("true") ? INDEX_NONE : 0;
			}
			else
			{
				//This is easily doable, just need to keep track of all structs used and define them as well as a ctor function signature with all values decomposed into float1/2/3/4 etc
				//Then call said function here with the same decomposition literal values.
				UE_LOG(LogNiagaraEditor, Warning, TEXT("Constants of struct types are currently unsupported."));
			}
		}
	}

	return Var;
}

FNiagaraTypeDefinition UEdGraphSchema_Niagara::PinToTypeDefinition(const UEdGraphPin* Pin) const
{
	if (Pin->PinType.PinCategory == PinCategoryType && Pin->PinType.PinSubCategoryObject != nullptr)
	{
		return CastChecked<const UScriptStruct>(Pin->PinType.PinSubCategoryObject.Get());
	}
	else if (Pin->PinType.PinCategory == PinCategoryClass)
	{
		return FNiagaraTypeDefinition(CastChecked<const UClass>(Pin->PinType.PinSubCategoryObject.Get()));
	}
	return FNiagaraTypeDefinition();
}

FEdGraphPinType UEdGraphSchema_Niagara::TypeDefinitionToPinType(FNiagaraTypeDefinition TypeDef)const
{
	if (TypeDef.GetClass())
	{
		return FEdGraphPinType(PinCategoryClass, FString(), const_cast<UClass*>(TypeDef.GetClass()), EPinContainerType::None, false, FEdGraphTerminalType());
	}
	else
	{
		//TODO: Are base types better as structs or done like BPS as a special name?
		return FEdGraphPinType(PinCategoryType, FString(), const_cast<UScriptStruct*>(TypeDef.GetScriptStruct()), EPinContainerType::None, false, FEdGraphTerminalType());
	}
}

bool UEdGraphSchema_Niagara::IsSystemConstant(const FNiagaraVariable& Variable)const
{
	return UNiagaraComponent::GetSystemConstants().Find(Variable) != INDEX_NONE;
}

FNiagaraTypeDefinition UEdGraphSchema_Niagara::GetTypeDefForProperty(const UProperty* Property)const
{
	if (Property->IsA(UFloatProperty::StaticClass()))
	{
		return FNiagaraTypeDefinition::GetFloatDef();
	}
	else if (Property->IsA(UIntProperty::StaticClass()))
	{
		return FNiagaraTypeDefinition::GetIntDef();
	}
	else if (Property->IsA(UBoolProperty::StaticClass()))
	{
		return FNiagaraTypeDefinition::GetBoolDef();
	}	
	else if (const UStructProperty* StructProp = CastChecked<UStructProperty>(Property))
	{
		return FNiagaraTypeDefinition(StructProp->Struct);
	}

	check(0);
	return FNiagaraTypeDefinition::GetFloatDef();//Some invalid type?
}

void UEdGraphSchema_Niagara::GetBreakLinkToSubMenuActions(class FMenuBuilder& MenuBuilder, UEdGraphPin* InGraphPin)
{
	// Make sure we have a unique name for every entry in the list
	TMap< FString, uint32 > LinkTitleCount;

	// Add all the links we could break from
	for (TArray<class UEdGraphPin*>::TConstIterator Links(InGraphPin->LinkedTo); Links; ++Links)
	{
		UEdGraphPin* Pin = *Links;
		FString TitleString = Pin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView).ToString();
		FText Title = FText::FromString(TitleString);
		if (!Pin->PinName.IsEmpty())
		{
			TitleString = FString::Printf(TEXT("%s (%s)"), *TitleString, *Pin->PinName);

			// Add name of connection if possible
			FFormatNamedArguments Args;
			Args.Add(TEXT("NodeTitle"), Title);
			Args.Add(TEXT("PinName"), Pin->GetDisplayName());
			Title = FText::Format(LOCTEXT("BreakDescPin", "{NodeTitle} ({PinName})"), Args);
		}

		uint32 &Count = LinkTitleCount.FindOrAdd(TitleString);

		FText Description;
		FFormatNamedArguments Args;
		Args.Add(TEXT("NodeTitle"), Title);
		Args.Add(TEXT("NumberOfNodes"), Count);

		if (Count == 0)
		{
			Description = FText::Format(LOCTEXT("BreakDesc", "Break link to {NodeTitle}"), Args);
		}
		else
		{
			Description = FText::Format(LOCTEXT("BreakDescMulti", "Break link to {NodeTitle} ({NumberOfNodes})"), Args);
		}
		++Count;
		MenuBuilder.AddMenuEntry(Description, Description, FSlateIcon(), FUIAction(
			FExecuteAction::CreateUObject((UEdGraphSchema_Niagara*const)this, &UEdGraphSchema::BreakSinglePinLink, const_cast<UEdGraphPin*>(InGraphPin), *Links)));
	}
}

void UEdGraphSchema_Niagara::ConvertNumericPinToType(UEdGraphPin* InGraphPin, FNiagaraTypeDefinition TypeDef)
{
	if (PinToTypeDefinition(InGraphPin) != TypeDef)
	{
		UNiagaraNode* Node = Cast<UNiagaraNode>(InGraphPin->GetOwningNode());
		if (Node)
		{
			FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "NiagaraEditorChangeNumericPinType", "Change Pin Type"));
			if (false == Node->ConvertNumericPinToType(InGraphPin, TypeDef))
			{
				Transaction.Cancel();
			}
		}
	}
}

void UEdGraphSchema_Niagara::GetNumericConversionToSubMenuActions(class FMenuBuilder& MenuBuilder, UEdGraphPin* InGraphPin)
{
	// Add all the types we could convert to
	for (const FNiagaraTypeDefinition& TypeDef : FNiagaraTypeDefinition::GetNumericTypes())
	{
		FText Title = TypeDef.GetNameText();

		FText Description;
		FFormatNamedArguments Args;
		Args.Add(TEXT("TypeTitle"), Title);
		Description = FText::Format(LOCTEXT("NumericConversionText", "{TypeTitle}"), Args);
		MenuBuilder.AddMenuEntry(Description, Description, FSlateIcon(), FUIAction(
			FExecuteAction::CreateUObject((UEdGraphSchema_Niagara*const)this, &UEdGraphSchema_Niagara::ConvertNumericPinToType, const_cast<UEdGraphPin*>(InGraphPin), FNiagaraTypeDefinition(TypeDef))));
	}
}

void UEdGraphSchema_Niagara::GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const
{
	if (InGraphPin)
	{
		MenuBuilder->BeginSection("EdGraphSchema_NiagaraPinActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
		{
			if (PinToTypeDefinition(InGraphPin) == FNiagaraTypeDefinition::GetGenericNumericDef() && InGraphPin->LinkedTo.Num() == 0)
			{
				MenuBuilder->AddSubMenu(
					LOCTEXT("ConvertNumericSpecific", "Convert Numeric To..."),
					LOCTEXT("ConvertNumericSpecificToolTip", "Convert Numeric pin to specific typed pin."),
				FNewMenuDelegate::CreateUObject((UEdGraphSchema_Niagara*const)this, &UEdGraphSchema_Niagara::GetNumericConversionToSubMenuActions, const_cast<UEdGraphPin*>(InGraphPin)));
			}

			// Only display the 'Break Link' option if there is a link to break!
			if (InGraphPin->LinkedTo.Num() > 0)
			{
				MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().BreakPinLinks);

				// add sub menu for break link to
				if (InGraphPin->LinkedTo.Num() > 1)
				{
					MenuBuilder->AddSubMenu(
						LOCTEXT("BreakLinkTo", "Break Link To..."),
						LOCTEXT("BreakSpecificLinks", "Break a specific link..."),
						FNewMenuDelegate::CreateUObject((UEdGraphSchema_Niagara*const)this, &UEdGraphSchema_Niagara::GetBreakLinkToSubMenuActions, const_cast<UEdGraphPin*>(InGraphPin)));
				}
				else
				{
					((UEdGraphSchema_Niagara*const)this)->GetBreakLinkToSubMenuActions(*MenuBuilder, const_cast<UEdGraphPin*>(InGraphPin));
				}
			}
		}
		MenuBuilder->EndSection();
	}
	else if (InGraphNode)
	{
	}

	Super::GetContextMenuActions(CurrentGraph, InGraphNode, InGraphPin, MenuBuilder, bIsDebugging);
}

#undef LOCTEXT_NAMESPACE
