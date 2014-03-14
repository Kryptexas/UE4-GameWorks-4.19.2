// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SynthBenchmarkPrivatePCH.h"
#include "RenderCore.h"
#include "RHI.h"
#include "RendererInterface.h"

float RayIntersectBenchmark();
float FractalBenchmark();


DEFINE_LOG_CATEGORY_STATIC(LogSynthBenchmark, Log, All);

// to prevent compiler optimizations
static float GGlobalStateObject = 0.0f;

class FSynthBenchmark : public ISynthBenchmark
{
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	
	/** ISynthBenchmark implementation */
	virtual void Run(FSynthBenchmarkResults& InOut, bool bGPUBenchmark, uint32 WorkScale, bool bDebugOut) const OVERRIDE;
};

IMPLEMENT_MODULE( FSynthBenchmark, SynthBenchmark )

void FSynthBenchmark::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}

void FSynthBenchmark::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

// @param RunCount should be around 10 but can be adjusted for precision 
// @param Function should run for about 3 ms
// @return performance index: 100 is a developer machine: Intel Xeon E5-2660 2.2GHz
static float RunBenchmark(uint32 RunCount, float (*Function)())
{
	float Sum = 0;

	for(uint32 i = 0; i < RunCount; ++i)
	{
		FPlatformMisc::MemoryBarrier();
		// todo: compiler reorder might be an issue, use _ReadWriteBarrier or something like http://svn.openimageio.org/oiio/tags/Release-0.6.3/src/include/tbb/machine/windows_em64t.h
		const double StartTime = FPlatformTime::Seconds();

		FPlatformMisc::MemoryBarrier();

		GGlobalStateObject += Function();

		FPlatformMisc::MemoryBarrier();
		Sum += (float)(FPlatformTime::Seconds() - StartTime);
		FPlatformMisc::MemoryBarrier();
	}
	
	return Sum / RunCount;
}

