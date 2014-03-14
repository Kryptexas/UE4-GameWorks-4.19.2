// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __SSkeletonWidget_h__
#define __SSkeletonWidget_h__

#include "Slate.h"

/**
 * This below code is to select skeleton from the list 
 */
#define LOCTEXT_NAMESPACE "SkeletonWidget" 

struct FBoneTrackPair 
{
	FName Bone1;
	FName Bone2;

	FBoneTrackPair(FName InBone1, FName InBone2)
		: Bone1(InBone1)
		, Bone2(InBone2)
	{
	}
};

class SBonePairRow : public SMultiColumnTableRow< TSharedPtr<FBoneTrackPair> >
{
public:
	SLATE_BEGIN_ARGS(SBonePairRow){}
		SLATE_ARGUMENT( TSharedPtr<FBoneTrackPair>, BonePair)
	SLATE_END_ARGS()

	/**
	 * Construct child widgets that comprise this widget.
	 *
	 * @param InArgs  Declaration from which to construct this widget
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		this->BonePair = InArgs._BonePair;

		SMultiColumnTableRow< TSharedPtr<FBoneTrackPair> >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE
	{
		TSharedPtr<SBorder> Border1, Border2;

		if (ColumnName == TEXT("Curretly Selected"))
		{
			return SAssignNew(Border1, SBorder) .Padding(2)
				.Content()
				[
					SNew(STextBlock)
					.Text(BonePair->Bone1.ToString())
				];
		}
		else 
		{
			if (BonePair->Bone2 == NAME_None)
			{
				return SAssignNew(Border2, SBorder) .Padding(2)
					.ColorAndOpacity(FLinearColor(1.f, 0.f, 0.f))
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MissingBone", "Missing"))
					];
			}
			else
			{
				return SAssignNew(Border2, SBorder) .Padding(2)
					.Content()
					[
						SNew(STextBlock)
						.Text(BonePair->Bone2.ToString())
					];
			}
		}
	}
private:
	TSharedPtr<FBoneTrackPair> BonePair;
};

class SSkeletonWidget : public SCompoundWidget
{
public:

	USkeleton * GetSelectedSkeleton() const
	{ 
		return CurSelectedSkeleton; 
	}
protected:
	USkeleton * CurSelectedSkeleton;
};

/** 1 columns - just show bone list **/
class SSkeletonListWidget : public SSkeletonWidget
{
public:
	SLATE_BEGIN_ARGS( SSkeletonListWidget ){}
	SLATE_END_ARGS()
public:
	// WIDGETS

	void Construct(const FArguments& InArgs);

	void SkeletonSelectionChanged(const FAssetData& AssetData);

	TSharedRef<ITableRow> GenerateSkeletonRow( USkeleton* InSkeleton, const TSharedRef<STableViewBase>& OwnerTable )
	{
		return
			SNew( STableRow< USkeleton* >, OwnerTable )
			. Content()
			[
				SNew(STextBlock)
				.Text(InSkeleton->GetFullName())	
			];
	}


	TSharedRef<ITableRow> GenerateSkeletonBoneRow( TSharedPtr<FName> InBoneName, const TSharedRef<STableViewBase>& OwnerTable )
	{
		return
			SNew( STableRow< TSharedPtr<FName> >, OwnerTable )
			. Content()
			[
				SNew(STextBlock)
				.Text(InBoneName->ToString())	
			];
	}

private:
	SVerticalBox::FSlot* BoneListSlot;
	TArray< TSharedPtr<FName> > BoneList;
};

/** 2 columns - bone pair widget **/
class SSkeletonCompareWidget : public SSkeletonWidget
{
public:
	SLATE_BEGIN_ARGS( SSkeletonCompareWidget )
		: _Object(NULL)
		{}

		SLATE_ARGUMENT( UObject*, Object )
		SLATE_ARGUMENT( TArray<FName>*, BoneNames )
	SLATE_END_ARGS()
public:
	// WIDGETS

	void Construct(const FArguments& InArgs);

	void SkeletonSelectionChanged(const FAssetData& AssetData);

	TSharedRef<ITableRow> GenerateBonePairRow( TSharedPtr<FBoneTrackPair> InBonePair, const TSharedRef<STableViewBase>& OwnerTable )
	{
		return
			SNew( SBonePairRow, OwnerTable )
			.BonePair(InBonePair);
	}

private:
	TArray<FName>	BoneNames;
	SVerticalBox::FSlot * BonePairSlot;
	TArray< TSharedPtr<FBoneTrackPair> >	BonePairList;
};

class SSkeletonSelectorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SSkeletonSelectorWindow )
		: _Object(NULL)
		{}

		SLATE_ARGUMENT( UObject*, Object )
		SLATE_ARGUMENT( TSharedPtr<SWindow>, WidgetWindow )
	SLATE_END_ARGS()
