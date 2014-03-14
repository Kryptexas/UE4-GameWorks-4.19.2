// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Slate.h"
#include "HighResScreenshot.h"

class SHighResScreenshotDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SHighResScreenshotDialog ){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	SHighResScreenshotDialog();

	virtual ~SHighResScreenshotDialog()
	{
		Window.Reset();
	}

	void SetWindow( TSharedPtr<SWindow> InWindow )
	{
		Window = InWindow;
	}

	void SetWorld( UWorld* InWorld )
	{
		World = InWorld;
	}

	void SetCaptureRegionWidget(TSharedPtr<class SCaptureRegionWidget> InCaptureRegionWidget)
	{
		CaptureRegionWidget = InCaptureRegionWidget;
	}

	void SetCaptureRegion(const FIntRect& InCaptureRegion)
	{
		Config.UnscaledCaptureRegion = InCaptureRegion;
		Config.TargetViewport->Invalidate();
	}

	FHighResScreenshotConfig& GetConfig()
	{
		return Config;
	}

	TSharedPtr<class SCaptureRegionWidget> GetCaptureRegionWidget()
	{
		return CaptureRegionWidget;
	}

	static TWeakPtr<class SWindow> OpenDialog(UWorld* InWorld, FViewport* InViewport, TSharedPtr<SCaptureRegionWidget> InCaptureRegionWidget = TSharedPtr<class SCaptureRegionWidget>());
	static bool IsOpen();

	static TWeakPtr<SHighResScreenshotDialog> GetCurrentDialog();

private:

	FReply OnCaptureClicked();
	FReply OnSelectCaptureRegionClicked();
	FReply OnSelectCaptureCancelRegionClicked();
	FReply OnSelectCaptureAcceptRegionClicked();
	FReply OnSetFullViewportCaptureRegionClicked();

	void OnResolutionMultiplierChanged( float NewValue, ETextCommit::Type CommitInfo )
	{
		NewValue = FMath::Clamp(NewValue, FHighResScreenshotConfig::MinResolutionMultipler, FHighResScreenshotConfig::MaxResolutionMultipler);
		Config.ResolutionMultiplier = NewValue;
		Config.ResolutionMultiplierScale = (NewValue - FHighResScreenshotConfig::MinResolutionMultipler) / (FHighResScreenshotConfig::MaxResolutionMultipler - FHighResScreenshotConfig::MinResolutionMultipler);
	}

	void OnResolutionMultiplierSliderChanged( float NewValue )
	{
		Config.ResolutionMultiplierScale = NewValue;
		Config.ResolutionMultiplier = FMath::Round(FMath::Lerp(FHighResScreenshotConfig::MinResolutionMultipler, FHighResScreenshotConfig::MaxResolutionMultipler, NewValue));
	}

	void OnMaskEnabledChanged( ESlateCheckBoxState::Type NewValue )
	{
		Config.bMaskEnabled = (NewValue == ESlateCheckBoxState::Checked);
		Config.TargetViewport->GetClient()->GetEngineShowFlags()->HighResScreenshotMask = Config.bMaskEnabled;
		Config.TargetViewport->Invalidate();
	}

	void OnBufferVisualizationDumpEnabledChanged( ESlateCheckBoxState::Type NewValue )
	{
		Config.bDumpBufferVisualizationTargets = (NewValue == ESlateCheckBoxState::Checked);
	}

	EVisibility GetSpecifyCaptureRegionVisibility() const
	{
		return bCaptureRegionControlsVisible ? EVisibility::Hidden : EVisibility::Visible;
	}

	EVisibility GetCaptureRegionControlsVisibility() const
	{
		return bCaptureRegionControlsVisible ? EVisibility::Visible : EVisibility::Hidden;
	}

	TOptional<float> GetResolutionMultiplier() const
	{
		return TOptional<float>(Config.ResolutionMultiplier);
	}

	float GetResolutionMultiplierSlider() const
	{
		return Config.ResolutionMultiplierScale;
	}

	ESlateCheckBoxState::Type GetMaskEnabled() const
	{
		return Config.bMaskEnabled ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	ESlateCheckBoxState::Type GetBufferVisualizationDumpEnabled() const
	{
		return Config.bDumpBufferVisualizationTargets ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	bool IsCaptureRegionEditingAvailable() const
	{
		return CaptureRegionWidget.IsValid();
	}
	
	static void WindowClosedHandler(const TSharedRef<SWindow>& InWindow);

	TSharedPtr<SWindow> Window;
	TSharedPtr<class SCaptureRegionWidget> CaptureRegionWidget;
	TSharedPtr<SButton> CaptureRegionButton;
	TSharedPtr<SHorizontalBox> RegionCaptureActiveControlRoot;
	UWorld* World;
	FHighResScreenshotConfig& Config;
	bool bCaptureRegionControlsVisible;

	static TWeakPtr<SWindow> CurrentWindow;
	static TWeakPtr<SHighResScreenshotDialog> CurrentDialog;
	static bool bMaskVisualizationWasEnabled;
};