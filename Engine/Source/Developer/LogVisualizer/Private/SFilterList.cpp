// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"
#include "SFilterList.h"

/////////////////////
// SLogFilterList
/////////////////////

#define LOCTEXT_NAMESPACE "SLogVisualizer"

/** A class for check boxes in the filter list. If you double click a filter checkbox, you will enable it and disable all others */
class SFilterCheckBox : public SCheckBox
{
public:
	void SetOnFilterDoubleClicked(const FOnClicked& NewFilterDoubleClicked)
	{
		OnFilterDoubleClicked = NewFilterDoubleClicked;
	}

	void SetOnFilterMiddleButtonClicked(const FOnClicked& NewFilterMiddleButtonClicked)
	{
		OnFilterMiddleButtonClicked = NewFilterMiddleButtonClicked;
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnFilterDoubleClicked.IsBound())
		{
			return OnFilterDoubleClicked.Execute();
		}
		else
		{
			return SCheckBox::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
		}
	}

	virtual FReply OnMouseButtonUp(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && OnFilterMiddleButtonClicked.IsBound())
		{
			return OnFilterMiddleButtonClicked.Execute();
		}
		else
		{
			return SCheckBox::OnMouseButtonUp(InMyGeometry, InMouseEvent);
		}
	}

private:
	FOnClicked OnFilterDoubleClicked;
	FOnClicked OnFilterMiddleButtonClicked;
};

