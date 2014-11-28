// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* A list of filters currently applied to an asset view.
*/
class STimeline : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STimeline){}
		SLATE_EVENT(FOnItemSelectionChanged, OnItemSelectionChanged)
		SLATE_ATTRIBUTE(TSharedPtr<IVisualLoggerInterface>, VisualLoggerInterface)
	SLATE_END_ARGS();

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnSelect();
	virtual void OnDeselect();

	/** Constructs this widget with InArgs */
	virtual ~STimeline();
	void Construct(const FArguments& InArgs, TSharedPtr<class SVisualLoggerView> VisualLoggerView, TSharedPtr<class FSequencerTimeSliderController> InTimeSliderController, TSharedPtr<class STimelinesContainer> InContainer, const FVisualLogDevice::FVisualLogEntryItem& Entry);
	bool IsSelected() const;
	bool IsEntryHidden(const FVisualLogDevice::FVisualLogEntryItem&) const;
	const FSlateBrush* GetBorder() const;

	void OnFiltersChanged();
	void OnSearchChanged(const FText& Filter);
	void UpdateVisibility();
	void AddEntry(const FVisualLogDevice::FVisualLogEntryItem& Entry) { Entries.Add(Entry); }
	const TArray<FVisualLogDevice::FVisualLogEntryItem>& GetEntries() { return Entries; }
	FName GetName() { return Name; }
	void HandleLogVisualizerSettingChanged(FName Name);

protected:
	TArray<FVisualLogDevice::FVisualLogEntryItem> Entries;
	TArray<const FVisualLogDevice::FVisualLogEntryItem*> HiddenEntries;
	TSharedPtr<class STimelinesContainer> Owner;
	TSharedPtr<class STimelineBar> TimelineBar;
	TSharedPtr<IVisualLoggerInterface> VisualLoggerInterface;

	FName Name;
	FString SearchFilter;
};
