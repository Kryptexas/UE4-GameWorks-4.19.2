// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Audio
{
	// Wavetable oscillator types
	namespace EWaveTable
	{
		enum Type
		{
			None,
			SineWaveTable,
			SawWaveTable,
			TriangleWaveTable,
			SquareWaveTable,
			BandLimitedSawWaveTable,
			BandLimitedTriangleWaveTable,
			BandLimitedSquareWaveTable,
			Custom
		};
	}

	class FWaveTableOsc;

	// A factory interface for creating custom wave tables
	class ICustomWaveTableOscFactory
	{
	public:
		// Creates  custom wave table with the given requested size. Custom table doesn't necessarily have to honor the requested size.
		virtual FWaveTableOsc* CreateCustomWaveTable(const int32 RequestedWaveTableSize) = 0;
	};

	// A wave table oscillator class
	class FWaveTableOsc
	{
	public:
		// Constructor
		FWaveTableOsc();

		// Virtual Destructor
		virtual ~FWaveTableOsc();

		// Initialize the wave table oscillator
		void Init(const int32 InSampleRate, const float InFrequencyHz);

		// Sets the sample rate of the oscillator.
		void SetSampleRate(const int32 InSampleRate);

		// Resets the wave table read indices.
		void Reset();

		// Sets the amount to scale and add to the output of the wave table
		void SetScaleAdd(const float InScale, const float InAdd);

		// Returns the type of the wave table oscillator.
		EWaveTable::Type GetType() const { return WaveTableType; }

		// Sets the frequency of the wave table oscillator.
		void SetFrequencyHz(const float InFrequencyHz);

		// Returns the frequency of the wave table oscillator.
		float GetFrequencyHz() const { return FrequencyHz; }

		// Processes the wave table, outputs the normal and quad phase (optional) values 
		void ProcessAudio(float* OutputNormalPhase, float* OutputQuadPhase = nullptr);

		// Sets the factory interface to use to create a custom wave table
		static void SetCustomWaveTableOscFactory(ICustomWaveTableOscFactory* InCustomWaveTableOscFactory);

		// Creates a wave table using internal factories for standard wave tables or uses custom wave table factor if it exists.
		static FWaveTableOsc* CreateWaveTable(const EWaveTable::Type WaveTableType, const int32 WaveTableSize = 1024);

	protected:
		void UpdateFrequency();

		// Custom wave table factory
		static ICustomWaveTableOscFactory* CustomWaveTableOscFactory;

		// The wave table buffer
		float* WaveTableBuffer;

		// The wave table buffer size
		int32 WaveTableBufferSize;

		// The frequency of the output (given the sample rate)
		float FrequencyHz;

		// The sample rate of the oscillator
		int32 SampleRate;

		// Normal phase read index
		float NormalPhaseReadIndex;

		// The quad-phase read index
		float QuadPhaseReadIndex;

		// The phase increment (based on frequency)
		float PhaseIncrement;

		// Amount to scale the output by
		float OutputScale;

		// Amount to add to the output
		float OutputAdd;

		// The wave table oscillator type
		EWaveTable::Type WaveTableType;
	};

}
