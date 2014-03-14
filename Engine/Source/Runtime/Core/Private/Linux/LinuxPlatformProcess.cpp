// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinuxPlatformProcess.cpp: Linux implementations of Process functions
=============================================================================*/

#include "CorePrivate.h"
#include "LinuxPlatformRunnableThread.h"
#include <spawn.h>
#include <sys/wait.h>
#include <sys/resource.h>

namespace PlatformProcessLimits
{
	enum
	{
		MaxComputerName	= 128,
		MaxBaseDirLength= MAX_PATH + 1,
		MaxArgvParameters = 256
	};
};

const TCHAR* FLinuxPlatformProcess::ComputerName()
{
	static bool bHaveResult = false;
	static TCHAR CachedResult[ PlatformProcessLimits::MaxComputerName ];
	if (!bHaveResult)
	{
		struct utsname name;
		const char * SysName = name.nodename;
		if(uname(&name))
		{
			SysName = "Linux Computer";
		}

		FCString::Strcpy(CachedResult, ARRAY_COUNT(CachedResult) - 1, ANSI_TO_TCHAR(SysName));
		CachedResult[ARRAY_COUNT(CachedResult) - 1] = 0;
		bHaveResult = true;
	}
	return CachedResult;
}

const TCHAR* FLinuxPlatformProcess::BaseDir()
{
	static bool bHaveResult = false;
	static TCHAR CachedResult[ PlatformProcessLimits::MaxBaseDirLength ];

	if (!bHaveResult)
	{
		char SelfPath[ PlatformProcessLimits::MaxBaseDirLength ] = {0};
		readlink( "/proc/self/exe", SelfPath, ARRAY_COUNT(SelfPath) - 1);
		SelfPath[ARRAY_COUNT(SelfPath) - 1] = 0;

		FCString::Strcpy(CachedResult, ARRAY_COUNT(CachedResult) - 1, ANSI_TO_TCHAR(dirname(SelfPath)));
		CachedResult[ARRAY_COUNT(CachedResult) - 1] = 0;
		FCString::Strcat(CachedResult, ARRAY_COUNT(CachedResult) - 1, TEXT("/"));
		bHaveResult = true;
	}
	return CachedResult;
}

bool FLinuxPlatformProcess::SetProcessLimits(EProcessResource::Type Resource, uint64 Limit)
{
	rlimit NativeLimit;
	NativeLimit.rlim_cur = Limit;
	NativeLimit.rlim_max = Limit;

	int NativeResource = RLIMIT_AS;

	switch(Resource)
	{
		case EProcessResource::VirtualMemory:
			NativeResource = RLIMIT_AS;
			break;

		default:
			UE_LOG(LogHAL, Warning, TEXT("Unkown resource type %d"), Resource);
			return false;
	}

	if (setrlimit(NativeResource, &NativeLimit) != 0)
	{
		UE_LOG(LogHAL, Warning, TEXT("setrlimit() failed with error %d (%s)\n"), errno, ANSI_TO_TCHAR(strerror(errno)));
		return false;
	}

	return true;
}

const TCHAR* FLinuxPlatformProcess::ExecutableName(bool bRemoveExtension)
{
	static bool bHaveResult = false;
	static TCHAR CachedResult[ PlatformProcessLimits::MaxBaseDirLength ];
	if (!bHaveResult)
	{
		char SelfPath[ PlatformProcessLimits::MaxBaseDirLength ] = {0};
		readlink( "/proc/self/exe", SelfPath, ARRAY_COUNT(SelfPath) - 1);
		SelfPath[ARRAY_COUNT(SelfPath) - 1] = 0;

		FCString::Strcpy(CachedResult, ARRAY_COUNT(CachedResult) - 1, ANSI_TO_TCHAR(basename(SelfPath)));
		CachedResult[ARRAY_COUNT(CachedResult) - 1] = 0;
		bHaveResult = true;
	}
	return CachedResult;
}


void FLinuxPlatformProcess::ClosePipe( void* ReadPipe, void* WritePipe )
{
	//close(*(int*)WritePipe);
	UE_LOG(LogHAL, Fatal, TEXT("FGenericPlatformProcess::ClosePipe not implemented on this platform"));
}

