// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "SIntroTutorials.h"
#include "IDocumentation.h"
#include "SoundDefinitions.h"

#define LOCTEXT_NAMESPACE "IntroTutorials"

const FString SIntroTutorials::HomePath = TEXT("Shared/Tutorials");

/** 
 * A widget used to control the display of the dynamic titles in a tutorial.
 */
class STutorialStageTicker : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS( STutorialStageTicker ) {}

	/** The current excerpt to display */
	SLATE_ATTRIBUTE( FString, CurrentExcerpt )

	/** The progress string for the current excerpt (e.g. 4/16) */
	SLATE_ATTRIBUTE( FString, CurrentProgressString )

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs )
	{
		ChildSlot
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("Tutorials.CurrentExcerpt"))		
			]
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(InArgs._CurrentExcerpt)
				.TextStyle(FEditorStyle::Get(), "Tutorials.CurrentExcerpt")
			]
		];
	}
};

/** 
 * A widget used for consistency of appearance of tutorial navigation buttons
 */
class STutorialNavigationButton : public SButton
{
public:
	SLATE_BEGIN_ARGS( STutorialNavigationButton ) {}

	/** The image to display */
	SLATE_ARGUMENT( FName, ImageName )

	/** The text to display */
	SLATE_ATTRIBUTE( FText, Text )

	/** Called when the button is clicked */
	SLATE_EVENT( FOnClicked, OnClicked )

	/** Whether the image should be displayed on the left or right */
	SLATE_ARGUMENT( bool, ImageOnLeft )

	/** Whether the image should be displayed on the left or right */
	SLATE_ARGUMENT(bool, ImageOnly)

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs )
	{
		SButton::Construct(SButton::FArguments()
			.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.ToolTipText(InArgs._ToolTipText)
			.OnClicked(InArgs._OnClicked)
			[
				MakeContent(InArgs._ImageOnLeft, InArgs._ImageOnly, InArgs._Text, InArgs._ImageName)
			]);
	}

private:
	/** Make the button content */
	TSharedRef<SWidget> MakeContent(bool bImageOnLeft, bool bImageOnly, const TAttribute<FText>& InText, const FName& InImageName)
	{
		if (bImageOnly)
		{
			return MakeImage(InImageName);
		}
		else if(bImageOnLeft)
		{
			return SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Padding(FMargin(4.f, 0.f))
				[
					MakeImage(InImageName)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					MakeText(InText)
				];
		}
		else
		{
			return SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding( FMargin(4.f,0.f) )
				.VAlign(VAlign_Center)
				[
					MakeText(InText)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					MakeImage(InImageName)
				];
		}
	}

	/** Make the image content */
	TSharedRef<SWidget> MakeImage( const FName& InImageName )
	{
		return SNew(SImage) 
				.Image(FEditorStyle::GetBrush(InImageName))
				.ColorAndOpacity(this, &STutorialNavigationButton::GetColor);
	}

	/** Make the text content */
	TSharedRef<SWidget> MakeText(const TAttribute<FText>& InText)
	{
		return SNew(STextBlock)
				.Text(InText)
				.TextStyle(FEditorStyle::Get(), "Tutorials.NavigationButtons")
				.ColorAndOpacity(this, &STutorialNavigationButton::GetColor);
	}

	/** Get the color according to state */
	FSlateColor GetColor() const
	{
		if(IsEnabled())
		{
			return IsHovered() ? FEditorStyle::GetSlateColor("Tutorials.ButtonHighlightColor") : FEditorStyle::GetColor("Tutorials.ButtonColor");
		}

		// The default disabled effect tries to de-saturate the color, but the color
		// is already fairly desaturated, so we need to supply a different color when the 
		// button is disabled for it to show up.
		return FEditorStyle::GetSlateColor("Tutorials.ButtonDisabledColor");
	}
};

