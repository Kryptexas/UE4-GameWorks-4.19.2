// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeParameterMapBase.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraEditorUtilities.h"
#include "SNiagaraGraphNodeConvert.h"
#include "INiagaraCompiler.h"
#include "NiagaraNodeOutput.h"
#include "ScopedTransaction.h"
#include "SNiagaraGraphPinAdd.h"
#include "NiagaraGraph.h"
#include "NiagaraComponent.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraConstants.h"
#include "NiagaraParameterCollection.h"

#include "IAssetTools.h"
#include "AssetRegistryModule.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "NiagaraNodeParameterMapBase"

UNiagaraNodeParameterMapBase::UNiagaraNodeParameterMapBase() : UNiagaraNodeWithDynamicPins()
{

}

TArray<FNiagaraParameterMapHistory> UNiagaraNodeParameterMapBase::GetParameterMaps(UNiagaraScriptSourceBase* InSource, FString EmitterNameOverride)
{
	TArray<FNiagaraParameterMapHistory> OutputParameterMapHistories;
	UNiagaraScriptSource* Base = Cast<UNiagaraScriptSource>(InSource);
	if (Base != nullptr)
	{
		OutputParameterMapHistories = GetParameterMaps(Base->NodeGraph, EmitterNameOverride);
	}
	return OutputParameterMapHistories;

}

TArray<FNiagaraParameterMapHistory> UNiagaraNodeParameterMapBase::GetParameterMaps(UNiagaraGraph* InGraph, FString EmitterNameOverride)
{
	TArray<UNiagaraNodeOutput*> OutputNodes;
	InGraph->FindOutputNodes(OutputNodes);
	TArray<FNiagaraParameterMapHistory> OutputParameterMapHistories;

	for (UNiagaraNodeOutput* FoundOutputNode : OutputNodes)
	{
		OutputParameterMapHistories.Append(GetParameterMaps(FoundOutputNode, false, EmitterNameOverride));
	}

	return OutputParameterMapHistories;
}

TArray<FNiagaraParameterMapHistory> UNiagaraNodeParameterMapBase::GetParameterMaps(UNiagaraNodeOutput* InGraphEnd, bool bLimitToOutputScriptType, FString EmitterNameOverride)
{
	const UEdGraphSchema_Niagara* Schema = Cast<UEdGraphSchema_Niagara>(InGraphEnd->GetSchema());
	
	FNiagaraParameterMapHistoryBuilder Builder;
	if (!EmitterNameOverride.IsEmpty())
	{
		Builder.EnterEmitter(EmitterNameOverride, nullptr);
	}

	if (bLimitToOutputScriptType)
	{
		Builder.EnableScriptWhitelist(true, InGraphEnd->GetUsage());
	}
	
	Builder.BuildParameterMaps(InGraphEnd);
	
	if (!EmitterNameOverride.IsEmpty())
	{
		Builder.ExitEmitter(EmitterNameOverride, nullptr);
	}
	
	return Builder.Histories;
}


bool UNiagaraNodeParameterMapBase::AllowNiagaraTypeForAddPin(const FNiagaraTypeDefinition& InType)
{
	return  InType != FNiagaraTypeDefinition::GetGenericNumericDef();
}