/**
* A single filter in the filter list. Can be removed by clicking the remove button on it.
*/
class SLogFilter : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnRequestRemove, const TSharedRef<SLogFilter>& /*FilterToRemove*/);
	DECLARE_DELEGATE_OneParam(FOnRequestEnableOnly, const TSharedRef<SLogFilter>& /*FilterToEnable*/);
	DECLARE_DELEGATE(FOnRequestDisableAll);
	DECLARE_DELEGATE(FOnRequestRemoveAll);

	SLATE_BEGIN_ARGS(SLogFilter){}

		/** If this is an front end filter, this is the filter object */
		SLATE_ARGUMENT(FName, FilterName)

		SLATE_ARGUMENT(FLinearColor, ColorCategory)

		/** Invoked when the filter toggled */
		SLATE_EVENT(SLogFilterList::FOnFilterChanged, OnFilterChanged)

		/** Invoked when a request to remove this filter originated from within this filter */
		SLATE_EVENT(FOnRequestRemove, OnRequestRemove)

		/** Invoked when a request to enable only this filter originated from within this filter */
		SLATE_EVENT(FOnRequestEnableOnly, OnRequestEnableOnly)

		/** Invoked when a request to disable all filters originated from within this filter */
		SLATE_EVENT(FOnRequestDisableAll, OnRequestDisableAll)

		/** Invoked when a request to remove all filters originated from within this filter */
		SLATE_EVENT(FOnRequestRemoveAll, OnRequestRemoveAll)

		SLATE_END_ARGS()

		/** Constructs this widget with InArgs */
		void Construct(const FArguments& InArgs)
		{
			bEnabled = true;
			OnFilterChanged = InArgs._OnFilterChanged;
			OnRequestRemove = InArgs._OnRequestRemove;
			OnRequestEnableOnly = InArgs._OnRequestEnableOnly;
			OnRequestDisableAll = InArgs._OnRequestDisableAll;
			OnRequestRemoveAll = InArgs._OnRequestRemoveAll;
			FilterName = InArgs._FilterName;
			ColorCategory = InArgs._ColorCategory;
			Verbosity = ELogVerbosity::All;

			// Get the tooltip and color of the type represented by this filter
			FilterColor = ColorCategory;

			ChildSlot
				[
					SNew(SBorder)
					.Padding(0)
					.BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 0.2f))
					.BorderImage(FEditorStyle::GetBrush("ContentBrowser.FilterButtonBorder"))
					[
						SAssignNew(ToggleButtonPtr, SFilterCheckBox)
						.Style(FEditorStyle::Get(), "ContentBrowser.FilterButton")
						.ToolTipText(this, &SLogFilter::GetTooltipString)
						.Padding(this, &SLogFilter::GetFilterNamePadding)
						.IsChecked(this, &SLogFilter::IsChecked)
						.OnCheckStateChanged(this, &SLogFilter::FilterToggled)
						/*.OnGetMenuContent(this, &SLogFilter::GetRightClickMenuContent)*/
						.ForegroundColor(this, &SLogFilter::GetFilterForegroundColor)
						[
							SNew(STextBlock)
							.ColorAndOpacity(this, &SLogFilter::GetFilterNameColorAndOpacity)
							.Font(FEditorStyle::GetFontStyle("ContentBrowser.FilterNameFont"))
							.ShadowOffset(FVector2D(1.f, 1.f))
							.Text(this, &SLogFilter::GetCaptionString)
						]
					]
				];

			ToggleButtonPtr->SetOnFilterDoubleClicked(FOnClicked::CreateSP(this, &SLogFilter::FilterDoubleClicked));
			ToggleButtonPtr->SetOnFilterMiddleButtonClicked(FOnClicked::CreateSP(this, &SLogFilter::FilterMiddleButtonClicked));
		}

		FText GetCaptionString() const
		{
			FString CaptionString;
			const FString VerbosityStr = FOutputDevice::VerbosityToString(Verbosity);
			if (Verbosity != ELogVerbosity::VeryVerbose)
			{
				CaptionString = FString::Printf(TEXT("%s [%s]"), *GetFilterNameAsString().Replace(TEXT("Log"), TEXT(""), ESearchCase::CaseSensitive), *VerbosityStr.Mid(0, 1));
			}
			else
			{
				CaptionString = FString::Printf(TEXT("%s [VV]"), *GetFilterNameAsString().Replace(TEXT("Log"), TEXT(""), ESearchCase::CaseSensitive));
			}
			return FText::FromString(CaptionString);
		}

		FText GetTooltipString() const
		{
			return FText::FromString( 
				IsEnabled() ? 
				FString::Printf(TEXT("Enabled '%s' category for '%s' verbosity and lower\nRight click to change verbosity"), *GetFilterNameAsString(), FOutputDevice::VerbosityToString(Verbosity)) 
				: 
				FString::Printf(TEXT("Disabled '%s' category"), *GetFilterNameAsString())
			);
		}
	/** Sets whether or not this filter is applied to the combined filter */
	void SetEnabled(bool InEnabled)
	{
		if (InEnabled != bEnabled)
		{
			bEnabled = InEnabled;
			OnFilterChanged.ExecuteIfBound();
		}
	}

	/** Returns true if this filter contributes to the combined filter */
	bool IsEnabled() const
	{
		return bEnabled;
	}

	void SetVerbosity(ELogVerbosity::Type InVerbosity)
	{
		Verbosity = InVerbosity;
	}

	ELogVerbosity::Type GetVerbosity() const
	{
		return Verbosity;
	}

	/** Returns the display name for this filter */
	FORCEINLINE FString GetFilterNameAsString() const
	{
		if (FilterName == NAME_None)
		{
			return TEXT("UnknownFilter");
		}

		return FilterName.ToString();
	}

	FORCEINLINE FName GetFilterName() const
	{
		return FilterName;
	}

