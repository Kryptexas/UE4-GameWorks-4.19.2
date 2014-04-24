// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"

#include "BlueprintUtilities.h"
#include "BlueprintEditorUtils.h"
#include "GraphEditorDragDropAction.h"
#include "BPVariableDragDropAction.h"
#include "SBlueprintPalette.h"

#define LOCTEXT_NAMESPACE "VariableDragDropAction"

FKismetVariableDragDropAction::FKismetVariableDragDropAction()
	: VariableName(NAME_None)
	, bControlDrag(false)
	, bAltDrag(false)
{
}

void FKismetVariableDragDropAction::GetLinksThatWillBreak(	UEdGraphNode* Node, UProperty* NewVariableProperty, 
						   TArray<class UEdGraphPin*>& OutBroken)
{
	if(UK2Node_Variable* VarNodeUnderCursor = Cast<UK2Node_Variable>(Node))
	{
		if(const UEdGraphSchema_K2* Schema = Cast<const UEdGraphSchema_K2>(VarNodeUnderCursor->GetSchema()) )
		{
			FEdGraphPinType NewPinType;
			Schema->ConvertPropertyToPinType(NewVariableProperty,NewPinType);
			if(UEdGraphPin* Pin = VarNodeUnderCursor->FindPin(VarNodeUnderCursor->GetVarNameString()))
			{
				for(TArray<class UEdGraphPin*>::TIterator i(Pin->LinkedTo);i;i++)
				{
					UEdGraphPin* Link = *i;
					if(false == Schema->ArePinTypesCompatible(NewPinType, Link->PinType))
					{
						OutBroken.Add(Link);
					}
				}
			}
		}
	}
}

