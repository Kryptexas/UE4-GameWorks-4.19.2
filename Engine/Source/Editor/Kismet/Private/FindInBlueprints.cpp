// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintEditorCommands.h"
#include "BlueprintEditor.h"
#include "FindInBlueprints.h"
#include "Editor/UnrealEd/Public/Kismet2/DebuggerCommands.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistryModule.h"
#include "FindInBlueprintUtils.h"

#define LOCTEXT_NAMESPACE "FindInBlueprints"

//////////////////////////////////////////////////////////////////////////
// FBlueprintSearchResult

FFindInBlueprintsResult::FFindInBlueprintsResult(const FString& InValue ) 
	:Value(InValue), DuplicationIndex(0), Class(NULL), Pin(NULL), GraphNode(NULL)
{
}

FFindInBlueprintsResult::FFindInBlueprintsResult( const FString& InValue, TSharedPtr<FFindInBlueprintsResult>& InParent, UClass* InClass, int InDuplicationIndex ) 
	:Value(InValue), DuplicationIndex(InDuplicationIndex), Parent(InParent), Class(InClass), Pin(NULL), GraphNode(NULL)
{	
}

FFindInBlueprintsResult::FFindInBlueprintsResult( const FString& InValue, TSharedPtr<FFindInBlueprintsResult>& InParent, UEdGraphPin* InPin) 
	: Value(InValue), DuplicationIndex(0), Parent(InParent), Class(InPin->GetClass()), Pin(InPin), GraphNode(NULL)
{
}

FFindInBlueprintsResult::FFindInBlueprintsResult( const FString& InValue, TSharedPtr<FFindInBlueprintsResult>& InParent, UEdGraphNode* InNode) 
	: Value(InValue), DuplicationIndex(0), Parent(InParent), Class(InNode->GetClass()), Pin(NULL), GraphNode(InNode)
{
	if(GraphNode.IsValid())
	{
		CommentText = GraphNode->NodeComment;
	}
}

/* Get a node from the blueprint and the find result */
template<class NodeType>
inline bool GetNode(UBlueprint* Blueprint, const FFindInBlueprintsResult& Result, UEdGraphNode*& OutNode)
{
	if(Result.Class == NodeType::StaticClass()) 
	{
		TArray<NodeType*> Nodes;
		FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes );

		int FoundCount = 0;
		for(auto It(Nodes.CreateConstIterator());It;++It)
		{
			NodeType* Node = *It;
			const FString NodePath = FindInBlueprintsUtil::GetNodeTypePath(Node) + "::" + Node->GetNodeTitle(ENodeTitleType::ListView) ;
			if(NodePath == Result.Value)
			{
				if(FoundCount == Result.DuplicationIndex)
				{
					OutNode = Node;
					break;
				}
				else
				{
					FoundCount++;
				}
			}
		}
	}
	return (OutNode != NULL);
}

FReply FFindInBlueprintsResult::OnClick()
{
	UBlueprint* Blueprint = GetParentBlueprint();
	if(Blueprint)
	{
		if (GraphNode.IsValid() )
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(GraphNode.Get(), /*bRequestRename=*/false);
		}
		else if (Pin.IsValid())
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Pin.Get(), /*bRequestRename=*/false);
		}
		else
		{
			UEdGraphNode* OutNode = NULL;
			if(	GetNode<UEdGraphNode>(Blueprint, *this, OutNode))
			{
				FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(OutNode, /*bRequestRename=*/false);
			}
		}
	}
	return FReply::Handled();
}

FString FFindInBlueprintsResult::GetCategory() const
{
	if(Class == UK2Node_CallFunction::StaticClass())
	{
		return LOCTEXT("CallFuctionCat", "Function Call").ToString();
	}
	else if(Class == UK2Node_MacroInstance::StaticClass())
	{
		return LOCTEXT("MacroCategory", "Macro").ToString();
	}
	else if(Class == UK2Node_Event::StaticClass())
	{
		return LOCTEXT("EventCat", "Event").ToString();
	}
	else if(Class == UK2Node_VariableGet::StaticClass())
	{
		return LOCTEXT("VariableGetCategory", "Variable Get").ToString();
	}
	else if(Class == UK2Node_VariableSet::StaticClass())
	{
		return LOCTEXT("VariableSetCategory", "Variable Set").ToString();
	}
	else if(Class == UEdGraphPin::StaticClass())
	{
		return LOCTEXT("PinCategory", "Pin").ToString();
	}
	else
	{
		return LOCTEXT("NodeCategory", "Node").ToString();
	}
	return FString();
}

