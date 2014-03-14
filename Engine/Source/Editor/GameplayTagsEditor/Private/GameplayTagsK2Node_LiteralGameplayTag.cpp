// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsEditorModulePrivatePCH.h"
#include "KismetCompiler.h"
#include "EngineKismetLibraryClasses.h"
#include "GameplayTags.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()

UGameplayTagsK2Node_LiteralGameplayTag::UGameplayTagsK2Node_LiteralGameplayTag(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UGameplayTagsK2Node_LiteralGameplayTag::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, K2Schema->PC_Struct, TEXT(""), FGameplayTagContainer::StaticStruct(), false, false, TEXT("TagIn"));
	CreatePin(EGPD_Output, K2Schema->PC_Struct, TEXT(""), FGameplayTagContainer::StaticStruct(), false, false, K2Schema->PN_ReturnValue);
}

FLinearColor UGameplayTagsK2Node_LiteralGameplayTag::GetNodeTitleColor() const
{
	return FLinearColor(1.0f, 0.51f, 0.0f);
}

FString UGameplayTagsK2Node_LiteralGameplayTag::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "LiteralGameplayTag", "Make Literal GameplayTagContainer").ToString();
}

bool UGameplayTagsK2Node_LiteralGameplayTag::CanCreateUnderSpecifiedSchema( const UEdGraphSchema* Schema ) const
{
	return Schema->IsA(UEdGraphSchema_K2::StaticClass());
}

void UGameplayTagsK2Node_LiteralGameplayTag::GetMenuEntries( FGraphContextMenuBuilder& ContextMenuBuilder ) const
{
	Super::GetMenuEntries(ContextMenuBuilder);
	
	const FString FunctionCategory(TEXT("Call Function"));

	UK2Node* EnumNodeTemplate = ContextMenuBuilder.CreateTemplateNode<UGameplayTagsK2Node_LiteralGameplayTag>();

	const FString Category = FunctionCategory + TEXT("|Game");
	const FString MenuDesc = EnumNodeTemplate->GetNodeTitle(ENodeTitleType::ListView);
	const FString Tooltip = EnumNodeTemplate->GetTooltip();
	const FString Keywords = EnumNodeTemplate->GetKeywords();

	TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, Category, MenuDesc, Tooltip, 0, Keywords);
	NodeAction->NodeTemplate = EnumNodeTemplate;
}

void UGameplayTagsK2Node_LiteralGameplayTag::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	if (CompilerContext.bIsFullCompile)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// Get The input and output pins to our node
		UEdGraphPin* TagInPin = FindPin(TEXT("TagIn"));
		UEdGraphPin* TagOutPin = FindPinChecked(Schema->PN_ReturnValue);

		// Create a Make Struct
		UK2Node_MakeStruct* MakeStructNode = SourceGraph->CreateBlankNode<UK2Node_MakeStruct>();
		MakeStructNode->StructType = FGameplayTagContainer::StaticStruct();
		MakeStructNode->AllocateDefaultPins();

		// Create a Make Array
		UK2Node_MakeArray* MakeArrayNode = SourceGraph->CreateBlankNode<UK2Node_MakeArray>();
		MakeArrayNode->AllocateDefaultPins();
		
		// Connect the output of our MakeArray to the Input of our MakeStruct so it sets the Array Type
		UEdGraphPin* InPin = MakeStructNode->FindPin( TEXT("Tags") );
		if( InPin )
		{
			InPin->MakeLinkTo( MakeArrayNode->GetOutputPin() );
		}

		// Add the FName Values to the MakeArray input pins
		UEdGraphPin* ArrayInputPin = NULL;
		FString TagString = TagInPin->GetDefaultAsString();

		if( TagString.StartsWith( TEXT("(") ) && TagString.EndsWith( TEXT(")") ) )
		{
			TagString = TagString.LeftChop(1);
			TagString = TagString.RightChop(1);

			FString ReadTag;
			FString Remainder;
			int32 MakeIndex = 0;
			while( TagString.Split( TEXT(","), &ReadTag, &Remainder ) ) 
			{
				TagString = Remainder;

				ArrayInputPin = MakeArrayNode->FindPin( FString::Printf( TEXT("[%d]"), MakeIndex ) );
				ArrayInputPin->PinType.PinCategory = TEXT("name");
				ReadTag.Split( "=", NULL, &ReadTag );
				ArrayInputPin->DefaultValue = ReadTag;

				MakeIndex++;
				MakeArrayNode->AddInputPin();
			}
			if( !Remainder.IsEmpty() )
			{
				ArrayInputPin = MakeArrayNode->FindPin( FString::Printf( TEXT("[%d]"), MakeIndex ) );
				ArrayInputPin->PinType.PinCategory = TEXT("name");
				Remainder.Split( "=", NULL, &Remainder );
				ArrayInputPin->DefaultValue = Remainder;
			}
		}

		// Move the Output of the MakeArray to the Output of our node
		UEdGraphPin* OutPin = MakeStructNode->FindPin( MakeStructNode->StructType->GetName() );
		if( OutPin && TagOutPin )
		{
			OutPin->PinType = TagOutPin->PinType; // Copy type so it uses the right actor subclass
			CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*TagOutPin, *OutPin), this);
		}

		// Break any links to the expanded node
		BreakAllNodeLinks();
	}
}
