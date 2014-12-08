// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "Slate/SceneViewport.h"
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

	void SetCaptureRegionWidget(TSharedPtr<class SCaptureRegionWidget> InCaptureRegionWidget)
	{
		CaptureRegionWidget = InCaptureRegionWidget;
	}

	void SetCaptureRegion(const FIntRect& InCaptureRegion)
	{
		Config.UnscaledCaptureRegion = InCaptureRegion;
		auto ConfigViewport = Config.TargetViewport.Pin();
		if (ConfigViewport.IsValid())
		{
			ConfigViewport->Invalidate();
		}
	}

	FHighResScreenshotConfig& GetConfig()
	{
		return Config;
	}

	TSharedPtr<class SCaptureRegionWidget> GetCaptureRegionWidget()
	{
		return CaptureRegionWidget;
	}

	static TWeakPtr<class SWindow> OpenDialog(const TSharedPtr<FSceneViewport>& InViewport, TSharedPtr<SCaptureRegionWidget> InCaptureRegionWidget = TSharedPtr<class SCaptureRegionWidget>());

private:

	FReply OnCaptureClicked();
	FReply OnSelectCaptureRegionClicked();
	FReply OnSelectCaptureCancelRegionClicked();
	FReply OnSelectCaptureAcceptRegionClicked();
	FReply OnSetFullViewportCaptureRegionClicked();
	FReply OnSetCameraSafeAreaCaptureRegionClicked();

	bool IsSetCameraSafeAreaCaptureRegionEnabled() const;

	void OnResolutionMultiplierChanged( float NewValue, ETextCommit::Type CommitInfo )
	{
		NewValue = FMath::Clamp(NewValue, FHighResScreenshotConfig::MinResolutionMultipler, FHighResScreenshotConfig::MaxResolutionMultipler);
		Config.ResolutionMultiplier = NewValue;
		Config.ResolutionMultiplierScale = (NewValue - FHighResScreenshotConfig::MinResolutionMultipler) / (FHighResScreenshotConfig::MaxResolutionMultipler - FHighResScreenshotConfig::MinResolutionMultipler);
	}

	void OnResolutionMultiplierSliderChanged( float NewValue )
	{
		Config.ResolutionMultiplierScale = NewValue;
		Config.ResolutionMultiplier = FMath::RoundToFloat(FMath::Lerp(FHighResScreenshotConfig::MinResolutionMultipler, FHighResScreenshotConfig::MaxResolutionMultipler, NewValue));
	}

	void OnMaskEnabledChanged( ESlateCheckBoxState::Type NewValue )
	{
		Config.bMaskEnabled = (NewValue == ESlateCheckBoxState::Checked);
		auto ConfigViewport = Config.TargetViewport.Pin();
		if (ConfigViewport.IsValid())
		{
			ConfigViewport->GetClient()->GetEngineShowFlags()->HighResScreenshotMask = Config.bMaskEnabled;
			ConfigViewport->Invalidate();
		}
	}

	void OnHDREnabledChanged(ESlateCheckBoxState::Type NewValue)
	{
		Config.SetHDRCapture(NewValue == ESlateCheckBoxState::Checked);
		auto ConfigViewport = Config.TargetViewport.Pin();
		if (ConfigViewport.IsValid())
		{
			ConfigViewport->Invalidate();
		}
	}

	void OnBufferVisualizationDumpEnabledChanged(ESlateCheckBoxState::Type NewValue)
	{
		bool bEnabled = (NewValue == ESlateCheckBoxState::Checked);
		Config.bDumpBufferVisualizationTargets = bEnabled;
		SetHDRUIEnableState(bEnabled);
	}

	EVisibility GetSpecifyCaptureRegionVisibility() const
	{
		return bCaptureRegionControlsVisible ? EVisibility::Hidden : EVisibility::Visible;
	}

	EVisibility GetCaptureRegionControlsVisibility() const
	{
		return bCaptureRegionControlsVisible ? EVisibility::Visible : EVisibility::Hidden;
	}

	void SetCaptureRegionControlsVisibility(bool bVisible)
	{
		bCaptureRegionControlsVisible = bVisible;
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

	ESlateCheckBoxState::Type GetHDRCheckboxUIState() const
	{
		return Config.bCaptureHDR ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	ESlateCheckBoxState::Type GetBufferVisualizationDumpEnabled() const
	{
		return Config.bDumpBufferVisualizationTargets ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	bool IsCaptureRegionEditingAvailable() const
	{
		return CaptureRegionWidget.IsValid();
	}
	
	void SetHDRUIEnableState(bool bEnable)
	{
		HDRCheckBox->SetEnabled(bEnable);
		HDRLabel->SetEnabled(bEnable);
	}

	static void WindowClosedHandler(const TSharedRef<SWindow>& InWindow);

	static void ResetViewport();

	TSharedPtr<SWindow> Window;
	TSharedPtr<class SCaptureRegionWidget> CaptureRegionWidget;
	TSharedPtr<SButton> CaptureRegionButton;
	TSharedPtr<SHorizontalBox> RegionCaptureActiveControlRoot;
	TSharedPtr<SCheckBox> HDRCheckBox;
	TSharedPtr<STextBlock> HDRLabel;

	FHighResScreenshotConfig& Config;
	bool bCaptureRegionControlsVisible;

	static TWeakPtr<SWindow> CurrentWindow;
	static TWeakPtr<SHighResScreenshotDialog> CurrentDialog;
	static bool bMaskVisualizationWasEnabled;
};