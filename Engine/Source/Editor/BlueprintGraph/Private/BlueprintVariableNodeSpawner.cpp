// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintVariableNodeSpawner.h"
#include "K2Node_Variable.h"
#include "EditorStyleSettings.h"	// for bShowFriendlyNames
#include "Editor/EditorEngine.h"	// for GetFriendlyName()
#include "ObjectEditorUtils.h"		// for GetCategory()
#include "EdGraphSchema_K2.h"		// for ConvertPropertyToPinType()
#include "EditorCategoryUtils.h"	// for BuildCategoryString()

#define LOCTEXT_NAMESPACE "BlueprintVariableNodeSpawner"

/*******************************************************************************
 * UBlueprintVariableNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintVariableNodeSpawner* UBlueprintVariableNodeSpawner::Create(TSubclassOf<UK2Node_Variable> NodeClass, UProperty const* VarProperty, UObject* Outer/*= nullptr*/)
{
	check(VarProperty != nullptr);
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UBlueprintVariableNodeSpawner* NodeSpawner = NewObject<UBlueprintVariableNodeSpawner>(Outer);
	NodeSpawner->NodeClass = NodeClass;
	NodeSpawner->Field     = VarProperty;

	auto MemberVarSetupLambda = [](UEdGraphNode* NewNode, UField const* Field)
	{
		if (UProperty const* Property = Cast<UProperty>(Field))
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(NewNode);
			UClass* OwnerClass = Property->GetOwnerClass();
			bool const bIsSelfContext = Blueprint->SkeletonGeneratedClass->IsChildOf(OwnerClass);

			UK2Node_Variable* VarNode = CastChecked<UK2Node_Variable>(NewNode);
			VarNode->SetFromProperty(Property, bIsSelfContext);
		}
	};
	NodeSpawner->SetNodeFieldDelegate = FSetNodeFieldDelegate::CreateStatic(MemberVarSetupLambda);

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

	// @TODO: consider splitting out local variable spawners (since they don't 
	//        conform to UBlueprintFieldNodeSpawner
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
void UBlueprintVariableNodeSpawner::Prime()
{
	// we expect that you don't need a node template to construct menu entries
	// from this, so we choose not to pre-cache one here

	// all of these perform expensive FText::Format() operations and cache the 
	// results...
	GetDefaultMenuName();
	GetDefaultMenuCategory();
	GetDefaultMenuTooltip();
}

//------------------------------------------------------------------------------
FBlueprintNodeSpawnerSignature UBlueprintVariableNodeSpawner::GetSpawnerSignature() const
{
	FBlueprintNodeSpawnerSignature SpawnerSignature = Super::GetSpawnerSignature();
	if (IsLocalVariable())
	{
		SpawnerSignature.AddSubObject(LocalVarOuter);
		static const FName LocalVarSignatureKey(TEXT("LocalVarName"));
		SpawnerSignature.AddKeyValue(LocalVarSignatureKey, LocalVarDesc.VarName.ToString());
	}
	
	return SpawnerSignature;
}

//------------------------------------------------------------------------------
UEdGraphNode* UBlueprintVariableNodeSpawner::Invoke(UEdGraph* ParentGraph, FBindingSet const& Bindings, FVector2D const Location) const
{
	UEdGraphNode* NewNode = nullptr;
	// @TODO: consider splitting out local variable spawners (since they don't 
	//        conform to UBlueprintFieldNodeSpawner
	if (IsLocalVariable())
	{
		auto LocalVarSetupLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FName VarName, UObject const* VarOuter, FGuid VarGuid, FCustomizeNodeDelegate UserDelegate)
		{
			UK2Node_Variable* VarNode = CastChecked<UK2Node_Variable>(NewNode);
			VarNode->VariableReference.SetLocalMember(VarName, VarOuter->GetName(), VarGuid);
			UserDelegate.ExecuteIfBound(NewNode, bIsTemplateNode);
		};

		FCustomizeNodeDelegate PostSpawnDelegate = CustomizeNodeDelegate;
		if (UObject const* LocalVarOuter = GetVarOuter())
		{
			PostSpawnDelegate = FCustomizeNodeDelegate::CreateStatic(LocalVarSetupLambda, LocalVarDesc.VarName, LocalVarOuter, LocalVarDesc.VarGuid, CustomizeNodeDelegate);
		}

		NewNode = UBlueprintNodeSpawner::Invoke(ParentGraph, Bindings, Location, PostSpawnDelegate);
	}
	else
	{
		NewNode = Super::Invoke(ParentGraph, Bindings, Location);
	}

	return NewNode;
}

