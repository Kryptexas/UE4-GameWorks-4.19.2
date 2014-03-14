// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RenderingThread.h: Rendering thread definitions.
=============================================================================*/

#ifndef __RenderingThread_H__
#define __RenderingThread_H__

#include "RenderCore.h"
#include "TaskGraphInterfaces.h"

////////////////////////////////////
// Render thread API
////////////////////////////////////

/** @return True if called from the rendering thread, or if called from ANY thread during single threaded rendering */
extern RENDERCORE_API bool IsInRenderingThread();

/** @return True if called from the rendering thread. */
// Unlike IsInRenderingThread, this will always return false if we are running single threaded. It only returns true if this is actually a separate rendering thread. Mostly useful for checks
extern RENDERCORE_API bool IsInActualRenderingThread();

/** Thread used for rendering */
extern RENDERCORE_API FRunnableThread* GRenderingThread;

/**
 * Whether the renderer is currently running in a separate thread.
 * If this is false, then all rendering commands will be executed immediately instead of being enqueued in the rendering command buffer.
 */
extern RENDERCORE_API bool GIsThreadedRendering;

/**
 * Whether the rendering thread should be created or not.
 * Currently set by command line parameter and by the ToggleRenderingThread console command.
 */
extern RENDERCORE_API bool GUseThreadedRendering;

#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static FORCEINLINE void CheckNotBlockedOnRenderThread() {}
#else // #if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/** Whether the main thread is currently blocked on the rendering thread, e.g. a call to FlushRenderingCommands. */
	extern RENDERCORE_API bool GMainThreadBlockedOnRenderThread;

	/** Asserts if called from the main thread when the main thread is blocked on the rendering thread. */
	static FORCEINLINE void CheckNotBlockedOnRenderThread() { ensure(!GMainThreadBlockedOnRenderThread || !IsInGameThread()); }
#endif // #if (UE_BUILD_SHIPPING || UE_BUILD_TEST)


/** Starts the rendering thread. */
extern RENDERCORE_API void StartRenderingThread();

/** Stops the rendering thread. */
extern RENDERCORE_API void StopRenderingThread();

/**
 * Checks if the rendering thread is healthy and running.
 * If it has crashed, UE_LOG is called with the exception information.
 */
extern RENDERCORE_API void CheckRenderingThreadHealth();

/** Checks if the rendering thread is healthy and running, without crashing */
extern RENDERCORE_API bool IsRenderingThreadHealthy();

/** Advances stats for the rendering thread. */
extern RENDERCORE_API void RenderingThreadTick(int64 StatsFrame, int32 MasterDisableChangeTagStartFrame);

/**
 * Adds a task that must be completed either before the next scene draw or a flush rendering commands
 * This can be called from any thread though it probably doesn't make sense to call it from the render thread.
 * @param TaskToAdd, task to add as a pending render thread task
 */
extern RENDERCORE_API void AddFrameRenderPrerequisite(const FGraphEventRef& TaskToAdd);

/**
 * Gather the frame render prerequisites and make sure all render commands are at least queued
 */
extern RENDERCORE_API void AdvanceFrameRenderPrerequisite();

/**
 * Waits for the rendering thread to finish executing all pending rendering commands.  Should only be used from the game thread.
 */
extern RENDERCORE_API void FlushRenderingCommands();


////////////////////////////////////
// Render thread suspension
////////////////////////////////////

/**
 * Encapsulates stopping and starting the renderthread so that other threads can manipulate graphics resources.
 */
class RENDERCORE_API FSuspendRenderingThread
{
public:
	/**
	 *	Constructor that flushes and suspends the renderthread
	 *	@param bRecreateThread	- Whether the rendering thread should be completely destroyed and recreated, or just suspended.
	 */
	FSuspendRenderingThread( bool bRecreateThread );

	/** Destructor that starts the renderthread again */
	~FSuspendRenderingThread();

private:
	/** Whether we should use a rendering thread or not */
	bool bUseRenderingThread;

	/** Whether the rendering thread was currently running or not */
	bool bWasRenderingThreadRunning;

	/** Whether the rendering thread should be completely destroyed and recreated, or just suspended */
	bool bRecreateThread;
};

/** Helper macro for safely flushing and suspending the rendering thread while manipulating graphics resources */
#define SCOPED_SUSPEND_RENDERING_THREAD(bRecreateThread)	FSuspendRenderingThread SuspendRenderingThread(bRecreateThread)

////////////////////////////////////
// Render commands
////////////////////////////////////

#if UE_SERVER
#define ENQUEUE_UNIQUE_RENDER_COMMAND(TypeName,Code)
#define ENQUEUE_UNIQUE_RENDER_COMMAND(TypeName,Code)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,Code)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_CREATE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamValue1)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_DECLARE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamName1,ParamValue1,Code)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,Code)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_CREATE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamValue1,ParamType2,ParamValue2)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_DECLARE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,Code)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,Code)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_CREATE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_DECLARE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,Code)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,ParamType4,ParamName4,ParamValue4,Code)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_CREATE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3,ParamType4,ParamValue4)
#define ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_DECLARE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,ParamType4,ParamName4,ParamValue4,Code)
#else

/** The parent class of commands stored in the rendering command queue. */
class RENDERCORE_API FRenderCommand
{
public:
	// All render commands run on the render thread
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::RenderThread;
	}

	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		// Don't support tasks having dependencies on us, reduces task graph overhead tracking and dealing with subsequents
		return ESubsequentsMode::FireAndForget;
	}
};

//
// Macros for using render commands.
//

DECLARE_STATS_GROUP(TEXT("Render Thread Commands"), STATGROUP_RenderThreadCommands);

#define TASK_FUNCTION(Code) \
		void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) \
		{ \
			Code; \
		} 

