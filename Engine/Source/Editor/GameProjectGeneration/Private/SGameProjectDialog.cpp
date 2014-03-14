// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameProjectGenerationPrivatePCH.h"
#include "MainFrame.h"
#include "DesktopPlatformModule.h"
#include "IDocumentation.h"
#include "EngineBuildSettings.h"
#include "EngineAnalytics.h"
#include "AnalyticsEventAttribute.h"
#include "IAnalyticsProvider.h"

#define LOCTEXT_NAMESPACE "GameProjectGeneration"


/* SGameProjectDialog interface
*****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGameProjectDialog::Construct( const FArguments& InArgs )
{
	bool bAtLeastOneVisibleRecentProject = false;

	ProjectsTabVisibility = InArgs._AllowProjectOpening ? EVisibility::Visible : EVisibility::Collapsed;
	NewProjectTabVisibility = InArgs._AllowProjectCreate ? EVisibility::Visible : EVisibility::Collapsed;

	ChildSlot
	[
		// Drop shadow border
		SNew(SBorder)
			.Padding(10.0f )
			.BorderImage(FEditorStyle::GetBrush("ContentBrowser.ThumbnailShadow"))
			[
				SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush( "Docking.Tab.ContentAreaBrush"))
					.Padding(10.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew( SBorder )
											.Visibility(ProjectsTabVisibility)
											.BorderImage(this, &SGameProjectDialog::HandleProjectsTabHeaderBorderImage)
											.Padding(0)
											[
												SNew(SOverlay)

												+ SOverlay::Slot()
													.VAlign(VAlign_Top)
													[
														SNew(SBox)
															.HeightOverride(2.0f)
															[
																SNew(SImage)
																	.Image(this, &SGameProjectDialog::HandleProjectsTabHeaderImage)
																	.Visibility(EVisibility::HitTestInvisible)
															]
													]

												+ SOverlay::Slot()
													[
														SAssignNew(ProjectsTabButton, SButton)
															.ForegroundColor(FCoreStyle::Get().GetSlateColor("Foreground"))
															.ButtonStyle(FEditorStyle::Get(), TEXT("NoBorder"))
															.OnClicked(this, &SGameProjectDialog::HandleProjectsTabButtonClicked)
															.ContentPadding(FMargin(40, 5))
															.Text(LOCTEXT("ProjectsTabTitle", "Projects"))
															.TextStyle(FEditorStyle::Get(), TEXT("ProjectBrowser.Tab.Text"))
													]
											]
									]

								+ SHorizontalBox::Slot()
									.Padding(InArgs._AllowProjectOpening ? 10 : 0, 0)
									.AutoWidth()
									[
										SNew(SBorder)
											.Visibility(NewProjectTabVisibility)
											.BorderImage(this, &SGameProjectDialog::HandleNewProjectTabHeaderBorderImage)
											.Padding(0)
											[
												SNew(SOverlay)

												+ SOverlay::Slot()
													.VAlign(VAlign_Top)
													[
														SNew(SBox)
														.HeightOverride(2.0f)
														[
															SNew(SImage)
																.Image(this, &SGameProjectDialog::HandleNewProjectTabHeaderImage)
																.Visibility(EVisibility::HitTestInvisible)
														]
													]

												+ SOverlay::Slot()
													[
														SAssignNew(NewProjectTabButton, SButton)
															.ForegroundColor(FCoreStyle::Get().GetSlateColor("Foreground"))
															.ButtonStyle(FEditorStyle::Get(), "NoBorder")
															.OnClicked(this, &SGameProjectDialog::HandleNewProjectTabButtonClicked)
															.ContentPadding(FMargin(20, 5))
															.TextStyle(FEditorStyle::Get(), "ProjectBrowser.Tab.Text")
															.Text(LOCTEXT("NewProjectTabTitle", "New Project"))
															.ToolTip(IDocumentation::Get()->CreateToolTip(LOCTEXT("NewProjectTabTitle", "New Project"), nullptr, "Shared/LevelEditor", "NewProjectTab"))
													]
											]
									]

								+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									.HAlign(HAlign_Right)
									.Padding(0,5)
									[
										SNew(SBorder)
											.BorderImage(FEditorStyle::Get().GetBrush( "ToolPanel.GroupBorder" ))
											.Padding(3)
											[
												SAssignNew(MarketplaceButton, SButton)
													.Visibility( (FEngineBuildSettings::IsPerforceBuild() && !FEngineBuildSettings::IsInternalBuild()) ? EVisibility::Collapsed : EVisibility::Visible )
													.ForegroundColor(this, &SGameProjectDialog::HandleMarketplaceTabButtonForegroundColor)
													.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
													.OnClicked(this, &SGameProjectDialog::HandleMarketplaceTabButtonClicked)
													.ContentPadding(FMargin(20, 0))
													.ToolTipText(LOCTEXT("MarketplaceToolTip", "Check out the Marketplace to find new projects!"))
													.HAlign(HAlign_Center)
													.VAlign(VAlign_Center)
													.Content()
													[
														SNew(SHorizontalBox)

														+ SHorizontalBox::Slot()
															.Padding(2.0f)
															.AutoWidth()
															[
																SNew(SImage)
																	.Image(FEditorStyle::GetBrush("LevelEditor.OpenMarketplace")) 
															]

														+ SHorizontalBox::Slot()
															.Padding( 2.0f )
															[
																SNew(STextBlock)
																	.TextStyle(FEditorStyle::Get(), "ProjectBrowser.Tab.Text")
																	.Text(LOCTEXT("Marketplace", "Marketplace"))
															]
													]
											]
									]
							]

						+ SVerticalBox::Slot()
							[
								// custom content area
								SNew(SBorder)
									.ColorAndOpacity(this, &SGameProjectDialog::HandleCustomContentColorAndOpacity)
									.BorderImage(FEditorStyle::GetBrush("ProjectBrowser.Background"))
									.Padding(10.0f)
									[
										SAssignNew(ContentAreaSwitcher, SWidgetSwitcher)
											.WidgetIndex(InArgs._AllowProjectOpening ? 0 : 1)

										+ SWidgetSwitcher::Slot()
											[
												// project browser
												SAssignNew(ProjectBrowser, SProjectBrowser)
													.AllowProjectCreate(InArgs._AllowProjectCreate)
													.OnNewProjectScreenRequested(this, &SGameProjectDialog::ShowNewProjectTab)
											]

										+ SWidgetSwitcher::Slot()
											[
												// new project wizard
												SAssignNew(NewProjectWizard, SNewProjectWizard)
											]
									]
							]
					]
			]
	];

	ActiveTab = InArgs._AllowProjectOpening ? SGameProjectDialog::ProjectsTab : SGameProjectDialog::NewProjectTab;
	ActiveTab = !ProjectBrowser->HasProjects() && InArgs._AllowProjectCreate ? SGameProjectDialog::NewProjectTab : ActiveTab;

	if (ActiveTab == ProjectsTab)
	{
		ShowProjectBrowser();
	}
	else if (ActiveTab == NewProjectTab)
	{
		ShowNewProjectTab();
	}
	else
	{
		check(false);
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SWidget overrides
*****************************************************************************/