TSharedRef<SWidget> FFindInBlueprintsResult::CreateIcon() const
{
	FSlateColor IconColor = FSlateColor::UseForeground();
	const FSlateBrush* Brush = NULL;
	if(Class == UK2Node_Event::StaticClass())
	{
		Brush = FEditorStyle::GetBrush( TEXT("GraphEditor.FIB_Event") );
	}
	else if(Class == UK2Node_MacroInstance::StaticClass())
	{
		Brush =  FEditorStyle::GetBrush( TEXT("GraphEditor.FIB_MacroInstance") );
	}
	else if(Class == UK2Node_CallFunction::StaticClass())
	{
		Brush =  FEditorStyle::GetBrush( TEXT("GraphEditor.FIB_CallFunction") );
	}
	else if(Class == UK2Node_VariableGet::StaticClass())
	{
		Brush =  FEditorStyle::GetBrush( TEXT("GraphEditor.FIB_VariableGet") );
	}
	else if(Class == UK2Node_VariableSet::StaticClass())
	{
		Brush =  FEditorStyle::GetBrush( TEXT("GraphEditor.FIB_VariableSet") );
	}
	else if(Pin.IsValid())
	{
		if( Pin->PinType.bIsArray )
		{
			Brush = FEditorStyle::GetBrush( TEXT("GraphEditor.ArrayPinIcon") );
		}
		else if( Pin->PinType.bIsReference )
		{
			Brush = FEditorStyle::GetBrush( TEXT("GraphEditor.RefPinIcon") );
		}
		else
		{
			Brush = FEditorStyle::GetBrush( TEXT("GraphEditor.PinIcon") );
		}
		const UEdGraphSchema* Schema = Pin->GetSchema();
		IconColor = Schema->GetPinTypeColor(Pin->PinType);
	}
	else if (GraphNode.IsValid())
	{
		Brush = FEditorStyle::GetBrush( TEXT("GraphEditor.NodeGlyph") );
	}

	return 	SNew(SImage)
			.Image(Brush)
			.ColorAndOpacity(IconColor)
			.ToolTipText( GetCategory() );
}

FString FFindInBlueprintsResult::GetCommentText() const
{
	return CommentText;
}

FString FFindInBlueprintsResult::GetValueText() const
{
	FString DisplayVal;

	if( Pin.IsValid()
		&& (!Pin->DefaultValue.IsEmpty() || !Pin->AutogeneratedDefaultValue.IsEmpty() || Pin->DefaultObject || !Pin->DefaultTextValue.IsEmpty())
		&& (Pin->Direction == EGPD_Input)
		&& (Pin->LinkedTo.Num() == 0) )
	{
		if( Pin->DefaultObject )
		{
			const AActor* DefaultActor = Cast<AActor>(Pin->DefaultObject);
			DisplayVal = FString::Printf(TEXT("(%s)"), DefaultActor ? *DefaultActor->GetActorLabel() : *Pin->DefaultObject->GetName());
		}
		else if( !Pin->DefaultValue.IsEmpty() )
		{
			DisplayVal = FString::Printf(TEXT("(%s)"), *Pin->DefaultValue);
		}
		else if( !Pin->AutogeneratedDefaultValue.IsEmpty() )
		{
			DisplayVal = FString::Printf(TEXT("(%s)"), *Pin->AutogeneratedDefaultValue);
		}
		else if( !Pin->DefaultTextValue.IsEmpty() )
		{
			DisplayVal = FString::Printf(TEXT("(%s)"), *Pin->DefaultTextValue.ToString());
		}
	}

	return DisplayVal;
}

