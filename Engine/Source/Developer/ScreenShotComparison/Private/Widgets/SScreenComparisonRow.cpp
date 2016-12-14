// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Widgets/SScreenComparisonRow.h"
#include "Models/ScreenComparisonModel.h"
#include "Modules/ModuleManager.h"
#include "ISourceControlModule.h"
#include "ISourceControlOperation.h"
#include "SourceControlOperations.h"
#include "ISourceControlProvider.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SButton.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Interfaces/IImageWrapperModule.h"
#include "Framework/Application/SlateApplication.h"
#include "JsonObjectConverter.h"
#include "SScreenShotImagePopup.h"

#define LOCTEXT_NAMESPACE "SScreenShotBrowser"

void SScreenComparisonRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	Comparisons = InArgs._Comparisons;
	ScreenshotManager = InArgs._ScreenshotManager;
	ComparisonDirectory = InArgs._ComparisonDirectory;
	Model = InArgs._ComparisonResult;

	CachedActualImageSize = FIntPoint::NoneValue;

	switch ( Model->GetType() )
	{
		case EComparisonResultType::Added:
		{
			FString AddedFolder = ComparisonDirectory / Comparisons->IncomingPath / Model->Folder;
			IFileManager::Get().FindFilesRecursive(ExternalFiles, *AddedFolder, TEXT("*.*"), true, false);
			LoadMetadata();
			break;
		}
		case EComparisonResultType::Missing:
		{
			break;
		}
		case EComparisonResultType::Compared:
		{
			const FImageComparisonResult& ComparisonResult = Model->ComparisonResult.GetValue();
			FString IncomingImage = ComparisonDirectory / Comparisons->IncomingPath / ComparisonResult.IncomingFile;
			FString IncomingMetadata = FPaths::ChangeExtension(IncomingImage, TEXT("json"));
			LoadMetadata();

			ExternalFiles.Add(IncomingImage);
			ExternalFiles.Add(IncomingMetadata);
			break;
		}
	}

	SMultiColumnTableRow<TSharedPtr<FScreenComparisonModel>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

void SScreenComparisonRow::LoadMetadata()
{
	if ( !Model->Metadata.IsSet() )
	{
		FString IncomingMetadataFile;

		if ( Model->ComparisonResult.IsSet() )
		{
			const FImageComparisonResult& ComparisonResult = Model->ComparisonResult.GetValue();
			FString IncomingImage = ComparisonDirectory / Comparisons->IncomingPath / ComparisonResult.IncomingFile;
			IncomingMetadataFile = FPaths::ChangeExtension(IncomingImage, TEXT("json"));
		}
		else
		{
			TArray<FString> MetadataFiles;
			FString AddedFolder = ComparisonDirectory / Comparisons->IncomingPath / Model->Folder;
			IFileManager::Get().FindFilesRecursive(MetadataFiles, *AddedFolder, TEXT("*.json"), true, false);
			if ( MetadataFiles.Num() > 0 )
			{
				IncomingMetadataFile = MetadataFiles[0];
			}
		}

		if ( !IncomingMetadataFile.IsEmpty() )
		{
			FString Json;
			if ( FFileHelper::LoadFileToString(Json, *IncomingMetadataFile) )
			{
				FAutomationScreenshotMetadata Metadata;
				if ( FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Metadata, 0, 0) )
				{
					Model->Metadata = Metadata;
				}
			}
		}
	}
}