void SIntroTutorials::Construct( const FArguments& Args )
{
	ParentWindowPtr = Args._ParentWindow;
	HomeButtonVisibility = Args._HomeButtonVisibility;
	OnGotoNextTutorial = Args._OnGotoNextTutorial;

	DialogueAudioComponent = NULL;
	bCurrentSectionIsInteractive = false;
	CurrentExcerptIndex = 0;
	CurrentPageStartTime = 0.0;
	ParserConfiguration = FParserConfiguration::Create();
	ParserConfiguration->OnNavigate = FOnNavigate::CreateRaw(this, &SIntroTutorials::ChangePage);
	
	ChildSlot
	[
		SNew(SBorder)
		.Padding(5)
		.BorderImage(this, &SIntroTutorials::GetContentAreaBackground)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SVerticalBox)
				.Visibility(this, &SIntroTutorials::GetContentVisibility)
				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STutorialStageTicker)
						.CurrentExcerpt(this, &SIntroTutorials::GetCurrentExcerptTitle)
						.CurrentProgressString(this, &SIntroTutorials::GetProgressString)
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 8.0f))
					[
						SNew(SProgressBar)
						.Style( FEditorStyle::Get(), "Tutorials.ProgressBar")
						.Percent(this, &SIntroTutorials::GetProgress)
						.FillColorAndOpacity( FLinearColor(0.19f, 0.33f, 0.72f) )
					]
					+SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SAssignNew(ContentArea, SBorder)
						.Padding(6.f)
						.BorderImage(FEditorStyle::GetBrush("NoBorder"))
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
					[
						SNew(SSeparator)
						.SeparatorImage(FEditorStyle::GetBrush("Tutorials.Separator"))
					]
					+SVerticalBox::Slot()
					.Padding(FMargin(0.0f, 8.0f, 0.0f, 0.0f))
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						.Visibility(this, &SIntroTutorials::GetNavigationVisibility)
						+SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(STutorialNavigationButton)
							.ToolTipText(LOCTEXT("PreviousButtonTooltip", "Go back to the previous tutorial page.").ToString())
							.OnClicked(this, &SIntroTutorials::OnPreviousClicked)
							.IsEnabled(this, &SIntroTutorials::OnPreviousIsEnabled)
							.ImageName("Tutorials.Back")
							.Text(this, &SIntroTutorials::GetBackButtonText)
							.ImageOnLeft(true)
							.ImageOnly(false)
							.Visibility(this, &SIntroTutorials::GetBackButtonVisibility)
						]
						+SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(STutorialNavigationButton)
							.ToolTipText(LOCTEXT("HomeButtonTooltip", "Go back to the tutorial index.").ToString())
							.OnClicked(this, &SIntroTutorials::OnHomeClicked)
							.ImageName("Tutorials.Home")
							.ImageOnLeft(true)
							.ImageOnly(true)
							.Visibility(HomeButtonVisibility)
						]
						+SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(STutorialNavigationButton)
							.ToolTipText(LOCTEXT("NextButtonTooltip", "Go to the next tutorial page.").ToString())
							.OnClicked(this, &SIntroTutorials::OnNextClicked)
							.IsEnabled(this, &SIntroTutorials::OnNextIsEnabled)
							.ImageName("Tutorials.Next")
							.Text(this, &SIntroTutorials::GetNextButtonText)
							.ImageOnLeft(false)
							.ImageOnly(false)
							.Visibility(this, &SIntroTutorials::GetNextButtonVisibility)
						]
					]
				]
			]
			+SVerticalBox::Slot()
			[
				SAssignNew(HomeContentArea, SBorder)
				.Visibility(this, &SIntroTutorials::GetHomeContentVisibility)
				.Padding(FMargin(0,0,0,5))
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			]
		]
	];

	// Set the documentation style to what we want to use in the tutorials */
	DocumentationStyle
		.ContentStyle(TEXT("Tutorials.Content"))
		.BoldContentStyle(TEXT("Tutorials.BoldContent"))
		.NumberedContentStyle(TEXT("Tutorials.NumberedContent"))
		.Header1Style(TEXT("Tutorials.Header1"))
		.Header2Style(TEXT("Tutorials.Header2"))
		.HyperlinkButtonStyle(TEXT("Tutorials.Hyperlink.Button"))
		.HyperlinkTextStyle(TEXT("Tutorials.Hyperlink.Text"))
		.SeparatorStyle(TEXT("Tutorials.Separator"));

	InteractiveTutorials = MakeShareable(new FInteractiveTutorials(FSimpleDelegate::CreateSP(this, &SIntroTutorials::TriggerCompleted)));
	InteractiveTutorials->SetupEditorHooks();
	
	ChangePage(HomePath);
}

