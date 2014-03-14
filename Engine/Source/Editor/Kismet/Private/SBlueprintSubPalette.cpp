// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "SBlueprintSubPalette.h"
#include "AssetRegistryModule.h"
#include "BlueprintEditor.h"
#include "SBlueprintPalette.h"
#include "BPFunctionDragDropAction.h"
#include "BPVariableDragDropAction.h"
#include "BPDelegateDragDropAction.h"
#include "SBlueprintEditorToolbar.h"
#include "BlueprintPaletteFavorites.h"
#include "BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "BlueprintSubPalette"

/*******************************************************************************
* Static File Helpers
*******************************************************************************/

/**
 * An analytics hook, for tracking when a node was spawned from the palette 
 * (updates the "node creation stats" with a palette drag-placement flag).
 * 
 * @param  BlueprintEditorPtr	A pointer to the blueprint editor currently being worked in.
 */
static void OnNodePlacement(TWeakPtr<FBlueprintEditor> BlueprintEditorPtr)
{
	if( BlueprintEditorPtr.IsValid() )
	{
		BlueprintEditorPtr.Pin()->UpdateNodeCreationStats( ENodeCreateAction::PaletteDragPlacement );
	}
}

/**
 * Checks to see if the user can drop the currently dragged action to place its
 * associated node in the graph.
 * 
 * @param  DropActionIn			The action that will be executed when the user drops the dragged item.
 * @param  HoveredGraphIn		A pointer to the graph that the user currently has the item dragged over.
 * @param  ImpededReasonOut		If this returns false, this will be filled out with a reason to present the user with.
 * @return True is the dragged palette item can be dropped where it currently is, false if not.
 */