void FKismetVariableDragDropAction::HoverTargetChanged()
{
	UProperty* VariableProperty = GetVariableProperty();
	FString VariableString = VariableName.ToString();

	// Icon/text to draw on tooltip
	FSlateColor IconColor = FLinearColor::White;
	const FSlateBrush* StatusSymbol = FEditorStyle::GetBrush(TEXT("NoBrush")); 
	FText Message;

	UEdGraphPin* PinUnderCursor = GetHoveredPin();

	bool bBadSchema = false;
	if (UEdGraph* Graph = GetHoveredGraph())
	{
		if (Cast<const UEdGraphSchema_K2>(Graph->GetSchema()) == NULL)
		{
			bBadSchema = true;
		}
	}

	UEdGraphNode* VarNodeUnderCursor = Cast<UK2Node_Variable>(GetHoveredNode());

	if (bBadSchema)
	{
		StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
		Message = LOCTEXT("CannotCreateInThisSchema", "Cannot access variables in this type of graph");
	}
	else if (PinUnderCursor != NULL)
	{
		const UEdGraphSchema_K2* Schema = CastChecked<const UEdGraphSchema_K2>(PinUnderCursor->GetSchema());

		const bool bIsRead = PinUnderCursor->Direction == EGPD_Input;
		const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(PinUnderCursor->GetOwningNode());
		const bool bReadOnlyProperty = FBlueprintEditorUtils::IsPropertyReadOnlyInCurrentBlueprint(Blueprint, VariableProperty);
		const bool bCanWriteIfNeeded = bIsRead || !bReadOnlyProperty;

		FEdGraphPinType VariablePinType;
		Schema->ConvertPropertyToPinType(VariableProperty, VariablePinType);
		const bool bTypeMatch = Schema->ArePinTypesCompatible(VariablePinType, PinUnderCursor->PinType);

		FFormatNamedArguments Args;
		Args.Add(TEXT("PinUnderCursor"), FText::FromString(PinUnderCursor->PinName));
		Args.Add(TEXT("VariableName"), FText::FromString(VariableString));

		if (bTypeMatch && bCanWriteIfNeeded)
		{
			StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));

			if (bIsRead)
			{
				Message = FText::Format(LOCTEXT("MakeThisEqualThat", "Make {PinUnderCursor} = {VariableName}"), Args);
			}
			else
			{
				Message = FText::Format(LOCTEXT("MakeThisEqualThat", "Make {VariableName} = {PinUnderCursor}"), Args);
			}
		}
		else
		{
			StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
			if (!bCanWriteIfNeeded)
			{
				Message = FText::Format(LOCTEXT("ReadOnlyVar_Error", "Cannot write to read-only variable '{VariableName}'"), Args);
			}
			else
			{
				Message = FText::Format(LOCTEXT("NotCompatible_Error", "The type of '{VariableName}' is not compatible with {PinUnderCursor}"), Args);
			}
		}
	}
	else if (VarNodeUnderCursor != NULL)
	{
		const bool bIsRead = VarNodeUnderCursor->IsA(UK2Node_VariableGet::StaticClass());
		const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(VarNodeUnderCursor);
		const bool bReadOnlyProperty = FBlueprintEditorUtils::IsPropertyReadOnlyInCurrentBlueprint(Blueprint, VariableProperty);
		const bool bCanWriteIfNeeded = bIsRead || !bReadOnlyProperty;
		
		FFormatNamedArguments Args;
		Args.Add(TEXT("VariableName"), FText::FromString(VariableString));

		if (bCanWriteIfNeeded)
		{
			Args.Add(TEXT("ReadOrWrite"), bIsRead ? LOCTEXT("Read", "read") : LOCTEXT("Write", "write"));
			if(WillBreakLinks(VarNodeUnderCursor, VariableProperty))
			{
				StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OKWarn"));
				Message = FText::Format( LOCTEXT("ChangeNodeToWarnBreakLinks", "Change node to {ReadOrWrite} '{VariableName}', WARNING this will break links!"), Args);
			}
			else
			{
				StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				Message = FText::Format( LOCTEXT("ChangeNodeTo", "Change node to {ReadOrWrite} '{VariableName}'"), Args);
			}
		}
		else
		{
			StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
			Message = FText::Format( LOCTEXT("ReadOnlyVar_Error", "Cannot write to read-only variable '{VariableName}'"), Args);
		}
	}
	else if (HoveredCategoryName.Len() > 0)
	{
		// Find Blueprint that made this class and get category of variable
		FName Category = NAME_None;
		UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(VariableSourceClass.Get());
		if(Blueprint != NULL)
		{
			Category = FBlueprintEditorUtils::GetBlueprintVariableCategory(Blueprint, VariableProperty->GetFName() );
		}

		FName NewCategory = FName(*HoveredCategoryName,  FNAME_Find);

		// See if class is native
		UClass* OuterClass = CastChecked<UClass>(VariableProperty->GetOuter());
		const bool bIsNativeVar = (OuterClass->ClassGeneratedBy == NULL);

		FFormatNamedArguments Args;
		Args.Add(TEXT("VariableName"), FText::FromString(VariableString));
		Args.Add(TEXT("HoveredCategoryName"), FText::FromString(HoveredCategoryName));

		if (bIsNativeVar)
		{
			StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
			Message = FText::Format( LOCTEXT("ChangingCatagoryNotThisVar", "Cannot change category for variable '{VariableName}'"), Args );
		}
		else if (Category == NewCategory)
		{
			StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
			Message = FText::Format( LOCTEXT("ChangingCatagoryAlreadyIn", "Variable '{VariableName}' is already in category '{HoveredCategoryName}'"), Args );
		}
		else
		{
			StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
			Message = FText::Format( LOCTEXT("ChangingCatagoryOk", "Move variable '{VariableName}' to category '{HoveredCategoryName}'"), Args );
		}
	}
	else if (HoveredAction.IsValid())
	{
		if(HoveredAction.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Var* VarAction = (FEdGraphSchemaAction_K2Var*)HoveredAction.Pin().Get();
			FName TargetVarName = VarAction->GetVariableName();

			// Needs to have a valid index to move it (this excludes variables added through other means, like timelines/components
			int32 MoveVarIndex = INDEX_NONE;
			int32 TargetVarIndex = INDEX_NONE;
			UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(VariableSourceClass.Get());
			if(Blueprint != NULL)
			{
				MoveVarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VariableName);
				TargetVarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, TargetVarName);
			}

			FFormatNamedArguments Args;
			Args.Add(TEXT("VariableName"), FText::FromString(VariableString));
			Args.Add(TEXT("TargetVarName"), FText::FromName(TargetVarName));

			if(MoveVarIndex == INDEX_NONE)
			{
				StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				Message = FText::Format( LOCTEXT("MoveVarDiffClass", "Cannot reorder variable '{VariableName}'."), Args );
			}
			else if(TargetVarIndex == INDEX_NONE)
			{
				StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				Message = FText::Format( LOCTEXT("MoveVarOther", "Cannot reorder variable '{VariableName}' before '{TargetVarName}'."), Args );
			}
			else if(VariableName == TargetVarName)
			{
				StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				Message = FText::Format( LOCTEXT("MoveVarYourself", "Cannot reorder variable '{VariableName}' before itself."), Args );
			}
			else
			{
				StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				Message = FText::Format( LOCTEXT("MoveVarOK", "Reorder variable '{VariableName}' before '{TargetVarName}'"), Args );
			}
		}
	}
	// Draw variable icon
	else
	{
		StatusSymbol = FBlueprintEditor::GetVarIconAndColor(VariableSourceClass.Get(), VariableName, IconColor);
		Message = FText::FromString(VariableString);
	}

	SetSimpleFeedbackMessage(StatusSymbol, IconColor, Message);
}

