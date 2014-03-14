// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

DECLARE_MULTICAST_DELEGATE(FOnFiltersChanged);

struct FLogCategoryType
{
	FString CategoryName;

	ELogVerbosity::Type Verbosity;

	FLogCategoryType()
		: CategoryName()
		, Verbosity(ELogVerbosity::All)
	{

	}

	inline bool operator==(const FLogCategoryType& Other) const
	{
		return (CategoryName == Other.CategoryName);
	}
};

class SLogCategoryFilter : public SCompoundWidget
{
	typedef TSharedPtr<ELogVerbosity::Type> ELogVerbosityPtr;

public:

	SLATE_BEGIN_ARGS(SLogCategoryFilter) {}
	SLATE_END_ARGS()

	FOnFiltersChanged OnFiltersChanged;

	bool CanShowLogEntry(TSharedPtr<FVisLogEntry> LogEntry) const;

	bool CanShowLogLine(FString LogCategory, ELogVerbosity::Type LogVerbosity) const;

	void Construct(const FArguments& InArgs);

private:

	ELogVerbosity::Type GetVerbosityFromString(FString VerbosityStr) const;

	void GetDefaultFilters();

	void GenerateFilters();

	void GenerateSupportedVerbosityLevels();

	FString GetFilterText(int32 FilterIndex) const;

	FReply LaunchPopUp_OnClicked(int32 FilterIndex);

	void OnPopupTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo, int32 FilterIndex);

	void OnVerbosityLevelChanged(ELogVerbosityPtr InVerbosity, ESelectInfo::Type, int32 FilterIndex);

	TSharedRef<SWidget> GenerateVerbosityLevelItem(ELogVerbosityPtr InVerbosity);

	FString GetVerbosityLevelText(ELogVerbosityPtr InVerbosity) const;

	FString GetVerbosityLevelText(int32 FilterIndex) const;

	void ClosePopup();

	FReply AddFilter_OnClicked();

	FReply RemoveFilter_OnClicked(int32 FilterIndex);

	TArray<ELogVerbosityPtr> SupportedVerbosityLevels;

	TSharedPtr<SComboBox<ELogVerbosityPtr> > SupportedVerbosityLevelsCombo;

	TArray<FLogCategoryType> Filters;

	TSharedPtr<SHorizontalBox> FiltersBar;

	TSharedPtr<STextEntryPopup> PopupInput;

	TSharedPtr<SWindow> PopupWindow;
};