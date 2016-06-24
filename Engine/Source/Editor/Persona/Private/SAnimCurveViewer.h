// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Animation/AnimInstance.h"
#include "Persona.h"

//////////////////////////////////////////////////////////////////////////
// FDisplayedAnimCurveInfo

class FDisplayedAnimCurveInfo
{
public:
	FName Name;
	float Weight;
	bool bAutoFillData;

	/** Static function for creating a new item, but ensures that you can only have a TSharedRef to one */
	static TSharedRef<FDisplayedAnimCurveInfo> Make(const FName& Source)
	{
		return MakeShareable(new FDisplayedAnimCurveInfo(Source));
	}

protected:
	/** Hidden constructor, always use Make above */
	FDisplayedAnimCurveInfo(const FName& InSource)
		: Name( InSource )
		, Weight( 0 )
		, bAutoFillData(true)
	{}

	/** Hidden constructor, always use Make above */
	FDisplayedAnimCurveInfo() {}
};

typedef SListView< TSharedPtr<FDisplayedAnimCurveInfo> > SAnimCurveListType;

//////////////////////////////////////////////////////////////////////////
// SAnimCurveViewer

class SAnimCurveViewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SAnimCurveViewer )
		: _Persona()
	{}
		
		/* The Persona that owns this table */
		SLATE_ARGUMENT( TWeakPtr< FPersona >, Persona )

	SLATE_END_ARGS()

	/**
	* Slate construction function
	*
	* @param InArgs - Arguments passed from Slate
	*
	*/
	void Construct( const FArguments& InArgs );

	/**
	* Destructor - resets the animation curve
	*
	*/
	virtual ~SAnimCurveViewer();

	/**
	* Is registered with Persona to handle when its preview mesh is changed.
	*
	* @param NewPreviewMesh - The new preview mesh being used by Persona
	*
	*/
	void OnPreviewMeshChanged(class USkeletalMesh* NewPreviewMesh);

	/**
	* Is registered with Persona to handle when its preview asset is changed.
	*
	* Pose Asset will have to add curve manually
	*/
	void OnPreviewAssetChanged(class UAnimationAsset* NewPreviewAsset);

	/**
	* Filters the SListView when the user changes the search text box (NameFilterBox)
	*
	* @param SearchText - The text the user has typed
	*
	*/
	void OnFilterTextChanged( const FText& SearchText );

	/**
	* Filters the SListView when the user hits enter or clears the search box
	* Simply calls OnFilterTextChanged
	*
	* @param SearchText - The text the user has typed
	* @param CommitInfo - Not used
	*
	*/
	void OnFilterTextCommitted( const FText& SearchText, ETextCommit::Type CommitInfo );

	/**
	* Create a widget for an entry in the tree from an info
	*
	* @param InInfo - Shared pointer to the morph target we're generating a row for
	* @param OwnerTable - The table that owns this row
	*
	* @return A new Slate widget, containing the UI for this row
	*/
	TSharedRef<ITableRow> GenerateAnimCurveRow(TSharedPtr<FDisplayedAnimCurveInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	* Adds a morph target override or updates the weight for an existing one
	*
	* @param Name - Name of the morph target we want to override
	* @param Weight - How much of this morph target to apply (0.0 - 1.0)
	*
	*/
	void AddAnimCurveOverride( FName& Name, float Weight);
	void RemoveAnimCurveOverride(FName& Name);

	/**
	* Tells the AnimInstance to reset all of its morph target curves
	*
	*/
	void ResetAnimCurves();
	
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

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

	void RefreshCurveList();

private:

	/** Handler for context menus */
	TSharedPtr<SWidget> OnGetContextMenuContent() const;

	/**
	* Clears and rebuilds the table, according to an optional search string
	*
	* @param SearchText - Optional search string
	*
	*/
	void CreateAnimCurveList( const FString& SearchText = FString() );
	void CreateAnimCurveTypeList(TSharedRef<SVerticalBox> VerticalBox);

	void ApplyCustomCurveOverride(UAnimInstance* AnimInstance) const;
	void RefreshCachePreviewInstance();

	/** Pointer back to the Persona that owns us */
	TWeakPtr<FPersona> PersonaPtr;

	/** Box to filter to a specific morph target name */
	TSharedPtr<SSearchBox>	NameFilterBox;

	/** Widget used to display the list of animation curve */
	TSharedPtr<SAnimCurveListType> AnimCurveListView;

	/** A list of animation curve. Used by the AnimCurveListView. */
	TArray< TSharedPtr<FDisplayedAnimCurveInfo> > AnimCurveList;

	/** The skeletal mesh that we grab the animation curve from */
	UAnimInstance* CachedPreviewInstance;

	/** Current text typed into NameFilterBox */
	FText FilterText;

	int32 CurrentCurveFlag;

	TMap<FName, float> OverrideCurves;

	/** Curve delegate */
	FOnAddCustomAnimationCurves OnAddAnimationCurveDelegate;

	friend class SAnimCurveListRow;
	friend class SAnimCurveTypeList;
};