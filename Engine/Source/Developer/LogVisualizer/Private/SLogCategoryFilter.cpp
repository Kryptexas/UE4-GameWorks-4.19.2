// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"

void SLogCategoryFilter::Construct(const FArguments& InArgs)
{
	GenerateSupportedVerbosityLevels();

	GetDefaultFilters();

	ChildSlot
	[
		SAssignNew(FiltersBar, SHorizontalBox)
	];

	GenerateFilters();
}

void SLogCategoryFilter::GenerateSupportedVerbosityLevels()
{
	SupportedVerbosityLevels.Empty();
	SupportedVerbosityLevels.Add(MakeShareable(new ELogVerbosity::Type(ELogVerbosity::Display)));
	SupportedVerbosityLevels.Add(MakeShareable(new ELogVerbosity::Type(ELogVerbosity::Log)));
	SupportedVerbosityLevels.Add(MakeShareable(new ELogVerbosity::Type(ELogVerbosity::Verbose)));
	SupportedVerbosityLevels.Add(MakeShareable(new ELogVerbosity::Type(ELogVerbosity::All)));
}

ELogVerbosity::Type SLogCategoryFilter::GetVerbosityFromString(FString VerbosityStr) const
{
	for (int32 i = 0; i < ELogVerbosity::All; ++i)
	{
		if (VerbosityStr == FOutputDevice::VerbosityToString(ELogVerbosity::Type(i)))
		{
			return ELogVerbosity::Type(i);
		}
	}
	
	return ELogVerbosity::All;
}

void SLogCategoryFilter::GetDefaultFilters()
{
	Filters.Empty();

	TArray<FString> VisLogFilters;
	if (GConfig->GetArray(TEXT("VisLog"), TEXT("Filters"), VisLogFilters, GGameIni) > 0)
	{
		for (int32 Idx = 0; Idx < VisLogFilters.Num(); ++Idx)
		{
			FLogCategoryType Type;

			FString CategoryName, Verbosity;
			FString Filter = VisLogFilters[Idx];
			if (Filter.Split(FString("#"), &CategoryName, &Verbosity))
			{
				Type.CategoryName = CategoryName;
				Type.Verbosity = GetVerbosityFromString(Verbosity);
			}
			else
			{
				Type.CategoryName = Filter;
			}
			
			Filters.AddUnique(Type);
		}
	}
}

void SLogCategoryFilter::GenerateFilters()
{
	if (!FiltersBar.IsValid())
	{
		return;
	}

	FiltersBar->ClearChildren();
	for (int32 i = 0; i < Filters.Num(); ++i)
	{
		FiltersBar->AddSlot()
			.AutoWidth()
			.Padding(FMargin(5.0, 0.0f))
			[
				SNew(SButton)
				.Text(this, &SLogCategoryFilter::GetFilterText, i)
				.OnClicked(this, &SLogCategoryFilter::LaunchPopUp_OnClicked, i)
			];
	}

	FiltersBar->AddSlot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(FMargin(5.0, 0.0f))
		[
			SNew(SButton)
			.Text(FString(TEXT("+")))
			.OnClicked(this, &SLogCategoryFilter::AddFilter_OnClicked)
		];
}

FReply SLogCategoryFilter::AddFilter_OnClicked()
{
	FLogCategoryType Type;
	Type.CategoryName = FString(TEXT("LogCategory"));
	Filters.AddUnique(Type);

	GenerateFilters();

	return FReply::Handled();
}

FReply SLogCategoryFilter::RemoveFilter_OnClicked(int32 FilterIndex)
{
	if (FilterIndex >= 0 && FilterIndex < Filters.Num())
	{
		Filters.RemoveAt(FilterIndex, 1, true);

		GenerateFilters();

		ClosePopup();
	}

	return FReply::Handled();
}

FString SLogCategoryFilter::GetFilterText(int32 FilterIndex) const
{
	if (FilterIndex >= 0 && FilterIndex < Filters.Num())
	{
		return Filters[FilterIndex].CategoryName + FString(" (") + GetVerbosityLevelText(FilterIndex) + FString(")");
	}

	return FString();
}

