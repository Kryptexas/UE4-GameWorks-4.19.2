// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"
#include "SAnimCurveViewer.h"
#include "ObjectTools.h"
#include "AssetRegistryModule.h"
#include "ScopedTransaction.h"
#include "SSearchBox.h"

#define LOCTEXT_NAMESPACE "SAnimCurveViewer"

static const FName ColumnId_AnimCurveNameLabel( "Curve Name" );
static const FName ColumnID_AnimCurveWeightLabel( "Weight" );
static const FName ColumnID_AnimCurveEditLabel( "Edit" );

const float MaxMorphWeight = 5.f;

//////////////////////////////////////////////////////////////////////////
// SAnimCurveListRow

typedef TSharedPtr< FDisplayedAnimCurveInfo > FDisplayedAnimCurveInfoPtr;

class SAnimCurveListRow
	: public SMultiColumnTableRow< FDisplayedAnimCurveInfoPtr >
{
public:

	SLATE_BEGIN_ARGS( SAnimCurveListRow ) {}

	/** The item for this row **/
	SLATE_ARGUMENT( FDisplayedAnimCurveInfoPtr, Item )

	/* The SAnimCurveViewer that we push the morph target weights into */
	SLATE_ARGUMENT( class SAnimCurveViewer*, AnimCurveViewer )

	/* Widget used to display the list of animation curve */
	SLATE_ARGUMENT( TSharedPtr<SAnimCurveListType>, AnimCurveListView )

	/* Persona used to update the viewport when a weight slider is dragged */
	SLATE_ARGUMENT( TWeakPtr<FPersona>, Persona )

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView );

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;

private:

	/**
	* Called when the user changes the value of the SSpinBox
	*
	* @param NewWeight - The new number the SSpinBox is set to
	*
	*/
	void OnAnimCurveWeightChanged( float NewWeight );
	/**
	* Called when the user types the value and enters
	*
	* @param NewWeight - The new number the SSpinBox is set to
	*
	*/
	void OnAnimCurveWeightValueCommitted( float NewWeight, ETextCommit::Type CommitType);

	/** Auto fill check call back functions */
	void OnAnimCurveAutoFillChecked(ECheckBoxState InState);
	ECheckBoxState IsAnimCurveAutoFillChangedChecked() const;
	
	/**
	* Returns the weight of this morph target
	*
	* @return SearchText - The new number the SSpinBox is set to
	*
	*/
	float GetWeight() const;

	/* The SAnimCurveViewer that we push the morph target weights into */
	SAnimCurveViewer* AnimCurveViewer;

	/** Widget used to display the list of animation curve */
	TSharedPtr<SAnimCurveListType> AnimCurveListView;

	/** The name and weight of the morph target */
	FDisplayedAnimCurveInfoPtr	Item;

	/** Pointer back to the Persona that owns us */
	TWeakPtr<FPersona> PersonaPtr;
};

void SAnimCurveListRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	Item = InArgs._Item;
	AnimCurveViewer = InArgs._AnimCurveViewer;
	AnimCurveListView = InArgs._AnimCurveListView;
	PersonaPtr = InArgs._Persona;

	check( Item.IsValid() );

	SMultiColumnTableRow< FDisplayedAnimCurveInfoPtr >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
}

TSharedRef< SWidget > SAnimCurveListRow::GenerateWidgetForColumn( const FName& ColumnName )
{
	if ( ColumnName == ColumnId_AnimCurveNameLabel )
	{
		return
			SNew( SVerticalBox )

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 0.0f, 4.0f )
			.VAlign( VAlign_Center )
			[
				SNew( STextBlock )
				.Text( FText::FromName(Item->Name) )
				.HighlightText( AnimCurveViewer->GetFilterText() )
			];
	}
	else if ( ColumnName == ColumnID_AnimCurveWeightLabel )
	{
		// Encase the SSpinbox in an SVertical box so we can apply padding. Setting ItemHeight on the containing SListView has no effect :-(
		return
			SNew( SVerticalBox )

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 0.0f, 1.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SSpinBox<float> )
				.MinSliderValue(-1.f)
				.MaxSliderValue(1.f)
				.MinValue(-MaxMorphWeight)
				.MaxValue(MaxMorphWeight)
				.Value( this, &SAnimCurveListRow::GetWeight )
				.OnValueChanged( this, &SAnimCurveListRow::OnAnimCurveWeightChanged )
				.OnValueCommitted( this, &SAnimCurveListRow::OnAnimCurveWeightValueCommitted )
			];
	}
	else 
	{
		return
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 1.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SAnimCurveListRow::OnAnimCurveAutoFillChecked)
				.IsChecked(this, &SAnimCurveListRow::IsAnimCurveAutoFillChangedChecked)
			];
	}
}