private:
	/** Handler for when the filter checkbox is clicked */
	void FilterToggled(ESlateCheckBoxState::Type NewState)
	{
		bEnabled = NewState == ESlateCheckBoxState::Checked;
		OnFilterChanged.ExecuteIfBound();
	}

	/** Handler for when the filter checkbox is double clicked */
	FReply FilterDoubleClicked()
	{
		// Disable all other filters and enable this one.
		OnRequestDisableAll.ExecuteIfBound();
		bEnabled = true;
		OnFilterChanged.ExecuteIfBound();

		return FReply::Handled();
	}

	/** Handler for when the filter checkbox is middle button clicked */
	FReply FilterMiddleButtonClicked()
	{
		RemoveFilter();
		return FReply::Handled();
	}

	/** Handler to create a right click menu */
	TSharedRef<SWidget> GetRightClickMenuContent()
	{
		FMenuBuilder MenuBuilder(true, NULL);

		if (IsEnabled())
		{
			FText FiletNameAsText = FText::FromString(GetFilterNameAsString());
			MenuBuilder.BeginSection("VerbositySelection", LOCTEXT("VerbositySelection", "Current verbosity selection"));
			{
				for (int32 Index = ELogVerbosity::NoLogging+1; Index <= ELogVerbosity::VeryVerbose; Index++)
				{
					const FString VerbosityStr = FOutputDevice::VerbosityToString((ELogVerbosity::Type)Index);
					MenuBuilder.AddMenuEntry(
						FText::Format(LOCTEXT("UseVerbosity", "Use: {0}"), FText::FromString(VerbosityStr)),
						LOCTEXT("UseVerbosityTooltip", "Applay verbosity to selected flter."),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateSP(this, &SLogFilter::SetVerbosityFilter, Index))
						);
				}
			}
			MenuBuilder.EndSection();
		}

		return MenuBuilder.MakeWidget();
	}

	/** Removes this filter from the filter list */
	void SetVerbosityFilter(int32 SelectedVerbosityIndex)
	{
		Verbosity = (ELogVerbosity::Type)SelectedVerbosityIndex;
		OnFilterChanged.ExecuteIfBound();
	}

	void RemoveFilter()
	{
		TSharedRef<SLogFilter> Self = SharedThis(this);
		OnRequestRemove.ExecuteIfBound(Self);
	}

	/** Enables only this filter from the filter list */
	void EnableOnly()
	{
		TSharedRef<SLogFilter> Self = SharedThis(this);
		OnRequestEnableOnly.ExecuteIfBound(Self);
	}

	/** Disables all active filters in the list */
	void DisableAllFilters()
	{
		OnRequestDisableAll.ExecuteIfBound();
	}

	/** Removes all filters in the list */
	void RemoveAllFilters()
	{
		OnRequestRemoveAll.ExecuteIfBound();
	}

	/** Handler to determine the "checked" state of the filter checkbox */
	ESlateCheckBoxState::Type IsChecked() const
	{
		return bEnabled ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	/** Handler to determine the color of the checkbox when it is checked */
	FSlateColor GetFilterForegroundColor() const
	{
		return IsChecked() ? FilterColor : FLinearColor::White;
	}

	/** Handler to determine the padding of the checkbox text when it is pressed */
	FMargin GetFilterNamePadding() const
	{
		return ToggleButtonPtr->IsPressed() ? FMargin(3, 2, 4, 0) : FMargin(3, 1, 4, 1);
	}

	/** Handler to determine the color of the checkbox text when it is hovered */
	FSlateColor GetFilterNameColorAndOpacity() const
	{
		const float DimFactor = 0.75f;	
		return IsChecked() ? (IsHovered() ? ColorCategory * DimFactor : ColorCategory) : (IsHovered() ? FLinearColor::White : FLinearColor::White * DimFactor);
	}

private:
	/** Invoked when the filter toggled */
	SLogFilterList::FOnFilterChanged OnFilterChanged;

	/** Invoked when a request to remove this filter originated from within this filter */
	FOnRequestRemove OnRequestRemove;

	/** Invoked when a request to enable only this filter originated from within this filter */
	FOnRequestEnableOnly OnRequestEnableOnly;

	/** Invoked when a request to disable all filters originated from within this filter */
	FOnRequestDisableAll OnRequestDisableAll;

	/** Invoked when a request to remove all filters originated from within this filter */
	FOnRequestDisableAll OnRequestRemoveAll;

	/** true when this filter should be applied to the search */
	bool bEnabled;

	///** The asset type actions that are associated with this filter */
	//TWeakPtr<IAssetTypeActions> AssetTypeActions;

	///** If this is an front end filter, this is the filter object */
	//TSharedPtr<FFrontendFilter> FrontendFilter;

	FName FilterName;

	FLinearColor ColorCategory;

	ELogVerbosity::Type Verbosity;

	/** The button to toggle the filter on or off */
	TSharedPtr<SFilterCheckBox> ToggleButtonPtr;

	/** The color of the checkbox for this filter */
	FLinearColor FilterColor;
};

