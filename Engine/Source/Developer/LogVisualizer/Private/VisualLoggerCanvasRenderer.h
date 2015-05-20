// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

struct FVisualLoggerCanvasRenderer
{
public:
	FVisualLoggerCanvasRenderer() : bDirtyData(true)
	{}

	void DrawOnCanvas(class UCanvas* Canvas, class APlayerController*);
	void OnItemSelectionChanged(const struct FVisualLogEntry& EntryItem);
	void ObjectSelectionChanged(TSharedPtr<class STimeline> TimeLine);
	void DirtyCachedData() { bDirtyData = true; }

protected:
	void DrawHistogramGraphs(class UCanvas* Canvas, class APlayerController* );

private:
	bool bDirtyData;
	FVisualLogEntry SelectedEntry;
	TMap<FString, TArray<FString> > UsedGraphCategories;
	TWeakPtr<class STimeline> CurrentTimeLine;

	TArray<FVisualLogDataBlock> CachedDataBlocks;
	TArray<TArray<FVisualLogHistogramSample> > CachedHistogramSamples;
};
