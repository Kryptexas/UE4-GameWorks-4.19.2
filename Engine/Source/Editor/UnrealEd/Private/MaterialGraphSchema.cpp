// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialGraphSchema.cpp
=============================================================================*/

#include "UnrealEd.h"
#include "AssetData.h"
#include "ScopedTransaction.h"
#include "MaterialEditorUtilities.h"
#include "GraphEditorActions.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "MaterialGraphSchema"

////////////////////////////////////////
// FMaterialGraphSchemaAction_NewNode //

UEdGraphNode* FMaterialGraphSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	check(MaterialExpressionClass);

	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "MaterialEditorNewExpression", "Material Editor: New Expression"));

	UMaterialExpression* NewExpression = FMaterialEditorUtilities::CreateNewMaterialExpression(ParentGraph, MaterialExpressionClass, Location, bSelectNewNode, /*bAutoAssignResource*/true);

	if (NewExpression)
	{
		NewExpression->GraphNode->AutowireNewNode(FromPin);

		return NewExpression->GraphNode;
	}

	return NULL;
}

////////////////////////////////////////////////
// FMaterialGraphSchemaAction_NewFunctionCall //

UEdGraphNode* FMaterialGraphSchemaAction_NewFunctionCall::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "MaterialEditorNewFunctionCall", "Material Editor: New Function Call"));

	UMaterialExpressionMaterialFunctionCall* FunctionNode = CastChecked<UMaterialExpressionMaterialFunctionCall>(
																FMaterialEditorUtilities::CreateNewMaterialExpression(
																	ParentGraph, UMaterialExpressionMaterialFunctionCall::StaticClass(), Location, bSelectNewNode, /*bAutoAssignResource*/false));

	if (!FunctionNode->MaterialFunction)
	{
		UMaterialFunction* MaterialFunction = LoadObject<UMaterialFunction>(NULL, *FunctionPath, NULL, 0, NULL);
		UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(ParentGraph);
		if(FunctionNode->SetMaterialFunction(MaterialGraph->MaterialFunction, NULL, MaterialFunction))
		{
			FMaterialEditorUtilities::UpdateSearchResults(ParentGraph);
			FunctionNode->GraphNode->AutowireNewNode(FromPin);
			return FunctionNode->GraphNode;
		}
		else
		{
			FMaterialEditorUtilities::AddToSelection(ParentGraph, FunctionNode);
			FMaterialEditorUtilities::DeleteSelectedNodes(ParentGraph);
		}
	}

	return NULL;
}

///////////////////////////////////////////
// FMaterialGraphSchemaAction_NewComment //

UEdGraphNode* FMaterialGraphSchemaAction_NewComment::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "MaterialEditorNewComment", "Material Editor: New Comment") );

	UMaterialExpressionComment* NewComment = FMaterialEditorUtilities::CreateNewMaterialExpressionComment(ParentGraph, Location);

	if (NewComment)
	{
		return NewComment->GraphNode;
	}

	return NULL;
}

//////////////////////////////////////
// FMaterialGraphSchemaAction_Paste //

UEdGraphNode* FMaterialGraphSchemaAction_Paste::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	FMaterialEditorUtilities::PasteNodesHere(ParentGraph, Location);
	return NULL;
}

//////////////////////////
// UMaterialGraphSchema //

UMaterialGraphSchema::UMaterialGraphSchema(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PC_Mask = TEXT("mask");
	PC_Required = TEXT("required");
	PC_Optional = TEXT("optional");
	PC_MaterialInput = TEXT("materialinput");

	PSC_Red = TEXT("red");
	PSC_Green = TEXT("green");
	PSC_Blue = TEXT("blue");
	PSC_Alpha = TEXT("alpha");

	ActivePinColor = FLinearColor::White;
	InactivePinColor = FLinearColor(0.05f, 0.05f, 0.05f);
	AlphaPinColor = FLinearColor(0.5f, 0.5f, 0.5f);
}

void UMaterialGraphSchema::GetBreakLinkToSubMenuActions( class FMenuBuilder& MenuBuilder, UEdGraphPin* InGraphPin )
{
	// Make sure we have a unique name for every entry in the list
	TMap< FString, uint32 > LinkTitleCount;

	// Add all the links we could break from
	for(TArray<class UEdGraphPin*>::TConstIterator Links(InGraphPin->LinkedTo); Links; ++Links)
	{
		UEdGraphPin* Pin = *Links;
		FString TitleString = Pin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView);
		FText Title = FText::FromString( TitleString );
		if ( Pin->PinName != TEXT("") )
		{
			TitleString = FString::Printf(TEXT("%s (%s)"), *TitleString, *Pin->PinName);

			// Add name of connection if possible
			FFormatNamedArguments Args;
			Args.Add( TEXT("NodeTitle"), Title );
			Args.Add( TEXT("PinName"), FText::FromString( Pin->PinName ) );
			Title = FText::Format( LOCTEXT("BreakDescPin", "{NodeTitle} ({PinName})"), Args );
		}

		uint32 &Count = LinkTitleCount.FindOrAdd( TitleString );

		FText Description;
		FFormatNamedArguments Args;
		Args.Add( TEXT("NodeTitle"), Title );
		Args.Add( TEXT("NumberOfNodes"), Count );

		if ( Count == 0 )
		{
			Description = FText::Format( LOCTEXT("BreakDesc", "Break link to {NodeTitle}"), Args );
		}
		else
		{
			Description = FText::Format( LOCTEXT("BreakDescMulti", "Break link to {NodeTitle} ({NumberOfNodes})"), Args );
		}
		++Count;
		MenuBuilder.AddMenuEntry( Description, Description, FSlateIcon(), FUIAction(
			FExecuteAction::CreateUObject((UMaterialGraphSchema*const)this, &UMaterialGraphSchema::BreakSinglePinLink, const_cast< UEdGraphPin* >(InGraphPin), *Links) ) );
	}
}

void UMaterialGraphSchema::OnConnectToFunctionOutput(UEdGraphPin* InGraphPin, UEdGraphPin* InFuncPin)
{
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_CreateConnection", "Create Pin Link") );

	TryCreateConnection(InGraphPin, InFuncPin);
}

void UMaterialGraphSchema::OnConnectToMaterial(class UEdGraphPin* InGraphPin, int32 ConnIndex)
{
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_CreateConnection", "Create Pin Link") );

	UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(InGraphPin->GetOwningNode()->GetGraph());

	TryCreateConnection(InGraphPin, MaterialGraph->RootNode->GetInputPin(ConnIndex));
}

void UMaterialGraphSchema::GetPaletteActions(FGraphActionMenuBuilder& ActionMenuBuilder, const FString& CategoryName, bool bMaterialFunction) const
{
	if (CategoryName != TEXT("Functions"))
	{
		FMaterialEditorUtilities::GetMaterialExpressionActions(ActionMenuBuilder, bMaterialFunction);
		GetCommentAction(ActionMenuBuilder);
	}
	if (CategoryName != TEXT("Expressions"))
	{
		GetMaterialFunctionActions(ActionMenuBuilder);
	}
}