void SGameProjectDialog::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Play the intro fade in the first frame after the widget is created.
	// We start it now instead of Construct because there is a lot of elapsed time between Construct and when we
	// see the dialog and the beginning of the animation is cut off.
	if (FadeAnimation.IsAtStart())
	{
		FadeIn();
	}
}


/* SGameProjectDialog implementation
*****************************************************************************/

void SGameProjectDialog::FadeIn( )
{
	FadeAnimation = FCurveSequence();
	FadeCurve = FadeAnimation.AddCurve(0.f, 0.5f, ECurveEaseFunction::QuadOut);
	FadeAnimation.Play();
}


bool SGameProjectDialog::OpenProject( const FString& ProjectFile )
{
	FText FailReason;

	if (GameProjectUtils::OpenProject(ProjectFile, FailReason))
	{
		return true;
	}

	FMessageDialog::Open(EAppMsgType::Ok, FailReason);

	return false;
}


void SGameProjectDialog::ShowNewProjectTab( )
{
	if (ContentAreaSwitcher.IsValid() && NewProjectWizard.IsValid())
	{
		ContentAreaSwitcher->SetActiveWidget( NewProjectWizard.ToSharedRef());
		ActiveTab = NewProjectTab;
	}
}


FReply SGameProjectDialog::ShowProjectBrowser( )
{
	if (ContentAreaSwitcher.IsValid() && ProjectBrowser.IsValid())
	{
		ContentAreaSwitcher->SetActiveWidget(ProjectBrowser.ToSharedRef());
		ActiveTab = ProjectsTab;
	}

	return FReply::Handled();
}


