// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/* Item that matched the search results */
class FFindInBlueprintsResult
{
public: 
	/* Create a root */
	FFindInBlueprintsResult(const FString& InValue);
	
	/* Create a listing for a search result*/
	FFindInBlueprintsResult(const FString& InValue, TSharedPtr<FFindInBlueprintsResult>& InParent, UClass* InClass, int InDuplicationIndex );
	
	/* Create a listing for a pin result */
	FFindInBlueprintsResult(const FString& InValue, TSharedPtr<FFindInBlueprintsResult>& InParent, UEdGraphPin* InPin);
	
	/* Create a listing for a pin result */
	FFindInBlueprintsResult(const FString& InValue, TSharedPtr<FFindInBlueprintsResult>& InParent, UEdGraphNode* InNode);
	
	/* Called when user clicks on the search item */
	FReply OnClick();

	/* Get Category for this search result */
	FString GetCategory() const;

	/* Create an icon to represent the result */
	TSharedRef<SWidget>	CreateIcon() const;

	/* Gets the comment on this node if any */
	FString GetCommentText() const;
	
	/* Gets the value of the pin if any */
	FString GetValueText() const;

	/** gets the blueprint housing all these search results */
	UBlueprint* GetParentBlueprint() const;

	/*Any children listed under this category */
	TArray< TSharedPtr<FFindInBlueprintsResult> > Children;

	/*If it exists it is the blueprint*/
	TWeakPtr<FFindInBlueprintsResult> Parent;

	/*The meta string that was stored in the asset registry for this item */
	FString Value;

	/*The blueprint may have multiple instances of whatever we are looking for, this tells us which instance # we refer to*/
	int	DuplicationIndex;

	/*The class this item refers to */
	UClass* Class;

	/** The pin that this search result refers to */
	TWeakObjectPtr<UEdGraphPin> Pin;

	/** The graph node that this search result refers to (if not by asset registry or UK2Node) */
	TWeakObjectPtr<UEdGraphNode> GraphNode;

	/** Display text for comment information */
	FString CommentText;
};


/*Widget for searching for (functions/events) across all blueprints or just a single blueprint */
class SFindInBlueprints: public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SFindInBlueprints ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class FBlueprintEditor> InBlueprintEditor);

	/** Focuses this widget's search box, and changes the mode as well, and optionally the search terms */
	void FocusForUse(bool bSetFindWithinBlueprint, FString NewSearchTerms = FString(), bool bSelectFirstResult = false);

private:
	typedef TSharedPtr<FFindInBlueprintsResult> FSearchResult;
	typedef STreeView<FSearchResult> STreeViewType;

	/*Called when user changes the text they are searching for */
	void OnSearchTextChanged(const FText& Text);

	/*Called when user changes commits text to the search box */
	void OnSearchTextCommitted(const FText& Text, ETextCommit::Type CommitType);

	/** Called when the find mode checkbox is hit */
	void OnFindModeChanged(ESlateCheckBoxState::Type CheckState);

	/** Called to check what the find mode is for the checkbox */
	ESlateCheckBoxState::Type OnGetFindModeChecked() const;

	/* Get the children of a row */
	void OnGetChildren( FSearchResult InItem, TArray< FSearchResult >& OutChildren );

	/* Called when user clicks on a new result */
	void OnTreeSelectionChanged(FSearchResult Item, ESelectInfo::Type SelectInfo );

	/* Called when a new row is being generated */
	TSharedRef<ITableRow> OnGenerateRow(FSearchResult InItem, const TSharedRef<STableViewBase>& OwnerTable);

	/** Begins the search based on the SearchValue */
	void InitiateSearch();

	/* Find any results that contain all of the tokens, only for find in all blueprints */
	void MatchTokens(const TArray<FString>& Tokens);
	
	/* Find any results that contain all of the tokens, however, only in find within blueprints */
	void MatchTokensWithinBlueprint(const TArray<FString>& Tokens);

	/* Extract results from a single buffer */
	void Extract(const FString* Buffer, const TArray<FString> &Tokens, FSearchResult& BlueprintCategory, UClass* Class);

	/* Read the duplication count stored in the string */
	static int ReadDuplicateCount(const FString& ValueWCount, FString& OutValue );
	
	/** Determines if a string matches the search tokens */
	static bool StringMatchesSearchTokens(const TArray<FString>& Tokens, const FString& ComparisonString);

private:
	/** Pointer back to the blueprint editor that owns us */
	TWeakPtr<class FBlueprintEditor> BlueprintEditorPtr;
	
	/* The tree view displays the results */
	TSharedPtr<STreeViewType> TreeView;

	/** The search text box */
	TSharedPtr<class SSearchBox> SearchTextField;
	
	/* This buffer stores the currently displayed results */
	TArray<FSearchResult> ItemsFound;

	/** In Find Within Blueprint mode, we need to keep a handle on the root result, because it won't show up in the tree */
	FSearchResult RootSearchResult;

	/* The string to highlight in the results */
	FText HighlightText;

	/* The string to search for */
	FString	SearchValue;

	/** Should we search within the current blueprint only (rather than all blueprints) */
	bool bIsInFindWithinBlueprintMode;
};