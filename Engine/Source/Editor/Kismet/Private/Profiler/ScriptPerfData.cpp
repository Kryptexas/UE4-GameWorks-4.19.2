// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "ScriptPerfData.h"

//////////////////////////////////////////////////////////////////////////
// FScriptPerfData

FNumberFormattingOptions FScriptPerfData::StatNumberFormat;
bool FScriptPerfData::bAverageBlueprintStats = true;
bool FScriptPerfData::bUseRecentSampleBias = false;
float FScriptPerfData::RecentSampleBias = 0.2f;
float FScriptPerfData::HistoricalSampleBias = 0.8f;
float FScriptPerfData::EventPerformanceThreshold = 1.f;
float FScriptPerfData::ExclusivePerformanceThreshold = 0.2f;
float FScriptPerfData::InclusivePerformanceThreshold = 0.25f;
float FScriptPerfData::MaxPerformanceThreshold = 0.5f;
float FScriptPerfData::NodeTotalTimeWaterMark = 0.f;

void FScriptPerfData::AddEventTiming(const double ExclusiveTimingIn)
{
	const double TimingInMs = ExclusiveTimingIn * 1000;
	ExclusiveTiming = (TimingInMs * RecentSampleBias) + (ExclusiveTiming * HistoricalSampleBias);
	MaxTiming = FMath::Max<double>(MaxTiming, ExclusiveTiming);
	MinTiming = FMath::Min<double>(MinTiming, ExclusiveTiming);
	TotalTiming += ExclusiveTimingIn;
	NumSamples++;
}

void FScriptPerfData::AddInclusiveTiming(const double InclusiveNodeTimingIn, const bool bIncrementSamples, const bool bDiscreteTiming)
{
	// Discrete timings should negate the discrete nodetimings so they sum correctly later.
	const double TimingInMs = bDiscreteTiming ? ((InclusiveNodeTimingIn * 1000) - ExclusiveTiming) : (InclusiveNodeTimingIn * 1000);
	InclusiveTiming = (TimingInMs * RecentSampleBias) + (InclusiveTiming * HistoricalSampleBias);
	TotalTiming += InclusiveNodeTimingIn;
	if (bIncrementSamples)
	{
		NumSamples++;
	}
}

void FScriptPerfData::AddData(const FScriptPerfData& DataIn)
{
	if (DataIn.IsDataValid())
	{
		// Accumulate data, find min, max values
		ExclusiveTiming += DataIn.ExclusiveTiming;
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
		// Accumulate all data with the exception of samples for which we take the first valid value.
		const bool bFirstSample = MaxTiming == -MAX_dbl;
		ExclusiveTiming += DataIn.ExclusiveTiming;
		if (DataIn.InclusiveTiming > 0.0)
		{
			InclusiveTiming += DataIn.InclusiveTiming;
		}
		TotalTiming += DataIn.TotalTiming;
		if (bFirstSample)
		{
			NumSamples = DataIn.NumSamples;
			MaxTiming = DataIn.MaxTiming;
			MinTiming = DataIn.MinTiming;
		}
		else
		{
			MaxTiming += DataIn.MaxTiming;
			MinTiming += DataIn.MinTiming;
		}
	}
}

void FScriptPerfData::InitialiseFromDataSet(const TArray<TSharedPtr<FScriptPerfData>>& DataSet)
{
	Reset();
	for (auto DataPoint : DataSet)
	{
		if (DataPoint.IsValid())
		{
			ExclusiveTiming += DataPoint->ExclusiveTiming;
			InclusiveTiming += DataPoint->InclusiveTiming;
			TotalTiming += DataPoint->TotalTiming;
			// Either sum the samples to get the average or use the highest sample rate encountered in the dataset.
			NumSamples = bAverageBlueprintStats ? (NumSamples + DataPoint->NumSamples) : FMath::Max(NumSamples, DataPoint->NumSamples);
			MaxTiming = FMath::Max<double>(MaxTiming, DataPoint->MaxTiming);
			MinTiming = FMath::Min<double>(MinTiming, DataPoint->MinTiming);
		}
	}
}

void FScriptPerfData::Reset()
{
	ExclusiveTiming = 0.0;
	InclusiveTiming = 0.0;
	TotalTiming = 0.0;
	HottestPathHeatValue = 0.f;
	NumSamples = 0;
	MaxTiming = -MAX_dbl;
	MinTiming = MAX_dbl;
}

double FScriptPerfData::GetExclusiveTiming() const
{
	return bUseRecentSampleBias ? ExclusiveTiming : ((TotalTiming*1000.0)/NumSamples);
}

void FScriptPerfData::EnableBlueprintStatAverage(const bool bEnable)
{
	bAverageBlueprintStats = bEnable;
}

void FScriptPerfData::EnableRecentSampleBias(const bool bEnable)
{
	bUseRecentSampleBias = bEnable;
}

void FScriptPerfData::SetRecentSampleBias(const float RecentSampleBiasIn)
{
	RecentSampleBias = RecentSampleBiasIn;
	HistoricalSampleBias = 1.f - RecentSampleBias;
}

void FScriptPerfData::SetEventPerformanceThreshold(const float EventPerformanceThresholdIn)
{
	EventPerformanceThreshold = 1.f / EventPerformanceThresholdIn;
}

void FScriptPerfData::SetExclusivePerformanceThreshold(const float ExclusivePerformanceThresholdIn)
{
	ExclusivePerformanceThreshold = 1.f / ExclusivePerformanceThresholdIn;
}

float FScriptPerfData::GetNodeHeatLevel() const
{
	const float PerfMetric = StatType == EScriptPerfDataType::Event ? EventPerformanceThreshold : ExclusivePerformanceThreshold;
	return FMath::Min<float>(GetExclusiveTiming() * PerfMetric, 1.f);
}

void FScriptPerfData::SetInclusivePerformanceThreshold(const float InclusivePerformanceThresholdIn)
{
	InclusivePerformanceThreshold = 1.f / InclusivePerformanceThresholdIn;
}

float FScriptPerfData::GetInclusiveHeatLevel() const
{
	const float PerfMetric = StatType == EScriptPerfDataType::Event ? EventPerformanceThreshold : InclusivePerformanceThreshold;
	return FMath::Min<float>(GetInclusiveTiming() * InclusivePerformanceThreshold, 1.f);
}

void FScriptPerfData::SetMaxPerformanceThreshold(const float MaxPerformanceThresholdIn)
{
	MaxPerformanceThreshold = 1.f / MaxPerformanceThresholdIn;
}

float FScriptPerfData::GetMaxTimeHeatLevel() const
{
	return FMath::Min<float>(MaxTiming * MaxPerformanceThreshold, 1.f);
}

float FScriptPerfData::GetTotalHeatLevel() const
{
	NodeTotalTimeWaterMark = FMath::Max<float>(TotalTiming, NodeTotalTimeWaterMark);
	return FMath::Min<float>(TotalTiming / NodeTotalTimeWaterMark, 1.f);
}

FText FScriptPerfData::GetExclusiveTimingText() const
{
	const double Value = GetExclusiveTiming();
	if (Value > 0.0)
	{
		return FText::AsNumber(Value, &StatNumberFormat);
	}
	return FText::GetEmpty();
}

FText FScriptPerfData::GetInclusiveTimingText() const
{
	if (NumSamples > 0)
	{
		const double InclusiveTemp = GetInclusiveTiming();
		return FText::AsNumber(InclusiveTemp, &StatNumberFormat);
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