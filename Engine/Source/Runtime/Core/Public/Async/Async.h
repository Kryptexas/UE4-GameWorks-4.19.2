// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Future.h"
#include "TaskGraphInterfaces.h"


/**
 * Enumerates available asynchronous execution methods.
 */
enum class EAsyncExecution
{
	/** Execute in Task Graph (for short running tasks). */
	TaskGraph,

	/** Execute in separate thread (for long running tasks). */
	Thread,

	/** Execute in queued thread pool. */
	ThreadPool
};


/**
 * Template for asynchronous functions that are executed in the Task Graph system.
 */
template<typename ResultType>
class TAsyncTask
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InFunction The function to execute asynchronously.
	 * @param InPromise The promise object used to return the function's result.
	 */
	TAsyncTask(TFunction<ResultType()>&& InFunction, TPromise<ResultType>&& InPromise)
		: Function(MoveTemp(InFunction))
		, Promise(MoveTemp(InPromise))
	{ }

public:

	/**
	 * Performs the actual task.
	 *
	 * @param CurrentThread The thread that this task is executing on.
	 * @param MyCompletionGraphEvent The completion event.
	 */
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		Promise.SetValue(Function());
	}

	/**
	 * Returns the name of the thread that this task should run on.
	 *
	 * @return Always run on any thread.
	 */
	ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}

	/**
	 * Gets the future that will hold the asynchronous result.
	 *
	 * @return A TFuture object.
	 */
	TFuture<ResultType> GetFuture()
	{
		return Promise.GetFuture();
	}

	/**
	 * Gets the task's stats tracking identifier.
	 *
	 * @return Stats identifier.
	 */
	TStatId GetStatId() const
	{
		return GET_STATID(STAT_TaskGraph_OtherTasks);
	}

	/**
	 * Gets the mode for tracking subsequent tasks.
	 *
	 * @return Always track subsequent tasks.
	 */
	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::TrackSubsequents;
	}

private:

	/** The function to execute on the Task Graph. */
	TFunction<ResultType()> Function;

	/** The promise to assign the result to. */
	TPromise<ResultType> Promise;
};


/**
 * Template for asynchronous functions that are executed in a separate thread.
 */
template<typename ResultType>
class TAsyncRunnable
	: public FRunnable
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InFunction The function to execute asynchronously.
	 * @param InPromise The promise object used to return the function's result.
	 * @param InThreadFuture The thread that is running this task.
	 */
	TAsyncRunnable(TFunction<ResultType()>&& InFunction, TPromise<ResultType>&& InPromise, TFuture<FRunnableThread*>&& InThreadFuture)
		: Function(InFunction)
		, Promise(MoveTemp(InPromise))
		, ThreadFuture(MoveTemp(InThreadFuture))
	{ }

public:

	// FRunnable interface

	virtual uint32 Run() override
	{
		Promise.SetValue(Function());

		FRunnableThread* Thread = ThreadFuture.Get();

		delete Thread;
		delete this;

		return 0;
	}

private:

	/** The function to execute on the Task Graph. */
	TFunction<ResultType()> Function;

	/** The promise to assign the result to. */
	TPromise<ResultType> Promise;

	/** The thread running this task. */
	TFuture<FRunnableThread*> ThreadFuture;
};


/**
 * Executes a given function asynchronously.
 *
 * @param Execution The execution method to use, i.e. on Task Graph or in a separate thread.
 * @param Function The function to execute.
 * @result A TFuture object that will receive the return value from the function.
 */
template<typename ResultType>
TFuture<ResultType> Async(EAsyncExecution Execution, TFunction<ResultType()> Function)
{
	TPromise<ResultType> Promise;
	TFuture<ResultType> Future = Promise.GetFuture();

	switch (Execution)
	{
	case EAsyncExecution::TaskGraph:
		{
			TGraphTask<TAsyncTask<ResultType>>::CreateTask().ConstructAndDispatchWhenReady(MoveTemp(Function), MoveTemp(Promise));
		}
		break;
	
	case EAsyncExecution::Thread:
		{
			TPromise<FRunnableThread*> ThreadPromise;
			TAsyncRunnable<ResultType>* Runnable = new TAsyncRunnable<ResultType>(MoveTemp(Function), MoveTemp(Promise), ThreadPromise.GetFuture());
			FRunnableThread* RunnableThread = FRunnableThread::Create(Runnable, TEXT("TAsync"));

			check(RunnableThread != nullptr);

			ThreadPromise.SetValue(RunnableThread);
		}
		break;

	default:
		check(false); // not implemented yet!
	}

	return MoveTemp(Future);
}