bool FLinuxPlatformProcess::CreatePipe( void*& ReadPipe, void*& WritePipe )
{
	//int pipefd[2];
	//pipe(pipefd);
	//ReadPipe = &pipefd[0];
	//WritePipe = &pipefd[1];
	UE_LOG(LogHAL, Fatal, TEXT("FGenericPlatformProcess::CreatePipe not implemented on this platform"));
	return false;
}

FString FLinuxPlatformProcess::ReadPipe( void* ReadPipe )
{
	//BytesRead = read(*(int *)ReadPipe, Buffer, sizeof(Buffer) - 1)
	UE_LOG(LogHAL, Fatal, TEXT("FGenericPlatformProcess::ReadPipe not implemented on this platform"));
	return FString();
}

FRunnableThread* FLinuxPlatformProcess::CreateRunnableThread()
{
	return new FRunnableThreadLinux();
}

FProcHandle FLinuxPlatformProcess::CreateProc( const TCHAR* URL, const TCHAR* Parms, bool bLaunchDetached, bool bLaunchHidden, bool bLaunchReallyHidden, uint32* OutProcessID, int32 PriorityModifier, const TCHAR* OptionalWorkingDirectory, void* PipeWrite )
{
	FString AbsolutePath = FPaths::ConvertRelativePathToFull(URL);
	FString Commandline = AbsolutePath;
	Commandline += TEXT(" ");
	Commandline += Parms;

	UE_LOG(LogHAL, Verbose, TEXT("FLinuxPlatformProcess::CreateProc: '%s'"), *Commandline);

	TArray<FString> ArgvArray;
	int Argc = Commandline.ParseIntoArray(&ArgvArray, TEXT(" "), true);
	char * Argv[PlatformProcessLimits::MaxArgvParameters + 1] = { NULL };	// last argument is NULL, hence +1
	struct CleanupArgvOnExit
	{
		int Argc;
		char ** Argv;	// relying on it being long enough to hold Argc elements
		
		CleanupArgvOnExit( int InArgc, char *InArgv[] )
			:	Argc(InArgc)
			,	Argv(InArgv)
		{}

		~CleanupArgvOnExit()
		{
			for (int Idx = 0; Idx < Argc; ++Idx)
			{
				FMemory::Free(Argv[Idx]);
			}
		}
	} CleanupGuard(Argc, Argv);

	if (Argc > 0)	// almost always, unless there's no program name
	{
		if (Argc > PlatformProcessLimits::MaxArgvParameters)
		{
			UE_LOG(LogHAL, Warning, TEXT("FLinuxPlatformProcess::CreateProc: too many (%d) commandline arguments passed, will only pass %d"), 
				Argc, PlatformProcessLimits::MaxArgvParameters);
			Argc = PlatformProcessLimits::MaxArgvParameters;
		}

		for (int Idx = 0; Idx < Argc; ++Idx)
		{
			auto AnsiBuffer = StringCast<ANSICHAR>(*ArgvArray[Idx]);
			const char * Ansi = AnsiBuffer.Get();
			size_t AnsiSize = FCStringAnsi::Strlen(Ansi) + 1;
			check(AnsiSize);

			Argv[Idx] = reinterpret_cast< char* >( FMemory::Malloc(AnsiSize) );
			check(Argv[Idx]);

			FCStringAnsi::Strncpy(Argv[Idx], Ansi, AnsiSize);
		}

		// last Argv should be NULL
		check(Argc <= PlatformProcessLimits::MaxArgvParameters + 1);
		Argv[Argc] = NULL;
	}

	extern char ** environ;	// provided by libc
	pid_t ChildPid = -1;

	int ErrNo = posix_spawn(&ChildPid, TCHAR_TO_ANSI(*AbsolutePath), NULL, NULL, Argv, environ);
	if (ErrNo != 0)
	{
		UE_LOG(LogHAL, Fatal, TEXT("FLinuxPlatformProcess::CreateProc: posix_spawn() failed (%d, %s)"), ErrNo, ANSI_TO_TCHAR(strerror(ErrNo)));
		return FProcHandle();	// produce knowingly invalid handle if for some reason Fatal log (above) returns
	}
	else
	{
		UE_LOG(LogHAL, Log, TEXT("FLinuxPlatformProcess::CreateProc: spawned child %d"), ChildPid);
	}

	return FProcHandle( ChildPid );
}

