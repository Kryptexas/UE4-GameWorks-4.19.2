// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "Persona.h"
#include "GraphEditor.h"
#include "SNodePanel.h"
#include "SAnimCurvePanel.h"
#include "SAnimEditorBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/PoseAsset.h"


//////////////////////////////////////////////////////////////////////////
// FDisplayedPoseInfo

class FDisplayedPoseInfo
{
public:
	FName Name;
	float Weight;

	/** Static function for creating a new item, but ensures that you can only have a TSharedRef to one */
	static TSharedRef<FDisplayedPoseInfo> Make(const FName& Source)
	{
		return MakeShareable(new FDisplayedPoseInfo(Source));
	}

protected:
	/** Hidden constructor, always use Make above */
	FDisplayedPoseInfo(const FName& InSource)
		: Name(InSource)
		, Weight(0)
	{}

	/** Hidden constructor, always use Make above */
	FDisplayedPoseInfo() {}
};

typedef SListView< TSharedPtr<FDisplayedPoseInfo> > SPoseListType;

//////////////////////////////////////////////////////////////////////////
// FDisplayedCurveInfo

class FDisplayedCurveInfo
{
public:
	FName Name;

	/** Static function for creating a new item, but ensures that you can only have a TSharedRef to one */
	static TSharedRef<FDisplayedCurveInfo> Make(const FName& Source)
	{
		return MakeShareable(new FDisplayedCurveInfo(Source));
	}

protected:
	/** Hidden constructor, always use Make above */
	FDisplayedCurveInfo(const FName& InSource)
		: Name(InSource)
	{}

	/** Hidden constructor, always use Make above */
	FDisplayedCurveInfo() {}
};

typedef SListView< TSharedPtr<FDisplayedCurveInfo> > SCurveListType;
//////////////////////////////////////////////////////////////////////////
// SPoseViewer

class SPoseViewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPoseViewer)
		: _Persona()
	{}

	/* The Persona that owns this table */
	SLATE_ARGUMENT(TWeakPtr< FPersona >, Persona)
	SLATE_ARGUMENT(TWeakObjectPtr< UPoseAsset >, PoseAsset)

	SLATE_END_ARGS()

		/**
		* Slate construction function
		*
		* @param InArgs - Arguments passed from Slate
		*
		*/
		void Construct(const FArguments& InArgs);

	/**
	* Destructor - resets the animation curve
	*
	*/
	virtual ~SPoseViewer();

	/**
	* Is registered with Persona to handle when its preview mesh is changed.
	*
	* @param NewPreviewMesh - The new preview mesh being used by Persona
	*
	*/
	void OnPreviewMeshChanged(class USkeletalMesh* NewPreviewMesh);

	/**
	* Filters the SListView when the user changes the search text box (NameFilterBox)
	*
	* @param SearchText - The text the user has typed
	*
	*/
	void OnFilterTextChanged(const FText& SearchText);

	/**
	* Filters the SListView when the user hits enter or clears the search box
	* Simply calls OnFilterTextChanged
	*
	* @param SearchText - The text the user has typed
	* @param CommitInfo - Not used
	*
	*/
	void OnFilterTextCommitted(const FText& SearchText, ETextCommit::Type CommitInfo);

	/**
	* Create a widget for an entry in the tree from an info
	*
	* @param InInfo - Shared pointer to the morph target we're generating a row for
	* @param OwnerTable - The table that owns this row
	*
	* @return A new Slate widget, containing the UI for this row
	*/
	TSharedRef<ITableRow> GeneratePoseRow(TSharedPtr<FDisplayedPoseInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateCurveRow(TSharedPtr<FDisplayedCurveInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	* Provides state to the IsEnabled property of the delete morph targets button
	*
	*/
	bool IsPoseSelected() const;
	bool IsCurveSelected() const;

	/**
	* Handler for the delete poses button
	*/
	void OnDeletePoses();
	void OnDeleteCurves();

	void OnPastePoseNamesFromClipBoard(bool bSelectedOnly);

	/**
	* Adds a morph target override or updates the weight for an existing one
	*
	* @param Name - Name of the morph target we want to override
	* @param Weight - How much of this morph target to apply (0.0 - 1.0)
	*
	*/
	void AddPoseOverride(FName& Name, float Weight);
	void RemovePoseOverride(FName& Name);

	/**
	* Tells the AnimInstance to reset all of its morph target curves
	*
	*/
	void ResetPoses();

	/**
	* Accessor so our rows can grab the filtertext for highlighting
	*
	*/
	FText& GetFilterText() { return FilterText; }

	/**
	* Get Title text
	*/
	FText GetTitleText() const;
	/**
	* Refreshes the morph target list after an undo
	*/
	void OnPostUndo();

	bool ModifyName(FName OldName, FName NewName, bool bSilence = false);
	bool IsBasePose(FName PoseName) const;

private:

	/** Handler for context menus */
	TSharedPtr<SWidget> OnGetContextMenuContent() const;
	/** Handler for curve list context menu*/
	TSharedPtr<SWidget> OnGetContextMenuContentForCurveList() const;

	/**
	* Clears and rebuilds the table, according to an optional search string
	*
	* @param SearchText - Optional search string
	*
	*/
	void CreatePoseList(const FString& SearchText = FString());
	void CreateCurveList(const FString& SearchText = FString());

	void ApplyCustomCurveOverride(UAnimInstance* AnimInstance) const;
	void RefreshCachePreviewInstance();

	/** Pointer back to the Persona that owns us */
	TWeakPtr<FPersona> PersonaPtr;

	/** Pointer to the pose asset */
	TWeakObjectPtr<UPoseAsset> PoseAssetPtr;

	/** Box to filter to a specific morph target name */
	TSharedPtr<SSearchBox>	NameFilterBox;

	/** Widget used to display the list of animation curve */
	TSharedPtr<SPoseListType> PoseListView;

	/** A list of animation curve. Used by the PoseListView. */
	TArray< TSharedPtr<FDisplayedPoseInfo> > PoseList;

	/** Widget used to display the list of animation curve */
	TSharedPtr<SCurveListType> CurveListView;

	/** A list of animation curve. Used by the PoseListView. */
	TArray< TSharedPtr<FDisplayedCurveInfo> > CurveList;

	/** The skeletal mesh that we grab the animation curve from */
	TWeakObjectPtr<UAnimSingleNodeInstance> CachedPreviewInstance;

	/** Current text typed into NameFilterBox */
	FText FilterText;

	TMap<FName, float> OverrideCurves;

	/** Add curve delegate */
	FOnAddCustomAnimationCurves OnAddAnimationCurveDelegate;

	friend class SPoseListRow;
	friend class SCurveListRow;
};
//////////////////////////////////////////////////////////////////////////
// SPoseEditor

/** Overall animation sequence editing widget */
class SPoseEditor : public SAnimEditorBase
{
public:
	SLATE_BEGIN_ARGS( SPoseEditor )
		: _Persona()
		, _PoseAsset(NULL)
		{}

		SLATE_ARGUMENT( TSharedPtr<FPersona>, Persona )
		SLATE_ARGUMENT( UPoseAsset*, PoseAsset )
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	virtual ~SPoseEditor();

	virtual UAnimationAsset* GetEditorObject() const override { return PoseAssetObj; }

private:
	/** Pointer to the animation sequence being edited */
	UPoseAsset* PoseAssetObj;
};
