// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LaunchPrivatePCH.h"
#include <string.h>
#include <jni.h>
#include <pthread.h>
#include "AndroidJNI.h"
#include "AndroidEventManager.h"
#include "AndroidInputInterface.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <cstdio>
#include <sys/resource.h>
#include <dlfcn.h>
#include "AndroidWindow.h"

// Function pointer for retrieving joystick events
// Function has been part of the OS since Honeycomb, but only appeared in the
// NDK in r19. Querying via dlsym allows its use without tying to the newest
// NDK.
typedef float(*GetAxesType)(const AInputEvent*, int32_t axis, size_t pointer_index);
static GetAxesType GetAxes = NULL;

// List of default axes to query for each controller
// Ideal solution is to call out to Java and enumerate the list of axes.
static const int32_t AxisList[] =
{
	AMOTION_EVENT_AXIS_X,
    AMOTION_EVENT_AXIS_Y,
	AMOTION_EVENT_AXIS_Z,
    AMOTION_EVENT_AXIS_RZ,
    AMOTION_EVENT_AXIS_LTRIGGER,
    AMOTION_EVENT_AXIS_RTRIGGER,
	AMOTION_EVENT_AXIS_GAS,
    AMOTION_EVENT_AXIS_BRAKE,

	//These are DPAD analogs
    //AMOTION_EVENT_AXIS_HAT_X,
    //AMOTION_EVENT_AXIS_HAT_Y,
};

// -nostdlib means no crtbegin_so.o, so we have to provide our own __dso_handle and atexit()
extern "C"
{
	int atexit(void (*func)(void)) { return 0; }

	extern void *__dso_handle __attribute__((__visibility__ ("hidden")));
	void *__dso_handle;
}

extern void AndroidThunkCpp_ShowConsoleWindow();

/** The global EngineLoop instance */
FEngineLoop	GEngineLoop;

bool GShowConsoleWindowNextTick = false;

static void AndroidProcessEvents(struct android_app* state);

//Event thread stuff
static void* AndroidEventThreadWorker(void* param);

//Android event callback functions
static int32_t HandleInputCB(struct android_app* app, AInputEvent* event); //Touch and key input events
static void OnAppCommandCB(struct android_app* app, int32_t cmd); //Lifetime events

bool GHasInterruptionRequest = false;
bool GIsInterrupted = false;

void UpdateGameInterruptions()
{
	// Check for game suspension.
	if(GHasInterruptionRequest)
	{
		// Suspend the renderer.
		if(GUseThreadedRendering)
		{
			FlushRenderingCommands();
			StopRenderingThread();
		}
		else
		{
			RHIReleaseThreadOwnership();
		}

		// Flag the suspended state.
		GIsInterrupted = true;

		// Wait for resume.
		while(GHasInterruptionRequest)
		{
			FPlatformProcess::Sleep(0.1f);
		}

		// Flag the resume state.
		GIsInterrupted = false;

		// Reset the window surface.
		RHIAcquireThreadOwnership();
		RHIReleaseThreadOwnership();

		// Resume the renderer.
		if(GUseThreadedRendering)
		{
			StartRenderingThread();
		}
		else
		{
			RHIAcquireThreadOwnership();
		}
	}
}

static void InitCommandLine()
{
	static const uint32 CMD_LINE_MAX = 16384u;

	// initialize the command line to an empty string
	FCommandLine::Set(TEXT(""));

	// read in the command line text file from the sdcard if it exists
	FString CommandLineFilePath = FString("/mnt/sdcard/") + (GGameName[0] ? GGameName : TEXT("UE4Game")) + FString("/UE4CommandLine.txt");
	FILE* CommandLineFile = fopen(TCHAR_TO_UTF8(*CommandLineFilePath), "r");
	if(CommandLineFile == NULL)
	{
		// if that failed, try the lowercase version
		CommandLineFilePath = CommandLineFilePath.Replace(TEXT("UE4CommandLine.txt"), TEXT("ue4commandline.txt"));
		CommandLineFile = fopen(TCHAR_TO_UTF8(*CommandLineFilePath), "r");
	}

	if(CommandLineFile)
	{
		char CommandLine[CMD_LINE_MAX];
		fgets(CommandLine, ARRAY_COUNT(CommandLine) - 1, CommandLineFile);

		fclose(CommandLineFile);

		// chop off trailing spaces
		while (*CommandLine && isspace(CommandLine[strlen(CommandLine) - 1]))
		{
			CommandLine[strlen(CommandLine) - 1] = 0;
		}

		FCommandLine::Append(UTF8_TO_TCHAR(CommandLine));
	}
}

