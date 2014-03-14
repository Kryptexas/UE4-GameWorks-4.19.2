// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Developer/AssetTools/Public/IAssetTypeActions.h"
#include "GraphEditor.h"

/*List item that entry for a graph*/
struct FListItemGraphToDiff: public TSharedFromThis<FListItemGraphToDiff>
{
	FListItemGraphToDiff(class SBlueprintDiff* Diff, class UEdGraph* GraphOld, class UEdGraph* GraphNew, const FRevisionInfo& RevisionOld, const FRevisionInfo& RevisionNew);
	~FListItemGraphToDiff();

	/*Generate Widget for list item*/
	TSharedRef<SWidget> GenerateWidget() ;

	/*Get tooltip for list item */
	FText   GetToolTip() ;

	/*Get old(left) graph*/
	UEdGraph* GetGraphOld()const{return GraphOld;}

	/*Get new(right) graph*/
	UEdGraph* GetGraphNew()const{return GraphNew;}

private:

	/*Get icon to use by graph name*/
	static const FSlateBrush* GetIconForGraph(UEdGraph* Graph);

	/*Diff widget*/
	class SBlueprintDiff* Diff;

	/*The old graph(left)*/
	class UEdGraph*	GraphOld;

	/*The new graph(right)*/
	class UEdGraph* GraphNew;

	/*Description of Old and new graph*/
	FRevisionInfo	RevisionOld, RevisionNew;

	//////////////////////////////////////////////////////////////////////////
	// Diff list
	//////////////////////////////////////////////////////////////////////////

	typedef TSharedPtr<struct FDiffResultItem>	FSharedDiffOnGraph;
	typedef SListView<FSharedDiffOnGraph >		SListViewType;

public:

	/** Called when the Newer Graph is modified*/
	void OnGraphChanged( const FEdGraphEditAction& Action) ;

	/** Generate list of differences*/
	TSharedRef<SWidget> GenerateDiffListWidget() ;

	/** Build up the Diff Source Array*/
	void BuildDiffSourceArray();

	/** Called when user clicks on a new graph list item */
	void OnSelectionChanged(FSharedDiffOnGraph Item, ESelectInfo::Type SelectionType);

	/** Called when user presses key within the diff view */
	void KeyWasPressed( const FKeyboardEvent& InKeyboardEvent);

private:
	
	/** Called when user clicks button to go to next difference in graph */
	void NextDiff();

	/** Called when user clicks button to go to prev difference in graph */
	void PrevDiff();

	/** Get Index of the current diff that is selected */
	int32 GetCurrentDiffIndex() ;

	/* Called when a new row is being generated */
	TSharedRef<ITableRow> OnGenerateRow(FSharedDiffOnGraph ParamItem, const TSharedRef<STableViewBase>& OwnerTable );

	/** ListView of differences */
	TSharedPtr<SListViewType> DiffList;

	/** Source for list view */
	TArray<FSharedDiffOnGraph> DiffListSource;

	/** Key commands processed by this widget */
	TSharedPtr< FUICommandList > KeyCommands;
};

/* Visual Diff between two Blueprints*/
class  KISMET_API SBlueprintDiff: public SCompoundWidget
{

public:
	DECLARE_DELEGATE_TwoParams( FOpenInDefaults, class UBlueprint* , class UBlueprint* );

	SLATE_BEGIN_ARGS( SBlueprintDiff ){}
			SLATE_ARGUMENT( class UBlueprint*, BlueprintOld )
			SLATE_ARGUMENT(class UBlueprint*, BlueprintNew )
			SLATE_ARGUMENT( struct FRevisionInfo, OldRevision )
			SLATE_ARGUMENT( struct FRevisionInfo, NewRevision )
			SLATE_ARGUMENT( bool, ShowAssetNames )
			SLATE_EVENT(FOpenInDefaults, OpenInDefaults)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Called when a new Graph is clicked on by user */
	void OnGraphChanged(FListItemGraphToDiff* Diff);

private:
	/* Need to process keys for shortcuts to buttons */
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;