static bool CanPaletteItemBePlaced(TSharedPtr<FEdGraphSchemaAction> DropActionIn, UEdGraph* HoveredGraphIn, FText& ImpededReasonOut)
{
	bool bCanBePlaced = true;
	if (!DropActionIn.IsValid())
	{
		bCanBePlaced = false;
		ImpededReasonOut = LOCTEXT("InvalidDropAction", "Invalid action for placement");
	}
	else if (HoveredGraphIn == NULL)
	{
		bCanBePlaced = false;
		ImpededReasonOut = LOCTEXT("DropOnlyInGraph", "Nodes can only be placed inside the blueprint graph");
	}
	else if (UK2Node const* NodeToBePlaced = FK2SchemaActionUtils::ExtractNodeTemplateFromAction(DropActionIn))
	{
		UEdGraphSchema const* const GraphSchema = HoveredGraphIn->GetSchema();
		check(GraphSchema != NULL);

		bool bIsFunctionGraph = (GraphSchema->GetGraphType(HoveredGraphIn) == EGraphType::GT_Function);

		if (UK2Node_CallFunction const* CallFuncNode = Cast<UK2Node_CallFunction const>(NodeToBePlaced))
		{
			FName const FuncName = CallFuncNode->FunctionReference.GetMemberName();
			check(FuncName != NAME_None);
			UClass const* const FuncOwner = CallFuncNode->FunctionReference.GetMemberParentClass(CallFuncNode);
			check(FuncOwner != NULL);

			UFunction const* const Function = FindField<UFunction>(FuncOwner, FuncName);

			if (!HoveredGraphIn->GetSchema()->IsA(UEdGraphSchema_K2::StaticClass()))
			{
				bCanBePlaced = false;
				ImpededReasonOut = LOCTEXT("CannotCreateInThisSchema", "Cannot call functions in this type of graph");
			}
			else if (Function == NULL)
			{
				bCanBePlaced = false;
				ImpededReasonOut = LOCTEXT("InvalidFuncAction", "Invalid function for placement");
			}
			else if (bIsFunctionGraph && Function->HasMetaData(FBlueprintMetadata::MD_Latent))
			{
				bCanBePlaced = false;
				ImpededReasonOut = LOCTEXT("CannotCreateLatentInGraph", "Cannot call latent functions in function graphs");
			}
		}
		else if (UK2Node_Event const* EventNode = Cast<UK2Node_Event const>(NodeToBePlaced))
		{
			// function graphs cannot have more than one entry point
			if (bIsFunctionGraph)
			{
				bCanBePlaced = false;
				ImpededReasonOut = LOCTEXT("NoSecondEntryPoint", "Function graphs can only have one entry point");
			}
			else if (GraphSchema->GetGraphType(HoveredGraphIn) != EGraphType::GT_Ubergraph)
			{
				bCanBePlaced = false;
				ImpededReasonOut = LOCTEXT("NoEventsOnlyInUberGraphs", "Events can only be placed in event graphs");
			}
		}
		else if (Cast<UK2Node_SpawnActor const>(NodeToBePlaced) || Cast<UK2Node_SpawnActorFromClass const>(NodeToBePlaced))
		{
			UEdGraphSchema_K2 const* const K2Schema = Cast<UEdGraphSchema_K2 const>(GraphSchema);
			if (K2Schema && K2Schema->IsConstructionScript(HoveredGraphIn))
			{
				bCanBePlaced = false;
				ImpededReasonOut = LOCTEXT("NoSpawnActorInConstruction", "Cannot spawn actors from a construction script");
			}
		}

		bool bWillFocusOnExistingNode = (DropActionIn->GetTypeId() == FEdGraphSchemaAction_K2TargetNode::StaticGetTypeId());
		if (!bWillFocusOnExistingNode && DropActionIn->GetTypeId() == FEdGraphSchemaAction_K2AddEvent::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2AddEvent* AddEventAction = (FEdGraphSchemaAction_K2AddEvent*)DropActionIn.Get();
			bWillFocusOnExistingNode = AddEventAction->EventHasAlreadyBeenPlaced(FBlueprintEditorUtils::FindBlueprintForGraph(HoveredGraphIn));
		}
		
		// if this will instead focus on an existing node, reverse any previous decision... it is ok to drop!
		if (bWillFocusOnExistingNode)
		{
			bCanBePlaced = true;
			ImpededReasonOut = FText::GetEmpty();
		}
		// as a general catch-all, if a node cannot be pasted, it probably can't be created there
		else if (bCanBePlaced && !NodeToBePlaced->CanPasteHere(HoveredGraphIn, GraphSchema) && !bWillFocusOnExistingNode)
		{
			bCanBePlaced = false;
			ImpededReasonOut = LOCTEXT("CannotPaste", "Cannot place this node in this type of graph");
		}
	}

	return bCanBePlaced;
}

/*******************************************************************************
* FBlueprintPaletteCommands
*******************************************************************************/

class FBlueprintPaletteCommands : public TCommands<FBlueprintPaletteCommands>
{
public:
	FBlueprintPaletteCommands() : TCommands<FBlueprintPaletteCommands>
		( "BlueprintPalette"
		, LOCTEXT("PaletteContext", "Palette")
		, NAME_None
		, FEditorStyle::GetStyleSetName() )
	{
	}

	TSharedPtr<FUICommandInfo> RefreshPalette;

	/**
	 * Registers context menu commands for the blueprint palette.
	 */
	virtual void RegisterCommands() OVERRIDE
	{
		UI_COMMAND(RefreshPalette, "Refresh List", "Refreshes the list of nodes.", EUserInterfaceActionType::Button, FInputGesture());
	}
};

/*******************************************************************************
* SBlueprintSubPalette Public Interface
*******************************************************************************/

//------------------------------------------------------------------------------
SBlueprintSubPalette::~SBlueprintSubPalette()
{
	GEditor->OnBlueprintCompiled().RemoveAll(this);
}