void SAnimCurveListRow::OnAnimCurveAutoFillChecked(ECheckBoxState InState)
{
	Item->bAutoFillData = InState == ECheckBoxState::Checked;

	if (Item->bAutoFillData)
	{
		// clear value so that it can be filled up
		AnimCurveViewer->RemoveAnimCurveOverride(Item->Name);
	}
}

ECheckBoxState SAnimCurveListRow::IsAnimCurveAutoFillChangedChecked() const
{
	return (Item->bAutoFillData)? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void SAnimCurveListRow::OnAnimCurveWeightChanged( float NewWeight )
{
	Item->Weight = NewWeight;
	Item->bAutoFillData = false;

	AnimCurveViewer->AddAnimCurveOverride( Item->Name, Item->Weight);

	if (PersonaPtr.IsValid())
	{
		PersonaPtr.Pin()->RefreshViewport();
	}
}

void SAnimCurveListRow::OnAnimCurveWeightValueCommitted( float NewWeight, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
	{
		float NewValidWeight = FMath::Clamp(NewWeight, -MaxMorphWeight, MaxMorphWeight);
		Item->Weight = NewValidWeight;
		Item->bAutoFillData = false;

		AnimCurveViewer->AddAnimCurveOverride(Item->Name, Item->Weight);

		TArray< TSharedPtr< FDisplayedAnimCurveInfo > > SelectedRows = AnimCurveListView->GetSelectedItems();

		// ...then any selected rows need changing by the same delta
		for(auto ItemIt = SelectedRows.CreateIterator(); ItemIt; ++ItemIt)
		{
			TSharedPtr< FDisplayedAnimCurveInfo > RowItem = (*ItemIt);

			if(RowItem != Item) // Don't do "this" row again if it's selected
			{
				RowItem->Weight = NewValidWeight;
				RowItem->bAutoFillData = false;
				AnimCurveViewer->AddAnimCurveOverride(RowItem->Name, RowItem->Weight);
			}
		}

		if(PersonaPtr.IsValid())
		{
			PersonaPtr.Pin()->RefreshViewport();
		}
	}
}

float SAnimCurveListRow::GetWeight() const 
{ 
	if (PersonaPtr.IsValid() && Item->bAutoFillData)
	{
		USkeletalMeshComponent* SkelComp = PersonaPtr.Pin()->GetPreviewMeshComponent();
		UAnimInstance* AnimInstance = (SkelComp) ? SkelComp->GetAnimInstance() : nullptr;
		if (AnimInstance)
		{
			TMap<FName, float> CurveList;

			// make sure if they have value that's not same as saved value
			AnimInstance->GetAnimationCurveList(AnimCurveViewer->CurrentCurveFlag, CurveList);

			if (CurveList.Num() > 0)
			{
				float* CurrentValue = CurveList.Find(Item->Name);
				if (CurrentValue && *CurrentValue != 0.f)
				{
					return *CurrentValue;
				}
			}
			else
			{
				return 0.f;
			}
		}
	}

	return Item->Weight; 
}

//////////////////////////////////////////////////////////////////////////
// SAnimCurveTypeList

class SAnimCurveTypeList : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAnimCurveTypeList) {}

	/** The item for this row **/
	SLATE_ARGUMENT(int32, CurveFlags)

	/* The SAnimCurveViewer that we push the morph target weights into */
	SLATE_ARGUMENT(class SAnimCurveViewer*, AnimCurveViewer)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Auto fill check call back functions */
	void OnAnimCurveTypeShowlChecked(ECheckBoxState InState);
	ECheckBoxState IsAnimCurveTypeShowChangedChecked() const;

	FText GetAnimCurveType() const;
private:

	/* The SAnimCurveViewer that we push the morph target weights into */
	SAnimCurveViewer* AnimCurveViewer;

	/** The name and weight of the morph target */
	int32 CurveFlags;
};