void SLogFilterList::Construct(const FArguments& InArgs)
{
	OnGetContextMenu = InArgs._OnGetContextMenu;
	OnFilterChanged = InArgs._OnFilterChanged;

	ChildSlot
		[
			SAssignNew(FilterBox, SWrapBox)
			.UseAllottedWidth(true)
		];

	TSharedRef<SComboButton> GraphsFilterCombo =
		SNew(SComboButton)
		.ComboButtonStyle(FEditorStyle::Get(), "ContentBrowser.Filters.Style")
		.ForegroundColor(FLinearColor::White)
		.ContentPadding(0)
		.ToolTipText(LOCTEXT("AddFilterToolTip", "Add an asset filter."))
		.OnGetMenuContent(this, &SLogFilterList::MakeGraphsFilterMenu)
		.HasDownArrow(true)
		.ContentPadding(FMargin(1, 0))
		.ButtonContent()
		[
			SNew(STextBlock)
			.TextStyle(FEditorStyle::Get(), "ContentBrowser.Filters.Text")
			.Text(LOCTEXT("GraphFilters", "Graph Filters"))
		];

	FilterBox->AddSlot()
		.Padding(3, 3)
		[
			GraphsFilterCombo
		];

}

void SLogFilterList::CreateFiltersMenuCategoryForGraph(FMenuBuilder& MenuBuilder, FName MenuCategory) const
{
	auto FiltersFromGraph = GraphFilters[MenuCategory];
	for (auto Iter = FiltersFromGraph.CreateIterator(); Iter; ++Iter)
	{
		const FText& LabelText = FText::FromString(Iter->Name.ToString());
		MenuBuilder.AddMenuEntry(
			LabelText,
			FText::Format(LOCTEXT("FilterByTooltipPrefix", "Filter by {0}"), LabelText),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SLogFilterList::FilterByTypeClicked, MenuCategory, Iter->Name),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SLogFilterList::IsAssetTypeActionsInUse, MenuCategory, Iter->Name)),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
			);
	}
}

void SLogFilterList::FilterByTypeClicked(FName InGraphName, FName InDataName)
{
	if (GraphFilters.Contains(InGraphName))
	{
		bool bChanged = false;
		auto &Filters = GraphFilters[InGraphName];
		for (int32 Index = 0; Index < Filters.Num(); ++Index)
		{
			if (Filters[Index].Name == InDataName)
			{
				Filters[Index].bEnabled = !Filters[Index].bEnabled;
				bChanged = true;
			}
		}

		if (bChanged)
		{
			SomeFilterGetChanged();
		}
	}
}

bool SLogFilterList::IsAssetTypeActionsInUse(FName InGraphName, FName InDataName) const
{
	if (GraphFilters.Contains(InGraphName))
	{
		auto &Filters = GraphFilters[InGraphName];
		for (int32 Index = 0; Index < Filters.Num(); ++Index)
		{
			if (Filters[Index].Name == InDataName)
			{
				return Filters[Index].bEnabled;
			}
		}
	}

	return false;
}

void SLogFilterList::GraphFilterCategoryClicked(FName MenuCategory)
{
	const bool bNewSet =! IsGraphFilterCategoryInUse(MenuCategory);

	if (GraphFilters.Contains(MenuCategory))
	{
		bool bChanged = false;
		auto &Filters = GraphFilters[MenuCategory];
		for (int32 Index = 0; Index < Filters.Num(); ++Index)
		{
			Filters[Index].bEnabled = bNewSet;
			bChanged = true;
		}

		if (bChanged)
		{
			SomeFilterGetChanged();
		}
	}
}

bool SLogFilterList::IsGraphFilterCategoryInUse(FName MenuCategory) const
{
	if (GraphFilters.Contains(MenuCategory))
	{
		auto &Filters = GraphFilters[MenuCategory];
		for (int32 Index = 0; Index < Filters.Num(); ++Index)
		{
			if (Filters[Index].bEnabled)
			{
				return true;
			}
		}
	}

	return false;
}