UBlueprint* FFindInBlueprintsResult::GetParentBlueprint() const
{
	if (Parent.IsValid()) {return Parent.Pin()->GetParentBlueprint();}
	else {return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), NULL, *Value));}
}


//////////////////////////////////////////////////////////////////////////
// SBlueprintSearch

void SFindInBlueprints::Construct( const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor)
{
	BlueprintEditorPtr = InBlueprintEditor;

	bIsInFindWithinBlueprintMode = true;

	this->ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SAssignNew(SearchTextField, SSearchBox)
					.HintText(LOCTEXT("BlueprintSearchHint", "Enter function or event name to find references..."))
					.OnTextChanged(this, &SFindInBlueprints::OnSearchTextChanged)
					.OnTextCommitted(this, &SFindInBlueprints::OnSearchTextCommitted)
				]
				+SHorizontalBox::Slot()
				.Padding(2.f, 0.f)
				.AutoWidth()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(this, &SFindInBlueprints::OnFindModeChanged)
					.IsChecked(this, &SFindInBlueprints::OnGetFindModeChecked)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("BlueprintSearchModeChange", "Find In Current Blueprint Only").ToString())
					]
				]
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
				[
					SAssignNew(TreeView, STreeViewType)
					.ItemHeight(24)
					.TreeItemsSource( &ItemsFound )
					.OnGenerateRow( this, &SFindInBlueprints::OnGenerateRow )
					.OnGetChildren( this, &SFindInBlueprints::OnGetChildren )
					.OnSelectionChanged(this,&SFindInBlueprints::OnTreeSelectionChanged)
					.SelectionMode( ESelectionMode::Multi )
				]
			]
		];
}

void SFindInBlueprints::FocusForUse(bool bSetFindWithinBlueprint, FString NewSearchTerms, bool bSelectFirstResult)
{
	// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
	FWidgetPath FilterTextBoxWidgetPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked( SearchTextField.ToSharedRef(), FilterTextBoxWidgetPath );

	// Set keyboard focus directly
	FSlateApplication::Get().SetKeyboardFocus( FilterTextBoxWidgetPath, EKeyboardFocusCause::SetDirectly );

	// Set the filter mode
	bIsInFindWithinBlueprintMode = bSetFindWithinBlueprint;

	if (!NewSearchTerms.IsEmpty())
	{
		SearchTextField->SetText(FText::FromString(NewSearchTerms));
		InitiateSearch();

		// Select the first result
		if (bSelectFirstResult && ItemsFound.Num())
		{
			TreeView->SetSelection(ItemsFound[0]);
		}
	}
}

void SFindInBlueprints::OnSearchTextChanged( const FText& Text)
{
	SearchValue = Text.ToString();
}

void SFindInBlueprints::OnSearchTextCommitted( const FText& Text, ETextCommit::Type CommitType )
{
	if (CommitType == ETextCommit::OnEnter)
	{
		InitiateSearch();
	}
}

void SFindInBlueprints::OnFindModeChanged(ESlateCheckBoxState::Type CheckState)
{
	bIsInFindWithinBlueprintMode = CheckState == ESlateCheckBoxState::Checked;
}