FReply FKismetVariableDragDropAction::DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	UEdGraphPin* TargetPin = GetHoveredPin();

	if (TargetPin != NULL)
	{
		if (const UEdGraphSchema_K2* Schema = CastChecked<const UEdGraphSchema_K2>(TargetPin->GetSchema()))
		{
			UProperty* VariableProperty = GetVariableProperty();

			const bool bIsRead = TargetPin->Direction == EGPD_Input;
			const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(TargetPin->GetOwningNode());
			const bool bReadOnlyProperty = FBlueprintEditorUtils::IsPropertyReadOnlyInCurrentBlueprint(Blueprint, VariableProperty);
			const bool bCanWriteIfNeeded = bIsRead || !bReadOnlyProperty;

			FEdGraphPinType VariablePinType;
			Schema->ConvertPropertyToPinType(VariableProperty, VariablePinType);
			const bool bTypeMatch = Schema->ArePinTypesCompatible(VariablePinType, TargetPin->PinType);

			if (bTypeMatch && bCanWriteIfNeeded)
			{
				FEdGraphSchemaAction_K2NewNode Action;

				UK2Node_Variable* VarNode = bIsRead ? (UK2Node_Variable*)NewObject<UK2Node_VariableGet>() : (UK2Node_Variable*)NewObject<UK2Node_VariableSet>();
				Action.NodeTemplate = VarNode;

				UBlueprint* DropOnBlueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetPin->GetOwningNode()->GetGraph());
				ConfigureVarNode(VarNode, VariableName, VariableSourceClass.Get(), DropOnBlueprint);

				Action.PerformAction(TargetPin->GetOwningNode()->GetGraph(), TargetPin, GraphPosition);
			}
		}
	}

	return FReply::Handled();
}

