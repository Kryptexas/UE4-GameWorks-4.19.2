// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "SSkeletonWidget.h"
#include "AssetData.h"
#include "MainFrame.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "SkeletonWidget"

void SSkeletonListWidget::Construct(const FArguments& InArgs)
{
	CurSelectedSkeleton = NULL;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(USkeleton::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SSkeletonListWidget::SkeletonSelectionChanged);
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Column;

	this->ChildSlot
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SelectSkeletonLabel", "Select Skeleton: "))
			]

			+SVerticalBox::Slot() .FillHeight(1) .Padding(2)
				[
					SNew(SBorder)
					.Content()
					[
						ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
					]
				]

			+SVerticalBox::Slot() .FillHeight(1) .Padding(2)
				.Expose(BoneListSlot)

		];

	// Construct the BoneListSlot by clearing the skeleton selection. 
	SkeletonSelectionChanged(FAssetData());
}

void SSkeletonListWidget::SkeletonSelectionChanged(const FAssetData& AssetData)
{
	BoneList.Empty();
	CurSelectedSkeleton = Cast<USkeleton>(AssetData.GetAsset());

	if (CurSelectedSkeleton != NULL)
	{
		const FReferenceSkeleton & RefSkeleton = CurSelectedSkeleton->GetReferenceSkeleton();

		for (int32 I=0; I<RefSkeleton.GetNum(); ++I)
		{
			BoneList.Add( MakeShareable(new FName(RefSkeleton.GetBoneName(I))) ) ;
		}

		(*BoneListSlot)
			[
				SNew(SBorder) .Padding(2)
				.Content()
				[
					SNew(SListView< TSharedPtr<FName> >)
					.OnGenerateRow(this, &SSkeletonListWidget::GenerateSkeletonBoneRow)
					.ListItemsSource(&BoneList)
					.HeaderRow
					(
					SNew(SHeaderRow)
					+SHeaderRow::Column(TEXT("Bone Name"))
					.DefaultLabel(NSLOCTEXT("SkeletonWidget", "BoneName", "Bone Name"))
					)
				]
			];
	}
	else
	{
		(*BoneListSlot)
			[
				SNew(SBorder) .Padding(2)
				.Content()
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SkeletonWidget", "NoSkeletonIsSelected", "No skeleton is selected!"))
				]
			];
	}
}

void SSkeletonCompareWidget::Construct(const FArguments& InArgs)
{
	const UObject * Object = InArgs._Object;

	CurSelectedSkeleton = NULL;
	BoneNames = *InArgs._BoneNames;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(USkeleton::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SSkeletonCompareWidget::SkeletonSelectionChanged);
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Column;

	TSharedPtr<SToolTip> SkeletonTooltip = IDocumentation::Get()->CreateToolTip(FText::FromString("Pick a skeleton for this mesh"), NULL, FString("Shared/Editors/Persona"), FString("Skeleton"));

	this->ChildSlot
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight() 
				.Padding(2) 
				.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)
				 
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					+SHorizontalBox::Slot()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CurrentlySelectedSkeletonLabel_SelectSkeleton", "Select Skeleton"))
						.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 16))
						.ToolTip(SkeletonTooltip)
					]
					+SHorizontalBox::Slot()
					.FillWidth(1)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						IDocumentation::Get()->CreateAnchor(FString("Engine/Animation/Skeleton"))
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight() 
				.Padding(2, 10)
				.HAlign(HAlign_Fill)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

				+SVerticalBox::Slot()
				.AutoHeight() 
				.Padding(2) 
				.HAlign(HAlign_Fill)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CurrentlySelectedSkeletonLabel", "Currently Selected : "))
				]
				+SVerticalBox::Slot()
				.AutoHeight() 
				.Padding(2) 
				.HAlign(HAlign_Fill)
				[
					SNew(STextBlock)
					.Text(Object->GetFullName())
				]
			]

			+SVerticalBox::Slot() .FillHeight(1) .Padding(2)
				[
					SNew(SBorder)
					.Content()
					[
						ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
					]
				]

			+SVerticalBox::Slot() .FillHeight(1) .Padding(2)
				.Expose(BonePairSlot)
		];

	// Construct the BonePairSlot by clearing the skeleton selection. 
	SkeletonSelectionChanged(FAssetData());
}