TSharedRef<SWidget> SLogFilterList::MakeGraphsFilterMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	MenuBuilder.BeginSection( TEXT("Graphs"));
	{
		for (auto Iter = GraphFilters.CreateIterator(); Iter; ++Iter)
		{
			const FText& LabelText = FText::FromString(Iter->Key.ToString());
			MenuBuilder.AddSubMenu(
				LabelText,
					FText::Format(LOCTEXT("FilterByTooltipPrefix", "Filter by {0}"), LabelText),
					FNewMenuDelegate::CreateSP(this, &SLogFilterList::CreateFiltersMenuCategoryForGraph, Iter->Key),
					FUIAction(
					FExecuteAction::CreateSP(this, &SLogFilterList::GraphFilterCategoryClicked, Iter->Key),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(this, &SLogFilterList::IsGraphFilterCategoryInUse, Iter->Key)),
					NAME_None,
					EUserInterfaceActionType::ToggleButton
					);
		}
	}
	MenuBuilder.EndSection(); //ContentBrowserFilterBasicAsset
	

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	const FVector2D DisplaySize(
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Right - DisplayMetrics.PrimaryDisplayWorkAreaRect.Left,
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - DisplayMetrics.PrimaryDisplayWorkAreaRect.Top);

	return
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.MaxHeight(DisplaySize.Y * 0.5)
		[
			MenuBuilder.MakeWidget()
		];
}

void SLogFilterList::SomeFilterGetChanged()
{
	OnFilterChanged.ExecuteIfBound();
}

void SLogFilterList::AddFilter(const FString& InFilterName, FLinearColor InColorCategory)
{
	TSharedRef<SLogFilter> NewFilter =
		SNew(SLogFilter)
		.FilterName(*InFilterName)
		.ColorCategory(InColorCategory)
		.OnFilterChanged(this, &SLogFilterList::SomeFilterGetChanged)
		//.OnRequestRemove(this, &SLogFilterList::RemoveFilter)
		//.OnRequestDisableAll(this, &SLogFilterList::DisableAllFilters)
		//.OnRequestRemoveAll(this, &SLogFilterList::RemoveAllFilters);
		;
	NewFilter->SetEnabled(true);
	Filters.Add(NewFilter);

	FilterBox->AddSlot()
		.Padding(3, 3)
		[
			NewFilter
		];
}

void SLogFilterList::AddGraphFilter(const FString& InGraphName, const FString& InDataName, FLinearColor ColorCategory)
{
	if (!GraphFilters.Contains(*InGraphName))
	{
		auto Filters = GraphFilters.Add(*InGraphName);
	}
	else
	{
		auto &Filters = GraphFilters[*InGraphName];
		FSimpleGraphFilter NewFilter(*InDataName, true);
		if (!Filters.Contains(NewFilter))
		{
			Filters.Add(NewFilter);
		}
	}

}

bool SLogFilterList::IsFilterEnabled(const FString& InGraphName, const FString& InDataName, TEnumAsByte<ELogVerbosity::Type> Verbosity)
{
	if (GraphFilters.Contains(*InGraphName))
	{
		auto Filters = GraphFilters[*InGraphName];
		int32 Index = Filters.Find(FSimpleGraphFilter(*InDataName));
		if (Index != INDEX_NONE)
		{
			return Filters[Index].bEnabled;
		}
	}

	return true;
}

bool SLogFilterList::IsFilterEnabled(const FString& InFilterName, TEnumAsByte<ELogVerbosity::Type> Verbosity)
{
	const FName FilterName(*InFilterName);
	for (int32 Index = 0; Index < Filters.Num(); ++Index)
	{
		const SLogFilter& Filter = Filters[Index].Get();
		if (Filter.GetFilterName() == FilterName)
		{
			return Filter.IsEnabled() && Filter.GetVerbosity() >= Verbosity.GetValue();
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