/* SGameProjectDialog callbacks
*****************************************************************************/

FLinearColor SGameProjectDialog::HandleCustomContentColorAndOpacity( ) const
{
	return FLinearColor(1,1,1, FadeCurve.GetLerp());
}


FReply SGameProjectDialog::HandleMarketplaceTabButtonClicked( )
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	if (DesktopPlatform != nullptr)
	{
		TArray<FAnalyticsEventAttribute> EventAttributes;

		if (DesktopPlatform->OpenLauncher(false, TEXT("-OpenMarket")))
		{
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("OpenSucceeded"), TEXT("TRUE")));
		}
		else
		{
			EventAttributes.Add(FAnalyticsEventAttribute(TEXT("OpenSucceeded"), TEXT("FALSE")));

			if (EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("InstallMarketplacePrompt", "The Marketplace requires the Unreal Engine Launcher, which does not seem to be installed on your computer. Would you like to install it now?")))
			{
				if (!DesktopPlatform->OpenLauncher(true, TEXT("-OpenMarket")))
				{
					EventAttributes.Add(FAnalyticsEventAttribute(TEXT("InstallSucceeded"), TEXT("FALSE")));
					FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Sorry, there was a problem installing the Launcher.\nPlease try to install it manually!")));
				}
				else
				{
					EventAttributes.Add(FAnalyticsEventAttribute(TEXT("InstallSucceeded"), TEXT("TRUE")));
				}
			}
		}

		EventAttributes.Add(FAnalyticsEventAttribute(TEXT("Source"), TEXT("ProjectBrowser")));
		if( FEngineAnalytics::IsAvailable() )
		{
			FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.OpenMarketplace"), EventAttributes);
		}
	}

	return FReply::Handled();
}


FSlateColor SGameProjectDialog::HandleMarketplaceTabButtonForegroundColor( ) const
{
	//if (MarketplaceButton->IsHovered())
	//{
	//	return FLinearColor::Black;
	//}
	//else
	//{
		return FSlateColor::UseForeground();
	//}
}


FReply SGameProjectDialog::HandleNewProjectTabButtonClicked( )
{
	ShowNewProjectTab();

	return FReply::Handled();
}


const FSlateBrush* SGameProjectDialog::HandleNewProjectTabHeaderBorderImage( ) const
{
	if (ActiveTab == NewProjectTab)
	{
		return FEditorStyle::GetBrush("ProjectBrowser.Tab.ActiveBackground");
	}
	
	return FEditorStyle::GetBrush("ProjectBrowser.Tab.Background");
}


const FSlateBrush* SGameProjectDialog::HandleNewProjectTabHeaderImage( ) const
{
	if (NewProjectTabButton->IsPressed())
	{
		return FEditorStyle::GetBrush("ProjectBrowser.Tab.PressedHighlight");
	}

	if ((ActiveTab == NewProjectTab) || NewProjectTabButton->IsHovered())
	{
		return FEditorStyle::GetBrush("ProjectBrowser.Tab.ActiveHighlight");
	}
	
	return FEditorStyle::GetBrush("ProjectBrowser.Tab.Highlight");
}


FReply SGameProjectDialog::HandleProjectsTabButtonClicked( )
{
	ShowProjectBrowser();

	return FReply::Handled();
}


const FSlateBrush* SGameProjectDialog::HandleProjectsTabHeaderBorderImage( ) const
{
	if (ActiveTab == ProjectsTab)
	{
		return FEditorStyle::GetBrush("ProjectBrowser.Tab.ActiveBackground");
	}
	
	return FEditorStyle::GetBrush("ProjectBrowser.Tab.Background");
}


const FSlateBrush* SGameProjectDialog::HandleProjectsTabHeaderImage( ) const
{
	if (ProjectsTabButton->IsPressed())
	{
		return FEditorStyle::GetBrush("ProjectBrowser.Tab.PressedHighlight");
	}

	if ((ActiveTab == ProjectsTab) || ProjectsTabButton->IsHovered())
	{
		return FEditorStyle::GetBrush("ProjectBrowser.Tab.ActiveHighlight");
	}
	
	return FEditorStyle::GetBrush("ProjectBrowser.Tab.Highlight");
}


#undef LOCTEXT_NAMESPACE