void SSkeletonCompareWidget::SkeletonSelectionChanged(const FAssetData& AssetData)
{
	BonePairList.Empty();
	CurSelectedSkeleton = Cast<USkeleton>(AssetData.GetAsset());

	if (CurSelectedSkeleton != NULL)
	{
		for (int32 I=0; I<BoneNames.Num(); ++I)
		{
			if ( CurSelectedSkeleton->GetReferenceSkeleton().FindBoneIndex(BoneNames[I]) != INDEX_NONE )
			{
				BonePairList.Add( MakeShareable(new FBoneTrackPair(BoneNames[I], BoneNames[I])) );
			}
			else
			{
				BonePairList.Add( MakeShareable(new FBoneTrackPair(BoneNames[I], TEXT(""))) );
			}
		}

		(*BonePairSlot)
			[
				SNew(SBorder) .Padding(2)
				.Content()
				[
					SNew(SListView< TSharedPtr<FBoneTrackPair> >)
					.OnGenerateRow(this, &SSkeletonCompareWidget::GenerateBonePairRow)
					.ListItemsSource(&BonePairList)
					.HeaderRow
					(
					SNew(SHeaderRow)
					+SHeaderRow::Column(TEXT("Curretly Selected"))
					.DefaultLabel(NSLOCTEXT("SkeletonWidget", "CurrentlySelected", "Currently Selected").ToString())
					+SHeaderRow::Column(TEXT("Target Skeleton Bone"))
					.DefaultLabel(NSLOCTEXT("SkeletonWidget", "TargetSkeletonBone", "Target Skeleton Bone").ToString())
					)
				]
			];
	}
	else
	{
		(*BonePairSlot)
			[
				SNew(SBorder) .Padding(2)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoSkeletonSelectedLabel", "No skeleton is selected!"))
				]
			];
	}
}

void SSkeletonSelectorWindow::Construct(const FArguments& InArgs)
{
	UObject * Object = InArgs._Object;
	WidgetWindow = InArgs._WidgetWindow;
	SelectedSkeleton = NULL;
	if (Object == NULL)
	{
		ConstructWindow();
	}
	else if (Object->IsA(USkeletalMesh::StaticClass()))
	{
		ConstructWindowFromMesh(CastChecked<USkeletalMesh>(Object));
	}
	else if (Object->IsA(UAnimSet::StaticClass()))
	{
		ConstructWindowFromAnimSet(CastChecked<UAnimSet>(Object));
	}
}

void SSkeletonSelectorWindow::ConstructWindowFromAnimSet(UAnimSet* InAnimSet)
{
	TArray<FName>  *TrackNames = &InAnimSet->TrackBoneNames;

	TSharedRef<SVerticalBox> ContentBox = SNew(SVerticalBox)
		+SVerticalBox::Slot() 
		.FillHeight(1) 
		.Padding(2)
		[
			SAssignNew(SkeletonWidget, SSkeletonCompareWidget)
			.Object(InAnimSet)
			.BoneNames(TrackNames)	
		];

	ConstructButtons(ContentBox);

	ChildSlot
		[
			ContentBox
		];
}

void SSkeletonSelectorWindow::ConstructWindowFromMesh(USkeletalMesh* InSkeletalMesh)
{
	TArray<FName>  BoneNames;

	for (int32 I=0; I<InSkeletalMesh->RefSkeleton.GetNum(); ++I)
	{
		BoneNames.Add(InSkeletalMesh->RefSkeleton.GetBoneName(I));
	}

	TSharedRef<SVerticalBox> ContentBox = SNew(SVerticalBox)
		+SVerticalBox::Slot() .FillHeight(1) .Padding(2)
		[
			SAssignNew(SkeletonWidget, SSkeletonCompareWidget)
			.Object(InSkeletalMesh)
			.BoneNames(&BoneNames)
		];

	ConstructButtons(ContentBox);

	ChildSlot
		[
			ContentBox
		];
}

void SSkeletonSelectorWindow::ConstructWindow()
{
	TSharedRef<SVerticalBox> ContentBox = SNew(SVerticalBox)
		+SVerticalBox::Slot() .FillHeight(1) .Padding(2)
		[
			SAssignNew(SkeletonWidget, SSkeletonListWidget)
		];

	ConstructButtons(ContentBox);

	ChildSlot
		[
			ContentBox
		];
}