TSharedRef<SWidget> SScreenComparisonRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if ( ColumnName == "Name" )
	{
		if ( Model->Metadata.IsSet() )
		{
			return SNew(STextBlock).Text(FText::FromString(Model->Metadata->Name));
		}
		else
		{
			return SNew(STextBlock).Text(LOCTEXT("Unknown", "Unknown Test, no metadata discovered."));
		}
	}
	else if ( ColumnName == "Delta" )
	{
		if ( Model->ComparisonResult.IsSet() )
		{
			FNumberFormattingOptions Format;
			Format.MinimumFractionalDigits = 2;
			Format.MaximumFractionalDigits = 2;
			const FText GlobalDelta = FText::AsPercent(Model->ComparisonResult->GlobalDifference, &Format);
			const FText LocalDelta = FText::AsPercent(Model->ComparisonResult->MaxLocalDifference, &Format);

			const FText Differences = FText::Format(LOCTEXT("LocalvGlobalDelta", "{0} | {1}"), LocalDelta, GlobalDelta);
			return SNew(STextBlock).Text(Differences);
		}

		return SNew(STextBlock).Text(FText::AsPercent(1.0));
	}
	else if ( ColumnName == "Preview" )
	{
		switch ( Model->GetType() )
		{
			case EComparisonResultType::Added:
			{
				return BuildAddedView();
			}
			case EComparisonResultType::Missing:
			{
				return BuildMissingView();
			}
			case EComparisonResultType::Compared:
			{
				return
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						BuildComparisonPreview()
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.IsEnabled(this, &SScreenComparisonRow::CanReplace)
							.Text(LOCTEXT("Replace", "Replace"))
							.OnClicked(this, &SScreenComparisonRow::Replace)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(10, 0, 0, 0)
						[
							SNew(SButton)
							.IsEnabled(this, &SScreenComparisonRow::CanAddAsAlternative)
							.Text(LOCTEXT("AddAlternative", "Add As Alternative"))
							.OnClicked(this, &SScreenComparisonRow::AddAlternative)
						]
					];
			}
		}
	}

	return SNullWidget::NullWidget;
}

bool SScreenComparisonRow::CanUseSourceControl() const
{
	return ISourceControlModule::Get().IsEnabled();
}

TSharedRef<SWidget> SScreenComparisonRow::BuildMissingView()
{
	return
		SNew(SHorizontalBox)
		
		/*	+SHorizontalBox::Slot()
			[
				SNew(SButton)
				.IsEnabled(this, &SScreenComparisonRow::CanUseSourceControl)
				.Text(LOCTEXT("RemoveOld", "Remove Old"))
				.OnClicked(this, &SScreenComparisonRow::RemoveOld)
			]*/;
}

TSharedRef<SWidget> SScreenComparisonRow::BuildAddedView()
{
	TArray<FString> Images;

	FString AddedFolder = ComparisonDirectory / Comparisons->IncomingPath / Model->Folder;
	IFileManager::Get().FindFilesRecursive(ExternalFiles, *AddedFolder, TEXT("*.png"), true, false);

	//TODO Automation this is no good, we need to clear out the incoming directory beforehand.
	if ( Images.Num() > 0 )
	{
		UnapprovedBrush = LoadScreenshot(Images[0]);
	}

	return
		SNew(SVerticalBox)
		
		+ SVerticalBox::Slot()
		[
			SNew(SBox)
			.MaxDesiredHeight(100)
			.HAlign(HAlign_Left)
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFit)
				[
					SNew(SHorizontalBox)
		
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4.0f, 4.0f)
					[
						SNew(SImage)
						.Image(UnapprovedBrush.Get())
						.OnMouseButtonDown(this, &SScreenComparisonRow::OnImageClicked, UnapprovedBrush)
					]
				]
			]
		]
		
		+ SVerticalBox::Slot()
		[
			SNew(SButton)
			.IsEnabled(this, &SScreenComparisonRow::CanUseSourceControl)
			.Text(LOCTEXT("AddNew", "Add New!"))
			.OnClicked(this, &SScreenComparisonRow::AddNew)
		];
}