//------------------------------------------------------------------------------
FText UBlueprintVariableNodeSpawner::GetDefaultMenuName() const
{	
	if (CachedMenuName.IsOutOfDate())
	{
		FText VarName = GetVariableName();
		check(NodeClass != nullptr);

		// FText::Format() is slow, so we cache this to save on performance
		if (NodeClass->IsChildOf<UK2Node_VariableGet>())
		{
			CachedMenuName = FText::Format(LOCTEXT("GetterMenuName", "Get {0}"), VarName);
		}
		else if (NodeClass->IsChildOf<UK2Node_VariableSet>())
		{
			CachedMenuName = FText::Format(LOCTEXT("SetterMenuName", "Set {0}"), VarName);
		}
	}
	return CachedMenuName;
}

//------------------------------------------------------------------------------
FText UBlueprintVariableNodeSpawner::GetDefaultMenuCategory() const
{
	if (CachedCategory.IsOutOfDate())
	{
		FText VarSubCategory;
		if (IsLocalVariable())
		{
			VarSubCategory = FText::FromName(LocalVarDesc.Category);
		}
		else if (UProperty const* MemberVariable = GetVarProperty())
		{
			VarSubCategory = FText::FromString(FObjectEditorUtils::GetCategory(MemberVariable));
		}
		CachedCategory = FEditorCategoryUtils::BuildCategoryString(FCommonEditorCategory::Variables, VarSubCategory);
	}
	return CachedCategory;
}

//------------------------------------------------------------------------------
FText UBlueprintVariableNodeSpawner::GetDefaultMenuTooltip() const
{
	if (CachedTooltip.IsOutOfDate())
	{
		if (NodeClass->IsChildOf<UK2Node_VariableSet>())
		{
			if (IsLocalVariable())
			{
				CachedTooltip = UK2Node_VariableSet::GetBlueprintVarTooltip(LocalVarDesc);
			}
			else if (UProperty const* MemberVariable = GetVarProperty())
			{
				CachedTooltip = UK2Node_VariableSet::GetPropertyTooltip(MemberVariable);
			}
		}
		else if (NodeClass->IsChildOf<UK2Node_VariableGet>())
		{
			if (IsLocalVariable())
			{
				CachedTooltip = UK2Node_VariableGet::GetBlueprintVarTooltip(LocalVarDesc);
			}
			else if (UProperty const* MemberVariable = GetVarProperty())
			{
				CachedTooltip = UK2Node_VariableGet::GetPropertyTooltip(MemberVariable);
			}
		}
	}	
	return CachedTooltip;
}

//------------------------------------------------------------------------------
FString UBlueprintVariableNodeSpawner::GetDefaultSearchKeywords() const
{
	// @TODO: maybe UPROPERTY() fields should have keyword metadata like functions
	// 
	// add at least one character, so that the menu item doesn't attempt to
	// ping a node template 	
	return TEXT(" ");
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
		VarOuter = LocalVarOuter;
	}
	else if (UProperty const* MemberVariable = GetVarProperty())
	{
		VarOuter = MemberVariable->GetOuter();
	}	
	return VarOuter;
}

//------------------------------------------------------------------------------
UProperty const* UBlueprintVariableNodeSpawner::GetVarProperty() const
{
	// Get() does IsValid() checks for us
	return Cast<UProperty>(GetField());
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
	else if (UProperty const* MemberVariable = GetVarProperty())
	{
		VarName = bShowFriendlyNames ? FText::FromString(UEditorEngine::GetFriendlyName(MemberVariable)) : FText::FromName(MemberVariable->GetFName());
	}
	return VarName;
}

#undef LOCTEXT_NAMESPACE