void SAnimationRemapSkeleton::Construct( const FArguments& InArgs )
{
	OldSkeleton = InArgs._CurrentSkeleton;
	NewSkeleton = NULL;
	bRemapReferencedAssets = true;
	WidgetWindow = InArgs._WidgetWindow;

	// button 
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(USkeleton::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SAnimationRemapSkeleton::OnAssetSelectedFromPicker);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Column;
	AssetPickerConfig.ThumbnailScale = 0.0f;

	TSharedRef<SWidget> Widget = SNew(SBox);
	if(InArgs._ShowRemapOption)
	{
		Widget = SNew(SCheckBox)
				.IsChecked(this, &SAnimationRemapSkeleton::IsRemappingReferencedAssets)
				.OnCheckStateChanged(this, &SAnimationRemapSkeleton::OnRemappingReferencedAssetsChanged)
				[
					SNew(STextBlock).Text(LOCTEXT("RemapSkeleton_RemapAssets", "Remap referenced assets "))
				];
	}

	TSharedPtr<SToolTip> SkeletonTooltip = IDocumentation::Get()->CreateToolTip(FText::FromString("Pick a skeleton for this mesh"), NULL, FString("Shared/Editors/Persona"), FString("Skeleton"));

	this->ChildSlot
		[
		SNew (SVerticalBox)

		+SVerticalBox::Slot()
		.AutoHeight() 
		.Padding(2) 
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
				 
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			+SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CurrentlySelectedSkeletonLabel_SelectSkeleton", "Select Skeleton"))
				.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 16))
				.ToolTip(SkeletonTooltip)
			]
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				IDocumentation::Get()->CreateAnchor(FString("Engine/Animation/Skeleton"))
			]
		]

		+SVerticalBox::Slot()
		.AutoHeight() 
		.Padding(2, 10)
		.HAlign(HAlign_Fill)
		[
			SNew(SSeparator)
			.Orientation(Orient_Horizontal)
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(5)
		[
			SNew(STextBlock)
			.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10 ) )
			.ColorAndOpacity(FLinearColor::Red)
			.Text( LOCTEXT("RemapSkeleton_Warning_Title", "[WARNING]") )
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(STextBlock)
			.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10 ) )
			.ColorAndOpacity(FLinearColor::Red)
			.Text( InArgs._WarningMessage )
		]

		+SVerticalBox::Slot()
		.AutoHeight() 
		.Padding(5)
		[
			SNew(SSeparator)
		]

		+SVerticalBox::Slot()
		.MaxHeight(500)
		[
			SNew(SBox)
			.WidthOverride(400)
			[
				ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
			]
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			Widget
		]

		+SVerticalBox::Slot()
		.AutoHeight() 
		.Padding(5)
		[
			SNew(SSeparator)
		]

		+SVerticalBox::Slot()
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
				SNew(SButton) .HAlign(HAlign_Center)
				.Text(LOCTEXT("RemapSkeleton_Apply", "Select"))
				.IsEnabled(this, &SAnimationRemapSkeleton::CanApply)
				.OnClicked(this, &SAnimationRemapSkeleton::OnApply)
				.HAlign(HAlign_Center)
				.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
			]
			+SUniformGridPanel::Slot(1,0)
			[
				SNew(SButton) .HAlign(HAlign_Center)
				.Text(LOCTEXT("RemapSkeleton_Cancel", "Cancel"))
				.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked(this, &SAnimationRemapSkeleton::OnCancel)
			]
		]
	];
}

ESlateCheckBoxState::Type SAnimationRemapSkeleton::IsRemappingReferencedAssets() const
{
	return bRemapReferencedAssets ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SAnimationRemapSkeleton::OnRemappingReferencedAssetsChanged(ESlateCheckBoxState::Type InNewRadioState)
{
	bRemapReferencedAssets = (InNewRadioState == ESlateCheckBoxState::Checked);
}

bool SAnimationRemapSkeleton::CanApply() const
{
	return (NewSkeleton!=NULL && NewSkeleton!=OldSkeleton);
}

void SAnimationRemapSkeleton::OnAssetSelectedFromPicker(const FAssetData& AssetData)
{
	if (AssetData.GetAsset())
	{
		NewSkeleton = Cast<USkeleton>(AssetData.GetAsset());
	}
}

FReply SAnimationRemapSkeleton::OnApply()
{
	CloseWindow();
	return FReply::Handled();
}
FReply SAnimationRemapSkeleton::OnCancel()
{
	NewSkeleton = NULL;
	CloseWindow();
	return FReply::Handled();
}

void SAnimationRemapSkeleton::CloseWindow()
{
	if ( WidgetWindow.IsValid() )
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}
}