#define TASKNAME_FUNCTION(TypeName) \
		FORCEINLINE static const TCHAR* GetTaskName() \
		{ \
			return TEXT( #TypeName ); \
		} \
		FORCEINLINE static TStatId GetStatId() \
		{ \
			RETURN_QUICK_DECLARE_CYCLE_STAT(TypeName, STATGROUP_RenderThreadCommands); \
		}

/**
 * Declares a rendering command type with 0 parameters.
 */
#define ENQUEUE_UNIQUE_RENDER_COMMAND(TypeName,Code) \
	class EURCMacro_##TypeName : public FRenderCommand \
	{ \
	public: \
		TASKNAME_FUNCTION(TypeName) \
		TASK_FUNCTION(Code) \
	}; \
	{ \
		if(GIsThreadedRendering || !IsInGameThread()) \
		{ \
			CheckNotBlockedOnRenderThread(); \
			TGraphTask<EURCMacro_##TypeName>::CreateTask().ConstructAndDispatchWhenReady(); \
		} \
		else \
		{ \
			EURCMacro_##TypeName TempCommand; \
			FScopeCycleCounter EURCMacro_Scope(EURCMacro_##TypeName::GetStatId()); \
			TempCommand.DoTask(ENamedThreads::GameThread, FGraphEventRef() ); \
		} \
	}

/**
 * Declares a rendering command type with 1 parameters.
 */
#define ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,Code) \
	class EURCMacro_##TypeName : public FRenderCommand \
	{ \
	public: \
		typedef ParamType1 _ParamType1; \
		EURCMacro_##TypeName(const _ParamType1& In##ParamName1): \
		  ParamName1(In##ParamName1) \
		{} \
		TASK_FUNCTION(Code) \
		TASKNAME_FUNCTION(TypeName) \
	private: \
		ParamType1 ParamName1; \
	};

#define ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_CREATE(TypeName,ParamType1,ParamValue1) \
	{ \
		if(GIsThreadedRendering || !IsInGameThread()) \
		{ \
			CheckNotBlockedOnRenderThread(); \
			TGraphTask<EURCMacro_##TypeName>::CreateTask().template ConstructAndDispatchWhenReady<ParamType1>(ParamValue1); \
		} \
		else \
		{ \
			EURCMacro_##TypeName TempCommand(ParamValue1); \
			FScopeCycleCounter EURCMacro_Scope(EURCMacro_##TypeName::GetStatId()); \
			TempCommand.DoTask(ENamedThreads::GameThread, FGraphEventRef() ); \
		} \
	}

#define ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,Code) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,Code) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_CREATE(TypeName,ParamType1,ParamValue1)

#define ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_DECLARE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamName1,ParamValue1,Code) \
	template <typename TemplateParamName> \
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,Code)

#define ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_CREATE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamValue1) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_CREATE(TypeName<TemplateParamName>,ParamType1,ParamValue1)

/**
 * Declares a rendering command type with 2 parameters.
 */
#define ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,Code) \
	class EURCMacro_##TypeName : public FRenderCommand \
	{ \
	public: \
		typedef ParamType1 _ParamType1; \
		typedef ParamType2 _ParamType2; \
		EURCMacro_##TypeName(const _ParamType1& In##ParamName1,const _ParamType2& In##ParamName2): \
		  ParamName1(In##ParamName1), \
		  ParamName2(In##ParamName2) \
		{} \
		TASK_FUNCTION(Code) \
		TASKNAME_FUNCTION(TypeName) \
	private: \
		ParamType1 ParamName1; \
		ParamType2 ParamName2; \
	};

#define ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_CREATE(TypeName,ParamType1,ParamValue1,ParamType2,ParamValue2) \
	{ \
		if(GIsThreadedRendering || !IsInGameThread()) \
		{ \
			CheckNotBlockedOnRenderThread(); \
			TGraphTask<EURCMacro_##TypeName>::CreateTask().template ConstructAndDispatchWhenReady<ParamType1,ParamType2>(ParamValue1,ParamValue2); \
		} \
		else \
		{ \
			EURCMacro_##TypeName TempCommand(ParamValue1,ParamValue2); \
			FScopeCycleCounter EURCMacro_Scope(EURCMacro_##TypeName::GetStatId()); \
			TempCommand.DoTask(ENamedThreads::GameThread, FGraphEventRef() ); \
		} \
	}

#define ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,Code) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,Code) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_CREATE(TypeName,ParamType1,ParamValue1,ParamType2,ParamValue2)

#define ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_DECLARE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,Code) \
	template <typename TemplateParamName> \
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,Code)

#define ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_CREATE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamValue1,ParamType2,ParamValue2) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER_CREATE(TypeName<TemplateParamName>,ParamType1,ParamValue1,ParamType2,ParamValue2)

/**
 * Declares a rendering command type with 3 parameters.
 */
#define ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,Code) \
	class EURCMacro_##TypeName : public FRenderCommand \
	{ \
	public: \
		typedef ParamType1 _ParamType1; \
		typedef ParamType2 _ParamType2; \
		typedef ParamType3 _ParamType3; \
		EURCMacro_##TypeName(const _ParamType1& In##ParamName1,const _ParamType2& In##ParamName2,const _ParamType3& In##ParamName3): \
		  ParamName1(In##ParamName1), \
		  ParamName2(In##ParamName2), \
		  ParamName3(In##ParamName3) \
		{} \
		TASK_FUNCTION(Code) \
		TASKNAME_FUNCTION(TypeName) \
	private: \
		ParamType1 ParamName1; \
		ParamType2 ParamName2; \
		ParamType3 ParamName3; \
	};

#define ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_CREATE(TypeName,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3) \
	{ \
		if(GIsThreadedRendering || !IsInGameThread()) \
		{ \
			CheckNotBlockedOnRenderThread(); \
			TGraphTask<EURCMacro_##TypeName>::CreateTask().template ConstructAndDispatchWhenReady<ParamType1,ParamType2,ParamType3>(ParamValue1,ParamValue2,ParamValue3); \
		} \
		else \
		{ \
			EURCMacro_##TypeName TempCommand(ParamValue1,ParamValue2,ParamValue3); \
			FScopeCycleCounter EURCMacro_Scope(EURCMacro_##TypeName::GetStatId()); \
			TempCommand.DoTask(ENamedThreads::GameThread, FGraphEventRef() ); \
		} \
	}

#define ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,Code) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,Code) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_CREATE(TypeName,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3)

#define ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_DECLARE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,Code) \
	template <typename TemplateParamName> \
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,Code)

#define ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_CREATE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_CREATE(TypeName<TemplateParamName>,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3)

/**
 * Declares a rendering command type with 4 parameters.
 */
#define ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,ParamType4,ParamName4,ParamValue4,Code) \
	class EURCMacro_##TypeName : public FRenderCommand \
	{ \
	public: \
		typedef ParamType1 _ParamType1; \
		typedef ParamType2 _ParamType2; \
		typedef ParamType3 _ParamType3; \
		typedef ParamType4 _ParamType4; \
		EURCMacro_##TypeName(const _ParamType1& In##ParamName1,const _ParamType2& In##ParamName2,const _ParamType3& In##ParamName3,const _ParamType4& In##ParamName4): \
		  ParamName1(In##ParamName1), \
		  ParamName2(In##ParamName2), \
		  ParamName3(In##ParamName3), \
		  ParamName4(In##ParamName4) \
		{} \
		TASK_FUNCTION(Code) \
		TASKNAME_FUNCTION(TypeName) \
	private: \
		ParamType1 ParamName1; \
		ParamType2 ParamName2; \
		ParamType3 ParamName3; \
		ParamType4 ParamName4; \
	};

#define ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_CREATE(TypeName,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3,ParamType4,ParamValue4) \
	{ \
		if(GIsThreadedRendering || !IsInGameThread()) \
		{ \
			CheckNotBlockedOnRenderThread(); \
			TGraphTask<EURCMacro_##TypeName>::CreateTask().template ConstructAndDispatchWhenReady<ParamType1,ParamType2,ParamType3,ParamType4>(ParamValue1,ParamValue2,ParamValue3,ParamValue4); \
		} \
		else \
		{ \
			EURCMacro_##TypeName TempCommand(ParamValue1,ParamValue2,ParamValue3,ParamValue4); \
			FScopeCycleCounter EURCMacro_Scope(EURCMacro_##TypeName::GetStatId()); \
			TempCommand.DoTask(ENamedThreads::GameThread, FGraphEventRef() ); \
		} \
	}

#define ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,ParamType4,ParamName4,ParamValue4,Code) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,ParamType4,ParamName4,ParamValue4,Code) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_CREATE(TypeName,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3,ParamType4,ParamValue4)

#define ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_DECLARE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,ParamType4,ParamName4,ParamValue4,Code) \
	template <typename TemplateParamName> \
	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_DECLARE(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,ParamType4,ParamName4,ParamValue4,Code)

#define ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_CREATE_TEMPLATE(TypeName,TemplateParamName,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3,ParamType4,ParamValue4) \
	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER_CREATE(TypeName<TemplateParamName>,ParamType1,ParamValue1,ParamType2,ParamValue2,ParamType3,ParamValue3,ParamType4,ParamValue4)

#endif //UE_SERVER

////////////////////////////////////
// Render fences
////////////////////////////////////

 /**
 * Used to track pending rendering commands from the game thread.
 */
class RENDERCORE_API FRenderCommandFence
{
public:

	/**
	 * Adds a fence command to the rendering command queue.
	 * Conceptually, the pending fence count is incremented to reflect the pending fence command.
	 * Once the rendering thread has executed the fence command, it decrements the pending fence count.
	 */
	void BeginFence();

	/**
	 * Waits for pending fence commands to retire.
	 * @param bProcessGameThreadTasks, if true we are on a short callstack where it is safe to process arbitrary game thread tasks while we wait
	 */
	void Wait(bool bProcessGameThreadTasks = false) const;

	// return true if the fence is complete
	bool IsFenceComplete() const;

private:
	/** Graph event that represents completion of this fence **/
	mutable FGraphEventRef CompletionEvent;
};

////////////////////////////////////
// Deferred cleanup
////////////////////////////////////

/**
 * The base class of objects that need to defer deletion until the render command queue has been flushed.
 */
class RENDERCORE_API FDeferredCleanupInterface
{
public:
	virtual void FinishCleanup() = 0;
	virtual ~FDeferredCleanupInterface() {}
};

/**
 * A set of cleanup objects which are pending deletion.
 */
class FPendingCleanupObjects
{
	TArray<FDeferredCleanupInterface*> CleanupArray;
public:
	FPendingCleanupObjects();
	RENDERCORE_API ~FPendingCleanupObjects();
};

/**
 * Adds the specified deferred cleanup object to the current set of pending cleanup objects.
 */
extern RENDERCORE_API void BeginCleanup(FDeferredCleanupInterface* CleanupObject);

/**
 * Transfers ownership of the current set of pending cleanup objects to the caller.  A new set is created for subsequent BeginCleanup calls.
 * @return A pointer to the set of pending cleanup objects.  The called is responsible for deletion.
 */
extern RENDERCORE_API FPendingCleanupObjects* GetPendingCleanupObjects();

#endif