ESlateCheckBoxState::Type SFindInBlueprints::OnGetFindModeChecked() const
{
	return bIsInFindWithinBlueprintMode ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFindInBlueprints::InitiateSearch()
{
	TArray<FString> Tokens;
	if(SearchValue.Contains("\"") && SearchValue.ParseIntoArray(&Tokens, TEXT("\""), true)>0)
	{
		for( auto &TokenIt : Tokens )
		{
			// we have the token, we don't need the quotes anymore, they'll just confused the comparison later on
			TokenIt = TokenIt.TrimQuotes();
			// We remove the spaces as all later comparison strings will also be de-spaced
			TokenIt = TokenIt.Replace( TEXT( " " ), TEXT( "" ) );
		}

		// due to being able to handle multiple quoted blocks like ("Make Epic" "Game Now") we can end up with
		// and empty string between (" ") blocks so this simply removes them
		struct FRemoveMatchingStrings
		{ 
			bool operator()(const FString& RemovalCandidate) const
			{
				return RemovalCandidate.IsEmpty();
			}
		};
		Tokens.RemoveAll( FRemoveMatchingStrings() );
	}
	else
	{
		// unquoted search equivalent to a match-any-of search
		SearchValue.ParseIntoArray(&Tokens, TEXT(" "), true);
	}

	for (auto It(ItemsFound.CreateIterator());It;++It)
	{
		TreeView->SetItemExpansion(*It, false);
	}
	ItemsFound.Empty();
	if (Tokens.Num() > 0)
	{
		HighlightText = FText::FromString( SearchValue );
		if (bIsInFindWithinBlueprintMode)
		{
			MatchTokensWithinBlueprint(Tokens);
		}
		else
		{
			MatchTokens(Tokens);
		}
	}

	// Insert a fake result to inform user if none found
	if (ItemsFound.Num() == 0)
	{
		ItemsFound.Add(FSearchResult(new FFindInBlueprintsResult(LOCTEXT("BlueprintSearchNoResults", "No Results found").ToString())));
	}

	TreeView->RequestTreeRefresh();

	for (auto It(ItemsFound.CreateIterator()); It; ++It)
	{
		TreeView->SetItemExpansion(*It, true);
	}
}

void SFindInBlueprints::MatchTokens( const TArray<FString> &Tokens ) 
{
	RootSearchResult.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), Assets, true);
	for(auto It(Assets.CreateConstIterator());It;++It)
	{
		const FAssetData& AssetData = *It;
		//Each blueprints acts as a category
		FSearchResult BlueprintCategory = FSearchResult(new FFindInBlueprintsResult(AssetData.ObjectPath.ToString()));
		
		//find results within the blueprint 
		Extract(AssetData.TagsAndValues.Find("CallsFunctions"), Tokens, BlueprintCategory, UK2Node_CallFunction::StaticClass());
		Extract(AssetData.TagsAndValues.Find("MacroInstances"), Tokens, BlueprintCategory, UK2Node_MacroInstance::StaticClass());
		Extract(AssetData.TagsAndValues.Find("ImplementsFunction"), Tokens, BlueprintCategory, UK2Node_Event::StaticClass());
		Extract(AssetData.TagsAndValues.Find("VariableGet"), Tokens, BlueprintCategory, UK2Node_VariableGet::StaticClass());
		Extract(AssetData.TagsAndValues.Find("VariableSet"), Tokens, BlueprintCategory, UK2Node_VariableSet::StaticClass());
		Extract(AssetData.TagsAndValues.Find("MulticastDelegate"), Tokens, BlueprintCategory, UK2Node_BaseMCDelegate::StaticClass());
		Extract(AssetData.TagsAndValues.Find("DelegateSet"), Tokens, BlueprintCategory, UK2Node_DelegateSet::StaticClass());
		Extract(AssetData.TagsAndValues.Find("Comments"), Tokens, BlueprintCategory, UEdGraphNode::StaticClass());

		//if we got any matches and in FIB mode, add the blueprint as a category to the tree view
		if(BlueprintCategory->Children.Num() > 0 && !bIsInFindWithinBlueprintMode)
		{
			ItemsFound.Add(BlueprintCategory);
		}
	}
}

