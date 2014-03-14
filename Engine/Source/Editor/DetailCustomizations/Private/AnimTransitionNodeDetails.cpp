// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"

#include "AnimGraphDefinitions.h"

#include "AnimTransitionNodeDetails.h"
#include "KismetEditorUtilities.h"
#include "SKismetLinearExpression.h"

#define LOCTEXT_NAMESPACE "FAnimStateNodeDetails"

/////////////////////////////////////////////////////////////////////////

TSharedRef<IDetailCustomization> FAnimTransitionNodeDetails::MakeInstance()
{
	return MakeShareable( new FAnimTransitionNodeDetails );
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FAnimTransitionNodeDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	// Get a handle to the node we're viewing
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetDetailsView().GetSelectedObjects();
	for (int32 ObjectIndex = 0; !TransitionNode.IsValid() && (ObjectIndex < SelectedObjects.Num()); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			TransitionNode = Cast<UAnimStateTransitionNode>(CurrentObject.Get());
		}
	}

	bool bTransitionToConduit = false;
	if (UAnimStateTransitionNode* TransitionNodePtr = TransitionNode.Get())
	{
		UAnimStateNodeBase* NextState = TransitionNodePtr->GetNextState();
		bTransitionToConduit = (NextState != NULL) && (NextState->IsA<UAnimStateConduitNode>());
	}

	////////////////////////////////////////

	IDetailCategoryBuilder& TransitionCategory = DetailBuilder.EditCategory("Transition", TEXT("Transition") );

	if (bTransitionToConduit)
	{
		// Transitions to conduits are just shorthand for some other real transition;
		// All of the blend related settings are ignored, so hide them.
		DetailBuilder.HideProperty("Bidirectional");
		DetailBuilder.HideProperty("CrossfadeDuration");
		DetailBuilder.HideProperty("CrossfadeMode");
		DetailBuilder.HideProperty("LogicType");
		DetailBuilder.HideProperty("Priority Order");
	}
	else
	{
		TransitionCategory.AddCustomRow( TEXT("Transition") )
		[
			SNew( STextBlock )
			.Text( LOCTEXT("TransitionEventPropertiesCategoryLabel", "Transition") )
			.Font( IDetailLayoutBuilder::GetDetailFontBold() )
		];

		TransitionCategory.AddProperty("PriorityOrder").DisplayName( TEXT("Priority Order") );
		TransitionCategory.AddProperty("Bidirectional").DisplayName( TEXT("Bidirectional"));
		TransitionCategory.AddProperty("LogicType").DisplayName( TEXT("Blend Logic") );

		UAnimStateTransitionNode* TransNode = TransitionNode.Get();
		if (TransitionNode != NULL)
		{
			// The sharing option for the rule
			TransitionCategory.AddCustomRow( TEXT("Transition Rule Sharing") )
			[
				GetWidgetForInlineShareMenu(TEXT("Transition Rule Sharing"), TransNode->SharedRulesName, TransNode->bSharedRules,
					FOnClicked::CreateSP(this, &FAnimTransitionNodeDetails::OnPromoteToSharedClick, true),
					FOnClicked::CreateSP(this, &FAnimTransitionNodeDetails::OnUnshareClick, true), 
					FOnGetContent::CreateSP(this, &FAnimTransitionNodeDetails::OnGetShareableNodesMenu, true))
			];


// 			TransitionCategory.AddRow()
// 				[
// 					SNew( STextBlock )
// 					.Text( TEXT("Crossfade Settings") )
// 					.Font( IDetailLayoutBuilder::GetDetailFontBold() )
// 				];


			// Show the rule itself
			UEdGraphPin* CanExecPin = NULL;
			if (UAnimationTransitionGraph* TransGraph = Cast<UAnimationTransitionGraph>(TransNode->BoundGraph))
			{
				if (UAnimGraphNode_TransitionResult* ResultNode = TransGraph->GetResultNode())
				{
					CanExecPin = ResultNode->FindPin(TEXT("bCanEnterTransition"));
				}
			}
			TransitionCategory.AddCustomRow( CanExecPin ? CanExecPin->PinFriendlyName : FString() )
			[
				SNew(SKismetLinearExpression, CanExecPin)
			];
		}




		IDetailCategoryBuilder& CrossfadeCategory = DetailBuilder.EditCategory("BlendSettings", TEXT("BlendSettings") );
		if (TransitionNode != NULL)
		{
			// The sharing option for the crossfade settings
			CrossfadeCategory.AddCustomRow( TEXT("Transition Crossfade Sharing") )
			[
				GetWidgetForInlineShareMenu(TEXT("Transition Crossfade Sharing"), TransNode->SharedCrossfadeName, TransNode->bSharedCrossfade,
					FOnClicked::CreateSP(this, &FAnimTransitionNodeDetails::OnPromoteToSharedClick, false), 
					FOnClicked::CreateSP(this, &FAnimTransitionNodeDetails::OnUnshareClick, false), 
					FOnGetContent::CreateSP(this, &FAnimTransitionNodeDetails::OnGetShareableNodesMenu, false))
			];
		}

		//@TODO: Gate editing these on shared non-authorative ones
		CrossfadeCategory.AddProperty("CrossfadeDuration").DisplayName( TEXT("Duration") );
		CrossfadeCategory.AddProperty("CrossfadeMode").DisplayName( TEXT("Mode") );

		// Add a button that is only visible when blend logic type is custom
		CrossfadeCategory.AddCustomRow( LOCTEXT("EditBlendGraph", "Edit Blend Graph").ToString() )
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.FillWidth(1)
			.Padding(0,0,10.0f,0)
			[
				SNew(SButton)
				.HAlign(HAlign_Right)
				.OnClicked(this, &FAnimTransitionNodeDetails::OnClickEditBlendGraph)
				.Visibility( this, &FAnimTransitionNodeDetails::GetBlendGraphButtonVisibility )
				.Text(LOCTEXT("EditBlendGraph", "Edit Blend Graph").ToString())
			]
		];





		IDetailCategoryBuilder& NotificationCategory = DetailBuilder.EditCategory("Notifications", TEXT("Notifications") );

		NotificationCategory.AddCustomRow( TEXT("Start Transition Event") )
		[
			SNew( STextBlock )
			.Text( LOCTEXT("StartTransitionEventPropertiesCategoryLabel", "Start Transition Event") )
			.Font( IDetailLayoutBuilder::GetDetailFontBold() )
		];
		CreateTransitionEventPropertyWidgets(NotificationCategory, TEXT("TransitionStart"));


		NotificationCategory.AddCustomRow( TEXT("End Transition Event" ) ) 
		[
			SNew( STextBlock )
			.Text( LOCTEXT("EndTransitionEventPropertiesCategoryLabel", "End Transition Event" ) )
			.Font( IDetailLayoutBuilder::GetDetailFontBold() )
		];
		CreateTransitionEventPropertyWidgets(NotificationCategory, TEXT("TransitionEnd"));

		NotificationCategory.AddCustomRow( TEXT("Interrupt Transition Event") )
		[
			SNew( STextBlock )
			.Text( LOCTEXT("InterruptTransitionEventPropertiesCategoryLabel", "Interrupt Transition Event") )
			.Font( IDetailLayoutBuilder::GetDetailFontBold() )
		];
		CreateTransitionEventPropertyWidgets(NotificationCategory, TEXT("TransitionInterrupt"));
	}

	DetailBuilder.HideProperty("TransitionStart");
	DetailBuilder.HideProperty("TransitionEnd");
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

