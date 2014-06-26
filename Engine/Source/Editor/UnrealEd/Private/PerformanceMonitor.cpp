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
		const bool bMonitorEditorPerformance = GetDefault<ULevelEditorMiscSettings>()->bMonitorEditorPerformance;
		return bMonitorEditorPerformance ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	/** Called when the state of the "Show notification" check box changes */
	static void OnMonitorPerformanceChanged(ESlateCheckBoxState::Type NewState)
	{
		const bool bNewEnabledState = (NewState == ESlateCheckBoxState::Checked);

		auto* Settings = GetMutableDefault<ULevelEditorMiscSettings>();
		Settings->bMonitorEditorPerformance = bNewEnabledState;
		Settings->PostEditChange();
	}
};

FPerformanceMonitor::FPerformanceMonitor()
{
	LastEnableTime = 0;
	bIsNotificationAllowed = true;

	UEditorGameAgnosticSettings& Settings = GEditor->AccessGameAgnosticSettings();
	if (Settings.bApplyAutoScalabilityOnStartup)
	{
		Settings.bApplyAutoScalabilityOnStartup = false;
		Settings.PostEditChange();

		AutoApplyScalability();
	}

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

	// Set up the moving average sasmplers
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
	GWarn->BeginSlowTask(LOCTEXT("RunningBenchmark", "Running benchmark to determine optimum engine settings"), true);
	//GWarn->UpdateProgress(-1, -1);
	
	Scalability::FQualityLevels BenchmarkLevels = Scalability::BenchmarkQualityLevels();
	const Scalability::FQualityLevels ExistingLevels = Scalability::GetQualityLevels();
	
	// Make sure we don't turn settings up if the user has turned them down for any reason
	BenchmarkLevels.ResolutionQuality		= FMath::Min(BenchmarkLevels.ResolutionQuality,	ExistingLevels.ResolutionQuality);
	BenchmarkLevels.ViewDistanceQuality	= FMath::Min(BenchmarkLevels.ViewDistanceQuality,	ExistingLevels.ViewDistanceQuality);
	BenchmarkLevels.AntiAliasingQuality	= FMath::Min(BenchmarkLevels.AntiAliasingQuality,	ExistingLevels.AntiAliasingQuality);
	BenchmarkLevels.ShadowQuality			= FMath::Min(BenchmarkLevels.ShadowQuality,		ExistingLevels.ShadowQuality);
	BenchmarkLevels.PostProcessQuality	= FMath::Min(BenchmarkLevels.PostProcessQuality,	ExistingLevels.PostProcessQuality);
	BenchmarkLevels.TextureQuality		= FMath::Min(BenchmarkLevels.TextureQuality,		ExistingLevels.TextureQuality);
	BenchmarkLevels.EffectsQuality		= FMath::Min(BenchmarkLevels.EffectsQuality,		ExistingLevels.EffectsQuality);
	
	Scalability::SetQualityLevels(BenchmarkLevels);
	
	GWarn->EndSlowTask();
}

void FPerformanceMonitor::ShowPerformanceWarning(FText MessageText)
{
	auto AlreadyOpenItem = PerformanceWarningNotificationPtr.Pin();
	if (AlreadyOpenItem.IsValid())
	{
		AlreadyOpenItem->SetText(MessageText);
	}
	else if ((FPlatformTime::Seconds() - LastEnableTime) > 30.0)
	{
		// Only show a new one if we've not shown one for a while
		LastEnableTime = FPlatformTime::Seconds();

		// Create notification item
		FNotificationInfo Info(MessageText);
		Info.bFireAndForget = false;
		Info.FadeOutDuration = 1.0f;
		Info.ExpireDuration = 0.0f;
		Info.bUseLargeFont = false;

		Info.ButtonDetails.Add(
			FNotificationButtonInfo(
			LOCTEXT("PerformanceWarningScalability", "Tweak Scalability Settings"),
			LOCTEXT("PerformanceWarningScalabilityToolTip", "Opens the Scalability UI for manual adjustment."),
			FSimpleDelegate::CreateRaw(this, &FPerformanceMonitor::ShowScalabilityDialog),
			SNotificationItem::CS_None
			)
			);

		Info.ButtonDetails.Add(
			FNotificationButtonInfo(
			LOCTEXT("PerformanceWarningDismiss", "Ok"),
			LOCTEXT("PerformanceWarningDismissToolTip", "Close this notification."),
			FSimpleDelegate::CreateRaw(this, &FPerformanceMonitor::DismissAndDisableNotification),
			SNotificationItem::CS_None
			)
			);

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

	const bool bMonitorEditorPerformance = GetDefault<ULevelEditorMiscSettings>()->bMonitorEditorPerformance;
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

	if (!bLowFramerate)
	{
		HidePerformanceWarning();
	}
	else
	{
		enum MessagesEnum { Seconds, SecondsPercent, Minute, MinutePercent, Minutes, MinutesPercent };
		const FText Messages[] = {
			LOCTEXT("PerformanceWarningInProgress_Seconds",			"Your framerate has been under {Framerate} FPS for the past {SampleTime} seconds.\nYour graphics settings will be reduced next time you restart the editor."),
			LOCTEXT("PerformanceWarningInProgress_Seconds_Percent", "Your framerate has been under {Framerate} FPS for {Percentage}% of the past {SampleTime} seconds.\nYour graphics settings will be reduced next time you restart the editor."),

			LOCTEXT("PerformanceWarningInProgress_Minute",			"Your framerate has been under {Framerate} FPS for the past minute.\nYour graphics settings will be reduced next time you restart the editor."),
			LOCTEXT("PerformanceWarningInProgress_Minute_Percent",	"Your framerate has been under {Framerate} FPS for {Percentage}% of the last minute.\nYour graphics settings will be reduced next time you restart the editor."),

			LOCTEXT("PerformanceWarningInProgress_Minutes",			"Your framerate has been below {Framerate} FPS for the past {SampleTime} minutes.\nYour graphics settings will be reduced next time you restart the editor."),
			LOCTEXT("PerformanceWarningInProgress_Minutes_Percent", "Your framerate has been below {Framerate} FPS for {Percentage}% of the past {SampleTime} minutes.\nYour graphics settings will be reduced next time you restart the editor."),
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

		ShowPerformanceWarning(FText::Format(Messages[Message], Arguments));
	}
}

void FPerformanceMonitor::DismissAndDisableNotification()
{
	bIsNotificationAllowed = false;

	// Set the flag to say we need to run a benchmark on startup
	GEditor->AccessGameAgnosticSettings().bApplyAutoScalabilityOnStartup = true;
	GEditor->AccessGameAgnosticSettings().PostEditChange();

	GEditor->DisableRealtimeViewports();

	HidePerformanceWarning();
}

void FPerformanceMonitor::Reset()
{
	FineMovingAverage.Reset();
	CoarseMovingAverage.Reset();

	HidePerformanceWarning();
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
	auto ExistingWindow = ScalabilitySettingsWindowPtr.Pin();

	if (ExistingWindow.IsValid())
	{
		ExistingWindow->BringToFront();
	}
	else
	{
		// Turn off the auto-application of settings on start up if the user is manually tweaking them
		GEditor->AccessGameAgnosticSettings().bApplyAutoScalabilityOnStartup = false;
		GEditor->AccessGameAgnosticSettings().PostEditChange();

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