void SFindInBlueprints::MatchTokensWithinBlueprint(const TArray<FString>& Tokens)
{
	RootSearchResult.Reset();

	UBlueprint* Blueprint = BlueprintEditorPtr.Pin()->GetBlueprintObj();

	RootSearchResult = FSearchResult(new FFindInBlueprintsResult(Blueprint->GetPathName()));
		
	TArray<UK2Node*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes);

	for(auto It(Nodes.CreateConstIterator());It;++It)
	{
		UK2Node* Node = *It;
			
		const FString NodeName = Node->GetNodeTitle(ENodeTitleType::ListView);
		FSearchResult NodeResult(new FFindInBlueprintsResult(NodeName, RootSearchResult, Node));
		UK2Node* K2Node = Cast<UK2Node>(Node);
		AActor* NodeActor = K2Node ? K2Node->GetReferencedLevelActor() : NULL;
		FString NodeSearchString = NodeName + Node->GetClass()->GetName() + Node->NodeComment + (NodeActor ? NodeActor->GetName() + NodeActor->GetActorLabel() : FString());
		NodeSearchString = NodeSearchString.Replace( TEXT( " " ), TEXT( "" ) );

		bool bNodeMatchesSearch = StringMatchesSearchTokens(Tokens, NodeSearchString);

		for( TArray<UEdGraphPin*>::TIterator PinIt(Node->Pins); PinIt; ++PinIt )
		{
			UEdGraphPin* Pin = *PinIt;
			FString PinName = Pin->PinName;
			FString PinSearchString = Pin->PinName + Pin->PinFriendlyName + Pin->DefaultValue + Pin->PinType.PinCategory + Pin->PinType.PinSubCategory + (Pin->PinType.PinSubCategoryObject.IsValid() ? Pin->PinType.PinSubCategoryObject.Get()->GetFullName() : TEXT(""));
			PinSearchString = PinSearchString.Replace( TEXT( " " ), TEXT( "" ) );
			if ((Pin != NULL) && StringMatchesSearchTokens(Tokens, PinSearchString))
			{
				FSearchResult PinResult(new FFindInBlueprintsResult(PinName, NodeResult, Pin));
				NodeResult->Children.Add(PinResult);
			}
		}

		if ((NodeResult->Children.Num() > 0) || bNodeMatchesSearch)
		{
			ItemsFound.Add(NodeResult);
		}
	}

	// Now search for comment nodes containing our search
	TArray<UEdGraphNode_Comment*> CommentNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, CommentNodes);	
	if( CommentNodes.Num() != 0 )
	{
		// Create the root for the comment hit results
		const FString NodeName = FString::Printf(*LOCTEXT("BlueprintResultCommentResults", "Comments containing search term(s)").ToString() );
		FSearchResult RootCommentResult(new FFindInBlueprintsResult(NodeName, RootSearchResult, CommentNodes[0]->GetClass(), 0));		
		
		for(auto It(CommentNodes.CreateConstIterator());It;++It)
		{
			UEdGraphNode_Comment* Node = *It;
			const FString CommentNodeName = Node->NodeComment;
			FSearchResult NodeResult(new FFindInBlueprintsResult(CommentNodeName, RootCommentResult, Node));		
			FString NodeSearchString = CommentNodeName + Node->GetClass()->GetName() + Node->NodeComment;
			NodeSearchString = NodeSearchString.Replace( TEXT( " " ), TEXT( "" ) );

			bool bNodeMatchesSearch = StringMatchesSearchTokens(Tokens, NodeSearchString);

			if (bNodeMatchesSearch)
			{
				RootCommentResult->Children.Add( NodeResult );
			}			
		}	
		
		if (RootCommentResult->Children.Num() > 0)
		{
			ItemsFound.Add(RootCommentResult);
		}
	}
}

void SFindInBlueprints::Extract( const FString* Buffer, const TArray<FString> &Tokens, FSearchResult& BlueprintCategory, UClass* Class )
{
	if(!Buffer)
	{
		return;
	}

	TArray<FString> SubStrings;
	Buffer->ParseIntoArray(&SubStrings, TEXT(","), true);
	//go through each comma separated entry
	for(auto SubIt(SubStrings.CreateConstIterator());SubIt;++SubIt)
	{
		const FString SubString = *SubIt;

		// Remove the spaces from the test string to increase the chance of finding the item.
		const FString SafeSubString = SubString.Replace( TEXT( " " ), TEXT( "" ) );
		if(StringMatchesSearchTokens(Tokens, SafeSubString))
		{
			//found a match. Read the count and add that many entries
			FString SubStringWithoutCount;
			const int DuplicateCount = ReadDuplicateCount(SubString, SubStringWithoutCount);

			FString Comment;

			// Extracting comments are signified by the UEdGraphNode
			if(Class == UEdGraphNode::StaticClass())
			{
				// Comment meta-data is always arranged in form of "[comment]::[node name]". Parse the string and change the label the result will display as to the node name.
				TArray<FString> CommentSubStrings;
				SubStringWithoutCount.ParseIntoArray(&CommentSubStrings, TEXT("::"), true);

				// Certain searches will drop the comment string and find only the node name, so it is not guranteed to properly have the two expected strings and causing a crash.
				if(CommentSubStrings.Num() >= 2)
				{
					Comment = CommentSubStrings[0];
				}
			}

			for(int i = 0;i<DuplicateCount;++i)
			{
				FSearchResult Result(new FFindInBlueprintsResult(SubStringWithoutCount, BlueprintCategory, Class, i) );
				Result->CommentText = Comment;
				BlueprintCategory->Children.Add(Result);
			}
		}
	}
}

