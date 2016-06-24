// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"
#include "SPoseEditor.h"
#include "ObjectTools.h"
#include "AssetRegistryModule.h"
#include "ScopedTransaction.h"
#include "SSearchBox.h"
#include "Animation/AnimSingleNodeInstance.h"

#define LOCTEXT_NAMESPACE "AnimPoseEditor"

//////////////////////////////////////////////////////////////////////////
// SPoseEditor

void SPoseEditor::Construct(const FArguments& InArgs)
{
	PoseAssetObj = InArgs._PoseAsset;
	check(PoseAssetObj);

	SAnimEditorBase::Construct( SAnimEditorBase::FArguments()
		.Persona(InArgs._Persona)
		);

 	EditorPanels->AddSlot()
 	.AutoHeight()
 	.Padding(0, 10)
	[
 		SNew( SPoseViewer)
 		.Persona(InArgs._Persona)
		.PoseAsset(PoseAssetObj)
 	];
}

SPoseEditor::~SPoseEditor()
{
}

////////////////////////////////////
// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
static const FName ColumnId_PoseNameLabel("Pose Name");
static const FName ColumnID_PoseWeightLabel("Weight");
static const FName ColumnId_CurveNameLabel("Curve Name");

const float MaxPoseWeight = 1.f;

//////////////////////////////////////////////////////////////////////////
// SPoseListRow

typedef TSharedPtr< FDisplayedPoseInfo > FDisplayedPoseInfoPtr;

class SPoseListRow
	: public SMultiColumnTableRow< FDisplayedPoseInfoPtr >
{
public:

	SLATE_BEGIN_ARGS(SPoseListRow) {}

	/** The item for this row **/
	SLATE_ARGUMENT(FDisplayedPoseInfoPtr, Item)

	/* The SPoseViewer that we push the morph target weights into */
	SLATE_ARGUMENT(class SPoseViewer*, PoseViewer)

	/* Widget used to display the list of animation curve */
	SLATE_ARGUMENT(TSharedPtr<SPoseListType>, PoseListView)

	/* Persona used to update the viewport when a weight slider is dragged */
	SLATE_ARGUMENT(TWeakPtr<FPersona>, Persona)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView);

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:

	/**
	* Called when the user changes the value of the SSpinBox
	*
	* @param NewWeight - The new number the SSpinBox is set to
	*
	*/
	void OnPoseWeightChanged(float NewWeight);
	/**
	* Called when the user types the value and enters
	*
	* @param NewWeight - The new number the SSpinBox is set to
	*
	*/
	void OnPoseWeightValueCommitted(float NewWeight, ETextCommit::Type CommitType);

	/** Delegate to get labels root text from settings */
	FText GetName() const;

	/** Delegate to commit labels root text to settings */
	void OnNameCommitted(const FText& InText, ETextCommit::Type InCommitType) const;

	/**
	* Returns the weight of this morph target
	*
	* @return SearchText - The new number the SSpinBox is set to
	*
	*/
	float GetWeight() const;

	bool CanChangeWeight() const;
	/* The SPoseViewer that we push the morph target weights into */
	SPoseViewer* PoseViewer;

	/** Widget used to display the list of animation curve */
	TSharedPtr<SPoseListType> PoseListView;

	/** The name and weight of the morph target */
	FDisplayedPoseInfoPtr	Item;

	/** Pointer back to the Persona that owns us */
	TWeakPtr<FPersona> PersonaPtr;
};

FText SPoseListRow::GetName() const
{
	return (FText::FromName(Item->Name));
}

void SPoseListRow::OnNameCommitted(const FText& InText, ETextCommit::Type InCommitType) const
{
	// for now only allow enter
	// because it is important to keep the unique names per pose
	if (InCommitType == ETextCommit::OnEnter)
	{
		FName NewName = FName(*InText.ToString());
		FName OldName = Item->Name;

		if (PoseViewer->ModifyName(OldName, NewName))
		{
			Item->Name = NewName;
		}
	}
}

void SPoseListRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	Item = InArgs._Item;
	PoseViewer = InArgs._PoseViewer;
	PoseListView = InArgs._PoseListView;
	PersonaPtr = InArgs._Persona;

	check(Item.IsValid());

	SMultiColumnTableRow< FDisplayedPoseInfoPtr >::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef< SWidget > SPoseListRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == ColumnId_PoseNameLabel)
	{
		return
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.Text(this, &SPoseListRow::GetName)
				.ToolTipText(LOCTEXT("PoseName_ToolTip", "Modify Pose Name - Make sure this name is unique among all curves per skeleton."))
				.OnTextCommitted(this, &SPoseListRow::OnNameCommitted)
				.OnTextChanged(this, &SPoseListRow::OnNameCommitted, ETextCommit::Default)
			];
	}
	else 
	{
		// Encase the SSpinbox in an SVertical box so we can apply padding. Setting ItemHeight on the containing SListView has no effect :-(
		return
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SSpinBox<float>)
				.MinSliderValue(-1.f)
				.MaxSliderValue(1.f)
				.MinValue(-MaxPoseWeight)
				.MaxValue(MaxPoseWeight)
				.Value(this, &SPoseListRow::GetWeight)
				.OnValueChanged(this, &SPoseListRow::OnPoseWeightChanged)
				.OnValueCommitted(this, &SPoseListRow::OnPoseWeightValueCommitted)
				.IsEnabled(this, &SPoseListRow::CanChangeWeight)
			];
	}
}

void SPoseListRow::OnPoseWeightChanged(float NewWeight)
{
	Item->Weight = NewWeight;

	PoseViewer->AddPoseOverride(Item->Name, Item->Weight);

	if (PersonaPtr.IsValid())
	{
		PersonaPtr.Pin()->RefreshViewport();
	}
}

void SPoseListRow::OnPoseWeightValueCommitted(float NewWeight, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
	{
		float NewValidWeight = FMath::Clamp(NewWeight, -MaxPoseWeight, MaxPoseWeight);
		Item->Weight = NewValidWeight;

		PoseViewer->AddPoseOverride(Item->Name, Item->Weight);

		TArray< TSharedPtr< FDisplayedPoseInfo > > SelectedRows = PoseListView->GetSelectedItems();

		// ...then any selected rows need changing by the same delta
		for (auto ItemIt = SelectedRows.CreateIterator(); ItemIt; ++ItemIt)
		{
			TSharedPtr< FDisplayedPoseInfo > RowItem = (*ItemIt);

			if (RowItem != Item) // Don't do "this" row again if it's selected
			{
				RowItem->Weight = NewValidWeight;
				PoseViewer->AddPoseOverride(RowItem->Name, RowItem->Weight);
			}
		}

		if (PersonaPtr.IsValid())
		{
			PersonaPtr.Pin()->RefreshViewport();
		}
	}
}

float SPoseListRow::GetWeight() const
{
	return Item->Weight;
}

bool SPoseListRow::CanChangeWeight() const
{
	return PoseViewer->IsBasePose(Item->Name) == false;
}

//////////////////////////////////////////////////////////////////////////
// SCurveListRow

typedef TSharedPtr< FDisplayedCurveInfo > FDisplayedCurveInfoPtr;

class SCurveListRow
	: public SMultiColumnTableRow< FDisplayedCurveInfoPtr >
{
public:

	SLATE_BEGIN_ARGS(SCurveListRow) {}

	/** The item for this row **/
	SLATE_ARGUMENT(FDisplayedCurveInfoPtr, Item)

	/* Widget used to display the list of animation curve */
	SLATE_ARGUMENT(TSharedPtr<SCurveListType>, CurveListView)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView);

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:

	/** Delegate to get labels root text from settings */
	FText GetName() const;

	/** The name and weight of the morph target */
	FDisplayedCurveInfoPtr	Item;
};

FText SCurveListRow::GetName() const
{
	return (FText::FromName(Item->Name));
}

void SCurveListRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	Item = InArgs._Item;

	check(Item.IsValid());

	SMultiColumnTableRow< FDisplayedCurveInfoPtr >::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef< SWidget > SCurveListRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	// for now we have one colume
//	if (ColumnName == ColumnId_CurveNameLabel)
	{
		return
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SCurveListRow::GetName)
			];
	}
}

//////////////////////////////////////////////////////////////////////////
// SPoseViewer

