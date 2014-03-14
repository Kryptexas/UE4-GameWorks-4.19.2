// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "LevelEditor.h"
#include "HighresScreenshotUI.h"
#include "SCaptureRegionWidget.h"
#include "HighResScreenshot.h"

TWeakPtr<class SWindow> SHighResScreenshotDialog::CurrentWindow = NULL;
TWeakPtr<class SHighResScreenshotDialog> SHighResScreenshotDialog::CurrentDialog = NULL;
bool SHighResScreenshotDialog::bMaskVisualizationWasEnabled = false;

SHighResScreenshotDialog::SHighResScreenshotDialog()
: Config(GetHighResScreenshotConfig())
{
}

void SHighResScreenshotDialog::Construct( const FArguments& InArgs )
{
	this->ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.Padding(5)
					[
						SNew(SSplitter)
						.Orientation(Orient_Horizontal)
						+SSplitter::Slot()
						.Value(1)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew( STextBlock )
								.Text( NSLOCTEXT("HighResScreenshot", "ScreenshotSizeMultiplier", "Screenshot Size Multiplier") )
							]
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew( STextBlock )
								.Text( NSLOCTEXT("HighResScreenshot", "IncludeBufferVisTargets", "Include Buffer Visualization Targets") )
							]
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew( STextBlock )
								.Text( NSLOCTEXT("HighResScreenshot", "UseCustomDepth", "Use custom depth as mask") )
							]
						]
						+SSplitter::Slot()
						.Value(1)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew (SHorizontalBox)
								+SHorizontalBox::Slot()
								.FillWidth(1)
								[
									SNew( SNumericEntryBox<float> )
									.Value(this, &SHighResScreenshotDialog::GetResolutionMultiplier)
									.OnValueCommitted(this, &SHighResScreenshotDialog::OnResolutionMultiplierChanged)
								]
								+SHorizontalBox::Slot()
								.HAlign(HAlign_Fill)
								.Padding(5,0,0,0)
								.FillWidth(3)
								[
									SNew( SSlider )
									.Value(this, &SHighResScreenshotDialog::GetResolutionMultiplierSlider)
									.OnValueChanged(this, &SHighResScreenshotDialog::OnResolutionMultiplierSliderChanged)
								]
							]
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew( SCheckBox )
								.OnCheckStateChanged(this, &SHighResScreenshotDialog::OnBufferVisualizationDumpEnabledChanged)
								.IsChecked(this, &SHighResScreenshotDialog::GetBufferVisualizationDumpEnabled)
							]
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew( SCheckBox )
								.OnCheckStateChanged(this, &SHighResScreenshotDialog::OnMaskEnabledChanged)
								.IsChecked(this, &SHighResScreenshotDialog::GetMaskEnabled)
							]
						]
					]
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("HighresScreenshot.WarningStrip"))
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SBorder )
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew( STextBlock )
						.Text( NSLOCTEXT("HighResScreenshot", "CaptureWarningText", "Due to the high system requirements of a high resolution screenshot, very large multipliers might cause the graphics driver to become unresponsive and possibly crash. In these circumstances, please try using a lower multiplier") )
						.AutoWrapText(true)
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("HighresScreenshot.WarningStrip"))
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SGridPanel)
						+SGridPanel::Slot(0, 0)
						[
							SAssignNew(CaptureRegionButton, SButton)
							.IsEnabled(this, &SHighResScreenshotDialog::IsCaptureRegionEditingAvailable)
							.Visibility(this, &SHighResScreenshotDialog::GetSpecifyCaptureRegionVisibility)
							.ToolTipText(NSLOCTEXT("HighResScreenshot", "ScreenshotSpecifyCaptureRectangleTooltip", "Specify the region which will be captured by the screenshot"))
							.OnClicked(this, &SHighResScreenshotDialog::OnSelectCaptureRegionClicked)
							[
								SNew(SImage)
								.Image(FEditorStyle::GetBrush("HighresScreenshot.SpecifyCaptureRectangle"))
							]
						]
						+SGridPanel::Slot(0, 0)
						[
							SNew( SButton )
							.Visibility(this, &SHighResScreenshotDialog::GetCaptureRegionControlsVisibility)
							.ToolTipText( NSLOCTEXT("HighResScreenshot", "ScreenshotAcceptCaptureRegionTooltip", "Accept any changes made to the capture region") )
							.OnClicked( this, &SHighResScreenshotDialog::OnSelectCaptureAcceptRegionClicked )
							[
								SNew(SImage)
								.Image(FEditorStyle::GetBrush("HighresScreenshot.AcceptCaptureRegion"))
							]
						]
					]
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.AutoWidth()
					[
						SNew( SButton )
						.ToolTipText( NSLOCTEXT("HighResScreenshot", "ScreenshotDiscardCaptureRegionTooltip", "Discard any changes made to the capture region") )
						.Visibility(this, &SHighResScreenshotDialog::GetCaptureRegionControlsVisibility)
						.OnClicked( this, &SHighResScreenshotDialog::OnSelectCaptureCancelRegionClicked )
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("HighresScreenshot.DiscardCaptureRegion"))
						]
					]
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.AutoWidth()
					[
						SNew( SButton )
						.ToolTipText( NSLOCTEXT("HighResScreenshot", "ScreenshotFullViewportCaptureRegionTooltip", "Set the capture rectangle to the whole viewport") )
						.Visibility(this, &SHighResScreenshotDialog::GetCaptureRegionControlsVisibility)
						.OnClicked( this, &SHighResScreenshotDialog::OnSetFullViewportCaptureRegionClicked )
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("HighresScreenshot.FullViewportCaptureRegion"))
						]
					]
					+SHorizontalBox::Slot()
						// for padding
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.AutoWidth()
					[
						SNew( SButton )
						.ToolTipText( NSLOCTEXT("HighResScreenshot", "ScreenshotCaptureTooltop", "Take a screenshot") )
						.OnClicked( this, &SHighResScreenshotDialog::OnCaptureClicked )
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("HighresScreenshot.Capture"))
						]
					]
				]
			]
		];

	bCaptureRegionControlsVisible = false;
}