bool UMaterialGraphSchema::ConnectionCausesLoop(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const
{
	// Only nodes representing Expressions have outputs
	UMaterialGraphNode* OutputNode = CastChecked<UMaterialGraphNode>(OutputPin->GetOwningNode());

	TArray<UMaterialExpression*> InputExpressions;
	OutputNode->MaterialExpression->GetAllInputExpressions(InputExpressions);

	if (UMaterialGraphNode* InputNode = Cast<UMaterialGraphNode>(InputPin->GetOwningNode()))
	{
		return InputExpressions.Contains(InputNode->MaterialExpression);
	}

	// Simple connection to root node
	return false;
}

bool UMaterialGraphSchema::ArePinsCompatible(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin, FText& ResponseMessage) const
{
	UMaterialGraphNode_Base* InputNode = CastChecked<UMaterialGraphNode_Base>(InputPin->GetOwningNode());
	UMaterialGraphNode* OutputNode = CastChecked<UMaterialGraphNode>(OutputPin->GetOwningNode());
	uint32 InputType = InputNode->GetInputType(InputPin);
	uint32 OutputType = OutputNode->GetOutputType(OutputPin);

	bool bPinsCompatible = CanConnectMaterialValueTypes(InputType, OutputType);
	if (!bPinsCompatible)
	{
		TArray<FText> InputDescriptions;
		TArray<FText> OutputDescriptions;
		GetMaterialValueTypeDescriptions(InputType, InputDescriptions);
		GetMaterialValueTypeDescriptions(OutputType, OutputDescriptions);

		FString CombinedInputDescription;
		FString CombinedOutputDescription;
		for (int32 Index = 0; Index < InputDescriptions.Num(); ++Index)
		{
			if ( CombinedInputDescription.Len() > 0 )
			{
				CombinedInputDescription += TEXT(", ");
			}
			CombinedInputDescription += InputDescriptions[Index].ToString();
		}
		for (int32 Index = 0; Index < OutputDescriptions.Num(); ++Index)
		{
			if ( CombinedOutputDescription.Len() > 0 )
			{
				CombinedOutputDescription += TEXT(", ");
			}
			CombinedOutputDescription += OutputDescriptions[Index].ToString();
		}

		FFormatNamedArguments Args;
		Args.Add( TEXT("InputType"), FText::FromString(CombinedInputDescription) );
		Args.Add( TEXT("OutputType"), FText::FromString(CombinedOutputDescription) );
		ResponseMessage = FText::Format( LOCTEXT("IncompatibleDesc", "{OutputType} is not compatible with {InputType}"), Args );
	}

	return bPinsCompatible;
}

void UMaterialGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(ContextMenuBuilder.CurrentGraph);

	// Run thru all nodes and add any menu items they want to add
	Super::GetGraphContextActions(ContextMenuBuilder);

	// Get the Context Actions from Material Editor Module
	FMaterialEditorUtilities::GetMaterialExpressionActions(ContextMenuBuilder, MaterialGraph->MaterialFunction != NULL);

	// Get the Material Functions as well
	GetMaterialFunctionActions(ContextMenuBuilder);

	GetCommentAction(ContextMenuBuilder, ContextMenuBuilder.CurrentGraph);

	// Add Paste here if appropriate
	if (!ContextMenuBuilder.FromPin && FMaterialEditorUtilities::CanPasteNodes(ContextMenuBuilder.CurrentGraph))
	{
		const FText PasteDesc = LOCTEXT("PasteDesc", "Paste Here");
		const FText PasteToolTip = LOCTEXT("PasteToolTip", "Pastes copied items at this location.");
		TSharedPtr<FMaterialGraphSchemaAction_Paste> PasteAction(new FMaterialGraphSchemaAction_Paste(TEXT(""), PasteDesc.ToString(), PasteToolTip.ToString(), 0));
		ContextMenuBuilder.AddAction(PasteAction);
	}
}

void UMaterialGraphSchema::GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const
{
	if (InGraphPin)
	{
		const UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(CurrentGraph);
		MenuBuilder->BeginSection("MaterialGraphSchemaPinActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
		{
			// Only display the 'Break Link' option if there is a link to break!
			if (InGraphPin->LinkedTo.Num() > 0)
			{
				MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().BreakPinLinks);

				// add sub menu for break link to
				if(InGraphPin->LinkedTo.Num() > 1)
				{
					MenuBuilder->AddSubMenu(
						LOCTEXT("BreakLinkTo", "Break Link To..." ),
						LOCTEXT("BreakSpecificLinks", "Break a specific link..." ),
						FNewMenuDelegate::CreateUObject( (UMaterialGraphSchema*const)this, &UMaterialGraphSchema::GetBreakLinkToSubMenuActions, const_cast<UEdGraphPin*>(InGraphPin)));
				}
				else
				{
					((UMaterialGraphSchema*const)this)->GetBreakLinkToSubMenuActions(*MenuBuilder, const_cast<UEdGraphPin*>(InGraphPin));
				}
			}
		}
		MenuBuilder->EndSection();

		// add menu items to expression output for material connection
		if ( InGraphPin->Direction == EGPD_Output )
		{
			MenuBuilder->BeginSection("MaterialEditorMenuConnector2");
			{
				// If we are editing a material function, display options to connect to function outputs
				if (MaterialGraph->MaterialFunction)
				{
					for (int32 Index = 0; Index < MaterialGraph->Nodes.Num(); Index++)
					{
						UMaterialGraphNode* GraphNode = Cast<UMaterialGraphNode>(MaterialGraph->Nodes[Index]);
						if (GraphNode)
						{
							UMaterialExpressionFunctionOutput* FunctionOutput = Cast<UMaterialExpressionFunctionOutput>(GraphNode->MaterialExpression);
							if (FunctionOutput)
							{
								FFormatNamedArguments Arguments;
								Arguments.Add(TEXT("Name"), FText::FromString( *FunctionOutput->OutputName ));
								const FText Label = FText::Format( LOCTEXT( "ConnectToFunction", "Connect To {Name}" ), Arguments );
								const FText ToolTip = FText::Format( LOCTEXT( "ConnectToFunctionTooltip", "Connects to the function output {Name}" ), Arguments );
								MenuBuilder->AddMenuEntry(Label,
									ToolTip,
									FSlateIcon(),
									FUIAction(FExecuteAction::CreateUObject((UMaterialGraphSchema*const)this, &UMaterialGraphSchema::OnConnectToFunctionOutput, const_cast< UEdGraphPin* >(InGraphPin), GraphNode->GetInputPin(0))));
							}
						}
					}
				}
				else
				{
					for (int32 Index = 0; Index < MaterialGraph->MaterialInputs.Num(); ++Index)
					{
						if( MaterialGraph->IsInputVisible( Index ) )
						{
							FFormatNamedArguments Arguments;
							Arguments.Add(TEXT("Name"), MaterialGraph->MaterialInputs[Index].Name);
							const FText Label = FText::Format( LOCTEXT( "ConnectToInput", "Connect To {Name}" ), Arguments );
							const FText ToolTip = FText::Format( LOCTEXT( "ConnectToInputTooltip", "Connects to the material input {Name}" ), Arguments );
							MenuBuilder->AddMenuEntry(Label,
								ToolTip,
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateUObject((UMaterialGraphSchema*const)this, &UMaterialGraphSchema::OnConnectToMaterial, const_cast< UEdGraphPin* >(InGraphPin), Index)));
						}
					}
				}
			}
			MenuBuilder->EndSection(); //MaterialEditorMenuConnector2
		}
	}
	else if (InGraphNode)
	{
		//Moved all functionality to relevant node classes
	}

	Super::GetContextMenuActions(CurrentGraph, InGraphNode, InGraphPin, MenuBuilder, bIsDebugging);
}

