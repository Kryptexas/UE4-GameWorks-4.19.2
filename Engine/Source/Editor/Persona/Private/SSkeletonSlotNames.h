// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorObjectsTracker.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "SkeletonSlotNames"
/////////////////////////////////////////////////////
// FSkeletonTreeSummoner
struct FSkeletonSlotNamesSummoner : public FWorkflowTabFactory
{
public:
	FSkeletonSlotNamesSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	// Create a tooltip widget for the tab
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override
	{
		return  IDocumentation::Get()->CreateToolTip(LOCTEXT("WindowTooltip", "This tab lets you modify custom animation SlotName"), NULL, TEXT("Shared/Editors/Persona"), TEXT("AnimationSlotName_Window"));
	}
};

//////////////////////////////////////////////////////////////////////////
// FDisplayedSlotNameInfo

class FDisplayedSlotNameInfo
{
public:
	FName Name;

	/** Handle to editable text block for rename */
	TSharedPtr<SInlineEditableTextBlock> InlineEditableText;

	/** Static function for creating a new item, but ensures that you can only have a TSharedRef to one */
	static TSharedRef<FDisplayedSlotNameInfo> Make(const FName& NotifyName)
	{
		return MakeShareable(new FDisplayedSlotNameInfo(NotifyName));
	}

protected:
	/** Hidden constructor, always use Make above */
	FDisplayedSlotNameInfo(const FName& InNotifyName)
		: Name( InNotifyName )
	{}

	/** Hidden constructor, always use Make above */
	FDisplayedSlotNameInfo() {}
};

/** Widgets list type */
typedef SListView< TSharedPtr<FDisplayedSlotNameInfo> > SSlotNameListType;

class SSkeletonSlotNames : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS( SSkeletonSlotNames )
		: _Persona()
	{}

	SLATE_ARGUMENT( TWeakPtr<FPersona>, Persona )
		
		SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs);

	~SSkeletonSlotNames();

	/**
	* Accessor so our rows can grab the filter text for highlighting
	*
	*/
	FText& GetFilterText() { return FilterText; }

	/** Creates an editor object from the given type to be used in a details panel */
	UObject* ShowInDetailsView( UClass* EdClass );

	/** Clears the detail view of whatever we displayed last */
	void ClearDetailsView();

	/** This triggers a UI repopulation after undo has been called */
	void PostUndo();

	// FGCObject interface start
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	// FGCObject interface end

	/** When user attempts to commit the name of a track*/
	bool OnVerifyNotifyNameCommit( const FText& NewName, FText& OutErrorMessage, TSharedPtr<FDisplayedSlotNameInfo> Item );

	/** When user commits the name of a track*/
	void OnNotifyNameCommitted( const FText& NewName, ETextCommit::Type, TSharedPtr<FDisplayedSlotNameInfo> Item );

	/** Dummy handler to stop editable text boxes swallowing our list selected events */
	bool IsSelected(){return false;}
private:

	/** Called when the user changes the contents of the search box */
	void OnFilterTextChanged( const FText& SearchText );

	/** Called when the user changes the contents of the search box */
	void OnFilterTextCommitted( const FText& SearchText, ETextCommit::Type CommitInfo );

	/** Delegate handler for generating rows in SlotNameListView */ 
	TSharedRef<ITableRow> GenerateNotifyRow( TSharedPtr<FDisplayedSlotNameInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable );

	/** Delegate handler called when the user right clicks in SlotNameListView */
	TSharedPtr<SWidget> OnGetContextMenuContent() const;

	/** Delegate handler for when the user selects something in SlotNameListView */
	void OnNotifySelectionChanged( TSharedPtr<FDisplayedSlotNameInfo> Selection, ESelectInfo::Type SelectInfo );

	/** Delegate handler for determining whether we can show the delete menu options */
	bool CanPerformDelete() const;

	/** Delegate handler for deleting anim SlotName */
	void OnDeleteSlotName();

	/** Delegate handler for determining whether we can show the rename menu options */
	bool CanPerformRename() const;

	/** Delegate handler for renaming anim SlotName */
	void OnRenameSlotName();

	/** Wrapper that populates SlotNameListView using current filter test */
	void RefreshSlotNameListWithFilter();

	/** Populates SlotNameListView based on the skeletons SlotName and the supplied filter text */
	void CreateSlotNameList( const FString& SearchText = FString("") );

	/** handler for user selecting a Notify in SlotNameListView - populates the details panel */
	void ShowNotifyInDetailsView( FName NotifyName );

	/** Populates OutAssets with the AnimSequences that match Personas current skeleton */
	void GetCompatibleAnimMontage( TArray<class FAssetData>& OutAssets );

//	void GetCompatibleAnimBlueprints( TMultiMap<class UAnimBlueprint * , class UAnimGraphNode_Slot *>& OutAssets );

	/** Utility function to display notifications to the user */
	void NotifyUser( FNotificationInfo& NotificationInfo );

	/** The owning Persona instance */
	TWeakPtr<class FPersona> PersonaPtr;

	/** The skeleton we are currently editing */
	USkeleton* TargetSkeleton;

	/** SSearchBox to filter the notify list */
	TSharedPtr<SSearchBox>	NameFilterBox;

	/** Widget used to display the list of SlotName */
	TSharedPtr<SSlotNameListType> SlotNameListView;

	/** A list of SlotName. Used by the SlotNameListView. */
	TArray< TSharedPtr<FDisplayedSlotNameInfo> > NotifyList;

	/** Current text typed into NameFilterBox */
	FText FilterText;

	/** Tracks objects created for displaying in the details panel*/
	FEditorObjectTracker EditorObjectTracker;
};

#undef LOCTEXT_NAMESPACE