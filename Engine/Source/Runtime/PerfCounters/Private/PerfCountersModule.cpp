// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PerfCounters.h"

class FPerfCountersModule : public IPerfCountersModule
{
private:

	/** All created perf counter instances from this module */
	FPerfCounters* PerfCountersSingleton;

public:

	FPerfCountersModule() : 
		PerfCountersSingleton(NULL)
	{}

	void ShutdownModule()
	{
		if (PerfCountersSingleton)
		{
			delete PerfCountersSingleton;
			PerfCountersSingleton = nullptr;
		}
	}

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	virtual bool SupportsAutomaticShutdown() override
	{
		return false;
	}

	IPerfCounters* GetPerformanceCounters() const
	{
		return PerfCountersSingleton;
	}

	IPerfCounters* CreatePerformanceCounters(const FString& UniqueInstanceId) override
	{
		if (PerfCountersSingleton)
		{
			UE_LOG(LogPerfCounters, Display, TEXT("CreatePerformanceCounters: instance already exists, new instance not created."));
			return PerfCountersSingleton;
		}

		FString InstanceUID = UniqueInstanceId;
		if (InstanceUID.IsEmpty())
		{
			InstanceUID = FString::Printf(TEXT("perfcounters-of-pid-%d"), FPlatformProcess::GetCurrentProcessId());
		}

		FPerfCounters* PerfCounters = new FPerfCounters(InstanceUID);
		if (!PerfCounters->Initialize())
		{
			UE_LOG(LogPerfCounters, Warning, TEXT("CreatePerformanceCounters: could not create perfcounters"));
			delete PerfCounters;
			return nullptr;
		}

		PerfCountersSingleton = PerfCounters;
		return PerfCounters;
	}
};

IMPLEMENT_MODULE(FPerfCountersModule, PerfCounters)
DEFINE_LOG_CATEGORY(LogPerfCounters);

const FName IPerfCounters::Histograms::FrameTime(TEXT("FrameTime"));
const FName IPerfCounters::Histograms::FrameTimePeriodic(TEXT("FrameTimePeriodic"));
const FName IPerfCounters::Histograms::ServerReplicateActorsTime(TEXT("ServerReplicateActorsTime"));
const FName IPerfCounters::Histograms::SleepTime(TEXT("SleepTime"));
const FName IPerfCounters::Histograms::ZeroLoadFrameTime(TEXT("ZeroLoadFrameTime"));

void FHistogram::InitLinear(double MinTime, double MaxTime, double BinSize)
{
	double CurrentBinMin = MinTime;
	while (CurrentBinMin < MaxTime)
	{
		Bins.Add(FBin(CurrentBinMin, CurrentBinMin + BinSize));
		CurrentBinMin += BinSize;
	}
	Bins.Add(FBin(CurrentBinMin));	// catch-all-above bin
}

void FHistogram::InitHitchTracking()
{
	Bins.Empty();

	Bins.Add(FBin(0.0, 9.0));		// >= 120 fps
	Bins.Add(FBin(9.0, 17.0));		// 60 - 120 fps
	Bins.Add(FBin(17.0, 34.0));		// 30 - 60 fps
	Bins.Add(FBin(34.0, 50.0));		// 20 - 30 fps
	Bins.Add(FBin(50.0, 67.0));		// 15 - 20 fps
	Bins.Add(FBin(67.0, 100.0));	// 10 - 15 fps
	Bins.Add(FBin(100.0, 200.0));	// 5 - 10 fps
	Bins.Add(FBin(200.0, 300.0));	// < 5 fps
	Bins.Add(FBin(300.0, 500.0));
	Bins.Add(FBin(500.0, 750.0));
	Bins.Add(FBin(750.0, 1000.0));
	Bins.Add(FBin(1000.0, 1500.0));
	Bins.Add(FBin(1500.0, 2000.0));
	Bins.Add(FBin(2000.0, 2500.0));
	Bins.Add(FBin(2500.0, 5000.0));
	Bins.Add(FBin(5000.0));
}

void FHistogram::Reset()
{
	for (FBin& Bin : Bins)
	{
		Bin.Count = 0;
		Bin.Sum = 0.0;
	}
}

void FHistogram::AddMeasurement(double Value)
{
	if (LIKELY(Bins.Num()))
	{
		FBin& FirstBin = Bins[0];
		if (UNLIKELY(Value < FirstBin.MinValue))
		{
			return;
		}

		for (int BinIdx = 0, LastBinIdx = Bins.Num() - 1; BinIdx < LastBinIdx; ++BinIdx)
		{
			FBin& Bin = Bins[BinIdx];
			if (Bin.UpperBound > Value)
			{
				++Bin.Count;
				Bin.Sum += Value;
				return;
			}
		}

		// if we got here, Value did not fit in any of the previous bins
		FBin& LastBin = Bins.Last();
		++LastBin.Count;
		LastBin.Sum += Value;
	}
}

void FHistogram::DumpToAnalytics(const FString& ParamNamePrefix, TArray<TPair<FString, double>>& OutParamArray)
{
	if (LIKELY(Bins.Num()))
	{
		for (int BinIdx = 0, LastBinIdx = Bins.Num() - 1; BinIdx < LastBinIdx; ++BinIdx)
		{
			FBin& Bin = Bins[BinIdx];
			FString ParamName = FString::Printf(TEXT("_%.0f_%.0f"), Bin.MinValue, Bin.UpperBound);
			OutParamArray.Add(TPairInitializer<FString, double>(ParamNamePrefix + ParamName + TEXT("_Count"), Bin.Count));
			OutParamArray.Add(TPairInitializer<FString, double>(ParamNamePrefix + ParamName + TEXT("_Sum"), Bin.Sum));
		}

		FBin& LastBin = Bins.Last();
		FString ParamName = FString::Printf(TEXT("_%.0f_AndAbove"), LastBin.MinValue);
		OutParamArray.Add(TPairInitializer<FString, double>(ParamNamePrefix + ParamName + TEXT("_Count"), LastBin.Count));
		OutParamArray.Add(TPairInitializer<FString, double>(ParamNamePrefix + ParamName + TEXT("_Sum"), LastBin.Sum));
	}
}

void FHistogram::DumpToLog(const FString& HistogramName)
{
	UE_LOG(LogPerfCounters, Log, TEXT("Histogram '%s': %d bins"), *HistogramName, Bins.Num());

	if (LIKELY(Bins.Num()))
	{
		double TotalSum = 0;
		double TotalObservations = 0;

		for (int BinIdx = 0, LastBinIdx = Bins.Num() - 1; BinIdx < LastBinIdx; ++BinIdx)
		{
			FBin& Bin = Bins[BinIdx];
			UE_LOG(LogPerfCounters, Log, TEXT("Bin %4.0f - %4.0f: %5d observation(s) which sum up to %f"), Bin.MinValue, Bin.UpperBound, Bin.Count, Bin.Sum);

			TotalObservations += Bin.Count;
			TotalSum += Bin.Sum;
		}

		FBin& LastBin = Bins.Last();
		UE_LOG(LogPerfCounters, Log,     TEXT("Bin %4.0f +     : %5d observation(s) which sum up to %f"), LastBin.MinValue, LastBin.Count, LastBin.Sum);
		TotalObservations += LastBin.Count;
		TotalSum += LastBin.Sum;

		if (TotalObservations > 0)
		{
			UE_LOG(LogPerfCounters, Log, TEXT("Average value for observation: %f"), TotalSum / TotalObservations);
		}
	}
}
