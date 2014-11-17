// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintDelegateNodeSpawner.h"
#include "K2Node_Variable.h"
#include "EditorStyleSettings.h"	// for bShowFriendlyNames
#include "ObjectEditorUtils.h"		// for GetCategory()
#include "BlueprintEditorUtils.h"	// for FindBlueprintForNodeChecked()
#include "EditorCategoryUtils.h"

#define LOCTEXT_NAMESPACE "BlueprintDelegateNodeSpawner"

/*******************************************************************************
 * UBlueprintDelegateNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintDelegateNodeSpawner* UBlueprintDelegateNodeSpawner::Create(TSubclassOf<UK2Node_BaseMCDelegate> NodeClass, UMulticastDelegateProperty const* const Property, UObject* Outer/* = nullptr*/)
{
	check(Property != nullptr);
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UBlueprintDelegateNodeSpawner* NodeSpawner = NewObject<UBlueprintDelegateNodeSpawner>(Outer);
	NodeSpawner->Field     = Property;
	NodeSpawner->NodeClass = NodeClass;

	auto SetDelegateLambda = [](UEdGraphNode* NewNode, UField const* Field)
	{
		UMulticastDelegateProperty const* Property = Cast<UMulticastDelegateProperty>(Field);

		UK2Node_BaseMCDelegate* DelegateNode = Cast<UK2Node_BaseMCDelegate>(NewNode);
		if ((DelegateNode != nullptr) && (Property != nullptr))
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(NewNode);
			UClass* OwnerClass = Property->GetOwnerClass();
			bool const bIsSelfContext = Blueprint->SkeletonGeneratedClass->IsChildOf(OwnerClass);

			DelegateNode->SetFromProperty(Property, bIsSelfContext);
		}
	};
	NodeSpawner->SetNodeFieldDelegate = FSetNodeFieldDelegate::CreateStatic(SetDelegateLambda);

	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintDelegateNodeSpawner::UBlueprintDelegateNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
{
}

//------------------------------------------------------------------------------
FText UBlueprintDelegateNodeSpawner::GetDefaultMenuName(FBindingSet const& Bindings) const
{	
	FText MenuName = Super::GetDefaultMenuName(Bindings);

	UMulticastDelegateProperty const* Property = GetProperty();
	if ((NodeClass == nullptr) && (Property != nullptr))
	{
		bool const bShowFriendlyNames = GetDefault<UEditorStyleSettings>()->bShowFriendlyNames;
		MenuName = bShowFriendlyNames ? FText::FromString(UEditorEngine::GetFriendlyName(Property)) : FText::FromName(Property->GetFName());
	}

	return MenuName;
}

//------------------------------------------------------------------------------
FText UBlueprintDelegateNodeSpawner::GetDefaultMenuCategory() const
{
	FText PropertyCategory;
	if (UMulticastDelegateProperty const* Property = GetProperty())
	{
		PropertyCategory = FText::FromString(FObjectEditorUtils::GetCategory(Property));
	}

	if (PropertyCategory.IsEmpty())
	{
		PropertyCategory = FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Delegates);
	}
	return PropertyCategory;
}

//------------------------------------------------------------------------------
FName UBlueprintDelegateNodeSpawner::GetDefaultMenuIcon(FLinearColor& ColorOut) const
{
	FName BrushName = Super::GetDefaultMenuIcon(ColorOut);
	if (NodeClass == nullptr)
	{
		BrushName = TEXT("GraphEditor.Delegate_16x");
	}
	else if (UMulticastDelegateProperty const* Property = GetProperty())
	{
		FName    const PropertyName  = Property->GetFName();
		UStruct* const PropertyOwner = CastChecked<UStruct>(Property->GetOuterUField());

		BrushName = UK2Node_Variable::GetVariableIconAndColor(PropertyOwner, PropertyName, ColorOut);
	}
	return BrushName;
}

//------------------------------------------------------------------------------
UMulticastDelegateProperty const* UBlueprintDelegateNodeSpawner::GetProperty() const
{
	return Cast<UMulticastDelegateProperty>(GetField());
}

#undef LOCTEXT_NAMESPACE