/** RuleShare = true if we are sharing the rules of this transition (else we are implied to be sharing the crossfade settings) */
FReply FAnimTransitionNodeDetails::OnPromoteToSharedClick(bool RuleShare)
{
	TSharedPtr< SWindow > Parent = FSlateApplication::Get().GetActiveTopLevelWindow();
	if ( Parent.IsValid() )
	{
		// Show dialog to enter new track name
		TSharedRef<STextEntryPopup> TextEntry =
			SNew(STextEntryPopup)
			.Label( LOCTEXT("PromoteAnimTransitionNodeToSharedLabel", "Shared Transition Name") )
			.OnTextCommitted(this, &FAnimTransitionNodeDetails::PromoteToShared, RuleShare);

		// Show dialog to enter new event name
		FSlateApplication::Get().PushMenu(
			Parent.ToSharedRef(),
			TextEntry,
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
			);

		TextEntry->FocusDefaultWidget();
		TextEntryWidget = TextEntry;
	}

	return FReply::Handled();
}

void FAnimTransitionNodeDetails::PromoteToShared(const FText& NewTransitionName, ETextCommit::Type CommitInfo, bool bRuleShare)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		if (UAnimStateTransitionNode* TransNode = TransitionNode.Get())
		{
			if (bRuleShare)
			{
				TransNode->MakeRulesShareable(NewTransitionName.ToString());
				AssignUniqueColorsToAllSharedNodes(TransNode->GetGraph());
			}
			else
			{
				TransNode->MakeCrossfadeShareable(NewTransitionName.ToString());
			}
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

FReply FAnimTransitionNodeDetails::OnUnshareClick(bool bUnshareRule)
{
	if (UAnimStateTransitionNode* TransNode = TransitionNode.Get())
	{
		if (bUnshareRule)
		{
			TransNode->UnshareRules();
		}
		else
		{
			TransNode->UnshareCrossade();
		}
	}

	return FReply::Handled();
}

TSharedRef<SWidget> FAnimTransitionNodeDetails::OnGetShareableNodesMenu(bool bShareRules)
{
	FMenuBuilder MenuBuilder(true, NULL);

	FText SectionText;

	if (bShareRules)
	{
		SectionText = LOCTEXT("PickSharedAnimTransition", "Shared Transition Rules");
	}
	else
	{
		SectionText = LOCTEXT("PickSharedAnimCrossfadeSettings", "Shared Settings");
	}

	MenuBuilder.BeginSection("AnimTransitionSharableNodes", SectionText);

	if (UAnimStateTransitionNode* TransNode = TransitionNode.Get())
	{
		const UEdGraph* CurrentGraph = TransNode->GetGraph();

		// Loop through the graph and build a list of the unique shared transitions
		TMap<FString, UAnimStateTransitionNode*> SharedTransitions;

		for (int32 NodeIdx=0; NodeIdx < CurrentGraph->Nodes.Num(); NodeIdx++)
		{
			if (UAnimStateTransitionNode* GraphTransNode = Cast<UAnimStateTransitionNode>(CurrentGraph->Nodes[NodeIdx]))
			{
				if (bShareRules && !GraphTransNode->SharedRulesName.IsEmpty())
				{
					SharedTransitions.Add(GraphTransNode->SharedRulesName, GraphTransNode);
				}

				if (!bShareRules && !GraphTransNode->SharedCrossfadeName.IsEmpty())
				{
					SharedTransitions.Add(GraphTransNode->SharedCrossfadeName, GraphTransNode);
				}
			}
		}

		for (auto Iter = SharedTransitions.CreateIterator(); Iter; ++Iter)
		{
			FUIAction Action = FUIAction( FExecuteAction::CreateSP(this, &FAnimTransitionNodeDetails::BecomeSharedWith, Iter.Value(), bShareRules) );
			MenuBuilder.AddMenuEntry( FText::FromString( Iter.Key() ), LOCTEXT("ShaerdTransitionToolTip", "Use this shared transition"), FSlateIcon(), Action);
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void FAnimTransitionNodeDetails::BecomeSharedWith(UAnimStateTransitionNode* NewNode, bool bShareRules)
{
	if (UAnimStateTransitionNode* TransNode = TransitionNode.Get())
	{
		if (bShareRules)
		{
			TransNode->UseSharedRules(NewNode);
		}
		else
		{
			TransNode->UseSharedCrossfade(NewNode);
		}
	}
}

void FAnimTransitionNodeDetails::AssignUniqueColorsToAllSharedNodes(UEdGraph* CurrentGraph)
{
	TArray<UEdGraph*> SourceList;
	for (int32 idx=0; idx < CurrentGraph->Nodes.Num(); idx++)
	{
		if (UAnimStateTransitionNode* Node = Cast<UAnimStateTransitionNode>(CurrentGraph->Nodes[idx]))
		{
			if (Node->bSharedRules)
			{
				int32 colorIdx = SourceList.AddUnique(Node->BoundGraph)+1;

				FLinearColor SharedColor;
				SharedColor.R = (colorIdx & 1 ? 1.0f : 0.15f);
				SharedColor.G = (colorIdx & 2 ? 1.0f : 0.15f);
				SharedColor.B = (colorIdx & 4 ? 1.0f : 0.15f);
				SharedColor.A = 0.25f;

				// Storing this on the UAnimStateTransitionNode really bugs me. But its a pain to iterate over all the widget nodes at once
				// and we may want the shared color to be customizable in the details view
				Node->SharedColor = SharedColor;
			}
		}
	}
}

FReply FAnimTransitionNodeDetails::OnClickEditBlendGraph()
{
	if (UAnimStateTransitionNode* TransitionNodePtr = TransitionNode.Get())
	{
		if (TransitionNodePtr->CustomTransitionGraph != NULL)
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(TransitionNodePtr->CustomTransitionGraph);
		}
	}

	return FReply::Handled();
}

EVisibility FAnimTransitionNodeDetails::GetBlendGraphButtonVisibility() const
{
	if (UAnimStateTransitionNode* TransitionNodePtr = TransitionNode.Get())
	{
		if (TransitionNodePtr->LogicType == ETransitionLogicType::TLT_Custom)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Hidden;
}


void FAnimTransitionNodeDetails::CreateTransitionEventPropertyWidgets(IDetailCategoryBuilder& TransitionCategory, FString TransitionName)
{
	TSharedPtr<IPropertyHandle> NameProperty = TransitionCategory.GetParentLayout().GetProperty(*(TransitionName + TEXT(".NotifyName")));
	TSharedPtr<IPropertyHandle> NotifyProperty = TransitionCategory.GetParentLayout().GetProperty(*(TransitionName + TEXT(".Notify")));

	TransitionCategory.AddProperty( NameProperty )
		.DisplayName( LOCTEXT("CreateTransition_CustomBlueprintEvent", "Custom Blueprint Event").ToString() )
		.IsEnabled( TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP( this, &FAnimTransitionNodeDetails::ObjIsNULL, NotifyProperty ) ) );

	TransitionCategory.AddProperty( NotifyProperty )
		.DisplayName( LOCTEXT("CreateTransition_NotifyBlueprintName", "Blueprint Notify").ToString() )
		.IsEnabled( TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP( this, &FAnimTransitionNodeDetails::NameIsNone, NameProperty ) ) );
}

bool FAnimTransitionNodeDetails::NameIsNone(TSharedPtr<IPropertyHandle> Property) const
{
	if (Property.IsValid())
	{
		FName Name = NAME_None;
		FPropertyAccess::Result Result = Property->GetValue(Name);
		return (Name == NAME_None);
	}
	return true;
}

bool FAnimTransitionNodeDetails::ObjIsNULL(TSharedPtr<IPropertyHandle> Property) const
{
	if (Property.IsValid())
	{
		UObject* Obj = NULL;
		FPropertyAccess::Result Result = Property->GetValue(Obj);
		return (Obj == NULL);

	}
	return true;
}

TSharedRef<SWidget> FAnimTransitionNodeDetails::GetWidgetForInlineShareMenu(FString TypeName, FString SharedName, bool bIsCurrentlyShared,  FOnClicked PromoteClick, FOnClicked DemoteClick, FOnGetContent GetContentMenu)
{
	const FText SharedNameText = bIsCurrentlyShared ? FText::FromString(SharedName) : LOCTEXT("SharedTransition", "Use Shared");

	return
		SNew(SExpandableArea)
		.AreaTitle(TypeName)
		.InitiallyCollapsed(true)
		.BodyContent()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 2.0f )
			[
				SNew(SButton)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Fill)
				.OnClicked(bIsCurrentlyShared ? DemoteClick : PromoteClick)
				.Text(bIsCurrentlyShared ? LOCTEXT("UnshareLabel", "Unshare") : LOCTEXT("ShareLabel", "Promote To Shared"))
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 2.0f )
			[
				SNew( SBorder )
				.BorderImage( FEditorStyle::GetBrush( "ToolBar.Background" ) )
				[
					SNew( SComboButton )
					.ToolTipText(LOCTEXT("UseSharedAnimationTransition_ToolTip", "Use Shared Transition"))
					.OnGetMenuContent( GetContentMenu )
					.ContentPadding(0.0f)
					.ButtonStyle( FEditorStyle::Get(), "ToggleButton" )
					.ForegroundColor(FSlateColor::UseForeground())
					.ButtonContent()
					[
						SNew(SEditableTextBox)
						.IsReadOnly(true)
						.MinDesiredWidth(128)
						.Text( SharedNameText )
						.Font( FEditorStyle::GetFontStyle( TEXT( "MenuItem.Font" ) ) )
					]
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE