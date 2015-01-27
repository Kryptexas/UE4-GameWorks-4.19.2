// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "SLocalizationTargetStatusButton.h"
#include "LocalizationCommandletExecution.h"

#define LOCTEXT_NAMESPACE "LocalizationTargetStatusButton"

void SLocalizationTargetStatusButton::Construct(const FArguments& InArgs, ULocalizationTarget& InTarget)
{
	Target = &InTarget;

	SButton::Construct(
		SButton::FArguments()
		.ButtonStyle( FEditorStyle::Get(), TEXT("HoverHintOnly") )
		.OnClicked(this, &SLocalizationTargetStatusButton::OnClicked)
		.ToolTipText(this, &SLocalizationTargetStatusButton::GetToolTipText)
		);

		ChildSlot
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(this, &SLocalizationTargetStatusButton::GetImageBrush)
				.ColorAndOpacity(this, &SLocalizationTargetStatusButton::GetColorAndOpacity)
			];
}

const FSlateBrush* SLocalizationTargetStatusButton::GetImageBrush() const
{
	switch (Target->Settings.Status)
	{
	default:
	case ELocalizationTargetStatus::Unknown:
		return FEditorStyle::GetBrush("Icons.Warning");
		break;
	case ELocalizationTargetStatus::Clear:
		return FEditorStyle::GetBrush("Symbols.Check");
		break;
	case ELocalizationTargetStatus::ConflictsPresent:
		return FEditorStyle::GetBrush("Symbols.X");
		break;
	}
}

FSlateColor SLocalizationTargetStatusButton::GetColorAndOpacity() const
{
	switch (Target->Settings.Status)
	{
	default:
	case ELocalizationTargetStatus::Unknown:
		return FLinearColor::White;
		break;
	case ELocalizationTargetStatus::Clear:
		return FLinearColor::Green;
		break;
	case ELocalizationTargetStatus::ConflictsPresent:
		return FLinearColor::Red;
		break;
	}
}

FText SLocalizationTargetStatusButton::GetToolTipText() const
{
	switch (Target->Settings.Status)
	{
	default:
	case ELocalizationTargetStatus::Unknown:
		return LOCTEXT("StatusToolTip_Unknown", "Conflict report file not detected. Click to generate the conflict report.");
		break;
	case ELocalizationTargetStatus::Clear:
		return LOCTEXT("StatusToolTip_Clear", "No conflicts detected.");
		break;
	case ELocalizationTargetStatus::ConflictsPresent:
		return LOCTEXT("StatusToolTip_ConflictsPresent", "Conflicts detected. Click to open the conflict report.");
		break;
	}
}

FReply SLocalizationTargetStatusButton::OnClicked()
{
	switch (Target->Settings.Status)
	{
	case ELocalizationTargetStatus::Unknown:
		{
			// Generate conflict report.
			const FString ReportScriptPath = LocalizationConfigurationScript::GetReportScriptPath(Target->Settings);
			LocalizationConfigurationScript::GenerateReportScript(Target->Settings).Write(ReportScriptPath);
			TSharedPtr<FLocalizationCommandletProcess> ReportProcess = FLocalizationCommandletProcess::Execute(ReportScriptPath);
			if (ReportProcess.IsValid())
			{
				FPlatformProcess::WaitForProc(ReportProcess->GetHandle());
				Target->Settings.UpdateStatusFromConflictReport();
			}
		}
		break;
	case ELocalizationTargetStatus::Clear:
		// Do nothing.
		break;
	case ELocalizationTargetStatus::ConflictsPresent:
		FString ReportString;
		if (FFileHelper::LoadFileToString(ReportString, *LocalizationConfigurationScript::GetConflictReportPath(Target->Settings)))
		{
			TSharedPtr<SWindow> ReportWindow =
				SNew(SWindow)
				.Content()
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SMultiLineEditableText)
						.Text( FText::FromString(ReportString) )
					]
				];

			if (ReportWindow.IsValid())
			{
				FSlateApplication::Get().AddWindow(ReportWindow.ToSharedRef());
			}
		}
		break;
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE