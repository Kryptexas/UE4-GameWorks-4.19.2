// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Projects and plugins can be loaded at various points during startup.  This enum allows you to select which phase to load at.
 */
namespace ELoadingPhase
{
	enum Type
	{
		/** Loaded at the default loading point during startup (during engine init, after game modules are loaded.) */
		Default,

		/** Right after the default phase */
		PostDefault,

		/** Right before the default phase */
		PreDefault,

		/** Loaded before the engine is fully initialized, immediately after the config system has been initialized.  Necessary only for very low-level hooks */
		PostConfigInit,

		/** Loaded before the engine is fully initialized for modules that need to hook into the loading screen before it triggers */
		PreLoadingScreen,
		// NOTE: If you add a new value, make sure to update the ToString() method below!

		Max
	};

	inline static const TCHAR* ToString( const ELoadingPhase::Type Value )
	{
		switch( Value )
		{
		case Default:
			return TEXT( "Default" );

		case PostDefault:
			return TEXT( "PostDefault" );

		case PreDefault:
			return TEXT( "PreDefault" );

		case PostConfigInit:
			return TEXT( "PostConfigInit" );
			
		case PreLoadingScreen:
			return TEXT( "PreLoadingScreen" );

		default:
			ensureMsgf( false, TEXT( "Unrecognized ELoadingPhase value: %i" ), Value );
			return NULL;
		}
	}

	/**
	  * Returns a value for a phase that describes the order in which phases are loaded.
	  * A lower value means the phase is loaded earlier.
	  * These values may change as phases are added or removed.
	  */
	inline static int32 GetLoadOrderValue(  const ELoadingPhase::Type Phase )
	{
		switch( Phase )
		{
		case PostDefault:
			return 500;

		case Default:
			return 400;

		case PreDefault:
			return 300;
			
		case PreLoadingScreen:
			return 200;

		case PostConfigInit:
			return 100;

		default:
			ensureMsgf( false, TEXT( "Unrecognized ELoadingPhase value: %i" ), Phase );
			return 0;
		}
	}

	/** Returns the earlier phase between PhaseA and PhaseB */
	inline static Type GetEarlierPhase(Type PhaseA, Type PhaseB) 
	{
		const int32 PhaseAValue = GetLoadOrderValue(PhaseA);
		const int32 PhaseBValue = GetLoadOrderValue(PhaseB);

		return PhaseAValue < PhaseBValue ? PhaseA : PhaseB;
	}
};