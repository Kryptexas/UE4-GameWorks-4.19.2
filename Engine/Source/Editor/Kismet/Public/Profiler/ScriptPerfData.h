// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// EScriptPerfDataType

enum EScriptPerfDataType
{
	Event = 0,
	Node
};

//////////////////////////////////////////////////////////////////////////
// FScriptPerfData

class KISMET_API FScriptPerfData : public TSharedFromThis<FScriptPerfData>
{
public:

	FScriptPerfData(const EScriptPerfDataType StatTypeIn)
		: StatType(StatTypeIn)
		, ExclusiveTiming(0.0)
		, InclusiveTiming(0.0)
		, MaxTiming(-MAX_dbl)
		, MinTiming(MAX_dbl)
		, TotalTiming(0.0)
		, HottestPathHeatValue(0.f)
		, NumSamples(0)
	{
	}

	/** Add a single event node timing into the dataset */
	void AddEventTiming(const double ExclusiveTimingIn);

	/** Add a inclusive timing into the dataset */
	void AddInclusiveTiming(const double InclusiveNodeTimingIn, const bool bIncrementSamples, const bool bDiscreteTiming);

	/** Add data */
	void AddData(const FScriptPerfData& DataIn);

	/** Add branch data */
	void AddBranchData(const FScriptPerfData& DataIn);

	/** Initialise from data set */
	void InitialiseFromDataSet(const TArray<TSharedPtr<FScriptPerfData>>& DataSet);

	/** Increments sample count without affecting data */
	void TickSamples() { NumSamples++; }

	/** Reset the current data buffer and all derived data stats */
	void Reset();

	// Returns if this data structure has any valid data
	bool IsDataValid() const { return NumSamples > 0; }

	// Updates the various thresholds that control stats calcs and display.
	static void SetNumberFormattingForStats(const FNumberFormattingOptions& FormatIn) { StatNumberFormat = FormatIn; }
	static void EnableBlueprintStatAverage(const bool bEnable);
	static void EnableRecentSampleBias(const bool bEnable);
	static void SetRecentSampleBias(const float RecentSampleBiasIn);
	static void SetEventPerformanceThreshold(const float EventPerformanceThresholdIn);
	static void SetExclusivePerformanceThreshold(const float ExclusivePerformanceThresholdIn);
	static void SetInclusivePerformanceThreshold(const float InclusivePerformanceThresholdIn);
	static void SetMaxPerformanceThreshold(const float MaxPerformanceThresholdIn);

	// Returns the various stats this container holds
	double GetExclusiveTiming() const;
	double GetInclusiveTiming() const { return GetExclusiveTiming() + InclusiveTiming; }
	double GetMaxTiming() const { return MaxTiming; }
	double GetMinTiming() const { return MinTiming; }
	double GetTotalTiming() const { return TotalTiming; }
	int32 GetSampleCount() const { return NumSamples; }

	// Returns various performance heat levels for visual display
	float GetNodeHeatLevel() const;
	float GetInclusiveHeatLevel() const;
	float GetMaxTimeHeatLevel() const;
	float GetTotalHeatLevel() const;

	// Hottest path interface
	float GetHottestPathHeatLevel() const { return HottestPathHeatValue; }
	void SetHottestPathHeatLevel(const float HottestPathValue) { HottestPathHeatValue = HottestPathValue; }

	// Hottest endpoint interface
	float GetHottestEndpointHeatLevel() const { return HottestEndpointHeatValue; }
	void SetHottestEndpointHeatLevel(const float HottestEndpointValue) { HottestEndpointHeatValue = HottestEndpointValue; }

	// Returns the various stats this container holds in FText format
	FText GetTotalTimingText() const;
	FText GetInclusiveTimingText() const;
	FText GetExclusiveTimingText() const;
	FText GetMaxTimingText() const;
	FText GetMinTimingText() const;
	FText GetSamplesText() const;

private:

	/** The stat type for performance threshold comparisons */
	EScriptPerfDataType StatType;
	/** Exclusive timing */
	double ExclusiveTiming;
	/** Inclusive timing, pure and node timings */
	double InclusiveTiming;
	/** Max exclusive timing */
	double MaxTiming;
	/** Min exclusive timing */
	double MinTiming;
	/** Total time accrued */
	double TotalTiming;
	/** Hottest path value */
	float HottestPathHeatValue;
	/** Hottest endpoint value */
	float HottestEndpointHeatValue;
	/** The current inclusive sample count */
	int32 NumSamples;

	/** Number formatting options */
	static FNumberFormattingOptions StatNumberFormat;

	/** Sets blueprint level stats as averaged */
	static bool bAverageBlueprintStats;
	/** Enables the sample biasing */
	static bool bUseRecentSampleBias;
	/** Controls the bias between new and older samples */
	static float RecentSampleBias;
	/** Historical Sample Bias */
	static float HistoricalSampleBias;
	/** Event performance threshold */
	static float EventPerformanceThreshold;
	/** Exclusive performance threshold */
	static float ExclusivePerformanceThreshold;
	/** Node inclusive performance threshold */
	static float InclusivePerformanceThreshold;
	/** Node max performance threshold */
	static float MaxPerformanceThreshold;
	/** Total time watermark */
	static float NodeTotalTimeWaterMark;

};