void SPoseViewer::Construct(const FArguments& InArgs)
{
	PersonaPtr = InArgs._Persona;
	PoseAssetPtr = InArgs._PoseAsset;

	CachedPreviewInstance = nullptr;

	if (PersonaPtr.IsValid())
	{
		PersonaPtr.Pin()->RegisterOnPreviewMeshChanged(FPersona::FOnPreviewMeshChanged::CreateSP(this, &SPoseViewer::OnPreviewMeshChanged));
		PersonaPtr.Pin()->RegisterOnPostUndo(FPersona::FOnPostUndo::CreateSP(this, &SPoseViewer::OnPostUndo));
	}

	RefreshCachePreviewInstance();

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew(STextBlock)
			.Text(this, &SPoseViewer::GetTitleText)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2)
		[
			SNew(SHorizontalBox)
			// Filter entry
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew(NameFilterBox, SSearchBox)
				.SelectAllTextWhenFocused(true)
				.OnTextChanged(this, &SPoseViewer::OnFilterTextChanged)
				.OnTextCommitted(this, &SPoseViewer::OnFilterTextCommitted)
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)		// This is required to make the scrollbar work, as content overflows Slate containers by default
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(5)
			[
				SAssignNew(PoseListView, SPoseListType)
				.ListItemsSource(&PoseList)
				.OnGenerateRow(this, &SPoseViewer::GeneratePoseRow)
				.OnContextMenuOpening(this, &SPoseViewer::OnGetContextMenuContent)
				.ItemHeight(22.0f)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(ColumnId_PoseNameLabel)
					.DefaultLabel(LOCTEXT("PoseNameLabel", "Pose Name"))

					+ SHeaderRow::Column(ColumnID_PoseWeightLabel)
					.DefaultLabel(LOCTEXT("PoseWeightLabel", "Weight"))
				)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(5)
			[
				SAssignNew(CurveListView, SCurveListType)
				.ListItemsSource(&CurveList)
				.OnGenerateRow(this, &SPoseViewer::GenerateCurveRow)
				.OnContextMenuOpening(this, &SPoseViewer::OnGetContextMenuContentForCurveList)
				.ItemHeight(22.0f)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(ColumnId_CurveNameLabel)
					.DefaultLabel(LOCTEXT("CurveNameLabel", "Curve Name"))
				)
			]
		]
	];

	CreatePoseList();
	CreateCurveList();
}

FText SPoseViewer::GetTitleText() const
{
	UObject* PreviewAsset = nullptr;

	if (PersonaPtr.IsValid())
	{
		PreviewAsset = PersonaPtr.Pin()->GetPreviewAnimationAsset();
		if (!PreviewAsset)
		{
			PreviewAsset = PersonaPtr.Pin()->GetAnimBlueprint();
		}
	}

	return (PreviewAsset ? FText::FromString(PreviewAsset->GetName()) : LOCTEXT("PoseMeshNameLabel", "No Asset Present"));
}

void SPoseViewer::RefreshCachePreviewInstance()
{
	if (CachedPreviewInstance.IsValid() && OnAddAnimationCurveDelegate.IsBound())
	{
		CachedPreviewInstance.Get()->RemoveDelegate_AddCustomAnimationCurve(OnAddAnimationCurveDelegate);
	}
	CachedPreviewInstance = Cast<UAnimSingleNodeInstance>(PersonaPtr.Pin()->GetPreviewMeshComponent()->GetAnimInstance());

	if (CachedPreviewInstance.IsValid())
	{
		OnAddAnimationCurveDelegate.Unbind();
		OnAddAnimationCurveDelegate.BindRaw(this, &SPoseViewer::ApplyCustomCurveOverride);
		CachedPreviewInstance.Get()->AddDelegate_AddCustomAnimationCurve(OnAddAnimationCurveDelegate);
	}
}

void SPoseViewer::OnPreviewMeshChanged(class USkeletalMesh* NewPreviewMesh)
{
	RefreshCachePreviewInstance();
	CreatePoseList(NameFilterBox->GetText().ToString());
	CreateCurveList(NameFilterBox->GetText().ToString());
}

void SPoseViewer::OnFilterTextChanged(const FText& SearchText)
{
	FilterText = SearchText;

	CreatePoseList(SearchText.ToString());
	CreateCurveList(SearchText.ToString());
}

void SPoseViewer::OnFilterTextCommitted(const FText& SearchText, ETextCommit::Type CommitInfo)
{
	// Just do the same as if the user typed in the box
	OnFilterTextChanged(SearchText);
}