TSharedRef<ITableRow> SFindInBlueprints::OnGenerateRow( FSearchResult InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	bool bIsACategoryWidget = !bIsInFindWithinBlueprintMode && !InItem->Parent.IsValid();

	if (bIsACategoryWidget)
	{
		return SNew( STableRow< TSharedPtr<FFindInBlueprintsResult> >, OwnerTable )
			[
				SNew(SBorder)
				.VAlign(VAlign_Center)
				.BorderImage(FEditorStyle::GetBrush("PropertyWindow.CategoryBackground"))
				.Padding(FMargin(2.0f))
				.ForegroundColor(FEditorStyle::GetColor("PropertyWindow.CategoryForeground"))
				[
					SNew(STextBlock)
					.Text(InItem->Value)
					.ToolTipText(LOCTEXT("BlueprintCatSearchToolTip", "Blueprint").ToString())
				]
			];
	}
	else // Functions/Event/Pin widget
	{
		FString CommentText = InItem->GetCommentText();

		return SNew( STableRow< TSharedPtr<FFindInBlueprintsResult> >, OwnerTable )
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					InItem->CreateIcon()
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2,0)
				[
					SNew(STextBlock)
					.Text(InItem->Value)
					.HighlightText(HighlightText)
					.ToolTipText(FString::Printf(*LOCTEXT("BlueprintResultSearchToolTip", "%s : %s").ToString(), *InItem->GetCategory(),* InItem->Value))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(2,0)
				[
					SNew(STextBlock)
					.Text( InItem->GetValueText() )
					.HighlightText(HighlightText)
				]
				+SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(2,0)
				[
					SNew(STextBlock)
					.Text( CommentText.IsEmpty() ? FString() : FString::Printf(TEXT("Node Comment:[%s]"), *CommentText) )
					.ColorAndOpacity(FLinearColor::Yellow)
					.HighlightText(HighlightText)
				]
			];
	}
}

void SFindInBlueprints::OnGetChildren( FSearchResult InItem, TArray< FSearchResult >& OutChildren )
{
	OutChildren += InItem->Children;
}

void SFindInBlueprints::OnTreeSelectionChanged( FSearchResult Item , ESelectInfo::Type  )
{
	if(Item.IsValid())
	{
		Item->OnClick();
	}
}

int SFindInBlueprints::ReadDuplicateCount( const FString& ValueWCount,  FString& OutValue )
{
	TArray<FString> SubStrings;
	OutValue = ValueWCount;
	ValueWCount.ParseIntoArray(&SubStrings, TEXT("::"), true);
	if(SubStrings.Num() > 2)
	{
		const int Count =  FMath::Max(FCString::Atoi(*SubStrings.Last()), 1);
		if(Count != 1)
		{
			OutValue = OutValue.LeftChop(SubStrings.Last().Len()+2);
		}
		return Count;
	}
	else
	{
		return 1;
	}
}

bool SFindInBlueprints::StringMatchesSearchTokens(const TArray<FString>& Tokens, const FString& ComparisonString)
{
	bool bFoundAllTokens = true;
	//search the entry for each token, it must have all of them to pass
	for(auto TokIT(Tokens.CreateConstIterator());TokIT;++TokIT)
	{
		const FString& Token = *TokIT;
		if(!ComparisonString.Contains(Token))
		{
			bFoundAllTokens = false;
			break;
		}
	}
	return bFoundAllTokens;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