void SHighResScreenshotDialog::WindowClosedHandler(const TSharedRef<SWindow>& InWindow)
{
	FHighResScreenshotConfig& Config = GetHighResScreenshotConfig();
	if (CurrentDialog.IsValid())
	{
		TSharedPtr<SCaptureRegionWidget> CaptureRegionWidget = CurrentDialog.Pin()->GetCaptureRegionWidget();
		if (CaptureRegionWidget.IsValid())
		{
			CaptureRegionWidget->Deactivate(false);
		}

		// Restore mask visualization state from before window was opened
		if (Config.TargetViewport &&
			Config.TargetViewport->GetClient() &&
			Config.TargetViewport->GetClient()->GetEngineShowFlags())
		{
			Config.TargetViewport->GetClient()->GetEngineShowFlags()->HighResScreenshotMask = bMaskVisualizationWasEnabled;
		}
	}

	// Cleanup the config after each usage as it is a static and we don't want it to keep pointers or settings around between runs.
	Config.bDisplayCaptureRegion = false;
	Config.ChangeViewport(NULL);
	CurrentWindow.Reset();
	CurrentDialog.Reset();
}

bool SHighResScreenshotDialog::IsOpen()
{
	return CurrentWindow.IsValid();
}

TWeakPtr<class SWindow> SHighResScreenshotDialog::OpenDialog(UWorld* InWorld, FViewport* InViewport, TSharedPtr<SCaptureRegionWidget> InCaptureRegionWidget)
{
	// If there is already a UI window open and pointing at a different viewport, close it
	FHighResScreenshotConfig& Config = GetHighResScreenshotConfig();
	if (IsOpen() && InViewport != Config.TargetViewport)
	{
		if (CurrentWindow.IsValid())
		{
			CurrentDialog.Reset();
			CurrentWindow.Pin()->RequestDestroyWindow();
			CurrentWindow.Reset();
		}
	}

	if (!CurrentWindow.IsValid())
	{
		if (Config.TargetViewport != InViewport)
		{
			Config.ChangeViewport(InViewport);
		}

		TSharedRef<SHighResScreenshotDialog> Dialog = SNew(SHighResScreenshotDialog);
		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title( NSLOCTEXT("HighResScreenshot", "HighResolutionScreenshot", "High Resolution Screenshot") )
			.ClientSize(FVector2D(484,231))
			.SupportsMinimize(false)
			.SupportsMaximize(false)
			.FocusWhenFirstShown(true)
			[
				Dialog
			];

		Dialog->SetWorld(InWorld);
		Dialog->SetWindow(Window);
		Dialog->SetCaptureRegionWidget(InCaptureRegionWidget);
		FSlateApplication::Get().AddWindow(Window);
		Window->BringToFront();
		Window->SetOnWindowClosed(FOnWindowClosed::CreateStatic(&WindowClosedHandler));
		CurrentWindow = TWeakPtr<SWindow>(Window);
		CurrentDialog = TWeakPtr<SHighResScreenshotDialog>(Dialog);
		
		Config.bDisplayCaptureRegion = true;

		// Enable mask visualization if the mask is enabled
		bMaskVisualizationWasEnabled = Config.TargetViewport->GetClient()->GetEngineShowFlags()->HighResScreenshotMask;
		Config.TargetViewport->GetClient()->GetEngineShowFlags()->HighResScreenshotMask = Config.bMaskEnabled;

		return CurrentWindow;
	}
	else
	{
		CurrentWindow.Pin()->BringToFront();
	}

	return TWeakPtr<class SWindow>();
}

