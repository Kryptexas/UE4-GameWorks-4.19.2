// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*===================================================================================
	Scalability.h: Manager class for handling scalability settings
=====================================================================================*/

#pragma once

namespace Scalability
{ 
	/**
	 * Structure for holding the state of the engine scalability groups
	 * Actual engine state you can get though GetQualityLevels().
	**/
	struct ENGINE_API FQualityLevels
	{
		int32 ResolutionQuality;
		int32 ViewDistanceQuality;
		int32 AntiAliasingQuality;
		int32 ShadowQuality;
		int32 PostProcessQuality;
		int32 TextureQuality;
		int32 EffectsQuality;

		FQualityLevels()
		{
			SetDefaults();
		}
		
		// @param Value 0:low, 1:medium, 2:high, 3:epic
		void SetFromSingleQualityLevel(int32 Value);

		void SetBenchmarkFallback();

		void SetDefaults();
	};


	/** This is the only suggested way to set the current state - don't set CVars directly **/
	ENGINE_API void SetQualityLevels(const FQualityLevels& QualityLevels);

	/** This is the only suggested way to get the current state - don't get CVars directly */
	ENGINE_API FQualityLevels GetQualityLevels();

	/**  */
	ENGINE_API void InitScalabilitySystem();

	/** @parma IniName e.g. GEditorUserSettingsIni or GGameUserSettingsIni */
	ENGINE_API void LoadState(const FString& IniName);
	
	/** @parma IniName e.g. GEditorUserSettingsIni or GGameUserSettingsIni */
	ENGINE_API void SaveState(const FString& IniName);

	/** Run synthbenchmark and configure scalability based on results **/
	ENGINE_API FQualityLevels BenchmarkQualityLevels();

	/** Process a console command line **/
	ENGINE_API void ProcessCommand(const TCHAR* Cmd, FOutputDevice& Ar);

	/** Minimum single axis scale for render resolution */
	static const int32 MinResolutionScale = 50;

	/** Maximum single axis scale for render resolution */
	static const int32 MaxResolutionScale = 100;
}