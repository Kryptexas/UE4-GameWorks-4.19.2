// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "ScriptPerfData.h"

//////////////////////////////////////////////////////////////////////////
// FScriptPerfData

FNumberFormattingOptions FScriptPerfData::StatNumberFormat;
float FScriptPerfData::RecentSampleBias = 0.2f;
float FScriptPerfData::HistoricalSampleBias = 0.8f;
float FScriptPerfData::NodePerformanceThreshold = 1.f;
float FScriptPerfData::InclusivePerformanceThreshold = 1.5f;
float FScriptPerfData::MaxPerformanceThreshold = 2.f;

void FScriptPerfData::AddEventTiming(const double NodeTimingIn)
{
	const double TimingInMs = NodeTimingIn * 1000;
	NodeTiming = (TimingInMs * RecentSampleBias) + (NodeTiming * HistoricalSampleBias);
	MaxTiming = FMath::Max<double>(MaxTiming, NodeTiming);
	MinTiming = FMath::Min<double>(MinTiming, NodeTiming);
	TotalTiming += NodeTimingIn;
	NumSamples++;
}

void FScriptPerfData::AddPureChainTiming(const double PureNodeTimingIn)
{
	const double PureTimingInMs = PureNodeTimingIn * 1000;
	const double NewInclusiveTimingInMs = NodeTiming + PureTimingInMs;
	InclusiveTiming = (NewInclusiveTimingInMs * RecentSampleBias) + (InclusiveTiming * HistoricalSampleBias);
	TotalTiming += PureNodeTimingIn;
	MaxTiming = FMath::Max<double>(MaxTiming, InclusiveTiming);
	MinTiming = FMath::Min<double>(MinTiming, InclusiveTiming);
}

void FScriptPerfData::AddInclusiveTiming(const double InclusiveNodeTimingIn)
{
	const double TimingInMs = InclusiveNodeTimingIn * 1000;
	InclusiveTiming = (TimingInMs * RecentSampleBias) + (InclusiveTiming * HistoricalSampleBias);
	TotalTiming += InclusiveNodeTimingIn;
	NumSamples++;
	MaxTiming = FMath::Max<double>(MaxTiming, InclusiveTiming);
	MinTiming = FMath::Min<double>(MinTiming, InclusiveTiming);
}

void FScriptPerfData::AddData(const FScriptPerfData& DataIn)
{
	if (DataIn.IsDataValid())
	{
		// Accumulate data, find min, max values
		NodeTiming += DataIn.NodeTiming;
		if (DataIn.InclusiveTiming > 0.0)
		{
			InclusiveTiming += DataIn.InclusiveTiming;
		}
		TotalTiming += DataIn.TotalTiming;
		NumSamples += DataIn.NumSamples;
		MaxTiming = FMath::Max<double>(MaxTiming, DataIn.MaxTiming);
		MinTiming = FMath::Min<double>(MinTiming, DataIn.MinTiming);
	}
}

void FScriptPerfData::AddBranchData(const FScriptPerfData& DataIn)
{
	if (DataIn.IsDataValid())
	{
		NumSamples += DataIn.NumSamples;
	}
}

void FScriptPerfData::Reset()
{
	NodeTiming = 0.0;
	InclusiveTiming = 0.0;
	TotalTiming = 0.0;
	NumSamples = 0;
	MaxTiming = -MAX_dbl;
	MinTiming = MAX_dbl;
}

void FScriptPerfData::SetRecentSampleBias(const float RecentSampleBiasIn)
{
	RecentSampleBias = RecentSampleBiasIn;
	HistoricalSampleBias = 1.f - RecentSampleBias;
}

void FScriptPerfData::SetNodePerformanceThreshold(const float NodePerformanceThresholdIn)
{
	NodePerformanceThreshold = NodePerformanceThresholdIn;
}

FSlateColor FScriptPerfData::GetNodeHeatColor() const
{
	const float Value = 1.f - FMath::Min<float>(NodeTiming / NodePerformanceThreshold, 1.f);
	return FLinearColor(1.f, Value, Value);
}

void FScriptPerfData::SetInclusivePerformanceThreshold(const float InclusivePerformanceThresholdIn)
{
	InclusivePerformanceThreshold = InclusivePerformanceThresholdIn;
}

FSlateColor FScriptPerfData::GetInclusiveHeatColor() const
{
	const float Value = 1.f - FMath::Min<float>(InclusiveTiming / InclusivePerformanceThreshold, 1.f);
	return FLinearColor(1.f, Value, Value);
}

void FScriptPerfData::SetMaxPerformanceThreshold(const float MaxPerformanceThresholdIn)
{
	MaxPerformanceThreshold = MaxPerformanceThresholdIn;
}

FSlateColor FScriptPerfData::GetMaxTimeHeatColor() const
{
	const float Value = 1.f - FMath::Min<float>(MaxTiming / MaxPerformanceThreshold, 1.f);
	return FLinearColor(1.f, Value, Value);
}

FText FScriptPerfData::GetNodeTimingText() const
{
	if (NodeTiming > 0.0)
	{
		return FText::AsNumber(NodeTiming, &StatNumberFormat);
	}
	return FText::GetEmpty();
}

FText FScriptPerfData::GetInclusiveTimingText() const
{
	if (InclusiveTiming > 0.0)
	{
		return FText::AsNumber(InclusiveTiming, &StatNumberFormat);
	}
	return FText::GetEmpty();
}

FText FScriptPerfData::GetMaxTimingText() const
{
	if (MaxTiming != -MAX_dbl)
	{
		return FText::AsNumber(MaxTiming, &StatNumberFormat);
	}
	return FText::GetEmpty();
}

FText FScriptPerfData::GetMinTimingText() const
{
	if (MinTiming != MAX_dbl)
	{
		return FText::AsNumber(MinTiming, &StatNumberFormat);
	}
	return FText::GetEmpty();
}

FText FScriptPerfData::GetTotalTimingText() const
{
	if (TotalTiming > 0.0)
	{
		return FText::AsNumber(TotalTiming, &StatNumberFormat);
	}
	return FText::GetEmpty();
}

FText FScriptPerfData::GetSamplesText() const
{
	if (NumSamples > 0)
	{
		return FText::AsNumber(NumSamples);
	}
	return FText::GetEmpty();
}