TSharedRef<SWidget> UNiagaraNodeParameterMapBase::GenerateAddPinMenu(const FString& InWorkingPinName, SNiagaraGraphPinAdd* InPin)
{
	UNiagaraGraph* Graph = GetNiagaraGraph();
	bool IsModule = Graph->FindOutputNode(ENiagaraScriptUsage::Module) != nullptr || Graph->FindOutputNode(ENiagaraScriptUsage::DynamicInput) != nullptr 
		|| Graph->FindOutputNode(ENiagaraScriptUsage::Function) != nullptr;

	bool bSupportsAttributes = true;
	UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Graph->GetOuter());
	if (Source && IsModule)
	{
		UNiagaraScript* Script = Cast<UNiagaraScript>(Source->GetOuter());
		if (Script)
		{
			TArray<ENiagaraScriptUsage> Usages = Script->GetSupportedUsageContexts();
			if (Usages.Contains(ENiagaraScriptUsage::ParticleEventScript) || Usages.Contains(ENiagaraScriptUsage::ParticleSpawnScript) || Usages.Contains(ENiagaraScriptUsage::ParticleUpdateScript))
			{
				bSupportsAttributes = true;
			}
			else
			{
				bSupportsAttributes = false;
			}
		}
	}

	FString WorkingCustomName;
	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddSubMenu(LOCTEXT("CommonEngine", "Common Engine-Provided Variables"),
		LOCTEXT("CommonSystemTooltip", "Create an entry using one of the common engine variables."),
		FNewMenuDelegate::CreateLambda([InWorkingPinName, InPin, this](FMenuBuilder& SubMenuBuilder) { BuildEngineMenu(SubMenuBuilder, InWorkingPinName, InPin); }));

	MenuBuilder.AddSubMenu(LOCTEXT("LocalVars", "Existing Graph Variables"),
		LOCTEXT("LocalVarsTooltip", "Create an entry using existing graph variables."),
		FNewMenuDelegate::CreateLambda([InWorkingPinName, InPin, this](FMenuBuilder& SubMenuBuilder) { BuildLocalMenu(SubMenuBuilder, InWorkingPinName, InPin); }));

	if (bSupportsAttributes)
	{
		MenuBuilder.AddSubMenu(LOCTEXT("CommonAttributes", "Common Attributes"),
			LOCTEXT("CommonAttributesTooltip", "Create an entry using one of the common attributes."),
			FNewMenuDelegate::CreateLambda([InWorkingPinName, InPin, this](FMenuBuilder& SubMenuBuilder) { BuildCommonMenu(SubMenuBuilder, InWorkingPinName, InPin); }));

		WorkingCustomName = TEXT("Particles.") + InWorkingPinName;
		MenuBuilder.AddSubMenu(LOCTEXT("DefineAttribute", "Define Attribute"),
			LOCTEXT("SupportedTypesTooltip", "Create an entry in the particles namespace that you will name immediately after."),
			FNewMenuDelegate::CreateLambda([WorkingCustomName, InPin, this](FMenuBuilder& SubMenuBuilder) { BuildTypeMenu(SubMenuBuilder, WorkingCustomName, InPin); }));
	}

	WorkingCustomName = TEXT("Module.") + InWorkingPinName;
	MenuBuilder.AddSubMenu(LOCTEXT("DefineModuleLocal", "Define Module Local"),
		LOCTEXT("SupportedTypesTooltip", "Create an entry in the module namespace that you will name immediately after."),
		FNewMenuDelegate::CreateLambda([WorkingCustomName, InPin, this](FMenuBuilder& SubMenuBuilder) { BuildTypeMenu(SubMenuBuilder, WorkingCustomName, InPin); }));

	WorkingCustomName = TEXT("ParameterCollections.") + InWorkingPinName;
	MenuBuilder.AddSubMenu(LOCTEXT("ParameterCollectionsMenuTitle", "ParameterCollections"),
		LOCTEXT("ParameterCollectionsMenuTooltip", "Create a reference to a parameter in a Parameter Collection."),
		FNewMenuDelegate::CreateLambda([WorkingCustomName, InPin, this](FMenuBuilder& SubMenuBuilder) { BuildParameterCollectionsMenu(SubMenuBuilder, WorkingCustomName, InPin); }));

	return MenuBuilder.MakeWidget();
}

void UNiagaraNodeParameterMapBase::BuildCommonMenu(FMenuBuilder& InMenuBuilder, const FString& InWorkingName, SNiagaraGraphPinAdd* InPin)
{
	TArray<FNiagaraVariable> Variables = FNiagaraConstants::GetCommonParticleAttributes();

	for (FNiagaraVariable& NamespacedVar : Variables)
	{
		FText Desc = FNiagaraConstants::GetAttributeDescription(NamespacedVar);
		FString NamespacedName = NamespacedVar.GetName().ToString();
		InMenuBuilder.AddMenuEntry(
			FText::FromString(NamespacedName),
			FText::Format(LOCTEXT("AddButtonTypeEntryToolTipFormatCommon", "Add a reference to {0}. {1}"), FText::FromName(NamespacedVar.GetName()), Desc),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(InPin, &SNiagaraGraphPinAdd::OnAddType, NamespacedVar)));
	}
}

void UNiagaraNodeParameterMapBase::BuildParameterCollectionsMenu(FMenuBuilder& InMenuBuilder, const FString& InWorkingName, SNiagaraGraphPinAdd* InPin)
{
	//Create sub menus for parameter collections.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> CollectionAssets;
	AssetRegistryModule.Get().GetAssetsByClass(UNiagaraParameterCollection::StaticClass()->GetFName(), CollectionAssets);
	for (FAssetData& CollectionAsset : CollectionAssets)
	{
		UNiagaraParameterCollection* Collection = CastChecked<UNiagaraParameterCollection>(CollectionAsset.GetAsset());

		InMenuBuilder.AddSubMenu(FText::FromName(CollectionAsset.AssetName),
			FText::FromString(CollectionAsset.GetFullName()),
			FNewMenuDelegate::CreateLambda([Collection, InPin, this](FMenuBuilder& SubMenuBuilder) { BuildParameterCollectionMenu(SubMenuBuilder, Collection, InPin); }));
	}
}

void UNiagaraNodeParameterMapBase::BuildParameterCollectionMenu(FMenuBuilder& InMenuBuilder, UNiagaraParameterCollection* Collection, SNiagaraGraphPinAdd* InPin)
{
	for (const FNiagaraVariable& Parameter : Collection->GetParameters())
	{
		FText Desc = FText::GetEmpty();//FNiagaraConstants::GetAttributeDescription(Parameter);
		FString FriendlyName = Collection->FriendlyNameFromParameterName(Parameter.GetName().ToString());
		InMenuBuilder.AddMenuEntry(
			FText::FromString(FriendlyName),
			FText::Format(LOCTEXT("AddButtonTypeEntryToolTipFormatNPC", "Add a reference to Collection Parameter {0}. {1}"), FText::FromName(Parameter.GetName()), Desc),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(InPin, &SNiagaraGraphPinAdd::OnAddType, Parameter)));
	}
}