FReply FKismetVariableDragDropAction::DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	UK2Node_Variable* TargetNode = Cast<UK2Node_Variable>(GetHoveredNode());

	if (TargetNode && (VariableName != TargetNode->GetVarName()))
	{
		UProperty* VariableProperty = GetVariableProperty();
		const FString OldVarName = TargetNode->GetVarNameString();
		const UEdGraphSchema_K2* Schema = Cast<const UEdGraphSchema_K2>(TargetNode->GetSchema());

		TArray<class UEdGraphPin*> BadLinks;
		GetLinksThatWillBreak(TargetNode,VariableProperty,BadLinks);

		// Change the variable name and context
		UBlueprint* DropOnBlueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetNode->GetGraph());
		ConfigureVarNode(TargetNode, VariableName, VariableSourceClass.Get(), DropOnBlueprint);

		UEdGraphPin* Pin = TargetNode->FindPin(OldVarName);

		if ((Pin == NULL) || (Pin->LinkedTo.Num() == BadLinks.Num()) || (Schema == NULL))
		{
			TargetNode->GetSchema()->ReconstructNode(*TargetNode);
		}
		else 
		{
			FEdGraphPinType NewPinType;
			Schema->ConvertPropertyToPinType(VariableProperty,NewPinType);

			Pin->PinName = VariableName.ToString();
			Pin->PinType = NewPinType;

			//break bad links
			for(TArray<class UEdGraphPin*>::TIterator OtherPinIt(BadLinks);OtherPinIt;)
			{
				Pin->BreakLinkTo(*OtherPinIt);
			}
		}

		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void FKismetVariableDragDropAction::ConfigureVarNode(UK2Node_Variable* VarNode, FName VariableName, UClass* VariableSourceClass, UBlueprint* TargetBlueprint)
{
	// See if this is a 'self context' (ie. blueprint class is owner (or child of owner) of dropped var class)
	if ((VariableSourceClass == NULL) || TargetBlueprint->SkeletonGeneratedClass->IsChildOf(VariableSourceClass))
	{
		VarNode->VariableReference.SetSelfMember(VariableName);
	}
	else
	{
		VarNode->VariableReference.SetExternalMember(VariableName, VariableSourceClass);
	}
}


void FKismetVariableDragDropAction::MakeGetter(FVector2D GraphPosition, UEdGraph* Graph, FName VariableName, UClass* VariableSourceClass)
{
	check(Graph);

	UK2Node_VariableGet* GetVarNodeTemplate = NewObject<UK2Node_VariableGet>();
	UBlueprint* DropOnBlueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	ConfigureVarNode(GetVarNodeTemplate, VariableName, VariableSourceClass, DropOnBlueprint);

	FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_VariableGet>(Graph, GetVarNodeTemplate, GraphPosition);
}

void FKismetVariableDragDropAction::MakeSetter(FVector2D GraphPosition, UEdGraph* Graph, FName VariableName, UClass* VariableSourceClass)
{
	check(Graph);

	UK2Node_VariableSet* SetVarNodeTemplate = NewObject<UK2Node_VariableSet>();
	UBlueprint* DropOnBlueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	ConfigureVarNode(SetVarNodeTemplate, VariableName, VariableSourceClass, DropOnBlueprint);

	FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_VariableSet>(Graph, SetVarNodeTemplate, GraphPosition);
}

bool FKismetVariableDragDropAction::CanExecuteMakeSetter(UEdGraph* Graph, UProperty* VariableProperty, UClass* VariableSourceClass)
{
	check(VariableProperty);
	check(VariableSourceClass);

	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	const bool bReadOnlyProperty = FBlueprintEditorUtils::IsPropertyReadOnlyInCurrentBlueprint(Blueprint, VariableProperty);

	return (!bReadOnlyProperty) && (!VariableSourceClass->HasAnyClassFlags(CLASS_Const));
}