//------------------------------------------------------------------------------
void SBlueprintSubPalette::Construct(FArguments const& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	BlueprintEditorPtr = InBlueprintEditor;

	ChildSlot
 	[
		SNew(SBorder)
		.Padding(2.f)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 0.f, 2.f, 0.f, 0.f )
			[
				ConstructHeadingWidget(InArgs._Icon.Get(), InArgs._Title.Get(), InArgs._ToolTipText.Get())
			]

			+SVerticalBox::Slot()
			[
				SNew(SOverlay)
				+SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
				[
					SAssignNew(GraphActionMenu, SGraphActionMenu)
						.OnCreateWidgetForAction(this, &SBlueprintSubPalette::OnCreateWidgetForAction)
						.OnActionDragged(this, &SBlueprintSubPalette::OnActionDragged)
						.OnCollectAllActions(this, &SBlueprintSubPalette::CollectAllActions)
						.OnContextMenuOpening(this, &SBlueprintSubPalette::ConstructContextMenuWidget)
				]
			]
		]
	];

	CommandList = MakeShareable(new FUICommandList);
	// has to come after GraphActionMenu has been set
	BindCommands(CommandList);

	GEditor->OnBlueprintCompiled().AddSP(this, &SBlueprintSubPalette::RefreshActionsList, true);
	// Register with the Asset Registry to be informed when it is done loading up files.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().OnFilesLoaded().AddSP(this, &SBlueprintSubPalette::RefreshActionsList, true);
}

//------------------------------------------------------------------------------
UBlueprint* SBlueprintSubPalette::GetBlueprint() const
{
	UBlueprint* BlueprintBeingEdited = NULL;
	if (BlueprintEditorPtr.IsValid())
	{
		BlueprintBeingEdited = BlueprintEditorPtr.Pin()->GetBlueprintObj();
	}
	return BlueprintBeingEdited;
}

//------------------------------------------------------------------------------
TSharedPtr<FEdGraphSchemaAction> SBlueprintSubPalette::GetSelectedAction() const
{
	TArray< TSharedPtr<FEdGraphSchemaAction> > SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);

	return TSharedPtr<FEdGraphSchemaAction>( (SelectedActions.Num() > 0) ? SelectedActions[0] : NULL );
}

/*******************************************************************************
* Protected SBlueprintSubPalette Methods
*******************************************************************************/

//------------------------------------------------------------------------------
void SBlueprintSubPalette::RefreshActionsList(bool bPreserveExpansion)
{
	// Prevent refreshing the palette if we're in PIE
	if( !GIsPlayInEditorWorld )
	{
		SGraphPalette::RefreshActionsList(bPreserveExpansion);
	}
}

//------------------------------------------------------------------------------
TSharedRef<SWidget> SBlueprintSubPalette::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return SNew(SBlueprintPaletteItem, InCreateData, BlueprintEditorPtr.Pin())
		.ShowClassInTooltip(true);
}

//------------------------------------------------------------------------------
FReply SBlueprintSubPalette::OnActionDragged( const TArray< TSharedPtr<FEdGraphSchemaAction> >& InActions, const FPointerEvent& MouseEvent )
{
	if( InActions.Num() > 0 && InActions[0].IsValid() )
	{
		TSharedPtr<FEdGraphSchemaAction> InAction = InActions[0];
		auto AnalyticsDelegate = FNodeCreationAnalytic::CreateStatic(&OnNodePlacement, BlueprintEditorPtr);

		auto CanNodeBePlacedDelegate = FKismetDragDropAction::FCanBeDroppedDelegate::CreateStatic(&CanPaletteItemBePlaced);

		if(InAction->GetTypeId() == FEdGraphSchemaAction_K2NewNode::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2NewNode* NewNodeAction = (FEdGraphSchemaAction_K2NewNode*)InAction.Get();
			UK2Node_CallFunction* CallFuncNode = Cast<UK2Node_CallFunction>(NewNodeAction->NodeTemplate);
			if(CallFuncNode != NULL)
			{
				FMemberReference CallOnMember;
				UK2Node_CallFunctionOnMember* CallFuncOnMemberNode = Cast<UK2Node_CallFunctionOnMember>(CallFuncNode);
				if(CallFuncOnMemberNode != NULL)
				{
					CallOnMember = CallFuncOnMemberNode->MemberVariableToCallOn;
				}

				TSharedRef<FKismetFunctionDragDropAction> DragDropAction = FKismetFunctionDragDropAction::New(
					CallFuncNode->FunctionReference.GetMemberName(),
					CallFuncNode->FunctionReference.GetMemberParentClass(CallFuncNode), 
					CallOnMember, 
					AnalyticsDelegate,
					CanNodeBePlacedDelegate);

				return FReply::Handled().BeginDragDrop(DragDropAction);
			}
			else
			{
				return FReply::Handled().BeginDragDrop(FKismetDragDropAction::New(InAction, AnalyticsDelegate, CanNodeBePlacedDelegate));
			}
		}
		else if(InAction->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Var* VarAction = (FEdGraphSchemaAction_K2Var*)InAction.Get();

			UClass* VarClass = VarAction->GetVariableClass();
			if(VarClass != NULL)
			{
				return FReply::Handled().BeginDragDrop(FKismetVariableDragDropAction::New(VarAction->GetVariableName(), VarClass, AnalyticsDelegate));
			}
		}
		else if(InAction->GetTypeId() == FEdGraphSchemaAction_K2Delegate::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Delegate* DelegateAction = (FEdGraphSchemaAction_K2Delegate*)InAction.Get();
			UClass* VarClass = DelegateAction->GetDelegateClass();
			check(VarClass);
			if(VarClass != NULL)
			{
				return FReply::Handled().BeginDragDrop(FKismetDelegateDragDropAction::New( SharedThis( this ), DelegateAction->GetDelegateName(), VarClass, AnalyticsDelegate));
			}
		}
		else
		{
			return FReply::Handled().BeginDragDrop(FKismetDragDropAction::New(InAction, AnalyticsDelegate, CanNodeBePlacedDelegate));
		}
	}

	return FReply::Unhandled();
}