bool SIntroTutorials::IsHomeStyle() const
{
	return InteractiveTutorials->GetCurrentTutorialStyle() == ETutorialStyle::Home;
}

void SIntroTutorials::ChangePage(const FString& Path)
{
	bool const bPageExists = IDocumentation::Get()->PageExists(*Path);
	if (bPageExists)
	{
		TSharedPtr<IDocumentationPage> NewPage = IDocumentation::Get()->GetPage(Path, ParserConfiguration, DocumentationStyle);

		if (NewPage.IsValid() && (NewPage->GetNumExcerpts() > 0))
		{
			InteractiveTutorials->SetCurrentTutorial(Path);

			CurrentPage = NewPage;
			CurrentPage->GetExcerpts(Excerpts);
			SetCurrentExcerpt(0);

			CurrentPagePath = Path;
			CurrentPageStartTime = FPlatformTime::Seconds();

			// Set window title to current tutorial name
			if (ParentWindowPtr.IsValid())
			{
				ParentWindowPtr.Pin()->SetTitle(FText::FromString(GetCurrentTutorialName()));
			}
		}
		else
		{
			FNotificationInfo Info(FText::Format(LOCTEXT("PageOpenFailMessage", "Unable to access tutorial page \"{0}\"."), FText::FromString(Path)));
			Info.ExpireDuration = 3.0f;
			Info.bUseLargeFont = false;
			TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
			if (Notification.IsValid())
			{
				Notification->SetCompletionState(SNotificationItem::CS_Fail);
			}
		}
	}
}

void SIntroTutorials::SetContentArea()
{
	HomeContentArea->ClearContent();
	ContentArea->ClearContent();

	if (Excerpts.IsValidIndex(CurrentExcerptIndex))
	{
		auto& Excerpt = Excerpts[CurrentExcerptIndex];

		if ( !Excerpt.Content.IsValid() )
		{
			CurrentPage->GetExcerptContent( Excerpt );
		}

		TSharedPtr<SBorder> ContentAreaToUse;
		if(IsHomeStyle())
		{
			ContentAreaToUse = HomeContentArea;
		}
		else
		{
			ContentAreaToUse = ContentArea;
		}

		ContentAreaToUse->SetContent( 
			SNew(SScrollBox)
			+SScrollBox::Slot()
			[
				Excerpt.Content.ToSharedRef()
			]);
	}

	if (ParentWindowPtr.IsValid())
	{
		ParentWindowPtr.Pin()->BringToFront();
	}
}



void SIntroTutorials::PlayDialogue(UDialogueWave* InDialogueWave)
{
	if(InDialogueWave == NULL)
	{
		return;
	}

	// Set up the tutorial dialogue context
	if(TutorialDialogueContext.Speaker == NULL)
	{
		FString ZakVoicePath(TEXT("/Engine/Tutorial/Audio/Zak.Zak"));
		TutorialDialogueContext.Speaker = LoadObject<UDialogueVoice>(NULL, *ZakVoicePath);
		if(TutorialDialogueContext.Speaker != NULL)
		{
			TutorialDialogueContext.Speaker->AddToRoot();
		}

		TutorialDialogueContext.Targets.Reset();

		FString AudienceVoicePath(TEXT("/Engine/Tutorial/Audio/Audience.Audience"));
		UDialogueVoice* AudienceVoice = LoadObject<UDialogueVoice>(NULL, *AudienceVoicePath);
		if(AudienceVoice != NULL)
		{
			AudienceVoice->AddToRoot();
			TutorialDialogueContext.Targets.Add(AudienceVoice);
		}
	}

	USoundBase* Sound = InDialogueWave->GetWaveFromContext(TutorialDialogueContext);
	if(Sound != NULL)
	{
		// Create audio component
		if(DialogueAudioComponent == NULL)
		{
			DialogueAudioComponent = FAudioDevice::CreateComponent( Sound, NULL, NULL, false );
			DialogueAudioComponent->AddToRoot();
		}
		else
		{
			DialogueAudioComponent->Stop();
		}

		DialogueAudioComponent->Sound = Sound;

		DialogueAudioComponent->bAutoDestroy = false;
		DialogueAudioComponent->bIsUISound = true;
		DialogueAudioComponent->bAllowSpatialization = false;
		DialogueAudioComponent->bReverb = false;
		DialogueAudioComponent->Play();	
	}
}