TSharedRef<SWidget> SScreenComparisonRow::BuildComparisonPreview()
{
	const FImageComparisonResult& ComparisonResult = Model->ComparisonResult.GetValue();

	FString ApprovedFile = ComparisonDirectory / Comparisons->ApprovedPath / ComparisonResult.ApprovedFile;
	FString IncomingFile = ComparisonDirectory / Comparisons->IncomingPath / ComparisonResult.IncomingFile;
	FString DeltaFile = ComparisonDirectory / Comparisons->DeltaPath / ComparisonResult.ComparisonFile;

	ApprovedBrush = LoadScreenshot(ApprovedFile);
	UnapprovedBrush = LoadScreenshot(IncomingFile);
	ComparisonBrush = LoadScreenshot(DeltaFile);

	// Create the screen shot data widget.
	return 
		SNew(SBox)
		.MaxDesiredHeight(100)
		.HAlign(HAlign_Left)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			[
				SNew(SHorizontalBox)
		
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 4.0f)
				[
					SNew(SImage)
					.Image(ApprovedBrush.Get())
					.OnMouseButtonDown(this, &SScreenComparisonRow::OnImageClicked, ApprovedBrush)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 4.0f)
				[
					SNew(SImage)
					.Image(ComparisonBrush.Get())
					.OnMouseButtonDown(this, &SScreenComparisonRow::OnImageClicked, ComparisonBrush)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 4.0f)
				[
					SNew(SImage)
					.Image(UnapprovedBrush.Get())
					.OnMouseButtonDown(this, &SScreenComparisonRow::OnImageClicked, UnapprovedBrush)
				]
			]
		];
}

void SScreenComparisonRow::GetStatus()
{
	//if ( SourceControlStates.Num() == 0 )
	//{
	//	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	//	SourceControlProvider.GetState(SourceControlFiles, SourceControlStates, EStateCacheUsage::Use);
	//}
}

bool SScreenComparisonRow::CanAddNew() const
{
	return CanUseSourceControl();
}

FReply SScreenComparisonRow::AddNew()
{
	// Copy files to the approved
	const FString& LocalApprovedFolder = ScreenshotManager->GetLocalApprovedFolder();
	const FString ImportIncomingRoot = ComparisonDirectory / Comparisons->IncomingPath / TEXT("");

	TArray<FString> SourceControlFiles;

	for ( const FString& File : ExternalFiles )
	{
		FString RelativeFile = File;
		FPaths::MakePathRelativeTo(RelativeFile, *ImportIncomingRoot);
		
		FString DestFilePath = LocalApprovedFolder / RelativeFile;
		IFileManager::Get().Copy(*DestFilePath, *File, true, true);

		SourceControlFiles.Add(DestFilePath);
	}

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), SourceControlFiles) == ECommandResult::Failed )
	{
		//TODO Error
	}

	// Invalidate Status

	return FReply::Handled();
}

bool SScreenComparisonRow::CanReplace() const
{
	return CanUseSourceControl();
}

FReply SScreenComparisonRow::RemoveExistingApproved()
{
	TArray<FString> FilesToRemove;

	FString PlatformFolder = ComparisonDirectory / Comparisons->ApprovedPath / FPaths::GetPath(Model->ComparisonResult->ApprovedFile);
	IFileManager::Get().FindFilesRecursive(FilesToRemove, *PlatformFolder, TEXT("*.*"), true, false);

	// Copy files to the approved
	const FString& LocalApprovedFolder = ScreenshotManager->GetLocalApprovedFolder();
	const FString ImportApprovedRoot = ComparisonDirectory / Comparisons->ApprovedPath / TEXT("");

	TArray<FString> SourceControlFiles;

	for ( const FString& File : FilesToRemove )
	{
		FString RelativeFile = File;
		FPaths::MakePathRelativeTo(RelativeFile, *ImportApprovedRoot);

		FString DestFilePath = LocalApprovedFolder / RelativeFile;
		SourceControlFiles.Add(DestFilePath);

		IFileManager::Get().Delete(*DestFilePath, false, true, false);
	}

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FDelete>(), SourceControlFiles) == ECommandResult::Failed )
	{
		//TODO Error
	}

	// Invalidate Status

	return FReply::Handled();
}

FReply SScreenComparisonRow::Replace()
{
	// Delete all the existing files in this area
	RemoveExistingApproved();

	// Copy files to the approved
	const FString& LocalApprovedFolder = ScreenshotManager->GetLocalApprovedFolder();
	const FString ImportIncomingRoot = ComparisonDirectory / Comparisons->IncomingPath / TEXT("");

	TArray<FString> SourceControlFiles;

	for ( const FString& File : ExternalFiles )
	{
		FString RelativeFile = File;
		FPaths::MakePathRelativeTo(RelativeFile, *ImportIncomingRoot);

		FString DestFilePath = LocalApprovedFolder / RelativeFile;
		
		SourceControlFiles.Add(DestFilePath);
	}

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FRevert>(), SourceControlFiles) == ECommandResult::Failed )
	{
		//TODO Error
	}

	for ( const FString& File : ExternalFiles )
	{
		FString RelativeFile = File;
		FPaths::MakePathRelativeTo(RelativeFile, *ImportIncomingRoot);

		FString DestFilePath = LocalApprovedFolder / RelativeFile;
		IFileManager::Get().Copy(*DestFilePath, *File, true, true);
	}

	if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), SourceControlFiles) == ECommandResult::Failed )
	{
		//TODO Error
	}
	if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), SourceControlFiles) == ECommandResult::Failed )
	{
		//TODO Error
	}

	return FReply::Handled();
}