bool FProcHandle::IsRunning()
{
	if (bIsRunning)
	{
		check(!bHasBeenWaitedFor);	// check for the sake of internal consistency

		// check if actually running
		int KillResult = kill(Get(), 0);	// no actual signal is sent
		check(KillResult != -1 || errno != EINVAL);

		bIsRunning = (KillResult == 0 || (KillResult == -1 && errno == EPERM));

		// additional check if it's a zombie
		if (bIsRunning)
		{
			for(;;)	// infinite loop in case we get EINTR and have to repeat
			{
				siginfo_t SignalInfo;
				SignalInfo.si_pid = 0;	// if remains 0, treat as child was not waitable (i.e. was running)
				if (waitid(P_PID, Get(), &SignalInfo, WEXITED | WNOHANG | WNOWAIT))
				{
					if (errno != EINTR)
					{
						UE_LOG(LogHAL, Fatal, TEXT("FLinuxPlatformProcess::WaitForProc: waitid for pid %d failed (errno=%d, %s)"), 
							static_cast< int32 >(Get()), errno, ANSI_TO_TCHAR(strerror(errno)));
						break;	// exit the loop if for some reason Fatal log (above) returns
					}
				}
				else
				{
					// since we used WNOWAIT, we don't have to collect all the info 
					// even if child is indeed a zombie
					bIsRunning = ( SignalInfo.si_pid != Get() );
					break;
				}
			}
		}
	}

	return bIsRunning;
}

bool FProcHandle::GetReturnCode(int32* ReturnCodePtr)
{
	check(!bIsRunning || !"You cannot get a return code of a running process");
	if (!bHasBeenWaitedFor)
	{
		Wait();
	}

	if (ReturnCode < 0)
	{
		if (ReturnCodePtr != NULL)
		{
			*ReturnCodePtr = ReturnCode;
		}
		return true;
	}

	return false;
}

void FProcHandle::Wait()
{
	if (bHasBeenWaitedFor)
	{
		return;	// we could try waitpid() another time, but why
	}

	for(;;)	// infinite loop in case we get EINTR and have to repeat
	{
		siginfo_t SignalInfo;
		if (waitid(P_PID, Get(), &SignalInfo, WEXITED))
		{
			if (errno != EINTR)
			{
				UE_LOG(LogHAL, Fatal, TEXT("FLinuxPlatformProcess::WaitForProc: waitid for pid %d failed (errno=%d, %s)"), 
					static_cast< int32 >(Get()), errno, ANSI_TO_TCHAR(strerror(errno)));
				break;	// exit the loop if for some reason Fatal log (above) returns
			}
		}
		else
		{
			check(SignalInfo.si_pid == Get());

			ReturnCode = (SignalInfo.si_code == CLD_EXITED) ? SignalInfo.si_status : -1;
			bHasBeenWaitedFor = true;
			bIsRunning = false;	// set in advance
			break;
		}
	}
}

bool FLinuxPlatformProcess::IsProcRunning( FProcHandle & ProcessHandle )
{
	return ProcessHandle.IsRunning();
}

void FLinuxPlatformProcess::WaitForProc( FProcHandle & ProcessHandle )
{
	ProcessHandle.Wait();
}

void FLinuxPlatformProcess::TerminateProc( FProcHandle & ProcessHandle, bool KillTree )
{
	if (KillTree)
	{
		// TODO: enumerate the children
		check(!"Killing a subtree is not implemented yet");
	}

	int KillResult = kill(ProcessHandle.Get(), SIGTERM);	// graceful
	check(KillResult != -1 || errno != EINVAL);
}

bool FLinuxPlatformProcess::GetProcReturnCode( FProcHandle & ProcHandle, int32* ReturnCode )
{
	if (IsProcRunning(ProcHandle))
	{
		return false;
	}

	return ProcHandle.GetReturnCode(ReturnCode);
}
