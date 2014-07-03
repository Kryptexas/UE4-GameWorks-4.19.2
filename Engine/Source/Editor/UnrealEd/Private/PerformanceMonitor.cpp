// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "SScalabilitySettings.h"
#include "PerformanceMonitor.h"

#define LOCTEXT_NAMESPACE "PerformanceMonitor"

/** Scalability dialog widget */
class SScalabilitySettingsDialog : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SScalabilitySettingsDialog) {}
		SLATE_ARGUMENT(FOnClicked, OnDoneClicked)
	SLATE_END_ARGS()

public:

	/** Construct this widget */
	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SBorder)
			.HAlign(HAlign_Fill)
			.BorderImage(FEditorStyle::GetBrush("ChildWindow.Background"))
			[
				SNew(SBox)
				.WidthOverride(500.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.f)
					[
						SNew(STextBlock).Text(LOCTEXT("PerformanceWarningDescription",
						"The current performance of the editor seems to be low.\nUse the options below to reduce the amount of detail and increase performance.").ToString())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.f)
					[
						SNew(SScalabilitySettings)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.f)
					[
						SNew(STextBlock)
						.ToolTip(
							SNew(SToolTip)
							[
								SNew(SImage)
								.Image(FEditorStyle::GetBrush("Scalability.ScalabilitySettings"))
							]
						)
						.AutoWrapText(true)
						.Text(LOCTEXT("PerformanceWarningChangeLater",
							"You can modify these settings in future via \"Quick Settings\" button on the level editor toolbar and choosing \"Engine Scalability Settings\".").ToString())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						[
							SNew(SCheckBox)
							.OnCheckStateChanged(FOnCheckStateChanged::CreateStatic(&OnMonitorPerformanceChanged))
							.IsChecked(TAttribute<ESlateCheckBoxState::Type>::Create(&IsMonitoringPerformance))
							.Content()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("PerformanceWarningEnableDisableCheckbox", "Keep monitoring editor performance?"))
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("ScalabilityDone", "Done"))
							.OnClicked(InArgs._OnDoneClicked)
						]
					]
				]
			]
		];
	}

	/** Called to get the "Show notification" check box state */
	static ESlateCheckBoxState::Type IsMonitoringPerformance()
	{
		const bool bMonitorEditorPerformance = GEditor->GetEditorUserSettings().bMonitorEditorPerformance;
		return bMonitorEditorPerformance ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	/** Called when the state of the "Show notification" check box changes */
	static void OnMonitorPerformanceChanged(ESlateCheckBoxState::Type NewState)
	{
		const bool bNewEnabledState = (NewState == ESlateCheckBoxState::Checked);

		auto& Settings = GEditor->AccessEditorUserSettings();
		Settings.bMonitorEditorPerformance = bNewEnabledState;
		Settings.PostEditChange();
	}
};

FPerformanceMonitor::FPerformanceMonitor()
{
	LastEnableTime = 0;
	NotificationTimeout = 5;
	bIsNotificationAllowed = true;

	// Register the console variables we need
	IConsoleVariable* CVar = nullptr;

	CVar = IConsoleManager::Get().RegisterConsoleVariable(TEXT("PerfWarn.FineSampleTime"), 30, TEXT("How many seconds we sample the percentage for the fine-grained minimum FPS."), ECVF_Default);
	CVar->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FPerformanceMonitor::UpdateMovingAverageSamplers));
	CVar = IConsoleManager::Get().RegisterConsoleVariable(TEXT("PerfWarn.CoarseSampleTime"), 600, TEXT("How many seconds we sample the percentage for the coarse-grained minimum FPS."), ECVF_Default);
	CVar->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FPerformanceMonitor::UpdateMovingAverageSamplers));

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("PerfWarn.FineMinFPS"), 10, TEXT("The FPS threshold below which we warn about for fine-grained sampling."), ECVF_Default);
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("PerfWarn.CoarseMinFPS"), 20, TEXT("The FPS threshold below which we warn about for coarse-grained sampling."), ECVF_Default);

	IConsoleManager::Get().RegisterConsoleVariable(TEXT("PerfWarn.FinePercentThreshold"), 80, TEXT("The percentage of samples that fall below min FPS above which we warn for."), ECVF_Default);
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("PerfWarn.CoarsePercentThreshold"), 80, TEXT("The percentage of samples that fall below min FPS above which we warn for."), ECVF_Default);

	// Set up the moving average samplers
	UpdateMovingAverageSamplers();
}