public:
	UNREALED_API void Construct(const FArguments& InArgs);

	void ConstructWindowFromAnimSet(UAnimSet* InAnimSet);

	void ConstructWindowFromMesh(USkeletalMesh* InSkeletalMesh);

	void ConstructWindow();

	void ConstructButtons(TSharedRef<SVerticalBox> ParentBox)
	{
		ParentBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
			.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
			.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
			+SUniformGridPanel::Slot(0,0)
			[
				SNew(SButton)
				.Text(LOCTEXT("Accept", "Accept").ToString())
				.HAlign(HAlign_Center)
				.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked_Raw( this, &SSkeletonSelectorWindow::OnAccept )
			]
			+SUniformGridPanel::Slot(1,0)
			[
				SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel").ToString())
				.HAlign(HAlign_Center)
				.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked_Raw( this, &SSkeletonSelectorWindow::OnCancel )
			]
		];
	}

	FReply OnAccept()
	{
		SelectedSkeleton = SkeletonWidget->GetSelectedSkeleton();

		if (SelectedSkeleton!=NULL && WidgetWindow.IsValid())
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	FReply OnCancel()
	{
		if ( WidgetWindow.IsValid() )
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	USkeleton * GetSelectedSkeleton()
	{
		return SelectedSkeleton;
	}

private:
	TSharedPtr<SSkeletonWidget>			SkeletonWidget;
	TWeakPtr<SWindow>					WidgetWindow;
	USkeleton *							SelectedSkeleton;
};



/////////////////////////////////////////////
/** 
 * Slate panel for choose Skeleton for assets to relink
 */
class SAnimationRemapSkeleton : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAnimationRemapSkeleton)
		: _CurrentSkeleton(NULL)
		, _WidgetWindow(NULL)
		, _WarningMessage()
		, _ShowRemapOption(false)
		{}

		/** The anim sequences to compress */
		SLATE_ARGUMENT( USkeleton*, CurrentSkeleton )
		SLATE_ARGUMENT( TSharedPtr<SWindow>, WidgetWindow )
		SLATE_ARGUMENT( FText, WarningMessage )
		SLATE_ARGUMENT( bool, ShowRemapOption )

	SLATE_END_ARGS()	

	/**
	 * Constructs this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	/**
	 * Old Skeleton that was mapped 
	 * This data is needed to prevent users from selecting same skeleton
	 */
	USkeleton *OldSkeleton;
	/**
	 * New Skeleton that they would like to map to
	 */
	USkeleton *NewSkeleton;

	/**
	 * Whether we are remapping assets that are referenced by the assets the user selects to remap
	 */
	bool bRemapReferencedAssets;

	TWeakPtr<SWindow> WidgetWindow;

	/** Handlers for check box for remapping assets option */
	ESlateCheckBoxState::Type IsRemappingReferencedAssets() const;
	void OnRemappingReferencedAssetsChanged(ESlateCheckBoxState::Type InNewRadioState);

	/**
	 * return true if it can apply 
	 */
	bool CanApply() const;

	FReply OnApply();
	FReply OnCancel();

	void CloseWindow();

	/**
	 * Handler for when asset is selected
	 */
	void OnAssetSelectedFromPicker(const FAssetData& AssetData);

	/**
	 *  Show Modal window
	 *
	 * @param OldSkeleton		Old Skeleton to change from
	 * @param NewSkeleton(out)	New Selected Skeleton
	 *
	 * @return true if successfully selected new skeleton
	 */
	static UNREALED_API bool ShowModal(USkeleton * OldSkeleton, USkeleton * & NewSkeleton, const FText& WarningMessage, bool * bRemapReferencedAssets=NULL);
};

////////////////////////////////////////////////////
/** 
* FDlgRemapSkeleton
* 
* Wrapper class for SAnimationRemapSkeleton. This class creates and launches a dialog then awaits the
* result to return to the user. 
*/
class UNREALED_API FDlgRemapSkeleton 
{
public:
	FDlgRemapSkeleton( USkeleton * Skeleton );

	/**  Shows the dialog box and waits for the user to respond. */
	bool ShowModal();

	/** New Skeleton that is chosen **/
	USkeleton * NewSkeleton;

	/** true if you'd like to retarget skeletal meshes as well **/
	bool RetargetSkeletalMesh;

private:

	/** Cached pointer to the modal window */
	TSharedPtr<SWindow> DialogWindow;

	/** Cached pointer to the LOD Chain widget */
	TSharedPtr<class SAnimationRemapSkeleton> DialogWidget;
};


class SRemapFailures : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRemapFailures){}
		SLATE_ARGUMENT(TArray<FString>, FailedRemaps)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );
	static UNREALED_API void OpenRemapFailuresDialog(const TArray<FString>& InFailedRemaps);

private:
	TSharedRef<ITableRow> MakeListViewWidget(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);
	FReply CloseClicked();

private:
	TArray< TSharedPtr<FString> > FailedRemaps;
};

#undef LOCTEXT_NAMESPACE

#endif // __SSkeletonWidget_h__