bool SScreenComparisonRow::CanAddAsAlternative() const
{
	return CanUseSourceControl() && (Model->ComparisonResult->IncomingFile != Model->ComparisonResult->ApprovedFile);
}

FReply SScreenComparisonRow::AddAlternative()
{
	// Copy files to the approved
	const FString& LocalApprovedFolder = ScreenshotManager->GetLocalApprovedFolder();
	const FString ImportIncomingRoot = ComparisonDirectory / Comparisons->IncomingPath / TEXT("");

	TArray<FString> SourceControlFiles;

	for ( const FString& File : ExternalFiles )
	{
		FString RelativeFile = File;
		FPaths::MakePathRelativeTo(RelativeFile, *ImportIncomingRoot);

		FString DestFilePath = LocalApprovedFolder / RelativeFile;

		SourceControlFiles.Add(DestFilePath);
	}

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FRevert>(), SourceControlFiles) == ECommandResult::Failed )
	{
		//TODO Error
	}

	for ( const FString& File : ExternalFiles )
	{
		FString RelativeFile = File;
		FPaths::MakePathRelativeTo(RelativeFile, *ImportIncomingRoot);

		FString DestFilePath = LocalApprovedFolder / RelativeFile;
		IFileManager::Get().Copy(*DestFilePath, *File, true, true);
	}

	if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), SourceControlFiles) == ECommandResult::Failed )
	{
		//TODO Error
	}
	if ( SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), SourceControlFiles) == ECommandResult::Failed )
	{
		//TODO Error
	}

	return FReply::Handled();
}

//TSharedRef<SWidget> SScreenComparisonRow::BuildSourceControl()
//{
//	if ( ISourceControlModule::Get().IsEnabled() )
//	{
//		// note: calling QueueStatusUpdate often does not spam status updates as an internal timer prevents this
//		ISourceControlModule::Get().QueueStatusUpdate(CachedConfigFileName);
//
//		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
//	}
//}

TSharedPtr<FSlateDynamicImageBrush> SScreenComparisonRow::LoadScreenshot(FString ImagePath)
{
	TArray<uint8> RawFileData;
	if ( FFileHelper::LoadFileToArray(RawFileData, *ImagePath) )
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

		if ( ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()) )
		{
			const TArray<uint8>* RawData = NULL;
			if ( ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData) )
			{
				if ( FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(*ImagePath, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), *RawData) )
				{
					return MakeShareable(new FSlateDynamicImageBrush(*ImagePath, FVector2D(ImageWrapper->GetWidth(), ImageWrapper->GetHeight())));
				}
			}
		}
	}

	return nullptr;
}

FReply SScreenComparisonRow::OnImageClicked(const FGeometry& InGeometry, const FPointerEvent& InEvent, TSharedPtr<FSlateDynamicImageBrush> Image)
{
	TSharedRef<SScreenShotImagePopup> PopupImage =
		SNew(SScreenShotImagePopup)
		.ImageBrush(Image)
		.ImageSize(Image->ImageSize.IntPoint());

	auto ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared()).ToSharedRef();

	// Center ourselves in the parent window
	auto PopupWindow = SNew(SWindow)
		.IsPopupWindow(false)
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(Image->ImageSize)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		.FocusWhenFirstShown(true)
		.ActivateWhenFirstShown(true)
		.Content()
		[
			PopupImage
		];

	FSlateApplication::Get().AddWindowAsNativeChild(PopupWindow, ParentWindow, true);

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE