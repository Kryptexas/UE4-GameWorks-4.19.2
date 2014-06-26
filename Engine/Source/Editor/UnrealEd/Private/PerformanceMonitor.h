// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Helper class for a calculating a moving average. This works by maintaining an accumulator which is then sampled at regular intervals into the "Samples" array.
 */
struct FMovingAverage
{
	/** Constructor from a sample size, and sample rate */
	FMovingAverage(const int32 InSampleSize = 0, const float InSampleRateSeconds = 1.f)
		: SampleRateSeconds(InSampleRateSeconds)
		, CurrentSampleAccumulator(0)
		, CurrentSampleStartTime(0)
		, SampleSize(InSampleSize)
		, CurrentSampleCount(0)
		, NextSampleIndex(0)
		, SampleAverage(0)
	{
		Samples.Reserve(SampleSize);
	}
	
	/** Check if this data is reliable or not. Returns false if the sampler is not populated with enough data */
	inline float IsReliable() const
	{
		return SampleSize != 0 && Samples.Num() == SampleSize;
	}

	/** Reset this sampler to its default (unpopulated) state */
	inline void Reset()
	{
		*this = FMovingAverage(SampleSize, SampleRateSeconds);
	}

	/** Get the average of all the samples contained in this sampler */
	inline float GetAverage() const
	{
		return SampleAverage;
	}

	/** Allow this sampler to tick the frame time, and potentially save a new sample */
	void Tick(double CurrentTime, float Value)
	{
		if (SampleSize == 0)
		{
			return;
		}

		if (CurrentSampleStartTime == 0)
		{
			CurrentSampleStartTime = CurrentTime;
		}

		++CurrentSampleCount;
		CurrentSampleAccumulator += Value;

		if (CurrentTime - CurrentSampleStartTime > SampleRateSeconds)
		{
			// Limit to a minimum of 5 frames per second
			const float AccumulatorAverage = FMath::Max(CurrentSampleAccumulator / CurrentSampleCount, 5.f);

			if (Samples.Num() == SampleSize)
			{
				Samples[NextSampleIndex] = AccumulatorAverage;
			}
			else
			{
				Samples.Add(AccumulatorAverage);
			}

			NextSampleIndex = ++NextSampleIndex % SampleSize;
			ensure(NextSampleIndex >= 0 && NextSampleIndex < SampleSize);

			// calculate the average
			float Sum = 0.0;
			for (const auto& Sample : Samples)
			{
				Sum += Sample;
			}
			SampleAverage = Sum / Samples.Num();

			// reset the accumulator and counter
			CurrentSampleAccumulator = CurrentSampleCount = 0;
			CurrentSampleStartTime = CurrentTime;
		}
	}

	/** Return the percentage of samples that fall below the specified threshold */
	float PercentageBelowThreshold(const float Threshold) const
	{
		check(IsReliable());

		float NumBelowThreshold = 0;
		for (const auto& Sample : Samples)
		{
			if (Sample < Threshold)
			{
				++NumBelowThreshold;
			}
		}

		return (NumBelowThreshold / Samples.Num()) * 100.0f;
	}
private:
	/** The number of samples in the current accumulator */
	int32 CurrentSampleCount;
	/** The cumulative sum of samples for the current time period */
	float CurrentSampleAccumulator;
	/** The time at which we started accumulating the current sample */
	double CurrentSampleStartTime;

	/** The rate at which to store accumulated samples of FPS in seconds */
	float SampleRateSeconds;
	/** The maximum number of accumulated samples to store */
	int32 SampleSize;
	/** The actual average stored across all samples */
	float SampleAverage;

	/** The actual FPS samples */
	TArray<float> Samples;
	/** The index of the next sample to write to */
	int32 NextSampleIndex;
};

/** Notification class for performance warnings. */
class FPerformanceMonitor
	: public FTickableEditorObject
{

public:

	/** Constructor */
	FPerformanceMonitor();

	/** Destructor */
	~FPerformanceMonitor();

protected:

	/** FTickableEditorObject interface */
	virtual bool IsTickable() const override { return true; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FPerformanceMonitor, STATGROUP_Tickables); }

private:

	/** Starts the notification. */
	void ShowPerformanceWarning(FText MessageText);

	/** Run a benchmark and auto apply scalability settings */
	void AutoApplyScalability();

	/** Ends the notification. */
	void HidePerformanceWarning();

	/** Display the scalability dialog. */
	void ShowScalabilityDialog();

	/** Dismiss the notification and disable it for this session. */
	void DismissAndDisableNotification();

	/** Resets the performance warning data and hides the warning */
	void Reset();

	/** Update the moving average samplers to match the settings specified in the console variables */
	void UpdateMovingAverageSamplers(IConsoleVariable* Unused = nullptr);

	/** Moving average data for the fine and coarse-grained moving averages */
	FMovingAverage FineMovingAverage, CoarseMovingAverage;

	/** Tracks the last time the notification was started, used to avoid spamming. */
	double LastEnableTime;

	/** The notification window ptr */
	TWeakPtr<SNotificationItem> PerformanceWarningNotificationPtr;

	/** The Scalability Setting window we may be using */
	TWeakPtr<SWindow> ScalabilitySettingsWindowPtr;

	bool bIsNotificationAllowed;
};