void SLogCategoryFilter::OnVerbosityLevelChanged(ELogVerbosityPtr InVerbosity, ESelectInfo::Type, int32 FilterIndex)
{
	if (InVerbosity.IsValid() && FilterIndex >= 0 && FilterIndex < Filters.Num())
	{
		Filters[FilterIndex].Verbosity = *InVerbosity;

		ClosePopup();
	}
}

TSharedRef<SWidget> SLogCategoryFilter::GenerateVerbosityLevelItem(ELogVerbosityPtr InVerbosity)
{
	return SNew(STextBlock) 
		.Text(GetVerbosityLevelText(InVerbosity));
}

FString SLogCategoryFilter::GetVerbosityLevelText(ELogVerbosityPtr InVerbosity) const
{
	if (InVerbosity.IsValid())
	{
		return FString(FOutputDevice::VerbosityToString(*InVerbosity));
	}

	return FString();
}

FString SLogCategoryFilter::GetVerbosityLevelText(int32 FilterIndex) const
{
	if (FilterIndex >= 0 && FilterIndex < Filters.Num())
	{
		return FString(FOutputDevice::VerbosityToString(Filters[FilterIndex].Verbosity));
	}

	return FString();
}

FReply SLogCategoryFilter::LaunchPopUp_OnClicked(int32 FilterIndex)
{
	FText DefaultText;
	if (FilterIndex >= 0 && FilterIndex < Filters.Num())
	{
		DefaultText = FText::FromString(Filters[FilterIndex].CategoryName);
	}

	TSharedRef<STextEntryPopup> TextEntry = SAssignNew(PopupInput, STextEntryPopup)
		.ClearKeyboardFocusOnCommit(false)
		.OnTextCommitted(this, &SLogCategoryFilter::OnPopupTextCommitted, FilterIndex)
		.DefaultText(DefaultText);

	TSharedRef<SHorizontalBox> PopupContent = SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			TextEntry
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(SupportedVerbosityLevelsCombo, SComboBox<ELogVerbosityPtr>)
			.OptionsSource(&SupportedVerbosityLevels)
			.OnSelectionChanged(this, &SLogCategoryFilter::OnVerbosityLevelChanged, FilterIndex)
			.OnGenerateWidget(this, &SLogCategoryFilter::GenerateVerbosityLevelItem)
			[
				SNew(STextBlock)
				.Text(this, &SLogCategoryFilter::GetVerbosityLevelText, FilterIndex)
			]
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(FString(TEXT("-")))
			.OnClicked(this, &SLogCategoryFilter::RemoveFilter_OnClicked, FilterIndex)
		];

	PopupWindow = FSlateApplication::Get().PushMenu( 
		AsShared(),
		PopupContent,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
		);

	TextEntry->FocusDefaultWidget();

	return FReply::Handled();
}

void SLogCategoryFilter::OnPopupTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo, int32 FilterIndex)
{
	if ((CommitInfo == ETextCommit::OnEnter) && (NewText.ToString().Len() > 0))
	{
		if (FilterIndex >= 0 && FilterIndex < Filters.Num())
		{
			Filters[FilterIndex].CategoryName = NewText.ToString();
		}

		ClosePopup();
	}
}

void SLogCategoryFilter::ClosePopup()
{
	if (PopupWindow.IsValid())
	{
		PopupWindow->RequestDestroyWindow();

		OnFiltersChanged.Broadcast();
	}
}

bool SLogCategoryFilter::CanShowLogEntry(TSharedPtr<FVisLogEntry> LogEntry) const
{
	if (LogEntry.IsValid())
	{
		for (int32 i = 0; i < LogEntry->LogLines.Num(); ++i)
		{
			if (CanShowLogLine(LogEntry->LogLines[i].Category.ToString(), LogEntry->LogLines[i].Verbosity))
			{
				return true;
			}
		}
	}

	return false;
}

bool SLogCategoryFilter::CanShowLogLine(FString LogCategory, ELogVerbosity::Type LogVerbosity) const
{
	if (Filters.Num() == 0)
	{
		return true;
	}

	for (int32 i = 0; i < Filters.Num(); ++i)
	{
		if (Filters[i].CategoryName == LogCategory)
		{
			if (Filters[i].Verbosity >= LogVerbosity)
			{
				return true;
			}
		}
	}

	return false;
}
