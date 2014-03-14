// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 * This file contains the various draw mesh macros that display draw calls
 * inside of PIX.
 */

// Colors that are defined for a particular mesh type
// Each event type will be displayed using the defined color
#pragma once
#define DEC_LIGHT			FColor(255,0,0,255)
#define DEC_SKEL_MESH		FColor(255,0,255,255)
#define DEC_STATIC_MESH		FColor(0,128,255,255)
#define DEC_CANVAS			FColor(128,255,255,255)
#define DEC_SHADOW			FColor(128,128,128,255)
#define DEC_BSP				FColor(255,128,0,255)
#define DEC_PARTICLE		FColor(128,0,128,255)
// general scene rendering events
#define DEC_SCENE_ITEMS		FColor(128,128,128,255)

/** Platform specific function for setting the value of a counter that can be viewed in PIX. */
inline void appSetCounterValue(const TCHAR* CounterName, float Value) {}

// Disable draw mesh events for final builds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#define WANTS_DRAW_MESH_EVENTS 1
#else
#define WANTS_DRAW_MESH_EVENTS 0
#endif

#if WANTS_DRAW_MESH_EVENTS && PLATFORM_SUPPORTS_DRAW_MESH_EVENTS

	/**
	 * Class that logs draw events based upon class scope. Draw events can be seen
	 * in PIX
	 */
	struct FDrawEvent
	{
		/** Whether a draw event has been emitted or not. */
		bool bDrawEventHasBeenEmitted;

		/** Default constructor, initializing all member variables. */
		FDrawEvent()
		:	bDrawEventHasBeenEmitted( false )
		{}

		/**
		 * Terminate the event based upon scope
		 */
		~FDrawEvent()
		{
			if( bDrawEventHasBeenEmitted )
			{
				GDynamicRHI->PopEvent();
			}
		}
		/**
		 * Function for logging a PIX event with var args
		 */
		void CDECL Start(const FColor& Color,const TCHAR* Fmt, ...)
		{
			check( IsInRenderingThread_Internal() );
			va_list ptr;
			va_start(ptr, Fmt);
			TCHAR TempStr[256];
			// Build the string in the temp buffer
			FCString::GetVarArgs(TempStr,ARRAY_COUNT(TempStr),ARRAY_COUNT(TempStr)-1,Fmt,ptr);
			GDynamicRHI->PushEvent(TempStr);
			bDrawEventHasBeenEmitted = true;		
		}

		private:
		/** Helper function to determine if we are currently int the render thread. This is needed to avoid using IsInRenderThread() in a public header since it comes from RenderCore */
		ENGINE_API bool IsInRenderingThread_Internal();
	};

	// Macros to allow for scoping of draw events
	#define SCOPED_DRAW_EVENT(Name, Color) FDrawEvent Event_##Name; if(GEmitDrawEvents) Event_##Name.Start(Color, TEXT(#Name));
	#define SCOPED_DRAW_EVENTF(Name, Color, Format, ...) FDrawEvent Event_##Name; if(GEmitDrawEvents) Event_##Name.Start(Color, Format, ##__VA_ARGS__);
	#define SCOPED_CONDITIONAL_DRAW_EVENT(Name, Condition, Color) FDrawEvent Event_##Name; if(GEmitDrawEvents && (Condition)) Event_##Name.Start(Color, TEXT(#Name));
	#define SCOPED_CONDITIONAL_DRAW_EVENTF(Name, Condition, Color, Format, ...) FDrawEvent Event_##Name; if(GEmitDrawEvents && (Condition)) Event_##Name.Start(Color, Format, ##__VA_ARGS__);

#else

	#define SCOPED_DRAW_EVENT(...)
	#define SCOPED_DRAW_EVENTF(...)
	#define SCOPED_CONDITIONAL_DRAW_EVENT(...)
	#define SCOPED_CONDITIONAL_DRAW_EVENTF(...)

#endif