const FPinConnectionResponse UMaterialGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	// Make sure the pins are not on the same node
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionSameNode", "Both are on the same node"));
	}

	// Compare the directions
	const UEdGraphPin* InputPin = NULL;
	const UEdGraphPin* OutputPin = NULL;

	if (!CategorizePinsByDirection(A, B, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionIncompatible", "Directions are not compatible"));
	}

	// Check for new and existing loops
	FText ResponseMessage;
	if (ConnectionCausesLoop(InputPin, OutputPin))
	{
		ResponseMessage = LOCTEXT("ConnectionLoop", "Connection would cause loop");
		// TODO: re-enable this once we're sure any bugs are worked out of the loop checks
		//return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionLoop", "Connection would cause loop").ToString());
	}

	// Check for incompatible pins and get description if they cannot connect
	if (ResponseMessage.IsEmpty() && !ArePinsCompatible(InputPin, OutputPin, ResponseMessage))
	{
		// TODO: re-enable this once we're sure any bugs are worked out of the compatibility checks
		//return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ResponseMessage);
	}

	// Break existing connections on inputs only - multiple output connections are acceptable
	if (InputPin->LinkedTo.Num() > 0)
	{
		ECanCreateConnectionResponse ReplyBreakOutputs;
		if (InputPin == A)
		{
			ReplyBreakOutputs = CONNECT_RESPONSE_BREAK_OTHERS_A;
		}
		else
		{
			ReplyBreakOutputs = CONNECT_RESPONSE_BREAK_OTHERS_B;
		}
		if (ResponseMessage.IsEmpty())
		{
			ResponseMessage = LOCTEXT("ConnectionReplace", "Replace existing connections");
		}
		return FPinConnectionResponse(ReplyBreakOutputs, ResponseMessage);
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, ResponseMessage);
}

bool UMaterialGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	bool bModified = UEdGraphSchema::TryCreateConnection(A, B);

	if (bModified)
	{
		FMaterialEditorUtilities::UpdateMaterialAfterGraphChange(A->GetOwningNode()->GetGraph());
	}

	return bModified;
}

FLinearColor UMaterialGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	if (PinType.PinCategory == PC_Mask)
	{
		if (PinType.PinSubCategory == PSC_Red)
		{
			return FLinearColor::Red;
		}
		else if (PinType.PinSubCategory == PSC_Green)
		{
			return FLinearColor::Green;
		}
		else if (PinType.PinSubCategory == PSC_Blue)
		{
			return FLinearColor::Blue;
		}
		else if (PinType.PinSubCategory == PSC_Alpha)
		{
			return AlphaPinColor;
		}
	}
	else if (PinType.PinCategory == PC_Required)
	{
		return ActivePinColor;
	}
	else if (PinType.PinCategory == PC_Optional)
	{
		return InactivePinColor;
	}

	return ActivePinColor;
}

void UMaterialGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	bool bHasLinksToBreak = false;
	for (auto PinIt = TargetNode.Pins.CreateConstIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* Pin = *PinIt;
		for (auto LinkIt = Pin->LinkedTo.CreateConstIterator(); LinkIt; ++LinkIt)
		{
			if (*LinkIt)
			{
				bHasLinksToBreak = true;
			}
		}
	}

	Super::BreakNodeLinks(TargetNode);

	if (bHasLinksToBreak)
	{
		FMaterialEditorUtilities::UpdateMaterialAfterGraphChange(TargetNode.GetGraph());
	}
}

void UMaterialGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links") );

	bool bHasLinksToBreak = false;
	for (auto LinkIt = TargetPin.LinkedTo.CreateConstIterator(); LinkIt; ++LinkIt)
	{
		if (*LinkIt)
		{
			bHasLinksToBreak = true;
		}
	}

	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);

	// if this would notify the node then we need to re-compile material
	if (bSendsNodeNotifcation && bHasLinksToBreak)
	{
		FMaterialEditorUtilities::UpdateMaterialAfterGraphChange(TargetPin.GetOwningNode()->GetGraph());
	}
}

void UMaterialGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link") );

	bool bHasLinkToBreak = false;
	for (auto LinkIt = SourcePin->LinkedTo.CreateConstIterator(); LinkIt; ++LinkIt)
	{
		if (*LinkIt == TargetPin)
		{
			bHasLinkToBreak = true;
		}
	}

	Super::BreakSinglePinLink(SourcePin, TargetPin);

	if (bHasLinkToBreak)
	{
		FMaterialEditorUtilities::UpdateMaterialAfterGraphChange(SourcePin->GetOwningNode()->GetGraph());
	}
}

void UMaterialGraphSchema::DroppedAssetsOnGraph(const TArray<class FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const
{
	UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(Graph);
	const int32 LocOffsetBetweenNodes = 32;

	FVector2D ExpressionPosition = GraphPosition;

	for (int32 AssetIdx = 0; AssetIdx < Assets.Num(); ++AssetIdx)
	{
		bool bAddedNode = false;
		UObject* Asset = Assets[AssetIdx].GetAsset();
		UClass* MaterialExpressionClass = Cast<UClass>(Asset);
		UMaterialFunction* Func = Cast<UMaterialFunction>(Asset);
		UTexture* Tex = Cast<UTexture>(Asset);
		UMaterialParameterCollection* ParameterCollection = Cast<UMaterialParameterCollection>(Asset);
		
		if (MaterialExpressionClass && MaterialExpressionClass->IsChildOf(UMaterialExpression::StaticClass()))
		{
			FMaterialEditorUtilities::CreateNewMaterialExpression(Graph, MaterialExpressionClass, ExpressionPosition, true, true);
			bAddedNode = true;
		}
		else if ( Func )
		{
			UMaterialExpressionMaterialFunctionCall* FunctionNode = CastChecked<UMaterialExpressionMaterialFunctionCall>(
				FMaterialEditorUtilities::CreateNewMaterialExpression(Graph, UMaterialExpressionMaterialFunctionCall::StaticClass(), ExpressionPosition, true, false));

			if (!FunctionNode->MaterialFunction)
			{
				if(FunctionNode->SetMaterialFunction(MaterialGraph->MaterialFunction, NULL, Func))
				{
					FMaterialEditorUtilities::UpdateSearchResults(Graph);
				}
				else
				{
					FMaterialEditorUtilities::AddToSelection(Graph, FunctionNode);
					FMaterialEditorUtilities::DeleteSelectedNodes(Graph);

					continue;
				}
			}

			bAddedNode = true;
		}
		else if ( Tex )
		{
			UMaterialExpressionTextureSample* TextureSampleNode = CastChecked<UMaterialExpressionTextureSample>(
				FMaterialEditorUtilities::CreateNewMaterialExpression(Graph, UMaterialExpressionTextureSample::StaticClass(), ExpressionPosition, true, true) );
			TextureSampleNode->Texture = Tex;
			TextureSampleNode->AutoSetSampleType();

			FMaterialEditorUtilities::ForceRefreshExpressionPreviews(Graph);

			bAddedNode = true;
		}
		else if ( ParameterCollection )
		{
			UMaterialExpressionCollectionParameter* CollectionParameterNode = CastChecked<UMaterialExpressionCollectionParameter>(
				FMaterialEditorUtilities::CreateNewMaterialExpression(Graph, UMaterialExpressionCollectionParameter::StaticClass(), ExpressionPosition, true, true) );
			CollectionParameterNode->Collection = ParameterCollection;

			FMaterialEditorUtilities::ForceRefreshExpressionPreviews(Graph);

			bAddedNode = true;
		}

		if ( bAddedNode )
		{
			ExpressionPosition.X += LocOffsetBetweenNodes;
			ExpressionPosition.Y += LocOffsetBetweenNodes;
		}
	}
}

int32 UMaterialGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	return FMaterialEditorUtilities::GetNumberOfSelectedNodes(Graph);
}