void UNiagaraNodeParameterMapBase::BuildLocalMenu(FMenuBuilder& InMenuBuilder, const FString& InWorkingName, SNiagaraGraphPinAdd* InPin)
{
	UNiagaraGraph* Graph = GetNiagaraGraph();
	const TArray<FNiagaraParameterMapHistory> Histories = GetParameterMaps(Graph);

	TArray<FNiagaraVariable> Variables;
	for (const FNiagaraParameterMapHistory& History : Histories)
	{
		for (const FNiagaraVariable& Var : History.Variables)
		{
			Variables.AddUnique(Var);
		}
	}

	Variables.Sort([](const FNiagaraVariable& A, const FNiagaraVariable& B) { return (A.GetName() < B.GetName()); });

	for (FNiagaraVariable& NamespacedVar : Variables)
	{
		FString NamespacedName = NamespacedVar.GetName().ToString();
		InMenuBuilder.AddMenuEntry(
			FText::FromString(NamespacedName),
			FText::Format(LOCTEXT("AddButtonTypeEntryToolTipFormatSystem", "Add a reference to {0}"), FText::FromName(NamespacedVar.GetName())),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(InPin, &SNiagaraGraphPinAdd::OnAddType, NamespacedVar)));
	}
}

void UNiagaraNodeParameterMapBase::BuildEngineMenu(FMenuBuilder& InMenuBuilder, const FString& InWorkingName, SNiagaraGraphPinAdd* InPin)
{
	TArray<FNiagaraVariable> Variables = FNiagaraConstants::GetEngineConstants();
	Variables.Sort([](const FNiagaraVariable& A, const FNiagaraVariable& B) { return (A.GetName() < B.GetName()); });

	for (FNiagaraVariable& Var : Variables)
	{
		FText Desc = FNiagaraConstants::GetEngineConstantDescription(Var);
		FNiagaraVariable NamespacedVar = Var;
		FString NamespacedName = NamespacedVar.GetName().ToString();
		InMenuBuilder.AddMenuEntry(
			FText::FromString(NamespacedName),
			FText::Format(LOCTEXT("AddButtonTypeEntryToolTipFormatSystem", "Add a reference to {0}. {1}"), FText::FromName(Var.GetName()), Desc),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(InPin, &SNiagaraGraphPinAdd::OnAddType, NamespacedVar)));
	}
}

FText UNiagaraNodeParameterMapBase::GetPinDescriptionText(UEdGraphPin* Pin) const
{
	FNiagaraVariable Var = CastChecked<UEdGraphSchema_Niagara>(GetSchema())->PinToNiagaraVariable(Pin);

	const UNiagaraGraph* Graph = GetNiagaraGraph();
	const FNiagaraVariableMetaData* OldMetaData = Graph->GetMetaData(Var);
	if (OldMetaData)
	{
		ensure(OldMetaData->ReferencerNodes.Contains(MakeWeakObjectPtr<UObject>(const_cast<UNiagaraNodeParameterMapBase*>(this))));
		return OldMetaData->Description;
	}
	return FText::GetEmpty();
}

/** Called when a pin's description text is committed. */
void UNiagaraNodeParameterMapBase::PinDescriptionTextCommitted(const FText& Text, ETextCommit::Type CommitType, UEdGraphPin* Pin)
{
	FNiagaraVariable Var = CastChecked<UEdGraphSchema_Niagara>(GetSchema())->PinToNiagaraVariable(Pin);
	UNiagaraGraph* Graph = GetNiagaraGraph();
	if (FNiagaraConstants::IsNiagaraConstant(Var))
	{
		UE_LOG(LogNiagaraEditor, Error, TEXT("You cannot set the description for a Niagara internal constant \"%s\""),*Var.GetName().ToString());
		return;
	}
	FNiagaraVariableMetaData* OldMetaData = Graph->GetMetaData(Var);
	bool bSet = !Text.IsEmptyOrWhitespace();
	if (OldMetaData && !OldMetaData->Description.IsEmptyOrWhitespace())
	{
		bSet = true;
	}

	FScopedTransaction AddNewPinTransaction(LOCTEXT("Rename Pin Desc", "Changed variable description"));
	Modify();
	Pin->Modify();

	if (OldMetaData)
	{
		ensure(OldMetaData->ReferencerNodes.Contains(TWeakObjectPtr<UObject>(this)));
		Graph->Modify();
		OldMetaData->Description = Text;
	}
	else
	{
		Graph->Modify();
		FNiagaraVariableMetaData& NewMetaData = Graph->FindOrAddMetaData(Var);
		NewMetaData.Description = Text;
		NewMetaData.ReferencerNodes.Add(TWeakObjectPtr<UObject>(this));
	}
}


#undef LOCTEXT_NAMESPACE