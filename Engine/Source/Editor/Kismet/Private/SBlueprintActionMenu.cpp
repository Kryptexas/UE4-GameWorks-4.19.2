// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintEditorPrivatePCH.h"
#include "GraphEditor.h"
#include "SGraphActionMenu.h"
#include "SBlueprintActionMenu.h"
#include "SBlueprintPalette.h"
#include "BlueprintEditor.h"
#include "SMyBlueprint.h"
#include "BlueprintEditorUtils.h"
#include "K2ActionMenuBuilder.h" // for FBlueprintPaletteListBuilder
#include "BlueprintPaletteFavorites.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "SBlueprintGraphContextMenu"

/** Action to promote a pin to a variable */
USTRUCT()
struct FBlueprintAction_PromoteVariable : public FEdGraphSchemaAction
{
	FBlueprintAction_PromoteVariable()
		: FEdGraphSchemaAction()
	{
		Category = TEXT("");
		MenuDescription = LOCTEXT("PromoteToVariable", "Promote to variable").ToString();
		TooltipDescription = LOCTEXT("PromoteToVariable", "Promote to variable").ToString();
		Grouping = 1;
	}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction( class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) OVERRIDE
	{
		if( ( ParentGraph != NULL ) && ( FromPin != NULL ) )
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(ParentGraph);
			if( ( MyBlueprintEditor.IsValid() == true ) && ( Blueprint != NULL ) )
			{
				MyBlueprintEditor.Pin()->DoPromoteToVariable( Blueprint, FromPin );
			}
		}
		return NULL;		
	}
	// End of FEdGraphSchemaAction interface

	/* Pointer to the blueprint editor containing the blueprint in which we will promote the variable. */
	TWeakPtr<class FBlueprintEditor> MyBlueprintEditor;
};

class SPaletteItemFavoriteToggle : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SPaletteItemFavoriteToggle ) {}
	SLATE_END_ARGS()

	/**
	 * Static method for binding with delegates. Spawns an instance of the 
	 * favorite toggle.
	 * 
	 * @param  InCreateData	Data detailing the associated action for the containing action list item.
	 * @return A new widget for the hover overlay.
	 */
	static TSharedRef<SWidget> Create(FCreateWidgetForActionData* const InCreateData)
	{
		return SNew(SPaletteItemFavoriteToggle, InCreateData->Action);
	}

	/**
	 * Constructs a favorite-toggle widget (so that user can easily modify the 
	 * item's favorited state).
	 * 
	 * @param  InArgs			A set of slate arguments, defined above.
	 * @param  ActionPtrIn		The FEdGraphSchemaAction that the parent item represents.
	 * @param  BlueprintEdPtrIn	A pointer to the blueprint editor that the palette belongs to.
	 */
	void Construct(const FArguments& InArgs, TWeakPtr<FEdGraphSchemaAction> ActionPtrIn)
	{
		ActionPtr = ActionPtrIn;
		ChildSlot
		[
			SNew( SCheckBox )
				.Visibility(this, &SPaletteItemFavoriteToggle::IsVisibile)
				.ToolTipText(this, &SPaletteItemFavoriteToggle::GetToolTipText)
				.IsChecked(this, &SPaletteItemFavoriteToggle::GetFavoritedState)
 				.OnCheckStateChanged(this, &SPaletteItemFavoriteToggle::OnFavoriteToggled)
 				.Style(FEditorStyle::Get(), "Kismet.Palette.FavoriteToggleStyle")	
		];
	}