FPerformanceMonitor::~FPerformanceMonitor()
{
	IConsoleManager& Console = IConsoleManager::Get();
	Console.FindConsoleVariable(TEXT("PerfWarn.FineSampleTime"))->SetOnChangedCallback(FConsoleVariableDelegate());
	Console.FindConsoleVariable(TEXT("PerfWarn.CoarseSampleTime"))->SetOnChangedCallback(FConsoleVariableDelegate());
}

void FPerformanceMonitor::AutoApplyScalability()
{
	const Scalability::FQualityLevels ExistingLevels = Scalability::GetQualityLevels();
	Scalability::FQualityLevels NewLevels = GEditor->GetGameAgnosticSettings().EngineBenchmarkResult;
	
	// Make sure we don't turn settings up if the user has turned them down for any reason
	NewLevels.ResolutionQuality	= FMath::Min(NewLevels.ResolutionQuality,		ExistingLevels.ResolutionQuality);
	NewLevels.ViewDistanceQuality	= FMath::Min(NewLevels.ViewDistanceQuality,	ExistingLevels.ViewDistanceQuality);
	NewLevels.AntiAliasingQuality	= FMath::Min(NewLevels.AntiAliasingQuality,	ExistingLevels.AntiAliasingQuality);
	NewLevels.ShadowQuality		= FMath::Min(NewLevels.ShadowQuality,			ExistingLevels.ShadowQuality);
	NewLevels.PostProcessQuality	= FMath::Min(NewLevels.PostProcessQuality,		ExistingLevels.PostProcessQuality);
	NewLevels.TextureQuality		= FMath::Min(NewLevels.TextureQuality,			ExistingLevels.TextureQuality);
	NewLevels.EffectsQuality		= FMath::Min(NewLevels.EffectsQuality,			ExistingLevels.EffectsQuality);
	
	Scalability::SetQualityLevels(NewLevels);
	Scalability::SaveState(GEditorGameAgnosticIni);

	const bool bAutoApplied = true;
	Scalability::RecordQualityLevelsAnalytics(bAutoApplied);

	GEditor->DisableRealtimeViewports();

	// Reset the timers so as not to skew the data with the time it took to do the benchmark	
	FineMovingAverage.Reset();
	CoarseMovingAverage.Reset();
}

