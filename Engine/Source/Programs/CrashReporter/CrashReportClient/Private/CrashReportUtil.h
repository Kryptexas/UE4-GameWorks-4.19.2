// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

template <class TWorker>
class TOneShotTaskUsingDedicatedThread : FRunnable
{
public:
	/**
	 * Constructor
	 */
	TOneShotTaskUsingDedicatedThread()
		: bIsDone(1)
	{
	}

	/**
	 * Destructor
	 */
	~TOneShotTaskUsingDedicatedThread()
	{
		check(bIsDone);
	}

	TWorker& GetTask()
	{
		return Worker;
	}

	void StartBackgroundTask()
	{
		bIsDone = 0;

		bool bAutoDeleteThread = true;
		bool bAutoDeleteRunnable = false;
		FRunnableThread::Create(this, Worker.Name(), bAutoDeleteThread, bAutoDeleteRunnable);
	}

	/**
	 * No need to join the thread, since this can't be reused
	 */
	bool IsDone()
	{
		return bIsDone == 1;
	}

	bool IsWorkDone() const
	{
		return bIsDone == 1;
	}

private:
	virtual uint32 Run() OVERRIDE
	{
		Worker.DoWork();
		FPlatformAtomics::InterlockedIncrement(&bIsDone);
		return 0;
	}

	TWorker Worker;

	volatile int32 bIsDone;	
};


/**
 * Helper class for MakeDirectoryVisitor
 */
template <class FunctorType>
class FunctorDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
{
public:
	/**
	 * Pass a directory or filename on to the user-provided functor
	 * @param FilenameOrDirectory Full path to a file or directory
	 * @param bIsDirectory Whether the path refers to a file or directory
	 * @return Whether to carry on iterating
	 */
	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) OVERRIDE
	{
		return Functor(FilenameOrDirectory, bIsDirectory);
	}

	/**
	 * Move the provided functor into this object
	 */
	FunctorDirectoryVisitor(FunctorType&& FunctorInstance)
		: Functor(MoveTemp(FunctorInstance))
	{
	}

private:
	/** User-provided functor */
	FunctorType Functor;
};

/**
 * Convert a C++ functor into a IPlatformFile visitor object
 * @param FunctorInstance Function object to call for each directory item
 * @return Visitor object to be passed to platform directory visiting functions
 */
template <class Functor>
FunctorDirectoryVisitor<Functor> MakeDirectoryVisitor(Functor&& FunctorInstance)
{
	return FunctorDirectoryVisitor<Functor>(MoveTemp(FunctorInstance));
}

/**
 * Create a multiline string to display from an exception and callstack
 * @param Exception Exception description string
 * @param Callstack List of callstack entry strings
 * @return Multiline text
 */
inline FText FormatReportDescription(const FString& Exception, const TArray<FString>& Callstack)
{
	FString Diagnostic = Exception + "\n\n";

	for (const auto& Line: Callstack)
	{
		Diagnostic += Line + "\n";
	}
	return FText::FromString(Diagnostic);
}
