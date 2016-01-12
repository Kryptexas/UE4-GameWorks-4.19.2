// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Widgets/SCompoundWidget.h"

class FVisualLoggerTimeSliderController;
class SVisualLoggerView;
class STimeline;
class STimelinesBar;

/**
* A list of filters currently applied to an asset view.
*/
class STimelinesContainer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STimelinesContainer){}
	SLATE_END_ARGS()

	virtual ~STimelinesContainer();

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, TSharedRef<SVisualLoggerView>, TSharedRef<FVisualLoggerTimeSliderController> TimeSliderController);
	TSharedRef<SWidget> GetRightClickMenuContent();

	void OnTimelineSelected(TSharedPtr<STimelinesBar> Widget);
	void ChangeSelection(TSharedPtr<STimeline>, const FPointerEvent& MouseEvent);
	virtual bool SupportsKeyboardFocus() const override { return true; }

	void OnFiltersChanged();
	void OnSearchChanged(const FText& Filter);
	void OnFiltersSearchChanged(const FText& Filter);

	void ResetData();

	void GenerateReport();

	/**
	* Selects a node in the tree
	*
	* @param AffectedNode			The node to select
	* @param bSelect				Whether or not to select
	* @param bDeselectOtherNodes	Whether or not to deselect all other nodes
	*/
	void SetSelectionState(TSharedPtr<STimeline> AffectedNode, bool bSelect, bool bDeselectOtherNodes = true);

	/**
	* @return All currently selected nodes
	*/
	const TArray< TSharedPtr<STimeline> >& GetSelectedNodes() const { return CachedSelectedTimelines; }

	const TArray< TSharedPtr<STimeline> >& GetAllNodes() const { return TimelineItems; }

	/**
	* Returns whether or not a node is selected
	*
	* @param Node	The node to check for selection
	* @return true if the node is selected
	*/
	bool IsNodeSelected(TSharedPtr<STimeline> Node) const;

protected:
	void OnNewRowHandler(const FVisualLoggerDBRow& DBRow);
	void OnNewItemHandler(const FVisualLoggerDBRow& BDRow, int32 ItemIndex);
	void OnObjectSelectionChanged(const TArray<FName>& RowNames);
	void OnRowChangedVisibility(const FName&);

protected:
	TSharedPtr<SVerticalBox> ContainingBorder;
	TSharedPtr<FVisualLoggerTimeSliderController> TimeSliderController;
	TSharedPtr<SVisualLoggerView> VisualLoggerView;

	TArray<TSharedPtr<STimeline> > TimelineItems;
	TArray<TSharedPtr<STimeline> > CachedSelectedTimelines;
	float CachedMinTime, CachedMaxTime;

	FText CurrentSearchText;
};