FReply FKismetVariableDragDropAction::DroppedOnPanel( const TSharedRef< SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph)
{	
	if (Cast<const UEdGraphSchema_K2>(Graph.GetSchema()) != NULL)
	{
		UProperty* VariableProperty = GetVariableProperty();
		UClass* Outer = CastChecked<UClass>(VariableProperty->GetOuter());

		// call analytics
		AnalyticCallback.ExecuteIfBound();

		// Asking for a getter
		if (bControlDrag)
		{
			MakeGetter(GraphPosition, &Graph, VariableName, Outer);
		}
		// Asking for a setter
		else if (bAltDrag && CanExecuteMakeSetter(&Graph, VariableProperty, Outer))
		{
			MakeSetter(GraphPosition, &Graph, VariableName, Outer);
		}
		// Show selection menu
		else
		{
			FMenuBuilder MenuBuilder(true, NULL);
			const FText VariableNameText = FText::FromName( VariableName );

			MenuBuilder.BeginSection("BPVariableDroppedOn", VariableNameText );

			MenuBuilder.AddMenuEntry(
				LOCTEXT("CreateGetVariable", "Get"),
				FText::Format( LOCTEXT("CreateVariableGetterToolTip", "Create Getter for variable '{0}'\n(Ctrl-drag to automatically create a getter)"), VariableNameText ),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateStatic(&FKismetVariableDragDropAction::MakeGetter,GraphPosition,&Graph,VariableName,Outer), FCanExecuteAction())
				);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("CreateSetVariable", "Set"),
				FText::Format( LOCTEXT("CreateVariableSetterToolTip", "Create Setter for variable '{0}'\n(Alt-drag to automatically create a setter)"), VariableNameText ),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateStatic(&FKismetVariableDragDropAction::MakeSetter,GraphPosition,&Graph,VariableName,Outer),
				FCanExecuteAction::CreateStatic(&FKismetVariableDragDropAction::CanExecuteMakeSetter, &Graph, VariableProperty, Outer ))
				);

			TSharedRef< SWidget > PanelWidget = Panel;
			// Show dialog to choose getter vs setter
			FSlateApplication::Get().PushMenu(
				PanelWidget,
				MenuBuilder.MakeWidget(),
				ScreenPosition,
				FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu)
				);

			MenuBuilder.EndSection();
		}
	}

	return FReply::Handled();
}



FReply FKismetVariableDragDropAction::DroppedOnAction(TSharedRef<FEdGraphSchemaAction> Action)
{
	if(Action->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Var* VarAction = (FEdGraphSchemaAction_K2Var*)&Action.Get();

		// Only let you drag and drop if variables are from same BP class, and not onto itself
		UBlueprint* BP = UBlueprint::GetBlueprintFromClass(VariableSourceClass.Get());
		FName TargetVarName = VarAction->GetVariableName();
		if( (BP != NULL) && 
			(VariableName != TargetVarName) && 
			(VariableSourceClass == VarAction->GetVariableClass()) )
		{
			bool bMoved = FBlueprintEditorUtils::MoveVariableBeforeVariable(BP, VariableName, TargetVarName, true);
			// If we moved successfully
			if(bMoved)
			{
				// Change category of var to match the one we dragged on to as well
				FName MovedVarCategory = FBlueprintEditorUtils::GetBlueprintVariableCategory(BP, VariableName);
				FName TargetVarCategory = FBlueprintEditorUtils::GetBlueprintVariableCategory(BP, TargetVarName);
				if(MovedVarCategory != TargetVarCategory)
				{
					FBlueprintEditorUtils::SetBlueprintVariableCategory(BP, VariableName, TargetVarCategory, true);
				}

				// Update Blueprint after changes so they reflect in My Blueprint tab.
				FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
			}
		}

		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply FKismetVariableDragDropAction::DroppedOnCategory(FString Category)
{
	UE_LOG(LogTemp, Log, TEXT("Dropped %s on Category %s"), *VariableName.ToString(), *Category);

	UBlueprint* BP = UBlueprint::GetBlueprintFromClass(VariableSourceClass.Get());
	if(BP != NULL)
	{
		// Check this is actually a different category
		FName CurrentCategory = FBlueprintEditorUtils::GetBlueprintVariableCategory(BP, VariableName);
		if(FName(*Category) != CurrentCategory)
		{
			FBlueprintEditorUtils::SetBlueprintVariableCategory(BP, VariableName, FName(*Category), false);
		}
	}

	return FReply::Handled();
}



#undef LOCTEXT_NAMESPACE
