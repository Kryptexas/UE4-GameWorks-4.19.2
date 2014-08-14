// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintVariableNodeSpawner.h"
#include "K2Node_Variable.h"
#include "EditorStyleSettings.h"	// for bShowFriendlyNames
#include "Editor/EditorEngine.h"	// for GetFriendlyName()
#include "ObjectEditorUtils.h"		// for GetCategory()
#include "EdGraphSchema_K2.h"		// for ConvertPropertyToPinType()

#define LOCTEXT_NAMESPACE "BlueprintVariableNodeSpawner"

/*******************************************************************************
 * UBlueprintVariableNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintVariableNodeSpawner* UBlueprintVariableNodeSpawner::Create(TSubclassOf<UK2Node_Variable> NodeClass, UProperty* VarProperty, UObject* Outer/*= nullptr*/)
{
	check(VarProperty != nullptr);
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UBlueprintVariableNodeSpawner* NodeSpawner = NewObject<UBlueprintVariableNodeSpawner>(Outer);
	NodeSpawner->NodeClass      = NodeClass;
	NodeSpawner->MemberVariable = VarProperty;
	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintVariableNodeSpawner* UBlueprintVariableNodeSpawner::Create(TSubclassOf<UK2Node_Variable> NodeClass, UEdGraph* VarContext, FBPVariableDescription const& VarDesc, UObject* Outer/*= nullptr*/)
{
	check(VarContext != nullptr);
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UBlueprintVariableNodeSpawner* NodeSpawner = NewObject<UBlueprintVariableNodeSpawner>(Outer);
	NodeSpawner->NodeClass     = NodeClass;
	NodeSpawner->LocalVarOuter = VarContext;
	NodeSpawner->LocalVarDesc  = VarDesc;
	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintVariableNodeSpawner::UBlueprintVariableNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
{
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintVariableNodeSpawner::Invoke(UEdGraph* ParentGraph, FVector2D const Location) const
{
	FCustomizeNodeDelegate PostSpawnDelegate = CustomizeNodeDelegate;

	if (IsLocalVariable())
	{
		auto LocalVarSetupLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FName VarName, UObject const* VarOuter, FGuid VarGuid, FCustomizeNodeDelegate UserDelegate)
		{
			UK2Node_Variable* VarNode = CastChecked<UK2Node_Variable>(NewNode);
			VarNode->VariableReference.SetLocalMember(VarName, VarOuter->GetName(), VarGuid);
			UserDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);
		};

		if (UObject const* LocalVarOuter = GetVarOuter())
		{
			PostSpawnDelegate = FCustomizeNodeDelegate::CreateStatic(LocalVarSetupLambda, LocalVarDesc.VarName, LocalVarOuter, LocalVarDesc.VarGuid, CustomizeNodeDelegate);
		}
	}
	else if (MemberVariable.IsValid())
	{
		auto MemberVarSetupLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, UProperty const* Property, FCustomizeNodeDelegate UserDelegate)
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(NewNode);
			UClass* OwnerClass = Property->GetOwnerClass();
			bool const bIsSelfContext = Blueprint->SkeletonGeneratedClass->IsChildOf(OwnerClass);

			UK2Node_Variable* VarNode = CastChecked<UK2Node_Variable>(NewNode);
			VarNode->SetFromProperty(Property, bIsSelfContext);
			UserDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);
		};

		UProperty const* Property = MemberVariable.Get();
		PostSpawnDelegate = FCustomizeNodeDelegate::CreateStatic(MemberVarSetupLambda, Property, CustomizeNodeDelegate);
	}

	return Super::Invoke(ParentGraph, Location, PostSpawnDelegate);
}

//------------------------------------------------------------------------------
FText UBlueprintVariableNodeSpawner::GetDefaultMenuName() const
{	
	FText MenuName = GetVariableName();
	if (NodeClass != nullptr)
	{
		if (NodeClass->IsChildOf<UK2Node_VariableGet>())
		{
			MenuName = FText::Format(LOCTEXT("GetterMenuName", "Get {0}"), MenuName);
		}
		else if (NodeClass->IsChildOf<UK2Node_VariableSet>())
		{
			MenuName = FText::Format(LOCTEXT("SetterMenuName", "Set {0}"), MenuName);
		}
	}

	return MenuName;
}

//------------------------------------------------------------------------------
FText UBlueprintVariableNodeSpawner::GetDefaultMenuCategory() const
{
	FText VarSubCategory;
	if (IsLocalVariable())
	{
		VarSubCategory = FText::FromName(LocalVarDesc.Category);
	}
	else if (MemberVariable.IsValid())
	{
		VarSubCategory = FText::FromString(FObjectEditorUtils::GetCategory(MemberVariable.Get()));
	}
	return FText::Format(LOCTEXT("VariableCategory", "Variables|{0}"), VarSubCategory);
}

//------------------------------------------------------------------------------
FName UBlueprintVariableNodeSpawner::GetDefaultMenuIcon(FLinearColor& ColorOut) const
{
	return UK2Node_Variable::GetVarIconFromPinType(GetVarType(), ColorOut);
}

//------------------------------------------------------------------------------
bool UBlueprintVariableNodeSpawner::IsLocalVariable() const
{
	return (LocalVarDesc.VarName != NAME_None);
}

//------------------------------------------------------------------------------
UObject const* UBlueprintVariableNodeSpawner::GetVarOuter() const
{
	UObject const* VarOuter = nullptr;
	if (IsLocalVariable())
	{
		if (LocalVarOuter.IsValid())
		{
			VarOuter = LocalVarOuter.Get();
		}
	}
	else if (MemberVariable.IsValid())
	{
		VarOuter = MemberVariable->GetOuter();
	}	
	return VarOuter;
}

//------------------------------------------------------------------------------
UProperty const* UBlueprintVariableNodeSpawner::GetVarProperty() const
{
	UProperty const* VarProperty = nullptr;
	if (MemberVariable.IsValid())
	{
		VarProperty = MemberVariable.Get();
	}
	return VarProperty;
}

//------------------------------------------------------------------------------
FEdGraphPinType UBlueprintVariableNodeSpawner::GetVarType() const
{
	FEdGraphPinType VarType;
	if (IsLocalVariable())
	{
		VarType = LocalVarDesc.VarType;
	}
	else if (UProperty const* VarProperty = GetVarProperty())
	{
		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		K2Schema->ConvertPropertyToPinType(VarProperty, VarType);
	}
	return VarType;
}

//------------------------------------------------------------------------------
FText UBlueprintVariableNodeSpawner::GetVariableName() const
{
	FText VarName;

	bool bShowFriendlyNames = GetDefault<UEditorStyleSettings>()->bShowFriendlyNames;
	if (IsLocalVariable())
	{
		VarName = bShowFriendlyNames ? FText::FromString(LocalVarDesc.FriendlyName) : FText::FromName(LocalVarDesc.VarName);
	}
	else if (MemberVariable.IsValid())
	{
		VarName = bShowFriendlyNames ? FText::FromString(UEditorEngine::GetFriendlyName(MemberVariable.Get())) : FText::FromName(MemberVariable->GetFName());
	}
	return VarName;
}

#undef LOCTEXT_NAMESPACE