bool SAnimationRemapSkeleton::ShowModal(USkeleton * OldSkeleton, USkeleton * & NewSkeleton, const FText& WarningMessage, bool * bRemapReferencedAssets)
{
	TSharedPtr<class SAnimationRemapSkeleton> DialogWidget;

	TSharedPtr<SWindow> DialogWindow = SNew(SWindow)
		.Title( LOCTEXT("RemapSkeleton", "Select Skeleton") )
		.SupportsMinimize(false) .SupportsMaximize(false)
		.SizingRule( ESizingRule::Autosized );

	TSharedPtr<SBorder> DialogWrapper = 
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SAssignNew(DialogWidget, SAnimationRemapSkeleton)
			.CurrentSkeleton(OldSkeleton)
			.WidgetWindow(DialogWindow)
			.WarningMessage(WarningMessage)
			.ShowRemapOption(bRemapReferencedAssets != NULL)
		];

	DialogWindow->SetContent(DialogWrapper.ToSharedRef());

	GEditor->EditorAddModalWindow(DialogWindow.ToSharedRef());

	NewSkeleton = DialogWidget.Get()->NewSkeleton;

	if(bRemapReferencedAssets)
	{
		*bRemapReferencedAssets = DialogWidget.Get()->bRemapReferencedAssets;
	}

	return (NewSkeleton != NULL && NewSkeleton != OldSkeleton);
}

////////////////////////////////////////////////////

FDlgRemapSkeleton::FDlgRemapSkeleton( USkeleton * Skeleton )
{
	if (FSlateApplication::IsInitialized())
	{
		DialogWindow = SNew(SWindow)
			.Title( LOCTEXT("RemapSkeleton", "Select Skeleton") )
			.SupportsMinimize(false) .SupportsMaximize(false)
			.SizingRule( ESizingRule::Autosized );

		TSharedPtr<SBorder> DialogWrapper = 
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4.0f)
			[
				SAssignNew(DialogWidget, SAnimationRemapSkeleton)
				.CurrentSkeleton(Skeleton)
				.WidgetWindow(DialogWindow)
			];

		DialogWindow->SetContent(DialogWrapper.ToSharedRef());
	}
}

bool FDlgRemapSkeleton::ShowModal()
{
	GEditor->EditorAddModalWindow(DialogWindow.ToSharedRef());

	NewSkeleton = DialogWidget.Get()->NewSkeleton;

	return (NewSkeleton != NULL && NewSkeleton != DialogWidget.Get()->OldSkeleton);
}

//////////////////////////////////////////////////////////////
void SRemapFailures::Construct( const FArguments& InArgs )
{
	for ( auto RemapIt = InArgs._FailedRemaps.CreateConstIterator(); RemapIt; ++RemapIt )
	{
		FailedRemaps.Add( MakeShareable( new FString(*RemapIt) ) );
	}

	ChildSlot
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush") )
			.Padding(FMargin(4, 8, 4, 4))
			[
				SNew(SVerticalBox)

				// Title text
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock) .Text( LOCTEXT("RemapFailureTitle", "The following assets could not be Remaped.") )
				]

				// Failure list
				+SVerticalBox::Slot()
				.Padding(0, 8)
				.FillHeight(1.f)
				[
					SNew(SBorder)
					.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
					[
						SNew(SListView<TSharedPtr<FString>>)
						.ListItemsSource(&FailedRemaps)
						.SelectionMode(ESelectionMode::None)
						.OnGenerateRow(this, &SRemapFailures::MakeListViewWidget)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
					.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
					.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
					+SUniformGridPanel::Slot(0,0)
					[
						SNew(SButton) 
						.HAlign(HAlign_Center)
						.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
						.Text(LOCTEXT("RemapFailuresCloseButton", "Close"))
						.OnClicked(this, &SRemapFailures::CloseClicked)
					]
				]
			]
		];
}

void SRemapFailures::OpenRemapFailuresDialog(const TArray<FString>& InFailedRemaps)
{
	TSharedRef<SWindow> RemapWindow = SNew(SWindow)
		.Title(LOCTEXT("FailedRemapsDialog", "Failed Remaps"))
		.ClientSize(FVector2D(800,400))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SRemapFailures).FailedRemaps(InFailedRemaps)
		];

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));

	if ( MainFrameModule.GetParentWindow().IsValid() )
	{
		FSlateApplication::Get().AddWindowAsNativeChild(RemapWindow, MainFrameModule.GetParentWindow().ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(RemapWindow);
	}
}

TSharedRef<ITableRow> SRemapFailures::MakeListViewWidget(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
		[
			SNew(STextBlock) .Text( *Item.Get() )
		];
}

FReply SRemapFailures::CloseClicked()
{
	FWidgetPath WidgetPath;
	TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared(), WidgetPath);

	if ( Window.IsValid() )
	{
		Window->RequestDestroyWindow();
	}

	return FReply::Handled();
}
#undef LOCTEXT_NAMESPACE 