TSharedRef<ITableRow> SPoseViewer::GeneratePoseRow(TSharedPtr<FDisplayedPoseInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(InInfo.IsValid());

	return
		SNew(SPoseListRow, OwnerTable)
		.Persona(PersonaPtr)
		.Item(InInfo)
		.PoseViewer(this)
		.PoseListView(PoseListView);
}

TSharedRef<ITableRow> SPoseViewer::GenerateCurveRow(TSharedPtr<FDisplayedCurveInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(InInfo.IsValid());

	return
		SNew(SCurveListRow, OwnerTable)
		.Item(InInfo)
		.CurveListView(CurveListView);
}

bool SPoseViewer::IsPoseSelected() const
{
	// @todo: make sure not to delete base Curve
	TArray< TSharedPtr< FDisplayedPoseInfo > > SelectedRows = PoseListView->GetSelectedItems();
	return SelectedRows.Num() > 0;
}

bool SPoseViewer::IsCurveSelected() const
{
	// @todo: make sure not to delete base pose
	TArray< TSharedPtr< FDisplayedCurveInfo > > SelectedRows = CurveListView->GetSelectedItems();
	return SelectedRows.Num() > 0;
}

void SPoseViewer::OnDeletePoses()
{
	TArray< TSharedPtr< FDisplayedPoseInfo > > SelectedRows = PoseListView->GetSelectedItems();

	FScopedTransaction Transaction(LOCTEXT("DeletePoses", "Delete Poses"));
	PoseAssetPtr.Get()->Modify();

	TArray<FName> PosesToDelete;
	for (int RowIndex = 0; RowIndex < SelectedRows.Num(); ++RowIndex)
	{
		PosesToDelete.Add(SelectedRows[RowIndex]->Name);
	}

	PoseAssetPtr.Get()->DeletePoses(PosesToDelete);

	CreatePoseList(NameFilterBox->GetText().ToString());
}

void SPoseViewer::OnDeleteCurves()
{
	TArray< TSharedPtr< FDisplayedCurveInfo > > SelectedRows = CurveListView->GetSelectedItems();

	FScopedTransaction Transaction(LOCTEXT("DeleteCurves", "Delete Curves"));
	PoseAssetPtr.Get()->Modify();

	TArray<FName> CurvesToDelete;
	for (int RowIndex = 0; RowIndex < SelectedRows.Num(); ++RowIndex)
	{
		CurvesToDelete.Add(SelectedRows[RowIndex]->Name);
	}

	PoseAssetPtr.Get()->DeleteCurves(CurvesToDelete);

	CreateCurveList(NameFilterBox->GetText().ToString());
}

void SPoseViewer::OnPastePoseNamesFromClipBoard(bool bSelectedOnly)
{
	FString PastedString;

	FPlatformMisc::ClipboardPaste(PastedString);

	if (PastedString.IsEmpty() == false)
	{
		TArray<FString> ListOfNames;
		PastedString.ParseIntoArrayLines(ListOfNames);

		if (ListOfNames.Num() > 0)
		{
			TArray<FName> PosesToRename;
			if (bSelectedOnly)
			{
				TArray< TSharedPtr< FDisplayedPoseInfo > > SelectedRows = PoseListView->GetSelectedItems();

				for (int RowIndex = 0; RowIndex < SelectedRows.Num(); ++RowIndex)
				{
					PosesToRename.Add(SelectedRows[RowIndex]->Name);
				}
			}
			else
			{
				for (auto& PoseItem : PoseList)
				{
					PosesToRename.Add(PoseItem->Name);
				}
			}

			if (PosesToRename.Num() > 0)
			{
				FScopedTransaction Transaction(LOCTEXT("PasteNames", "Paste Curve Names"));
				PoseAssetPtr.Get()->Modify();

				int32 TotalItemCount = FMath::Min(PosesToRename.Num(), ListOfNames.Num());

				for (int32 PoseIndex = 0; PoseIndex < TotalItemCount; ++PoseIndex)
				{ 
					ModifyName(PosesToRename[PoseIndex], FName(*ListOfNames[PoseIndex]), true);
				}

				CreatePoseList(NameFilterBox->GetText().ToString());
			}
		}
	}

}