//------------------------------------------------------------------------------
void SBlueprintSubPalette::BindCommands(TSharedPtr<FUICommandList> CommandListIn) const
{
	FBlueprintPaletteCommands::Register();
	FBlueprintPaletteCommands const& PaletteCommands = FBlueprintPaletteCommands::Get();

	CommandListIn->MapAction(
		PaletteCommands.RefreshPalette,
		FExecuteAction::CreateSP(this, &SBlueprintSubPalette::RefreshActionsList, true)
	);
}

//------------------------------------------------------------------------------
TSharedPtr<SWidget> SBlueprintSubPalette::ConstructContextMenuWidget() const
{
	FMenuBuilder MenuBuilder(/* bInShouldCloseWindowAfterMenuSelection =*/true, CommandList);
	GenerateContextMenuEntries(MenuBuilder);
	return MenuBuilder.MakeWidget();
}

//------------------------------------------------------------------------------
void SBlueprintSubPalette::GenerateContextMenuEntries(FMenuBuilder& MenuBuilder) const
{
	FBlueprintPaletteCommands const& PaletteCommands = FBlueprintPaletteCommands::Get();
	MenuBuilder.AddMenuEntry(PaletteCommands.RefreshPalette);
}

/*******************************************************************************
* Private SBlueprintSubPalette Methods
*******************************************************************************/

//------------------------------------------------------------------------------
TSharedRef<SVerticalBox> SBlueprintSubPalette::ConstructHeadingWidget(FSlateBrush const* const Icon, FString const& TitleText, FString const& ToolTipText)
{
	TSharedPtr<SToolTip> ToolTip;
	SAssignNew(ToolTip, SToolTip).Text(ToolTipText);

	FTextBlockStyle TitleStyle = FTextBlockStyle()
		.SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 10))
		.SetColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f));

	return SNew(SVerticalBox)
			.ToolTip(ToolTip)
			// so we still get tooltip text for an empty SHorizontalBox
			.Visibility(EVisibility::Visible) 
		+SVerticalBox::Slot()
			.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.f, 2.f)
			[
				SNew(SImage).Image(Icon)
			]

			+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.f, 2.f)
			[
				SNew(STextBlock)
					.Text(TitleText)
					.TextStyle(&TitleStyle)
			]
		]
		+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 2.f, 0.f, 5.f)
		[
			SNew(SBorder)
				// use the border's padding to actually create the horizontal line
				.Padding(1.f)
				.BorderImage(FEditorStyle::GetBrush(TEXT("Menu.Separator")))
		];	
}

#undef LOCTEXT_NAMESPACE