private:
	/**
	 * Used to determine the toggle's visibility (this is only visible when the 
	 * owning item is being hovered over, and the associated action can be favorited).
	 *
	 * @return True if this toggle switch should be showing, false if not.
	 */
	EVisibility IsVisibile() const
	{
		bool bNoFavorites = false;
		GConfig->GetBool(TEXT("BlueprintEditor.Palette"), TEXT("bUseLegacyLayout"), bNoFavorites, GEditorIni);

		EVisibility CurrentVisibility = EVisibility::Collapsed;
		if (!bNoFavorites && GEditor->EditorUserSettings->BlueprintFavorites->CanBeFavorited(ActionPtr.Pin()))
		{
			CurrentVisibility = EVisibility::Visible;
		}

		return CurrentVisibility;
	}

	/**
	 * Retrieves tooltip that describes the current favorited state of the 
	 * associated action.
	 * 
	 * @return Text describing what this toggle will do when you click on it.
	 */
	FText GetToolTipText() const
	{
		if (GetFavoritedState() == ESlateCheckBoxState::Checked)
		{
			return LOCTEXT("Unfavorite", "Click to remove this item from your favorites.");
		}
		return LOCTEXT("Favorite", "Click to add this item to your favorites.");
	}

	/**
	 * Checks on the associated action's favorite state, and returns a 
	 * corresponding checkbox state to match.
	 * 
	 * @return ESlateCheckBoxState::Checked if the associated action is already favorited, ESlateCheckBoxState::Unchecked if not.
	 */
	ESlateCheckBoxState::Type GetFavoritedState() const
	{
		ESlateCheckBoxState::Type FavoriteState = ESlateCheckBoxState::Unchecked;
		if (GEditor->EditorUserSettings->BlueprintFavorites->IsFavorited(ActionPtr.Pin()))
		{
			FavoriteState = ESlateCheckBoxState::Checked;
		}
		return FavoriteState;
	}

	/**
	 * Triggers when the user clicks this toggle, adds or removes the associated
	 * action to the user's favorites.
	 * 
	 * @param  InNewState	The new state that the user set the checkbox to.
	 */
	void OnFavoriteToggled(ESlateCheckBoxState::Type InNewState)
	{
		if (InNewState == ESlateCheckBoxState::Checked)
		{
			GEditor->EditorUserSettings->BlueprintFavorites->AddFavorite(ActionPtr.Pin());
		}
		else
		{
			GEditor->EditorUserSettings->BlueprintFavorites->RemoveFavorite(ActionPtr.Pin());
		}
	}	

private:
	/** The action that the owning palette entry represents */
	TWeakPtr<FEdGraphSchemaAction> ActionPtr;
};

SBlueprintActionMenu::~SBlueprintActionMenu()
{
	OnClosedCallback.ExecuteIfBound();
	OnCloseReasonCallback.ExecuteIfBound(bActionExecuted, ContextToggleIsChecked() == ESlateCheckBoxState::Checked, DraggedFromPins.Num() > 0);
}

void SBlueprintActionMenu::Construct( const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InEditor )
{
	bActionExecuted = false;

	this->GraphObj = InArgs._GraphObj;
	this->DraggedFromPins = InArgs._DraggedFromPins;
	this->NewNodePosition = InArgs._NewNodePosition;
	this->OnClosedCallback = InArgs._OnClosedCallback;
	this->bAutoExpandActionMenu = InArgs._AutoExpandActionMenu;
	this->EditorPtr = InEditor;
	this->OnCloseReasonCallback = InArgs._OnCloseReason;

	// Generate the context display; showing the user what they're picking something for
	//@TODO: Should probably be somewhere more schema-sensitive than the graph panel!
	FSlateColor TypeColor;
	FString TypeOfDisplay;
	const FSlateBrush* ContextIcon = NULL;

	if (DraggedFromPins.Num() == 1)
	{
		UEdGraphPin* OnePin = DraggedFromPins[0];

		const UEdGraphSchema* Schema = OnePin->GetSchema();
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		if (!Schema->IsA(UEdGraphSchema_K2::StaticClass()) || !K2Schema->IsExecPin(*OnePin))
		{
			// Get the type color and icon
			TypeColor = Schema->GetPinTypeColor(OnePin->PinType);
			ContextIcon = FEditorStyle::GetBrush( OnePin->PinType.bIsArray ? TEXT("Graph.ArrayPin.Connected") : TEXT("Graph.Pin.Connected") );
		}
	}

	// Build the widget layout
	SBorder::Construct( SBorder::FArguments()
		.BorderImage( FEditorStyle::GetBrush("Menu.Background") )
		.Padding(5)
		[
			// Achieving fixed width by nesting items within a fixed width box.
			SNew(SBox)
			.WidthOverride(400)
			.HeightOverride(400)
			[
				SNew(SVerticalBox)

				// TYPE OF SEARCH INDICATOR
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2, 2, 2, 5)
				[
					SNew(SHorizontalBox)

					// Type pill
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, (ContextIcon != NULL) ? 5 : 0, 0)
					[
						SNew(SImage)
						.ColorAndOpacity(TypeColor)
						.Visibility(this, &SBlueprintActionMenu::GetTypeImageVisibility)
						.Image(ContextIcon)
					]

					// Search context description
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SBlueprintActionMenu::GetSearchContextDesc)
						.Font(FEditorStyle::GetFontStyle(FName("BlueprintEditor.ActionMenu.ContextDescriptionFont")))
						.ToolTip(IDocumentation::Get()->CreateToolTip(
							LOCTEXT("BlueprintActionMenuContextTextTooltip", "Describes the current context of the action list"),
							NULL,
							TEXT("Shared/Editors/BlueprintEditor"),
							TEXT("BlueprintActionMenuContextText")))
						.WrapTextAt(280)
					]

					// Context Toggle
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged(this, &SBlueprintActionMenu::OnContextToggleChanged)
						.IsChecked(this, &SBlueprintActionMenu::ContextToggleIsChecked)
						.ToolTip(IDocumentation::Get()->CreateToolTip(
							LOCTEXT("BlueprintActionMenuContextToggleTooltip", "Should the list be filtered to only actions that make sense in the current context?"),
							NULL,
							TEXT("Shared/Editors/BlueprintEditor"),
							TEXT("BlueprintActionMenuContextToggle")))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("BlueprintActionMenuContextToggle", "Context Sensitive").ToString())
						]
					]
				]

				// ACTION LIST 
				+SVerticalBox::Slot()
				[
					SAssignNew(GraphActionMenu, SGraphActionMenu)
					.OnActionSelected(this, &SBlueprintActionMenu::OnActionSelected)
					.OnCreateWidgetForAction( SGraphActionMenu::FOnCreateWidgetForAction::CreateSP(this, &SBlueprintActionMenu::OnCreateWidgetForAction) )
					.OnCollectAllActions(this, &SBlueprintActionMenu::CollectAllActions)
					.OnCreateHoverOverlayWidget(SGraphActionMenu::FOnCreateWidgetForAction::CreateStatic(&SPaletteItemFavoriteToggle::Create))
				]
			]
		]
	);
}