TSharedPtr<SWidget> SPoseViewer::OnGetContextMenuContent() const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, NULL);

	FUIAction Action = FUIAction(FExecuteAction::CreateSP(this, &SPoseViewer::OnPastePoseNamesFromClipBoard, false));
	FText MenuLabel = LOCTEXT("PasteAllPoseNamesButtonLabel", "Paste All Pose Names");
	FText MenuToolTip = LOCTEXT("PasteAllPoseNamesButtonTooltip", "Paste All Pose Names From Clipboard");
	MenuBuilder.AddMenuEntry(MenuLabel, MenuToolTip, FSlateIcon(), Action);

	MenuBuilder.BeginSection("PoseAction", LOCTEXT("Poseaction", "Selected Item Actions"));
	{
	 	Action = FUIAction( FExecuteAction::CreateSP( this, &SPoseViewer::OnDeletePoses ), 
	 									FCanExecuteAction::CreateSP( this, &SPoseViewer::IsPoseSelected ) );
	 	MenuLabel = LOCTEXT("DeletePoseButtonLabel", "Delete");
	 	MenuToolTip = LOCTEXT("DeletePoseButtonTooltip", "Deletes the selected animation pose.");
	 	MenuBuilder.AddMenuEntry(MenuLabel, MenuToolTip, FSlateIcon(), Action);

		Action = FUIAction(FExecuteAction::CreateSP(this, &SPoseViewer::OnPastePoseNamesFromClipBoard, true),
			FCanExecuteAction::CreateSP(this, &SPoseViewer::IsPoseSelected));
		MenuLabel = LOCTEXT("PastePoseNamesButtonLabel", "Paste Selected");
		MenuToolTip = LOCTEXT("PastePoseNamesButtonTooltip", "Paste the selected pose names from ClipBoard.");
		MenuBuilder.AddMenuEntry(MenuLabel, MenuToolTip, FSlateIcon(), Action);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> SPoseViewer::OnGetContextMenuContentForCurveList() const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, NULL);

	MenuBuilder.BeginSection("CurveAction", LOCTEXT("CurveActions", "Selected Item Actions"));
	{
		FUIAction Action = FUIAction(FExecuteAction::CreateSP(this, &SPoseViewer::OnDeleteCurves),
			FCanExecuteAction::CreateSP(this, &SPoseViewer::IsCurveSelected));
		const FText MenuLabel = LOCTEXT("DeleteCurveButtonLabel", "Delete");
		const FText MenuToolTip = LOCTEXT("DeleteCurveButtonTooltip", "Deletes the selected animation curve.");
		MenuBuilder.AddMenuEntry(MenuLabel, MenuToolTip, FSlateIcon(), Action);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SPoseViewer::CreatePoseList(const FString& SearchText)
{
	PoseList.Empty();

	UAnimInstance* AnimInstance = CachedPreviewInstance.IsValid() ? CachedPreviewInstance.Get() : nullptr;
	if (AnimInstance && PoseAssetPtr.IsValid())
	{
		UPoseAsset* PoseAsset = PoseAssetPtr.Get();

		TArray<FSmartName> PoseNames = PoseAsset->GetPoseNames();
		if (PoseNames.Num() > 0)
		{
			bool bDoFiltering = !SearchText.IsEmpty();

			for (const auto& PoseSmartName : PoseNames)
			{
				FName PoseName = PoseSmartName.DisplayName;
				if (bDoFiltering && !PoseName.ToString().Contains(SearchText))
				{
					continue; // Skip items that don't match our filter
				}

				TSharedRef<FDisplayedPoseInfo> Info = FDisplayedPoseInfo::Make(PoseName);
				PoseList.Add(Info);
			}
		}
		else
		{
			// else print somewhere no curves?
		}
	}

	PoseListView->RequestListRefresh();
}

void SPoseViewer::CreateCurveList(const FString& SearchText)
{
	CurveList.Empty();

	UAnimInstance* AnimInstance = CachedPreviewInstance.IsValid() ? CachedPreviewInstance.Get() : nullptr;
	if (AnimInstance && PoseAssetPtr.IsValid())
	{
		UPoseAsset* PoseAsset = PoseAssetPtr.Get();

		TArray<FSmartName> CurveNames = PoseAsset->GetCurveNames();
		if (CurveNames.Num() > 0)
		{
			bool bDoFiltering = !SearchText.IsEmpty();

			for (const auto& CurveSmartName : CurveNames)
			{
				FName CurveName = CurveSmartName.DisplayName;
				if (bDoFiltering && !CurveName.ToString().Contains(SearchText))
				{
					continue; // Skip items that don't match our filter
				}

				TSharedRef<FDisplayedCurveInfo> Info = FDisplayedCurveInfo::Make(CurveName);
				CurveList.Add(Info);
			}
		}
		else
		{
			// else print somewhere no curves?
		}
	}

	CurveListView->RequestListRefresh();
}

void SPoseViewer::AddPoseOverride(FName& Name, float Weight)
{
	float& Value = OverrideCurves.FindOrAdd(Name);
	Value = Weight;
}

void SPoseViewer::RemovePoseOverride(FName& Name)
{
	OverrideCurves.Remove(Name);
}

SPoseViewer::~SPoseViewer()
{
	if (PersonaPtr.IsValid())
	{
		// @Todo: change this in curve editor
		// and it won't work with this one delegate idea, so just think of a better way to do
		// if persona isn't there, we probably don't have the preview mesh either, so no valid anim instance
		if (CachedPreviewInstance.IsValid() && OnAddAnimationCurveDelegate.IsBound())
		{
			CachedPreviewInstance.Get()->RemoveDelegate_AddCustomAnimationCurve(OnAddAnimationCurveDelegate);
		}

		PersonaPtr.Pin()->UnregisterOnPreviewMeshChanged(this);
		PersonaPtr.Pin()->UnregisterOnPostUndo(this);
	}
}

void SPoseViewer::OnPostUndo()
{
	CreatePoseList(NameFilterBox->GetText().ToString());
	CreateCurveList(NameFilterBox->GetText().ToString());
}

void SPoseViewer::ApplyCustomCurveOverride(UAnimInstance* AnimInstance) const
{
	for (auto Iter = OverrideCurves.CreateConstIterator(); Iter; ++Iter)
	{
		// @todo we might want to save original curve flags? or just change curve to apply flags only
		AnimInstance->AddCurveValue(Iter.Key(), Iter.Value(), ACF_DrivesPose);
	}
}

bool SPoseViewer::ModifyName(FName OldName, FName NewName, bool bSilence)
{
	if (PersonaPtr.IsValid())
	{
		USkeleton* Skeleton = PersonaPtr.Pin()->GetSkeleton();
		if (Skeleton)
		{
			FScopedTransaction Transaction(LOCTEXT("RenamePoses", "Rename Pose"));

			PoseAssetPtr.Get()->Modify();
			Skeleton->Modify();

			// get smart nam
			const FSmartNameMapping::UID ExistingUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, NewName);
			// verify if this name exists in smart naming
			if (ExistingUID != FSmartNameMapping::MaxUID)
			{
				// warn users
				// if so, verify if this name is still okay
				if (!bSilence)
				{
					const EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("UseSameNameConfirm", "The name already exists. Would you like to reuse the name? This can cause conflict of curve data."));

					if (Response == EAppReturnType::No)
					{
						return false;
					}
				}

				// I think this might have to be delegate of the top window
				if (PoseAssetPtr.Get()->ModifyPoseName(OldName, NewName, &ExistingUID) == false)
				{
					FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NameAlreadyUsedByTheSameAsset", "The name is used by another pose within the same asset. Please choose another name."));
					return false;
				}
			}
			else
			{
				// I think this might have to be delegate of the top window
				if (PoseAssetPtr.Get()->ModifyPoseName(OldName, NewName, nullptr) == false)
				{
					FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NameAlreadyUsedByTheSameAsset", "The name is used by another pose within the same asset. Please choose another name."));
					return false;
				}
			}

			// now refresh pose data
			float* Value = OverrideCurves.Find(OldName);
			if (Value)
			{
				AddPoseOverride(NewName, *Value);
				RemovePoseOverride(OldName);
			}

			return true;
		}
	}

	return false;
}

bool SPoseViewer::IsBasePose(FName PoseName) const
{
	if (PoseAssetPtr.IsValid() && PoseAssetPtr->IsValidAdditive())
	{
		int32 PoseIndex = PoseAssetPtr->GetPoseIndexByName(PoseName);
		return (PoseIndex == PoseAssetPtr->GetBasePoseIndex());
	}

	return false;
}
#undef LOCTEXT_NAMESPACE