void FPerformanceMonitor::ShowPerformanceWarning(FText MessageText)
{
	static double MinNotifyTime = 30;
	if ((FPlatformTime::Seconds() - LastEnableTime) > MinNotifyTime)
	{
		// Only show a new one if we've not shown one for a while
		LastEnableTime = FPlatformTime::Seconds();
		NotificationTimeout = 5;

		// Create notification item
		FNotificationInfo Info(MessageText);
		Info.bFireAndForget = false;
		Info.FadeOutDuration = 4.0f;
		Info.ExpireDuration = 0.0f;
		Info.bUseLargeFont = false;

		Info.Hyperlink = FSimpleDelegate::CreateRaw(this, &FPerformanceMonitor::ShowScalabilityDialog);
		Info.HyperlinkText = LOCTEXT("PerformanceWarningScalability", "Tweak Manually");

		PerformanceWarningNotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void FPerformanceMonitor::HidePerformanceWarning()
{
	// Finished! Notify the UI.
	TSharedPtr<SNotificationItem> NotificationItem = PerformanceWarningNotificationPtr.Pin();

	if (NotificationItem.IsValid())
	{
		NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
		NotificationItem->Fadeout();

		PerformanceWarningNotificationPtr.Reset();
	}
}

void FPerformanceMonitor::Tick(float DeltaTime)
{
	if (GEngine->ShouldThrottleCPUUsage())
	{
		return;
	}

	extern ENGINE_API float GAverageFPS;
	FineMovingAverage.Tick(FPlatformTime::Seconds(), GAverageFPS);
	CoarseMovingAverage.Tick(FPlatformTime::Seconds(), GAverageFPS);

	const bool bMonitorEditorPerformance = GetDefault<UEditorUserSettings>()->bMonitorEditorPerformance;
	if( !bMonitorEditorPerformance || !bIsNotificationAllowed )
	{
		return;
	}

	FFormatNamedArguments Arguments;
	bool bLowFramerate = false;
	int32 SampleTime = 0;
	float PercentUnderTarget = 0.f;

	if (FineMovingAverage.IsReliable())
	{
		static IConsoleVariable* CVarMinFPS = IConsoleManager::Get().FindConsoleVariable(TEXT("PerfWarn.FineMinFPS"));
		static IConsoleVariable* CVarPercentThreshold = IConsoleManager::Get().FindConsoleVariable(TEXT("PerfWarn.FinePercentThreshold"));

		PercentUnderTarget = FineMovingAverage.PercentageBelowThreshold(CVarMinFPS->GetFloat());

		if (PercentUnderTarget >= CVarPercentThreshold->GetFloat())
		{
			Arguments.Add(TEXT("Framerate"), CVarMinFPS->GetInt());
			Arguments.Add(TEXT("Percentage"), FMath::FloorToFloat(PercentUnderTarget));
			SampleTime = IConsoleManager::Get().FindConsoleVariable(TEXT("PerfWarn.FineSampleTime"))->GetInt();

			bLowFramerate = true;
		}
	}


	if (!bLowFramerate && CoarseMovingAverage.IsReliable())
	{
		static IConsoleVariable* CVarMinFPS = IConsoleManager::Get().FindConsoleVariable(TEXT("PerfWarn.CoarseMinFPS"));
		static IConsoleVariable* CVarPercentThreshold = IConsoleManager::Get().FindConsoleVariable(TEXT("PerfWarn.CoarsePercentThreshold"));

		PercentUnderTarget = CoarseMovingAverage.PercentageBelowThreshold(CVarMinFPS->GetFloat());

		if (PercentUnderTarget >= CVarPercentThreshold->GetFloat())
		{
			Arguments.Add(TEXT("Framerate"), CVarMinFPS->GetInt());
			Arguments.Add(TEXT("Percentage"), FMath::FloorToFloat(PercentUnderTarget));
			SampleTime = IConsoleManager::Get().FindConsoleVariable(TEXT("PerfWarn.CoarseSampleTime"))->GetInt();

			bLowFramerate = true;
		}
	}

	auto AlreadyOpenItem = PerformanceWarningNotificationPtr.Pin();

	if (!bLowFramerate)
	{
		// Framerate is back up again - just reset everything and hide the timeout
		if (AlreadyOpenItem.IsValid())
		{
			Reset();
		}
	}
	else
	{
		// Choose an appropriate message
		enum MessagesEnum { Seconds, SecondsPercent, Minute, MinutePercent, Minutes, MinutesPercent };
		const FText Messages[] = {
			LOCTEXT("PerformanceWarningInProgress_Seconds",			"Your framerate has been under {Framerate} FPS for the past {SampleTime} seconds.\nApplying optimum engine settings in {TimeRemaining}s."),
			LOCTEXT("PerformanceWarningInProgress_Seconds_Percent", "Your framerate has been under {Framerate} FPS for {Percentage}% of the past {SampleTime} seconds.\nApplying optimum engine settings in {TimeRemaining}s."),

			LOCTEXT("PerformanceWarningInProgress_Minute",			"Your framerate has been under {Framerate} FPS for the past minute.\nApplying optimum engine settings in {TimeRemaining}s."),
			LOCTEXT("PerformanceWarningInProgress_Minute_Percent",	"Your framerate has been under {Framerate} FPS for {Percentage}% of the last minute.\nApplying optimum engine settings in {TimeRemaining}s."),

			LOCTEXT("PerformanceWarningInProgress_Minutes",			"Your framerate has been below {Framerate} FPS for the past {SampleTime} minutes.\nApplying optimum engine settings in {TimeRemaining}s."),
			LOCTEXT("PerformanceWarningInProgress_Minutes_Percent", "Your framerate has been below {Framerate} FPS for {Percentage}% of the past {SampleTime} minutes.\nApplying optimum engine settings in {TimeRemaining}s."),
		};

		int32 Message;
		if (SampleTime < 60)
		{
			Arguments.Add(TEXT("SampleTime"), SampleTime);
			Message = Seconds;
		}
		else if (SampleTime == 60)
		{
			Message = Minute;
		}
		else
		{
			Arguments.Add(TEXT("SampleTime"), SampleTime / 60);
			Message = Minutes;
		}

		// Use the message with the specific percentage on if applicable
		if (PercentUnderTarget <= 95.f)
		{
			++Message;
		}


		// Now update the notification or create a new one
		if (AlreadyOpenItem.IsValid())
		{
			NotificationTimeout -= DeltaTime;
			Arguments.Add(TEXT("TimeRemaining"), FMath::Max(1, FMath::CeilToInt(NotificationTimeout)));

			if (NotificationTimeout <= 0)
			{
				// Timed-out. Apply the settings.
				AutoApplyScalability();
				Reset();
			}
			else
			{
				AlreadyOpenItem->SetText(FText::Format(Messages[Message], Arguments));
			}
		}
		else
		{
			NotificationTimeout = 5;
			Arguments.Add(TEXT("TimeRemaining"), int(NotificationTimeout));

			ShowPerformanceWarning(FText::Format(Messages[Message], Arguments));
		}
	}
}

void FPerformanceMonitor::Reset()
{
	FineMovingAverage.Reset();
	CoarseMovingAverage.Reset();

	HidePerformanceWarning();
	bIsNotificationAllowed = true;
}

void FPerformanceMonitor::UpdateMovingAverageSamplers(IConsoleVariable* Unused)
{
	static const int NumberOfSamples = 50;
	IConsoleManager& Console = IConsoleManager::Get();

	float SampleTime = Console.FindConsoleVariable(TEXT("PerfWarn.FineSampleTime"))->GetFloat();
	FineMovingAverage = FMovingAverage(NumberOfSamples, SampleTime / NumberOfSamples);

	SampleTime = Console.FindConsoleVariable(TEXT("PerfWarn.CoarseSampleTime"))->GetFloat();
	CoarseMovingAverage = FMovingAverage(NumberOfSamples, SampleTime / NumberOfSamples);
}

void FPerformanceMonitor::ShowScalabilityDialog()
{
	Reset();
	bIsNotificationAllowed = false;

	auto ExistingWindow = ScalabilitySettingsWindowPtr.Pin();

	if (ExistingWindow.IsValid())
	{
		ExistingWindow->BringToFront();
	}
	else
	{
		// Create the window
		ScalabilitySettingsWindowPtr = ExistingWindow = SNew(SWindow)
			.Title(LOCTEXT("PerformanceWarningDialogTitle", "Scalability Options"))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.CreateTitleBar(true)
			.SizingRule(ESizingRule::Autosized);

		ExistingWindow->SetOnWindowClosed(FOnWindowClosed::CreateStatic([](const TSharedRef<SWindow>&, FPerformanceMonitor* PerfWarn){
			PerfWarn->Reset();
		}, this));

		ExistingWindow->SetContent(
			SNew(SScalabilitySettingsDialog)
			.OnDoneClicked(
				FOnClicked::CreateStatic([](TWeakPtr<SWindow> Window){
					auto WindowPin = Window.Pin();
					if (WindowPin.IsValid())
					{
						WindowPin->RequestDestroyWindow();
					}
					return FReply::Handled();
				}, ScalabilitySettingsWindowPtr)
			)
		);


		TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
		if (RootWindow.IsValid())
		{
			FSlateApplication::Get().AddModalWindow(ExistingWindow.ToSharedRef(), RootWindow);
		}
		else
		{
			FSlateApplication::Get().AddWindow(ExistingWindow.ToSharedRef());
		}
	}
}

#undef LOCTEXT_NAMESPACE