EVisibility SBlueprintActionMenu::GetTypeImageVisibility() const
{
	if (DraggedFromPins.Num() == 1 && EditorPtr.Pin()->GetIsContextSensitive())
	{
		UEdGraphPin* OnePin = DraggedFromPins[0];

		const UEdGraphSchema* Schema = OnePin->GetSchema();
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		if (!Schema->IsA(UEdGraphSchema_K2::StaticClass()) || !K2Schema->IsExecPin(*OnePin))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

FText SBlueprintActionMenu::GetSearchContextDesc() const
{
	bool bIsContextSensitive = EditorPtr.Pin()->GetIsContextSensitive();
	bool bHasPins = DraggedFromPins.Num() > 0;
	if (!bIsContextSensitive)
	{
		return LOCTEXT("MenuPrompt_AllPins", "All Possible Actions");
	}
	else if (!bHasPins)
	{
		return LOCTEXT("MenuPrompt_BlueprintActions", "All Actions for this Blueprint");
	}
	else if (DraggedFromPins.Num() == 1)
	{
		UEdGraphPin* OnePin = DraggedFromPins[0];

		const UEdGraphSchema* Schema = OnePin->GetSchema();
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		if (Schema->IsA(UEdGraphSchema_K2::StaticClass()) && K2Schema->IsExecPin(*OnePin))
		{
			return LOCTEXT("MenuPrompt_ExecPin", "Executable actions");
		}
		else
		{
			// Get the type string
			const FString TypeStringRaw = UEdGraphSchema_K2::TypeToString(OnePin->PinType);

			//@TODO: Add a parameter to TypeToString indicating the kind of formating requested
			const FString TypeString = (TypeStringRaw.Replace(TEXT("'"), TEXT(" "))).TrimTrailing();

			if (OnePin->Direction == EGPD_Input)
			{
				return FText::Format(LOCTEXT("MenuPrompt_InputPin", "Actions providing a {0}"), FText::FromString(TypeString));
			}
			else
			{
				return FText::Format(LOCTEXT("MenuPrompt_OutputPin", "Actions taking a {0}"), FText::FromString(TypeString));
			}
		}
	}
	else
	{
		return FText::Format(LOCTEXT("MenuPrompt_ManyPins", "Actions for {0} pins"), FText::AsNumber(DraggedFromPins.Num()));
	}
}

void SBlueprintActionMenu::OnContextToggleChanged(ESlateCheckBoxState::Type CheckState)
{
	EditorPtr.Pin()->GetIsContextSensitive() = CheckState == ESlateCheckBoxState::Checked;
	GraphActionMenu->RefreshAllActions(true, false);
}

ESlateCheckBoxState::Type SBlueprintActionMenu::ContextToggleIsChecked() const
{
	return EditorPtr.Pin()->GetIsContextSensitive() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SBlueprintActionMenu::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	// Build up the context object
	FBlueprintGraphActionListBuilder ContextMenuBuilder(GraphObj);
	bool bIsContextSensitive = EditorPtr.Pin()->GetIsContextSensitive();
	bool bHasPins = DraggedFromPins.Num() > 0;
	if (!bIsContextSensitive)
	{
		FBlueprintPaletteListBuilder PaletteBuilder(EditorPtr.Pin()->GetBlueprintObj());
		UEdGraphSchema_K2::GetAllActions(PaletteBuilder);
		//@TODO: Avoid this copy
		OutAllActions.Append(PaletteBuilder);
	}
	else if (!bHasPins)
	{
		// Pass in selected property
		FEdGraphSchemaAction_K2Var* SelectedVar = EditorPtr.Pin()->GetMyBlueprintWidget()->SelectionAsVar();
		if (SelectedVar != NULL)
		{
			ContextMenuBuilder.SelectedObjects.Add(SelectedVar->GetProperty());
		}
	}
	else
	{
		ContextMenuBuilder.FromPin = DraggedFromPins[0];
	}

	// Note: Cannot call GetGraphContextActions() during serialization and GC due to its use of FindObject()
	if(!GIsSavingPackage && !GIsGarbageCollecting)
	{
		// Determine all possible actions
		GraphObj->GetSchema()->GetGraphContextActions(ContextMenuBuilder);

		// Also try adding promote to variable if we can do so.
		TryInsertPromoteToVariable(ContextMenuBuilder, OutAllActions);
	}

	// Copy the added options back to the main list
	//@TODO: Avoid this copy
	OutAllActions.Append(ContextMenuBuilder);
}

TSharedRef<SEditableTextBox> SBlueprintActionMenu::GetFilterTextBox()
{
	return GraphActionMenu->GetFilterTextBox();
}


TSharedRef<SWidget> SBlueprintActionMenu::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData )
{
	InCreateData->bHandleMouseButtonDown = true;
	return	SNew(SBlueprintPaletteItem, InCreateData, EditorPtr.Pin());
}

void SBlueprintActionMenu::OnActionSelected( const TArray< TSharedPtr<FEdGraphSchemaAction> >& SelectedAction )
{
	for ( int32 ActionIndex = 0; ActionIndex < SelectedAction.Num(); ActionIndex++ )
	{
		if ( SelectedAction[ActionIndex].IsValid() && GraphObj != NULL )
		{
			// Don't dismiss when clicking on dummy action
			if ( !bActionExecuted && (SelectedAction[ActionIndex]->GetTypeId() != FEdGraphSchemaAction_Dummy::StaticGetTypeId()))
			{
				FSlateApplication::Get().DismissAllMenus();
				bActionExecuted = true;
			}

			UEdGraphNode* ResultNode = SelectedAction[ActionIndex]->PerformAction(GraphObj, DraggedFromPins, NewNodePosition);

			if ( ResultNode != NULL )
			{
				NewNodePosition.Y += UEdGraphSchema_K2::EstimateNodeHeight( ResultNode );
			}
		}
	}
}

void SBlueprintActionMenu::TryInsertPromoteToVariable( FGraphContextMenuBuilder &ContextMenuBuilder, FGraphActionListBuilderBase &OutAllActions )
{
	// If we can promote this to a variable add a menu entry to do so.
	const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(GraphObj->GetSchema());
	if ( (K2Schema != NULL) && ( ContextMenuBuilder.FromPin != NULL ) )
	{
		if( K2Schema ->CanPromotePinToVariable(*ContextMenuBuilder.FromPin) )
		{	
			TSharedPtr<FBlueprintAction_PromoteVariable> PromoteAction = TSharedPtr<FBlueprintAction_PromoteVariable>(new FBlueprintAction_PromoteVariable());
			PromoteAction->MyBlueprintEditor = EditorPtr;
			OutAllActions.AddAction( PromoteAction );
		}
	}
}

#undef LOCTEXT_NAMESPACE
