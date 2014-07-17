// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Dialogs/SOpenLevelDialog.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/MainFrame/Public/MainFrame.h"

#define LOCTEXT_NAMESPACE "SOpenLevelDialog"

void SOpenLevelDialog::CreateAndShowOpenLevelDialog(const FEditorFileUtils::FOnLevelsChosen& InOnLevelsChosen, bool bAllowMultipleSelection)
{
	const FVector2D WindowSize(1152.0f, 648.0f);

	TSharedRef<SWindow> OpenLevelWindow =
		SNew(SWindow)
		.Title(LOCTEXT("WindowHeader", "Open Level"))
		.ClientSize(WindowSize);

	OpenLevelWindow->SetContent( SNew(SOpenLevelDialog, InOnLevelsChosen, bAllowMultipleSelection) );

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	if ( MainFrameParentWindow.IsValid() )
	{
		FSlateApplication::Get().AddWindowAsNativeChild(OpenLevelWindow, MainFrameParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(OpenLevelWindow);
	}
}

SOpenLevelDialog::SOpenLevelDialog()
	: bConfiguredToAllowMultipleSelection(false)
{
}

void SOpenLevelDialog::Construct(const FArguments& InArgs, const FEditorFileUtils::FOnLevelsChosen& InOnLevelsChosen, bool bAllowMultipleSelection)
{
	OnLevelsChosen = InOnLevelsChosen;
	bConfiguredToAllowMultipleSelection = bAllowMultipleSelection;

	const FString DefaultPath = TEXT("/Game");

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = DefaultPath;
	PathPickerConfig.bFocusSearchBoxWhenOpened = false;
	PathPickerConfig.bAllowContextMenu = false;
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateSP(this, &SOpenLevelDialog::HandlePathSelected);
	TSharedPtr<SWidget> PathPicker = ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig);

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	AssetPickerConfig.Filter.PackagePaths.Add(FName(*DefaultPath));
	AssetPickerConfig.bAllowDragging = false;
	AssetPickerConfig.SelectionMode = bConfiguredToAllowMultipleSelection ? ESelectionMode::Multi : ESelectionMode::Single;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	AssetPickerConfig.ThumbnailScale = 0;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SOpenLevelDialog::OnAssetSelected);
	AssetPickerConfig.OnAssetsActivated = FOnAssetsActivated::CreateSP(this, &SOpenLevelDialog::OpenLevelFromAssetPicker);
	AssetPickerConfig.InitialAssetSelection = FAssetData(GEditor->GetEditorWorldContext().World());
	AssetPickerConfig.SetFilterDelegates.Add(&SetFilterDelegate);
	AssetPickerConfig.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	TSharedPtr<SWidget> AssetPicker = ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew(SSplitter)
		
			+SSplitter::Slot()
			.Value(0.25f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					PathPicker.ToSharedRef()
				]
			]

			+SSplitter::Slot()
			.Value(0.75f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					AssetPicker.ToSharedRef()
				]
			]
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(8, 6, 8, 4)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4, 2)
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenButton", "Open"))
					.IsEnabled(this, &SOpenLevelDialog::IsOpenButtonEnabled)
					.OnClicked(this, &SOpenLevelDialog::OnOpenClicked)
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4, 2)
				[
					SNew(SButton)
					.Text(LOCTEXT("CancelButton", "Cancel"))
					.OnClicked(this, &SOpenLevelDialog::OnCancelClicked)
				]
			]
		]
	];
}

void SOpenLevelDialog::HandlePathSelected(const FString& NewPath)
{
	FARFilter NewFilter;

	NewFilter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	NewFilter.PackagePaths.Add(FName(*NewPath));

	SetFilterDelegate.ExecuteIfBound(NewFilter);
}

bool SOpenLevelDialog::IsOpenButtonEnabled() const
{
	return LastSelectedAssets.Num() > 0;
}

FReply SOpenLevelDialog::OnOpenClicked()
{
	TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();
	if ( SelectedAssets.Num() > 0 )
	{
		OnLevelsChosen.ExecuteIfBound(SelectedAssets);
		CloseDialog();
	}
	
	return FReply::Handled();
}

FReply SOpenLevelDialog::OnCancelClicked()
{
	CloseDialog();

	return FReply::Handled();
}

void SOpenLevelDialog::OnAssetSelected(const FAssetData& AssetData)
{
	LastSelectedAssets = GetCurrentSelectionDelegate.Execute();
}

void SOpenLevelDialog::OpenLevelFromAssetPicker(const TArray<FAssetData>& SelectedAssets, EAssetTypeActivationMethod::Type ActivationType)
{
	const bool bCorrectActivationMethod = (ActivationType == EAssetTypeActivationMethod::DoubleClicked || ActivationType == EAssetTypeActivationMethod::Opened);
	if (SelectedAssets.Num() > 0 && bCorrectActivationMethod)
	{
		OnLevelsChosen.ExecuteIfBound(SelectedAssets);
		CloseDialog();
	}
}

void SOpenLevelDialog::CloseDialog()
{
	FWidgetPath WidgetPath;
	TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow(AsShared(), WidgetPath);

	if (ContainingWindow.IsValid())
	{
		ContainingWindow->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