//Main function called from the android entry point
int32 AndroidMain(struct android_app* state)
{
	FPlatformMisc::LowLevelOutputDebugString(L"Entered AndroidMain()");

	// adjust the file descriptor limits to allow as many open files as possible
	rlimit cur_fd_limit;
	{
		int result = getrlimit(RLIMIT_NOFILE, & cur_fd_limit);
		//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("(%d) Current fd limits: soft = %lld, hard = %lld"), result, cur_fd_limit.rlim_cur, cur_fd_limit.rlim_max);
	}
	{
		rlimit new_limit = cur_fd_limit;
		new_limit.rlim_cur = cur_fd_limit.rlim_max;
		new_limit.rlim_max = cur_fd_limit.rlim_max;
		int result = setrlimit(RLIMIT_NOFILE, &new_limit);
		//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("(%d) Setting fd limits: soft = %lld, hard = %lld"), result, new_limit.rlim_cur, new_limit.rlim_max);
	}
	{
		int result = getrlimit(RLIMIT_NOFILE, & cur_fd_limit);
		//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("(%d) Current fd limits: soft = %lld, hard = %lld"), result, cur_fd_limit.rlim_cur, cur_fd_limit.rlim_max);
	}

	// setup joystick support
	// r19 is the first NDK to include AMotionEvenet_getAxisValue in the headers
	// However, it has existed in the so since Honeycomb, query for the symbol
	// to determine whether to try controller support
	{
		void* Lib = dlopen("libandroid.so",0);
		if (Lib != NULL)
		{
			GetAxes = (GetAxesType)dlsym(Lib, "AMotionEvent_getAxisValue");
		}

		if (GetAxes != NULL)
		{
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Controller interface supported\n"));
		}
		else
		{
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Controller interface UNsupported\n"));
		}
	}

	// read the command line file
	InitCommandLine();
	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Final commandline: %s\n"), FCommandLine::Get());

	// initialize the engine
	GEngineLoop.PreInit(0, NULL, FCommandLine::Get());

	UE_LOG(LogAndroid, Display, TEXT("Passed PreInit()"));

	GLog->SetCurrentThreadAsMasterThread();

	GEngineLoop.Init();

	UE_LOG(LogAndroid, Log, TEXT("Passed GEngineLoop.Init()"));

	// tick until done
	while (!GIsRequestingExit)
	{
		FAppEventManager::GetInstance()->Tick();
		if(!FAppEventManager::GetInstance()->IsGamePaused())
		{
			GEngineLoop.Tick();

			float timeToSleep = 0.05f; //in seconds
			sleep(timeToSleep);
		}

#if !UE_BUILD_SHIPPING
		// show console window on next game tick
		if (GShowConsoleWindowNextTick)
		{
			GShowConsoleWindowNextTick = false;
			AndroidThunkCpp_ShowConsoleWindow();
		}
#endif
	}

	UE_LOG(LogAndroid, Log, TEXT("Exiting"));

	// exit out!
	GEngineLoop.Exit();

	return 0;
}



static void* AndroidEventThreadWorker( void* param )
{
	struct android_app* state = (struct android_app*)param;

	FPlatformMisc::LowLevelOutputDebugString(L"Entering event processing thread engine entry point");

	ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
	ALooper_addFd(looper, state->msgread, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, NULL,
		&state->cmdPollSource);
	state->looper = looper;

	FPlatformMisc::LowLevelOutputDebugString(L"Prepared looper for event thread");

	//Assign the callbacks
	state->onAppCmd = OnAppCommandCB;
	state->onInputEvent = HandleInputCB;

	FPlatformMisc::LowLevelOutputDebugString(L"Passed callback initialization");

	//continue to process events until the engine is shutting down
	while (!GIsRequestingExit)
	{
		static int count = 1;
		//LOGD("Event process loop #%d", count);
		count++;

		AndroidProcessEvents(state);

		float timeToSleep = 0.05f; //in seconds
		sleep(timeToSleep);
	}	

	UE_LOG(LogAndroid, Log, TEXT("Exiting"));

	return NULL;
}