void SAnimCurveTypeList::Construct(const FArguments& InArgs)
{
	CurveFlags = InArgs._CurveFlags;
	AnimCurveViewer = InArgs._AnimCurveViewer;

	ChildSlot
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 1.0f)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SAnimCurveTypeList::OnAnimCurveTypeShowlChecked)
			.IsChecked(this, &SAnimCurveTypeList::IsAnimCurveTypeShowChangedChecked)
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(3.0f, 1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(this, &SAnimCurveTypeList::GetAnimCurveType)
			.HighlightText(AnimCurveViewer->GetFilterText())
		]
	];
}

void SAnimCurveTypeList::OnAnimCurveTypeShowlChecked(ECheckBoxState InState)
{
	// clear value so that it can be filled up
	if (InState == ECheckBoxState::Checked)
	{
		AnimCurveViewer->CurrentCurveFlag |= CurveFlags;
	}
	else
	{
		AnimCurveViewer->CurrentCurveFlag &= ~CurveFlags;
	}

	AnimCurveViewer->RefreshCurveList();
}

ECheckBoxState SAnimCurveTypeList::IsAnimCurveTypeShowChangedChecked() const
{
	return (AnimCurveViewer->CurrentCurveFlag & CurveFlags) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FText SAnimCurveTypeList::GetAnimCurveType() const
{
	switch (CurveFlags)
	{
	case ACF_DrivesMorphTarget:
		return LOCTEXT("AnimCurveType_Morphtarget", "Morph Target");
	case ACF_TriggerEvent:
		return LOCTEXT("AnimCurveType_Event", "Event");
	case ACF_DrivesMaterial:
		return LOCTEXT("AnimCurveType_Material", "Material");
	case ACF_Metadata:
		return LOCTEXT("AnimCurveType_Metadata", "Meta Data");
	case ACF_DriveTrack:
		return LOCTEXT("AnimCurveType_Tranform", "Transform");
	case ACF_DrivesPose:
		return LOCTEXT("AnimCurveType_Pose", "Pose");
	}

	return LOCTEXT("AnimCurveType_Unknown", "Unknown");
}
//////////////////////////////////////////////////////////////////////////
// SAnimCurveViewer

void SAnimCurveViewer::Construct(const FArguments& InArgs)
{
	PersonaPtr = InArgs._Persona;
	CachedPreviewInstance = nullptr;

	if (PersonaPtr.IsValid())
	{
		PersonaPtr.Pin()->RegisterOnPreviewMeshChanged(FPersona::FOnPreviewMeshChanged::CreateSP(this, &SAnimCurveViewer::OnPreviewMeshChanged));
		PersonaPtr.Pin()->RegisterOnAnimChanged(FPersona::FOnAnimChanged::CreateSP(this, &SAnimCurveViewer::OnPreviewAssetChanged));
		PersonaPtr.Pin()->RegisterOnPostUndo(FPersona::FOnPostUndo::CreateSP(this, &SAnimCurveViewer::OnPostUndo));
	}

	// @todo fix this to be filtered
	CurrentCurveFlag = ACF_DrivesMorphTarget | ACF_DrivesMaterial | ACF_DrivesPose;

	RefreshCachePreviewInstance();
	 
	TSharedPtr<SVerticalBox> AnimTypeBoxContainer = SNew(SVerticalBox);

	ChildSlot
	[
		SNew( SVerticalBox )
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SNew( STextBlock )
			.Text( this, &SAnimCurveViewer::GetTitleText )
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0,2)
		[
			SNew(SHorizontalBox)
			// Filter entry
			+SHorizontalBox::Slot()
			.FillWidth( 1 )
			[
				SAssignNew( NameFilterBox, SSearchBox )
				.SelectAllTextWhenFocused( true )
				.OnTextChanged( this, &SAnimCurveViewer::OnFilterTextChanged )
				.OnTextCommitted( this, &SAnimCurveViewer::OnFilterTextCommitted )
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()		// This is required to make the scrollbar work, as content overflows Slate containers by default
		[
			SNew(SBox)
			.WidthOverride(150)
			.Content()
			[
				AnimTypeBoxContainer.ToSharedRef()
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight( 1.0f )		// This is required to make the scrollbar work, as content overflows Slate containers by default
		[
			SAssignNew( AnimCurveListView, SAnimCurveListType )
			.ListItemsSource( &AnimCurveList )
			.OnGenerateRow( this, &SAnimCurveViewer::GenerateAnimCurveRow )
			.OnContextMenuOpening( this, &SAnimCurveViewer::OnGetContextMenuContent )
			.ItemHeight( 22.0f )
			.HeaderRow
			(
				SNew( SHeaderRow )
				+ SHeaderRow::Column( ColumnId_AnimCurveNameLabel )
				.DefaultLabel( LOCTEXT( "AnimCurveNameLabel", "Curve Name" ) )

				+ SHeaderRow::Column( ColumnID_AnimCurveWeightLabel )
				.DefaultLabel( LOCTEXT( "AnimCurveWeightLabel", "Weight" ) )

				+ SHeaderRow::Column(ColumnID_AnimCurveEditLabel)
				.DefaultLabel(LOCTEXT("AnimCurveEditLabel", "Auto"))
			)
		]
	];

	CreateAnimCurveTypeList(AnimTypeBoxContainer.ToSharedRef());
	CreateAnimCurveList();
}

FText SAnimCurveViewer::GetTitleText() const
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

	return (PreviewAsset? FText::FromString(PreviewAsset->GetName()) : LOCTEXT("AnimCurveMeshNameLabel", "No Asset Present"));
}

void SAnimCurveViewer::RefreshCachePreviewInstance()
{
	if (CachedPreviewInstance && OnAddAnimationCurveDelegate.IsBound())
	{
		CachedPreviewInstance->RemoveDelegate_AddCustomAnimationCurve(OnAddAnimationCurveDelegate);
	}
	CachedPreviewInstance = Cast<UAnimInstance>(PersonaPtr.Pin()->GetPreviewMeshComponent()->GetAnimInstance());

	if (CachedPreviewInstance)
	{
		OnAddAnimationCurveDelegate.BindRaw(this, &SAnimCurveViewer::ApplyCustomCurveOverride);
		CachedPreviewInstance->AddDelegate_AddCustomAnimationCurve(OnAddAnimationCurveDelegate);
	}
}

void SAnimCurveViewer::OnPreviewMeshChanged(class USkeletalMesh* NewPreviewMesh)
{
	RefreshCachePreviewInstance();
	RefreshCurveList();
}

void SAnimCurveViewer::OnFilterTextChanged( const FText& SearchText )
{
	FilterText = SearchText;

	RefreshCurveList();
}

void SAnimCurveViewer::OnFilterTextCommitted( const FText& SearchText, ETextCommit::Type CommitInfo )
{
	// Just do the same as if the user typed in the box
	OnFilterTextChanged( SearchText );
}

TSharedRef<ITableRow> SAnimCurveViewer::GenerateAnimCurveRow(TSharedPtr<FDisplayedAnimCurveInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable)
{
	check( InInfo.IsValid() );

	return
		SNew( SAnimCurveListRow, OwnerTable )
		.Persona( PersonaPtr )
		.Item( InInfo )
		.AnimCurveViewer( this )
		.AnimCurveListView( AnimCurveListView );
}

TSharedPtr<SWidget> SAnimCurveViewer::OnGetContextMenuContent() const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, NULL);

	MenuBuilder.BeginSection("AnimCurveAction", LOCTEXT( "MorphsAction", "Selected Item Actions" ) );
// 	{
// 		FUIAction Action = FUIAction( FExecuteAction::CreateSP( this, &SAnimCurveViewer::OnDeleteAnimCurves ), 
// 									  FCanExecuteAction::CreateSP( this, &SAnimCurveViewer::CanPerformDelete ) );
// 		const FText Label = LOCTEXT("DeleteAnimCurveButtonLabel", "Delete");
// 		const FText ToolTip = LOCTEXT("DeleteAnimCurveButtonTooltip", "Deletes the selected animation curve.");
// 		MenuBuilder.AddMenuEntry( Label, ToolTip, FSlateIcon(), Action);
// 	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SAnimCurveViewer::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (PersonaPtr.IsValid())
	{
		UDebugSkelMeshComponent* MeshComponent = PersonaPtr.Pin()->GetPreviewMeshComponent();
		UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();

		if (AnimInstance)
		{
			TMap<FName, float> AnimCurves;
			AnimInstance->GetAnimationCurveList(CurrentCurveFlag, AnimCurves);
			// @fixme: this is not enough information to refresh curve, but it is better than not displaying because it didnt' tick yet
			if (AnimCurves.Num() != AnimCurveList.Num())
			{
				RefreshCurveList();
			}
		}
	}
}
void SAnimCurveViewer::CreateAnimCurveList( const FString& SearchText )
{
	AnimCurveList.Empty();

	UDebugSkelMeshComponent* MeshComponent = PersonaPtr.Pin()->GetPreviewMeshComponent();
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();

	if (AnimInstance)
	{
		TMap<FName, float> AnimCurves;
		AnimInstance->GetAnimationCurveList(CurrentCurveFlag, AnimCurves);

		if (AnimCurves.Num() > 0)
		{
			bool bDoFiltering = !SearchText.IsEmpty();

			for (auto Iter = AnimCurves.CreateConstIterator(); Iter; ++Iter)
			{
				FName CurveName = Iter.Key();
				if (bDoFiltering && !CurveName.ToString().Contains(SearchText))
				{
					continue; // Skip items that don't match our filter
				}

				TSharedRef<FDisplayedAnimCurveInfo> Info = FDisplayedAnimCurveInfo::Make(CurveName);
				AnimCurveList.Add(Info);
			}
		}
		else
		{
			// else print somewhere no curves?
		}
	}

	AnimCurveListView->RequestListRefresh();
}

void SAnimCurveViewer::CreateAnimCurveTypeList(TSharedRef<SVerticalBox> VerticalBox)
{
	TArray<int32> CurveFlagsToList;
	CurveFlagsToList.Add(ACF_DrivesMorphTarget);
	CurveFlagsToList.Add(ACF_TriggerEvent);
	CurveFlagsToList.Add(ACF_DrivesMaterial);
	CurveFlagsToList.Add(ACF_Metadata);
	CurveFlagsToList.Add(ACF_DriveTrack);
	CurveFlagsToList.Add(ACF_DrivesPose);

	for (int32 CurveFlagIndex = 0; CurveFlagIndex < CurveFlagsToList.Num(); ++CurveFlagIndex)
	{
		VerticalBox->AddSlot()
		.Padding(3, 1)
		[
			SNew(SAnimCurveTypeList)
			.CurveFlags(CurveFlagsToList[CurveFlagIndex])
			.AnimCurveViewer(this)
		];
	}
}

void SAnimCurveViewer::AddAnimCurveOverride( FName& Name, float Weight)
{
	float& Value = OverrideCurves.FindOrAdd(Name);
	Value = Weight;
}

void SAnimCurveViewer::RemoveAnimCurveOverride(FName& Name)
{
	OverrideCurves.Remove(Name);
}

SAnimCurveViewer::~SAnimCurveViewer()
{
	if ( PersonaPtr.IsValid() )
	{
		// if persona isn't there, we probably don't have the preview mesh either, so no valid anim instance
		if (CachedPreviewInstance && OnAddAnimationCurveDelegate.IsBound())
		{
			CachedPreviewInstance->RemoveDelegate_AddCustomAnimationCurve(OnAddAnimationCurveDelegate);
		}

		PersonaPtr.Pin()->UnregisterOnPreviewMeshChanged(this);
		PersonaPtr.Pin()->UnregisterOnAnimChanged(this);
		PersonaPtr.Pin()->UnregisterOnPostUndo(this);
	}
}

void SAnimCurveViewer::OnPostUndo()
{
	RefreshCurveList();
}

void SAnimCurveViewer::OnPreviewAssetChanged(class UAnimationAsset* NewAsset)
{
	RefreshCurveList();
}

void SAnimCurveViewer::ApplyCustomCurveOverride(UAnimInstance* AnimInstance) const
{
	for (auto Iter = OverrideCurves.CreateConstIterator(); Iter; ++Iter)
	{ 
		// @todo we might want to save original curve flags? or just change curve to apply flags only
		AnimInstance->AddCurveValue(Iter.Key(), Iter.Value(), CurrentCurveFlag);
	}
}

void SAnimCurveViewer::RefreshCurveList()
{
	CreateAnimCurveList(FilterText.ToString());
}
#undef LOCTEXT_NAMESPACE