void SIntroTutorials::PlaySound(const FString& WavePath)
{
	if (GEngine->GetAudioDevice())
	{
		// Try FindObject first
		USoundWave* Wave = FindObject<USoundWave>(ANY_PACKAGE, *WavePath);
		if(Wave == NULL)
		{
			// If not, load it now
			Wave = LoadObject<USoundWave>(NULL, *WavePath);
		}

		if (Wave != NULL)
		{
			FActiveSound NewActiveSound;
			NewActiveSound.Sound = Wave;
			NewActiveSound.bIsUISound = true;
			GEngine->GetAudioDevice()->AddNewActiveSound(NewActiveSound);
		}
	}
}

void SIntroTutorials::GotoPreviousPage()
{
	SetCurrentExcerpt(CurrentExcerptIndex - 1);
}

void SIntroTutorials::GotoNextPage()
{
	SetCurrentExcerpt(CurrentExcerptIndex + 1);
}

void SIntroTutorials::SetCurrentExcerpt(int32 NewExcerptIdx)
{
	if ( Excerpts.IsValidIndex(NewExcerptIdx) )
	{
		CurrentExcerptIndex = NewExcerptIdx;

		// see if we should skip forward from here
		while ( !IsLastPage() && InteractiveTutorials->ShouldSkipExcerpt(Excerpts[CurrentExcerptIndex].Name) )
		{
			CurrentExcerptIndex++;
		}

		SetContentArea();

		auto& Excerpt = Excerpts[CurrentExcerptIndex];

		UDialogueWave* const DialogueWave = InteractiveTutorials->GetDialogueForExcerpt(Excerpt.Name);
		if(DialogueWave != NULL)
		{
			PlayDialogue(DialogueWave);
		}
		InteractiveTutorials->SetCurrentExcerpt(Excerpt.Name);
		bCurrentSectionIsInteractive = InteractiveTutorials->IsExcerptInteractive(Excerpt.Name);
	}
}

void SIntroTutorials::TriggerCompleted()
{
	PlaySound(TEXT("/Engine/Tutorial/Audio/CompleteTrigger.CompleteTrigger"));

	InteractiveTutorials->OnExcerptCompleted( Excerpts[CurrentExcerptIndex].Name );

	GotoNextPage();
}


EVisibility SIntroTutorials::GetBackButtonVisibility() const
{
	// hide back button on first page
	return IsFirstPage() ? EVisibility::Hidden : EVisibility::Visible;
}

FText SIntroTutorials::GetBackButtonText() const
{
	// "home" on first page, "back" otherwise
	return LOCTEXT("BackLabel", "BACK");
}

EVisibility SIntroTutorials::GetNextButtonVisibility() const
{
	// hide next button on last page, unless we have a chain
	if(IsLastPage())
	{
		FString NextTutorial;
		if(OnGotoNextTutorial.IsBound())
		{
			NextTutorial = OnGotoNextTutorial.Execute(CurrentPagePath);
		}
		return NextTutorial.Len() > 0 ? EVisibility::Visible : EVisibility::Hidden;
	}
	else
	{
		return EVisibility::Visible;
	}
}

FText SIntroTutorials::GetNextButtonText() const
{
	// "home" on first page, "back" otherwise
	return LOCTEXT("NextLabel", "NEXT");
}

EVisibility SIntroTutorials::GetNavigationVisibility() const
{
	return (IsHomeStyle() && (Excerpts.Num() == 1)) ? EVisibility::Collapsed : EVisibility::Visible;
}

FString SIntroTutorials::GetCurrentTutorialName() const
{
	return CurrentPage->GetTitle();
}

FString SIntroTutorials::GetTimeRemaining() const
{
	if (Excerpts.Num() > 0)
	{
		FString VariableName = FString::Printf(TEXT("TimeRemaining%d"), CurrentExcerptIndex + 1);
		const FString* VariableValue = Excerpts[CurrentExcerptIndex].Variables.Find(VariableName);
		if(VariableValue != NULL)
		{
			return *VariableValue;
		}
	}

	return FString();
}

