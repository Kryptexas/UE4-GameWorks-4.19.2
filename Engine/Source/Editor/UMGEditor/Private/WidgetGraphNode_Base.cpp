// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "Animation/AnimNodeBase.h"
#include "BlueprintEditorUtils.h"
#include "WidgetGraphSchema.h"
#include "K2ActionMenuBuilder.h"

#define LOCTEXT_NAMESPACE "UMG_EDITOR"

/////////////////////////////////////////////////////
// FWidgetOptionalPinManager

struct FWidgetOptionalPinManager : public FOptionalPinManager
{
protected:
	class UWidgetGraphNode_Base* BaseNode;
	TArray<UEdGraphPin*>* OldPins;

	TMap<FString, UEdGraphPin*> OldPinMap;

public:
	FWidgetOptionalPinManager(class UWidgetGraphNode_Base* Node, TArray<UEdGraphPin*>* InOldPins)
		: BaseNode(Node)
		, OldPins(InOldPins)
	{
		if ( OldPins != NULL )
		{
			for ( auto PinIt = OldPins->CreateIterator(); PinIt; ++PinIt )
			{
				UEdGraphPin* Pin = *PinIt;
				OldPinMap.Add(Pin->PinName, Pin);
			}
		}
	}

	virtual void GetRecordDefaults(UProperty* TestProperty, FOptionalPinFromProperty& Record) const override
	{
		const UWidgetGraphSchema* Schema = GetDefault<UWidgetGraphSchema>();

		// Determine if this is a pose or array of poses
		UArrayProperty* ArrayProp = Cast<UArrayProperty>(TestProperty);
		UStructProperty* StructProp = Cast<UStructProperty>(( ArrayProp != NULL ) ? ArrayProp->Inner : TestProperty);
		const bool bIsPoseInput = ( StructProp != NULL ) && ( StructProp->Struct->IsChildOf(FPoseLinkBase::StaticStruct()) );

		//@TODO: Error if they specified two or more of these flags
		const bool bAlwaysShow = TestProperty->HasMetaData(Schema->NAME_AlwaysAsPin) || bIsPoseInput;
		const bool bOptional_ShowByDefault = TestProperty->HasMetaData(Schema->NAME_PinShownByDefault);
		const bool bOptional_HideByDefault = TestProperty->HasMetaData(Schema->NAME_PinHiddenByDefault);
		const bool bNeverShow = TestProperty->HasMetaData(Schema->NAME_NeverAsPin);

		Record.bCanToggleVisibility = bOptional_ShowByDefault || bOptional_HideByDefault;
		Record.bShowPin = bAlwaysShow || bOptional_ShowByDefault;
		// this was for anim node graph, not sure if widget graph node would need it
		Record.bPropertyIsCustomized = false;
	}

	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex, UProperty* Property) const override
	{
		if ( BaseNode != NULL )
		{
			BaseNode->CustomizePinData(Pin, SourcePropertyName, ArrayIndex);
		}
	}

	void PostInitNewPin(UEdGraphPin* Pin, FOptionalPinFromProperty& Record, int32 ArrayIndex, UProperty* Property, uint8* PropertyAddress) const override
	{
		check(PropertyAddress != NULL);
		check(Record.bShowPin);

		if ( OldPins == NULL )
		{
			// Initial construction of a visible pin; copy values from the struct
			FBlueprintEditorUtils::ExportPropertyToKismetDefaultValue(Pin, Property, PropertyAddress, BaseNode);
		}
		else if ( Record.bCanToggleVisibility )
		{
			if ( UEdGraphPin* OldPin = OldPinMap.FindRef(Pin->PinName) )
			{
				// Was already visible
			}
			else
			{
				// Showing a pin that was previously hidden, during a reconstruction
				// Convert the struct property into DefaultValue/DefaultValueObject
				FBlueprintEditorUtils::ExportPropertyToKismetDefaultValue(Pin, Property, PropertyAddress, BaseNode);
			}
		}
	}

	void PostRemovedOldPin(FOptionalPinFromProperty& Record, int32 ArrayIndex, UProperty* Property, uint8* PropertyAddress) const override
	{
		check(PropertyAddress != NULL);
		check(!Record.bShowPin);

		if ( Record.bCanToggleVisibility && ( OldPins != NULL ) )
		{
			const FString OldPinName = ( ArrayIndex != INDEX_NONE ) ? FString::Printf(TEXT("%s_%d"), *( Record.PropertyName.ToString() ), ArrayIndex) : Record.PropertyName.ToString();
			if ( UEdGraphPin* OldPin = OldPinMap.FindRef(OldPinName) )
			{
				// Pin was visible but it's now hidden
				// Convert DefaultValue/DefaultValueObject and push back into the struct
				FBlueprintEditorUtils::ImportKismetDefaultValueToProperty(OldPin, Property, PropertyAddress, BaseNode);
			}
		}
	}

	void AllocateDefaultPins(UStruct* SourceStruct, uint8* StructBasePtr)
	{
			RebuildPropertyList(BaseNode->ShowPinForProperties, SourceStruct);
			CreateVisiblePins(BaseNode->ShowPinForProperties, SourceStruct, EGPD_Input, BaseNode, StructBasePtr);
	}
};

