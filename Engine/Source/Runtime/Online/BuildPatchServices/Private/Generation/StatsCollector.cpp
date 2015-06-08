// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BuildPatchServicesPrivatePCH.h"

#include "StatsCollector.h"

inline uint32 GetTypeHash(volatile const int64* A)
{
	return PointerHash((const void*)A);
}

namespace FBuildPatchServices
{

	uint64 FStatsCollector::GetCycles()
	{
#if PLATFORM_WINDOWS
		LARGE_INTEGER Cycles;
		QueryPerformanceCounter(&Cycles);
		return Cycles.QuadPart;
#elif PLATFORM_MAC
		uint64 Cycles = mach_absolute_time();
		return Cycles;
#else
		return FPlatformTime::Cycles();
#endif
	}

	double FStatsCollector::CyclesToSeconds(const uint64 Cycles)
	{
		return FPlatformTime::GetSecondsPerCycle() * Cycles;
	}

	void FStatsCollector::AccumulateTimeBegin(uint64& TempValue)
	{
		TempValue = FStatsCollector::GetCycles();
	}

	void FStatsCollector::AccumulateTimeEnd(volatile int64* Stat, uint64& TempValue)
	{
		FPlatformAtomics::InterlockedAdd(Stat, FStatsCollector::GetCycles() - TempValue);
	}

	void FStatsCollector::Accumulate(volatile int64* Stat, int64 Amount)
	{
		FPlatformAtomics::InterlockedAdd(Stat, Amount);
	}

	void FStatsCollector::Set(volatile int64* Stat, int64 Value)
	{
		FPlatformAtomics::InterlockedExchange(Stat, Value);
	}

	class FStatsCollectorImpl
		: public FStatsCollector
	{
	public:
		FStatsCollectorImpl();
		~FStatsCollectorImpl();

		virtual volatile int64* CreateStat(const FString& Name, EStatFormat Type, int64 InitialValue = 0) override;
		virtual void LogStats(float TimeBetweenLogs = 0.0f) override;

	private:
		FCriticalSection DataCS;
		TMap<FString, volatile int64*> AddedStats;
		TMap<volatile int64*, FString> AtomicNameMap;
		TMap<volatile int64*, EStatFormat> AtomicFormatMap;
		uint64 LastLogged;
		int32 LongestName;
	};

	FStatsCollectorImpl::FStatsCollectorImpl()
		: LastLogged(FStatsCollector::GetCycles())
		, LongestName(0)
	{
	}

	FStatsCollectorImpl::~FStatsCollectorImpl()
	{
		FScopeLock ScopeLock(&DataCS);
		for (auto& Stat : AddedStats)
		{
			delete Stat.Value;
		}
		AddedStats.Empty();
		AtomicNameMap.Empty();
		AtomicFormatMap.Empty();
	}

	volatile int64* FStatsCollectorImpl::CreateStat(const FString& Name, EStatFormat Type, int64 InitialValue)
	{
		FScopeLock ScopeLock(&DataCS);
		if (AddedStats.Contains(Name) == false)
		{
			volatile int64* NewInteger = new volatile int64(InitialValue);
			AddedStats.Add(Name, NewInteger);
			AtomicNameMap.Add(NewInteger, Name + TEXT(": "));
			AtomicFormatMap.Add(NewInteger, Type);
			LongestName = FMath::Max<uint32>(LongestName, Name.Len() + 2);
			for (auto& Stat : AtomicNameMap)
			{
				while (Stat.Value.Len() < LongestName)
				{
					Stat.Value += TEXT(" ");
				}
			}
		}
		return AddedStats[Name];
	}

	void FStatsCollectorImpl::LogStats(float TimeBetweenLogs)
	{
		const uint64 Cycles = FStatsCollector::GetCycles();
		if (FStatsCollector::CyclesToSeconds(Cycles - LastLogged) >= TimeBetweenLogs)
		{
			LastLogged = Cycles;
			FScopeLock ScopeLock(&DataCS);
			GLog->Log(TEXT("/-------- FStatsCollector Log ---------------------"));
			for (auto& Stat : AtomicNameMap)
			{
				switch (AtomicFormatMap[Stat.Key])
				{
				case EStatFormat::Timer:
					GLog->Logf(TEXT("| %s%s"), *Stat.Value, *FPlatformTime::PrettyTime(FStatsCollector::CyclesToSeconds(*Stat.Key)));
					break;
				case EStatFormat::DataSize:
					GLog->Logf(TEXT("| %s%s"), *Stat.Value, *FText::AsMemory(*Stat.Key).ToString());
					break;
				case EStatFormat::DataSpeed:
					GLog->Logf(TEXT("| %s%s/s"), *Stat.Value, *FText::AsMemory(*Stat.Key).ToString());
					break;
				default:
					GLog->Logf(TEXT("| %s%lld"), *Stat.Value, *Stat.Key);
					break;
				}
			}
			GLog->Log(TEXT("\\--------------------------------------------------"));
		}
	}

	FStatsCollectorRef FStatsCollectorFactory::Create()
	{
		return MakeShareable(new FStatsCollectorImpl());
	}
}