EVisibility SIntroTutorials::GetContentVisibility() const
{
	return !IsHomeStyle() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SIntroTutorials::GetHomeContentVisibility() const
{
	return IsHomeStyle() ? EVisibility::Visible : EVisibility::Collapsed;
}


FReply SIntroTutorials::OnHomeClicked()
{
	OnGoHome.ExecuteIfBound();

	ChangePage(HomePath);

	return FReply::Handled();
}

FReply SIntroTutorials::OnPreviousClicked()
{
	PlaySound(TEXT("/Engine/Tutorial/Audio/PageBack.PageBack"));

	if (IsFirstPage())
	{
		return OnHomeClicked();
	}

	GotoPreviousPage();
	return FReply::Handled();
}

bool SIntroTutorials::OnPreviousIsEnabled() const
{
	return Excerpts.IsValidIndex(CurrentExcerptIndex) && InteractiveTutorials->CanManuallyReverseExcerpt( Excerpts[CurrentExcerptIndex].Name );
}

FReply SIntroTutorials::OnNextClicked()
{
	PlaySound(TEXT("/Engine/Tutorial/Audio/PageForward.PageForward"));

	if (IsLastPage())
	{
		if(OnGotoNextTutorial.IsBound())
		{
			FString NextPage = OnGotoNextTutorial.Execute(CurrentPagePath);
			if(NextPage.Len() > 0)
			{
				// Note: this currently limits us to chaining only two tutorials together
				ChangePage(NextPage);
				return FReply::Handled();
			}
		}

		return OnHomeClicked();
	}

	GotoNextPage();
	return FReply::Handled();
}

bool SIntroTutorials::OnNextIsEnabled() const
{
	return Excerpts.IsValidIndex(CurrentExcerptIndex) && InteractiveTutorials->CanManuallyAdvanceExcerpt(Excerpts[CurrentExcerptIndex].Name);
}

FString SIntroTutorials::GetCurrentExcerptTitle() const
{
	if (Excerpts.IsValidIndex(CurrentExcerptIndex))
	{
		// First try for unadorned 'StageTitle'
		FString VariableName = FString::Printf(TEXT("StageTitle"));
		const FString* VariableValue = Excerpts[CurrentExcerptIndex].Variables.Find(VariableName);
		if(VariableValue != NULL)
		{
			return *VariableValue;
		}

		// Then try 'StageTitle<StageNum>'
		VariableName = FString::Printf(TEXT("StageTitle%d"), CurrentExcerptIndex + 1);
		VariableValue = Excerpts[CurrentExcerptIndex].Variables.Find(VariableName);
		if(VariableValue != NULL)
		{
			return *VariableValue;
		}
	}

	return FText::Format(LOCTEXT("GenericStageTitle", "Step {0}"), FText::AsNumber(CurrentExcerptIndex + 1)).ToString();
}

FString SIntroTutorials::GetProgressString() const
{
	return FString::Printf(TEXT("%i/%i"), (CurrentExcerptIndex + 1), Excerpts.Num());
}

TOptional<float> SIntroTutorials::GetProgress() const
{
	if (Excerpts.Num() > 0)
	{
		return (float)(CurrentExcerptIndex) / (float)(Excerpts.Num() - 1);
	}
	return 0.0f;
}

const FSlateBrush* SIntroTutorials::GetContentAreaBackground() const
{
	return FEditorStyle::GetBrush("Tutorials.ContentAreaBackground");
}

int32 SIntroTutorials::GetCurrentExcerptIndex() const
{
	return CurrentExcerptIndex;
}

FString SIntroTutorials::GetCurrentPagePath() const
{
	return CurrentPagePath;
}

float SIntroTutorials::GetCurrentPageElapsedTime() const
{
	return (float)(FPlatformTime::Seconds() - CurrentPageStartTime);
}

void SIntroTutorials::SetOnGoHome(const FOnGoHome& InDelegate)
{
	OnGoHome = InDelegate;
}

bool SIntroTutorials::IsLastPage() const
{
	return (CurrentExcerptIndex == (Excerpts.Num() - 1));
}
bool SIntroTutorials::IsFirstPage() const
{
	return CurrentExcerptIndex == 0;
}

#undef LOCTEXT_NAMESPACE