//Called from the separate event processing thread
static void AndroidProcessEvents(struct android_app* state)
{
	int ident;
	int fdesc;
	int events;
	struct android_poll_source* source;

	while((ident = ALooper_pollAll(0, &fdesc, &events, (void**)&source)) >= 0)
	{
		// process this event
		if (source)
			source->process(state, source);
	}
}

pthread_t G_AndroidEventThread;

void android_main(struct android_app* state)
{
	FPlatformMisc::LowLevelOutputDebugString(L"Entering native app glue main function");

	pthread_attr_t otherAttr; 
	pthread_attr_init(&otherAttr);
	pthread_attr_setdetachstate(&otherAttr, PTHREAD_CREATE_DETACHED);
	pthread_create(&G_AndroidEventThread, &otherAttr, AndroidEventThreadWorker, state);

	FPlatformMisc::LowLevelOutputDebugString(L"Created event thread");

	// Make sure glue isn't stripped.
	app_dummy();

	//@todo android: replace with native activity, main loop off of UI thread, etc.
	AndroidMain(state);
}

//Called from the event process thread
static int32_t HandleInputCB(struct android_app* app, AInputEvent* event)
{
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
	{
		int action = AMotionEvent_getAction(event);
		int actionType = action & AMOTION_EVENT_ACTION_MASK;
		size_t actionPointer = (size_t)((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
		bool isActionTargeted = (actionType == AMOTION_EVENT_ACTION_POINTER_DOWN || actionType == AMOTION_EVENT_ACTION_POINTER_UP);

		// trap Joystick events first, with fallthrough if there is no joystick support
		if (((AInputEvent_getSource(event) & AINPUT_SOURCE_CLASS_JOYSTICK) != 0) &&
				(GetAxes != NULL) &&
				(actionType == AMOTION_EVENT_ACTION_MOVE))
		{
			const int axisCount = sizeof(AxisList)/sizeof(int32_t);
			int device = AInputEvent_getDeviceId(event);

			for (int axis = 0; axis < axisCount; axis++)
			{
				float val = GetAxes( event, AxisList[axis], 0);
				FAndroidInputInterface::JoystickAxisEvent( device, AxisList[axis], val);
			}
		}
		else
		{
			TArray<TouchInput> TouchesArray;
			
			TouchType type = TouchEnded;

			switch (actionType)
			{
			case AMOTION_EVENT_ACTION_DOWN:
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				type = TouchBegan;
				break;
			case AMOTION_EVENT_ACTION_MOVE:
				type = TouchMoved;
				break;
			case AMOTION_EVENT_ACTION_UP:
			case AMOTION_EVENT_ACTION_POINTER_UP:
			case AMOTION_EVENT_ACTION_CANCEL:
			case AMOTION_EVENT_ACTION_OUTSIDE:
				type = TouchEnded;
				break;
			}

			size_t pointerCount = AMotionEvent_getPointerCount(event);

			if (pointerCount == 0)
			{
				return 1;
			}

			ANativeWindow* Window = (ANativeWindow*)FPlatformMisc::GetHardwareWindow();
			if (!Window)
			{
				return 1;
			}

			int32_t Width = 0 ;
			int32_t Height = 0 ;

			if(Window)
			{
				FAndroidWindow::CalculateSurfaceSize(Window, Width, Height);
			}

			FPlatformRect ScreenRect = FAndroidWindow::GetScreenRect();

			if(isActionTargeted)
			{
				if(actionPointer < 0 || pointerCount < (int)actionPointer)
				{
					return 1;
				}

				int pointerId = AMotionEvent_getPointerId(event, actionPointer);
				float x = FMath::Min<float>(AMotionEvent_getX(event, actionPointer) / Width, 1.f);
				x *= (ScreenRect.Right - 1);
				float y = FMath::Min<float>(AMotionEvent_getY(event, actionPointer) / Height, 1.f);
				y *= (ScreenRect.Bottom - 1);

//  				UE_LOG(LogAndroid, Log, TEXT("Received targeted motion event from pointer %u (id %d) action %d: (%.2f, %.2f)"), actionPointer, pointerId, action, x, y);

				TouchInput TouchMessage;
				TouchMessage.Handle = pointerId;
				TouchMessage.Type = type;
				TouchMessage.Position = FVector2D(x, y);
				TouchMessage.LastPosition = FVector2D(x, y);		//@todo android: AMotionEvent_getHistoricalRawX
				TouchesArray.Add(TouchMessage);
			}
			else
			{
				for (size_t i = 0; i < pointerCount; ++i)
				{
					int pointerId = AMotionEvent_getPointerId(event, i);

					float x = FMath::Min<float>(AMotionEvent_getX(event, i) / Width, 1.f);
					x *= (ScreenRect.Right - 1);
					float y = FMath::Min<float>(AMotionEvent_getY(event, i) / Height, 1.f);
					y *= (ScreenRect.Bottom - 1);

//  					UE_LOG(LogAndroid, Log, TEXT("Received motion event from pointer %u (id %d) action %d: (%.2f, %.2f)"), i, action, AMotionEvent_getPointerId(event,i), x, y);

					TouchInput TouchMessage;
					TouchMessage.Handle = AMotionEvent_getPointerId(event, i);
					TouchMessage.Type = type;
					TouchMessage.Position = FVector2D(x, y);
					TouchMessage.LastPosition = FVector2D(x, y);		//@todo android: AMotionEvent_getHistoricalRawX
					TouchesArray.Add(TouchMessage);
				}
			}

			FAndroidInputInterface::QueueTouchInput(TouchesArray);

#if !UE_BUILD_SHIPPING
			if(pointerCount >= 4)
			{
				GShowConsoleWindowNextTick = true;
			}
#endif
		}

		return 1;
	}
	else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
	{
		int keyCode = AKeyEvent_getKeyCode(event);

		//Trap Joystick events first, with fallthrough if there is no joystick support
		if (((AInputEvent_getSource(event) & AINPUT_SOURCE_CLASS_JOYSTICK) != 0) && (GetAxes != NULL))
		{
			int device = AInputEvent_getDeviceId(event);
			bool down = AKeyEvent_getAction(event) != AKEY_EVENT_ACTION_UP;
			FAndroidInputInterface::JoystickButtonEvent( device, keyCode, down);
			FPlatformMisc::LowLevelOutputDebugStringf(L"Received gamepad button: %d", keyCode);
		}
		else
		{
			FPlatformMisc::LowLevelOutputDebugStringf(L"Received key event: %d", keyCode);

			FDeferredAndroidMessage Message;

			Message.messageType = AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP ? MessageType_KeyUp : MessageType_KeyDown; 
			Message.KeyEventData.unichar = keyCode;
			Message.KeyEventData.keyId = keyCode;
			Message.KeyEventData.isRepeat = AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_MULTIPLE;
			FAndroidInputInterface::DeferMessage(Message);
		}

		return 1;
	}

	return 0;
}

static void onNativeWindowResized(ANativeActivity* activity, ANativeWindow* window)
{
	FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_WINDOW_CHANGED,window);
}



//Called from the event process thread
static void OnAppCommandCB(struct android_app* app, int32_t cmd)
{
	FPlatformMisc::LowLevelOutputDebugStringf(L"OnAppCommandCB cmd: %u", cmd);

	switch (cmd)
	{
	case APP_CMD_SAVE_STATE:
		/**
		* Command from main thread: the app should generate a new saved state
		* for itself, to restore from later if needed.  If you have saved state,
		* allocate it with malloc and place it in android_app.savedState with
		* the size in android_app.savedStateSize.  The will be freed for you
		* later.
		*/
		// the OS asked us to save the state of the app
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_SAVE_STATE"));
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_SAVE_STATE);
		break;
	case APP_CMD_INIT_WINDOW:
		/**
		 * Command from main thread: a new ANativeWindow is ready for use.  Upon
		 * receiving this command, android_app->window will contain the new window
		 * surface.
		 */
		// get the window ready for showing
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_INIT_WINDOW"));
		FAppEventManager::GetInstance()->HandleWindowCreated(app->pendingWindow);
		break;
	case APP_CMD_TERM_WINDOW:
		/**
		 * Command from main thread: the existing ANativeWindow needs to be
		 * terminated.  Upon receiving this command, android_app->window still
		 * contains the existing window; after calling android_app_exec_cmd
		 * it will be set to NULL.
		 */
		// clean up the window because it is being hidden/closed
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_TERM_WINDOW"));
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_WINDOW_DESTROYED, NULL);
		
		break;
	case APP_CMD_LOST_FOCUS:
		/**
		 * Command from main thread: the app's activity window has lost
		 * input focus.
		 */
		// if the app lost focus, avoid unnecessary processing (like monitoring the accelerometer)
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_LOST_FOCUS"));
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_WINDOW_LOST_FOCUS, NULL);
		break;
	case APP_CMD_GAINED_FOCUS:
		/**
		 * Command from main thread: the app's activity window has gained
		 * input focus.
		 */
		// bring back a certain functionality, like monitoring the accelerometer
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_GAINED_FOCUS"));
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_WINDOW_GAINED_FOCUS, NULL);
		break;
	case APP_CMD_INPUT_CHANGED:
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_INPUT_CHANGED"));
		break;
	case APP_CMD_WINDOW_RESIZED:
		/**
		 * Command from main thread: the current ANativeWindow has been resized.
		 * Please redraw with its new size.
		 */
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_WINDOW_RESIZED"));
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_WINDOW_RESIZED );
		break;
	case APP_CMD_WINDOW_REDRAW_NEEDED:
		/**
		 * Command from main thread: the system needs that the current ANativeWindow
		 * be redrawn.  You should redraw the window before handing this to
		 * android_app_exec_cmd() in order to avoid transient drawing glitches.
		 */
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_WINDOW_REDRAW_NEEDED"));
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_WINDOW_REDRAW_NEEDED );
		break;
	case APP_CMD_CONTENT_RECT_CHANGED:
		/**
		 * Command from main thread: the content area of the window has changed,
		 * such as from the soft input window being shown or hidden.  You can
		 * find the new content rect in android_app::contentRect.
		 */
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_CONTENT_RECT_CHANGED"));
		break;
	case APP_CMD_CONFIG_CHANGED:
		/**
		 * Command from main thread: the current device configuration has changed.
		 */
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_CONFIG_CHANGED"));
		break;
	case APP_CMD_LOW_MEMORY:
		/**
		 * Command from main thread: the system is running low on memory.
		 * Try to reduce your memory use.
		 */
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_LOW_MEMORY"));
		break;
	case APP_CMD_START:
		/**
		 * Command from main thread: the app's activity has been started.
		 */
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_START"));
		app->activity->callbacks->onNativeWindowResized  = onNativeWindowResized; //currently not handled in glue code.
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_ON_START);
	
		break;
	case APP_CMD_RESUME:
		/**
		 * Command from main thread: the app's activity has been resumed.
		 */
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_RESUME"));

		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_ON_RESUME);

		break;
	case APP_CMD_PAUSE:
		/**
		 * Command from main thread: the app's activity has been paused.
		 */
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_PAUSE"));
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_ON_PAUSE);

		break;
	case APP_CMD_STOP:
		/**
		 * Command from main thread: the app's activity has been stopped.
		 */
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_STOP"));
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_ON_STOP);
		break;
	case APP_CMD_DESTROY:
		/**
		* Command from main thread: the app's activity is being destroyed,
		* and waiting for the app thread to clean up and exit before proceeding.
		*/
		UE_LOG(LogAndroid, Log, TEXT("Case APP_CMD_DESTROY"));
		FAppEventManager::GetInstance()->EnqueueAppEvent(APP_EVENT_STATE_ON_PAUSE);
		break;
	}
}

//Native-defined functions

//This function is declared in the Java-defined class, GameActivity.java: "public native void nativeConsoleCommand(String commandString);"
extern "C" void Java_com_epicgames_ue4_GameActivity_nativeConsoleCommand(JNIEnv* jenv, jobject thiz, jstring commandString)
{
	const char* javaChars = jenv->GetStringUTFChars(commandString, 0);

	new(GEngine->DeferredCommands) FString(UTF8_TO_TCHAR(javaChars));

	//Release the string
	jenv->ReleaseStringUTFChars(commandString, javaChars);
}