TSharedPtr<FEdGraphSchemaAction> UMaterialGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FMaterialGraphSchemaAction_NewComment));
}

void UMaterialGraphSchema::GetMaterialFunctionActions(FGraphActionMenuBuilder& ActionMenuBuilder) const
{
	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Collect a full list of assets with the specified class
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByClass(UMaterialFunction::StaticClass()->GetFName(), AssetDataList);

	for (int32 AssetIndex = 0; AssetIndex < AssetDataList.Num(); ++AssetIndex)
	{
		const FAssetData& AssetData = AssetDataList[AssetIndex];

		const bool bExposeToLibrary = AssetData.TagsAndValues.FindRef("bExposeToLibrary") == TEXT("TRUE");

		// If this was a function that was selected to be exposed to the library
		if ( bExposeToLibrary )
		{
			if(AssetData.IsAssetLoaded())
			{
				if(AssetData.GetAsset()->GetOutermost() == GetTransientPackage())
				{
					continue;
				}
			}

			// Gather the relevant information from the asset data
			const FString FunctionPathName = AssetData.ObjectPath.ToString();
			const FString Description = AssetData.TagsAndValues.FindRef("Description");
			TArray<FString> LibraryCategories;
			{
				const FString LibraryCategoriesString = AssetData.TagsAndValues.FindRef("LibraryCategories");
				if ( !LibraryCategoriesString.IsEmpty() )
				{
					UArrayProperty* LibraryCategoriesProperty = FindFieldChecked<UArrayProperty>(UMaterialFunction::StaticClass(), TEXT("LibraryCategories"));
					uint8* DestAddr = (uint8*)(&LibraryCategories);
					LibraryCategoriesProperty->ImportText(*LibraryCategoriesString, DestAddr, PPF_None, NULL, GWarn);
				}

				if ( LibraryCategories.Num() == 0 )
				{
					LibraryCategories.Add( LOCTEXT("UncategorizedMaterialFunction", "Uncategorized").ToString() );
				}
			}

			// Extract the object name from the path
			FString FunctionName = FunctionPathName;
			int32 PeriodIndex = FunctionPathName.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);

			if (PeriodIndex != INDEX_NONE)
			{
				FunctionName = FunctionPathName.Right(FunctionPathName.Len() - PeriodIndex - 1);
			}

			// For each category the function should belong to...
			for (int32 CategoryIndex = 0; CategoryIndex < LibraryCategories.Num(); CategoryIndex++)
			{
				const FString& CategoryName = LibraryCategories[CategoryIndex];
				TSharedPtr<FMaterialGraphSchemaAction_NewFunctionCall> NewFunctionAction(new FMaterialGraphSchemaAction_NewFunctionCall(
					CategoryName,
					FunctionName,
					Description, 0));
				ActionMenuBuilder.AddAction(NewFunctionAction);
				NewFunctionAction->FunctionPath = FunctionPathName;
			}
		}
	}
}

void UMaterialGraphSchema::GetCommentAction(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph) const
{
	if (!ActionMenuBuilder.FromPin)
	{
		const bool bIsManyNodesSelected = CurrentGraph ? (FMaterialEditorUtilities::GetNumberOfSelectedNodes(CurrentGraph) > 0) : false;
		const FText CommentDesc = LOCTEXT("CommentDesc", "New Comment");
		const FText MultiCommentDesc = LOCTEXT("MultiCommentDesc", "Create Comment from Selection");
		const FText CommentToolTip = LOCTEXT("CommentToolTip", "Creates a comment.");
		const FText MenuDescription = bIsManyNodesSelected ? MultiCommentDesc : CommentDesc;
		TSharedPtr<FMaterialGraphSchemaAction_NewComment> NewAction(new FMaterialGraphSchemaAction_NewComment(TEXT(""), MenuDescription.ToString(), CommentToolTip.ToString(), 0));
		ActionMenuBuilder.AddAction( NewAction );
	}
}

#undef LOCTEXT_NAMESPACE