UWidgetGraphNode_Base::UWidgetGraphNode_Base(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UWidgetGraphNode_Base::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = ( PropertyChangedEvent.Property != NULL ) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ( ( PropertyName == TEXT("bShowPin") ) )
	{
		GetSchema()->ReconstructNode(*this);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UWidgetGraphNode_Base::CreateOutputPins()
{
}

void UWidgetGraphNode_Base::InternalPinCreation(TArray<UEdGraphPin*>* OldPins)
{
	// preload required assets first before creating pins
	//PreloadRequiredAssets();

	if ( UStructProperty* NodeStruct = GetFNodeProperty() )
	{
		// Display any currently visible optional pins
		{
			FWidgetOptionalPinManager OptionalPinManager(this, OldPins);
			OptionalPinManager.AllocateDefaultPins(NodeStruct->Struct, NodeStruct->ContainerPtrToValuePtr<uint8>(this));
		}

		// Create the output pin, if needed
		CreateOutputPins();
	}
}

void UWidgetGraphNode_Base::AllocateDefaultPins()
{
	InternalPinCreation(NULL);
}

void UWidgetGraphNode_Base::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	InternalPinCreation(&OldPins);
}

bool UWidgetGraphNode_Base::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const
{
	return true;
	//return DesiredSchema->GetClass()->IsChildOf(UWidgetGraphSchema::StaticClass());
}

void UWidgetGraphNode_Base::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	CreateDefaultMenuEntry(ContextMenuBuilder);
}

TSharedPtr<FEdGraphSchemaAction_K2NewNode> UWidgetGraphNode_Base::CreateDefaultMenuEntry(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UWidgetGraphNode_Base* TemplateNode = NewObject<UWidgetGraphNode_Base>(GetTransientPackage(), GetClass());

	FString Category = TemplateNode->GetNodeCategory();
	FText MenuDesc = TemplateNode->GetNodeTitle(ENodeTitleType::ListView);
	FString Tooltip = TemplateNode->GetTooltip();
	FString Keywords = TemplateNode->GetKeywords();

	TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, Category, MenuDesc, Tooltip, 0, Keywords);
	NodeAction->NodeTemplate = TemplateNode;
	NodeAction->SearchTitle = TemplateNode->GetNodeSearchTitle();

	return NodeAction;
}

void UWidgetGraphNode_Base::GetPinAssociatedProperty(UScriptStruct* NodeType, UEdGraphPin* InputPin, UProperty*& OutProperty, int32& OutIndex)
{
	OutProperty = NULL;
	OutIndex = INDEX_NONE;

	//@TODO: Name-based hackery, avoid the roundtrip and better indicate when it's an array pose pin
	int32 UnderscoreIndex = InputPin->PinName.Find(TEXT("_"));
	if (UnderscoreIndex != INDEX_NONE)
	{
		FString ArrayName = InputPin->PinName.Left(UnderscoreIndex);
		int32 ArrayIndex = FCString::Atoi(*(InputPin->PinName.Mid(UnderscoreIndex + 1)));

		if (UArrayProperty* ArrayProperty = FindField<UArrayProperty>(NodeType, *ArrayName))
		{
			OutProperty = ArrayProperty;
			OutIndex = ArrayIndex;
		}
	}
	else
	{
		if (UProperty* Property = FindField<UProperty>(NodeType, *(InputPin->PinName)))
		{
			OutProperty = Property;
			OutIndex = INDEX_NONE;
		}
	}
}

// Gets the menu category this node belongs in
FString UWidgetGraphNode_Base::GetNodeCategory() const
{
	return LOCTEXT("Widget", "Widget").ToString();
}

// customize pin data based on the input
void UWidgetGraphNode_Base::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{

}

FLinearColor UWidgetGraphNode_Base::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

UScriptStruct* UWidgetGraphNode_Base::GetFNodeType() const
{
	UScriptStruct* BaseFStruct = FWidgetNode_Base::StaticStruct();

	for ( TFieldIterator<UProperty> PropIt(GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt )
	{
		if ( UStructProperty* StructProp = Cast<UStructProperty>(*PropIt) )
		{
			if ( StructProp->Struct->IsChildOf(BaseFStruct) )
			{
				return StructProp->Struct;
			}
		}
	}

	return NULL;
}

UStructProperty* UWidgetGraphNode_Base::GetFNodeProperty() const
{
	UScriptStruct* BaseFStruct = FWidgetNode_Base::StaticStruct();

	for ( TFieldIterator<UProperty> PropIt(GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt )
	{
		if ( UStructProperty* StructProp = Cast<UStructProperty>(*PropIt) )
		{
			if ( StructProp->Struct->IsChildOf(BaseFStruct) )
			{
				return StructProp;
			}
		}
	}

	return NULL;
}

#undef LOCTEXT_NAMESPACE