	typedef TSharedPtr<FListItemGraphToDiff>	FGraphToDiff;
	typedef SListView<FGraphToDiff >	SListViewType;

	/** Bring these revisions of graph into focus on main display*/
	void FocusOnGraphRevisions( class UEdGraph* GraphOld, class UEdGraph* GraphNew , FListItemGraphToDiff* Diff);

	/*Create a list item entry graph that exists in at least one of the blueprints */
	void CreateGraphEntry(class UEdGraph* GraphOld, class UEdGraph* GraphNew);

	/* Called when a new row is being generated */
	TSharedRef<ITableRow> OnGenerateRow(FGraphToDiff ParamItem, const TSharedRef<STableViewBase>& OwnerTable );

	/*Called when user clicks on a new graph list item */
	void OnSelectionChanged(FGraphToDiff Item, ESelectInfo::Type SelectionType);

	void OnDiffListSelectionChanged(const TSharedPtr<struct FDiffResultItem>& TheDiff, FListItemGraphToDiff* GraphDiffer);

	/* Handler for blueprint changing. */
	void OnBlueprintChanged(UBlueprint* InBlueprint);
		
	/** Disable the focus on a particular pin */
	void DisablePinDiffFocus();

	/*User toggles the option to lock the views between the two blueprints */
	FReply	OnToggleLockView();

	/*User clicks defaults view button to display defaults in remote diff tool */
	FReply  OnOpenInDefaults();

	/*Reset the graph editor, called when user switches graphs to display*/
	void    ResetGraphEditors();

	/*Get the image to show for the toggle lock option*/
	const FSlateBrush* GetLockViewImage() const;

	/* This buffer stores the currently displayed results */
	TArray< FGraphToDiff> Graphs;

	/** Get Graph editor associated with this Graph */
	SGraphEditor* GetGraphEditorForGraph(UEdGraph* Graph) const ;

	/*panel used to display the blueprint*/
	struct FDiffPanel
	{
		FDiffPanel();

		/*Generate this panel based on the specified graph*/
		void	GeneratePanel(UEdGraph* Graph, UEdGraph* GraphToDiff);

		/*Get the title to show at the top of the panel*/
		FString GetTitle() const;

		/* Called when user hits keyboard shortcut to copy nodes*/
		void CopySelectedNodes();

		/*Gets whatever nodes are selected in the Graph Editor*/
		FGraphPanelSelectionSet GetSelectedNodes() const;

		/*Can user copy any of the selected nodes?*/
		bool CanCopyNodes() const;

		/*The blueprint that owns the graph we are showing*/
		class UBlueprint*				Blueprint;

		/*The border around the graph editor, used to change the content when new graphs are set */
		TSharedPtr<SBorder>				GraphEditorBorder;

		/*The graph editor which does the work of displaying the graph*/
		TWeakPtr<class SGraphEditor>	GraphEditor;

		/*Revision information for this blueprint */
		FRevisionInfo					RevisionInfo;

		/*A name identifying which asset this panel is displaying */
		bool							bShowAssetName;

	private:
		/*Command list for this diff panel*/
		TSharedPtr<FUICommandList> GraphEditorCommands;
	};

	/*The two panels used to show the old & new revision*/ 
	FDiffPanel				PanelOld, PanelNew;
	
	/** If the two views should be locked */
	bool	bLockViews;

	/*Delegate to call when user wishes to view the defaults*/
	FOpenInDefaults	 OpenInDefaults;

	/** Border Widget, inside is the current graphs being diffed, we can replace content to change the graph*/
	TSharedPtr<SBorder>	DiffListBorder;

	/** The ListView containing the graphs the user can select */
	TSharedPtr<SListViewType>	GraphsToDiff;

	/** The last pin the user clicked on */
	UEdGraphPin* LastPinTarget;

	/** The last other pin the user clicked on */
	UEdGraphPin* LastOtherPinTarget;

	friend struct FListItemGraphToDiff;
};