FReply SHighResScreenshotDialog::OnSelectCaptureRegionClicked()
{
	// Only enable the capture region widget if the owning viewport gave us one
	if (CaptureRegionWidget.IsValid())
	{
		CaptureRegionWidget->Activate(Config.UnscaledCaptureRegion.Width() == -1 || Config.UnscaledCaptureRegion.Height() == -1);
		bCaptureRegionControlsVisible = true;
	}
	return FReply::Handled();
}

FReply SHighResScreenshotDialog::OnCaptureClicked()
{
	GScreenshotResolutionX = Config.TargetViewport->GetSizeXY().X * Config.ResolutionMultiplier;
	GScreenshotResolutionY = Config.TargetViewport->GetSizeXY().Y * Config.ResolutionMultiplier;
	FIntRect ScaledCaptureRegion = Config.UnscaledCaptureRegion;

	if (Config.UnscaledCaptureRegion.Width() == -1 || Config.UnscaledCaptureRegion.Height() == -1)
	{
		ScaledCaptureRegion = FIntRect(0, 0, Config.TargetViewport->GetSizeXY().X, Config.TargetViewport->GetSizeXY().Y);
	}

	ScaledCaptureRegion.Clip(FIntRect(FIntPoint::ZeroValue, Config.TargetViewport->GetSizeXY()));
	ScaledCaptureRegion *= Config.ResolutionMultiplier;
	Config.CaptureRegion = ScaledCaptureRegion;

	// Trigger the screenshot on the owning viewport
	Config.TargetViewport->TakeHighResScreenShot();
	Config.ChangeViewport(NULL);

	return FReply::Handled();
}

FReply SHighResScreenshotDialog::OnSelectCaptureCancelRegionClicked()
{
	if (CaptureRegionWidget.IsValid())
	{
		CaptureRegionWidget->Deactivate(false);
	}

	// Hide the Cancel/Accept buttons, show the Edit button
	bCaptureRegionControlsVisible = false;
	return FReply::Handled();
}

FReply SHighResScreenshotDialog::OnSelectCaptureAcceptRegionClicked()
{
	if (CaptureRegionWidget.IsValid())
	{
		CaptureRegionWidget->Deactivate(true);
	}

	// Hide the Cancel/Accept buttons, show the Edit button
	bCaptureRegionControlsVisible = false;
	return FReply::Handled();
}

FReply SHighResScreenshotDialog::OnSetFullViewportCaptureRegionClicked()
{
	Config.UnscaledCaptureRegion = FIntRect(0, 0, -1, -1);
	Config.TargetViewport->Invalidate();
	CaptureRegionWidget->Reset();
	return FReply::Handled();
}

TWeakPtr<SHighResScreenshotDialog> SHighResScreenshotDialog::GetCurrentDialog()
{
	return CurrentDialog;
}