void FSynthBenchmark::Run(FSynthBenchmarkResults& InOut, bool bGPUBenchmark, uint32 WorkScale, bool bDebugOut) const
{
	check(WorkScale > 0);

	if(!bGPUBenchmark)
	{
		// run a very quick GPU benchmark (less confidence but at least we get some numbers)
		// it costs little time and we get some stats
		WorkScale = 1;
	}

	const double StartTime = FPlatformTime::Seconds();

	UE_LOG(LogSynthBenchmark, Display, TEXT("FSynthBenchmark (V0.92):"));
	UE_LOG(LogSynthBenchmark, Display, TEXT("==============="));
	
#if UE_BUILD_DEBUG
	UE_LOG(LogSynthBenchmark, Display, TEXT("         Note: Values are not trustable because this is a DEBUG build!"));
#endif
	
	UE_LOG(LogSynthBenchmark, Display, TEXT("Main Processor:"));

	// developer machine: Intel Xeon E5-2660 2.2GHz
	// divided by the actual value on a developer machine to normalize the results
	// Index should be around 100 +-4 on developer machine in a development build (should be the same in shipping)

	InOut.CPUStats[0] = FSynthBenchmarkStat(TEXT("RayIntersect"), 0.02561f, TEXT("s/Run"));
	InOut.CPUStats[0].SetMeasuredTime(RunBenchmark(WorkScale, RayIntersectBenchmark));

	InOut.CPUStats[1] = FSynthBenchmarkStat(TEXT("Fractal"), 0.0286f, TEXT("s/Run"));
	InOut.CPUStats[1].SetMeasuredTime(RunBenchmark(WorkScale, FractalBenchmark));

	for(uint32 i = 0; i < ARRAY_COUNT(InOut.CPUStats); ++i)
	{
		UE_LOG(LogSynthBenchmark, Display, TEXT("         ... %f %s '%s'"), InOut.CPUStats[i].GetMeasuredTime(), InOut.CPUStats[i].GetValueType(), InOut.CPUStats[i].GetDesc());
	}

	UE_LOG(LogSynthBenchmark, Display, TEXT(""));

	bool bAppIs64Bit = (sizeof(void*) == 8);

	UE_LOG(LogSynthBenchmark, Display, TEXT("  CompiledTarget_x_Bits: %s"), bAppIs64Bit ? TEXT("64") : TEXT("32"));
	UE_LOG(LogSynthBenchmark, Display, TEXT("  UE_BUILD_SHIPPING: %d"), UE_BUILD_SHIPPING);
	UE_LOG(LogSynthBenchmark, Display, TEXT("  UE_BUILD_TEST: %d"), UE_BUILD_TEST);
	UE_LOG(LogSynthBenchmark, Display, TEXT("  UE_BUILD_DEBUG: %d"), UE_BUILD_DEBUG);

	UE_LOG(LogSynthBenchmark, Display, TEXT("  TotalPhysicalGBRam: %d"), FPlatformMemory::GetPhysicalGBRam());
	UE_LOG(LogSynthBenchmark, Display, TEXT("  NumberOfCores (physical): %d"), FPlatformMisc::NumberOfCores());
	UE_LOG(LogSynthBenchmark, Display, TEXT("  NumberOfCores (logical): %d"), FPlatformMisc::NumberOfCoresIncludingHyperthreads());

	UE_LOG(LogSynthBenchmark, Display, TEXT("  CPU Perf Index 0: %.1f"), InOut.CPUStats[0].ComputePerfIndex());
	UE_LOG(LogSynthBenchmark, Display, TEXT("  CPU Perf Index 1: %.1f"), InOut.CPUStats[1].ComputePerfIndex());
	
	// separator line
	UE_LOG(LogSynthBenchmark, Display, TEXT(" "));

	UE_LOG(LogSynthBenchmark, Display, TEXT("Graphics:"));

	UE_LOG(LogSynthBenchmark, Display, TEXT("  Adapter Name: '%s'"), *GRHIAdapterName);
	UE_LOG(LogSynthBenchmark, Display, TEXT("  (On Optimus the name might be wrong, memory should be ok)"));
	UE_LOG(LogSynthBenchmark, Display, TEXT("  Vendor Id: 0x%x"), GRHIVendorId);

	{
		FTextureMemoryStats Stats;

		RHIGetTextureMemoryStats(Stats);

		if(Stats.AreHardwareStatsValid())
		{
			UE_LOG(LogSynthBenchmark, Display, TEXT("  GPU Memory: %d/%d/%d MB"), 
				FMath::DivideAndRoundUp(Stats.DedicatedVideoMemory, (int64)(1024 * 1024) ),
				FMath::DivideAndRoundUp(Stats.DedicatedSystemMemory, (int64)(1024 * 1024) ),
				FMath::DivideAndRoundUp(Stats.SharedSystemMemory, (int64)(1024 * 1024) ));
		}
	}

	// not always done - cost some time.
	if(bGPUBenchmark)
	{
		IRendererModule& RendererModule = FModuleManager::LoadModuleChecked<IRendererModule>(TEXT("Renderer"));

		RendererModule.GPUBenchmark(InOut, WorkScale, bDebugOut);

		UE_LOG(LogSynthBenchmark, Display, TEXT("  GPU Perf Index 0: %.1f"), InOut.GPUStats[0].ComputePerfIndex());
		UE_LOG(LogSynthBenchmark, Display, TEXT("  GPU Perf Index 1: %.1f"), InOut.GPUStats[1].ComputePerfIndex());
		UE_LOG(LogSynthBenchmark, Display, TEXT("  GPU Perf Index 2: %.1f"), InOut.GPUStats[2].ComputePerfIndex());
		UE_LOG(LogSynthBenchmark, Display, TEXT("  GPU Perf Index 3: %.1f"), InOut.GPUStats[3].ComputePerfIndex());
		UE_LOG(LogSynthBenchmark, Display, TEXT("  GPU Perf Index 4: %.1f"), InOut.GPUStats[4].ComputePerfIndex());
	}
	
	UE_LOG(LogSynthBenchmark, Display, TEXT("  CPUIndex: %.1f"), InOut.CPUStats->ComputePerfIndex());
	UE_LOG(LogSynthBenchmark, Display, TEXT("  GPUIndex: %.1f"), InOut.GPUStats->ComputePerfIndex());
	UE_LOG(LogSynthBenchmark, Display, TEXT(""));
	UE_LOG(LogSynthBenchmark, Display, TEXT("         ... Total Time: %f sec"),  (float)(FPlatformTime::Seconds() - StartTime));
}