// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ApplePlatformStackWalk.mm: Apple implementations of stack walk functions
=============================================================================*/

#include "CorePrivate.h"
#include <execinfo.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <cxxabi.h>

// Internal helper functions not exposed to public
int32 GetModuleImageSize( const struct mach_header* Header )
{
	struct load_command *CurrentCommand = (struct load_command *)( (char *)Header + sizeof(struct mach_header) );
	int32 ModuleSize = 0;
	
	// Check header
	if( Header->magic != MH_MAGIC )
		return 0;
	
	for( int32 i = 0; i < Header->ncmds; i++ )
	{
		if( CurrentCommand->cmd == LC_SEGMENT )
		{
			struct segment_command *SegmentCommand = (struct segment_command *) CurrentCommand;
			ModuleSize += SegmentCommand->vmsize;
		}
		
		CurrentCommand = (struct load_command *)( (char *)CurrentCommand + CurrentCommand->cmdsize );
	}
	
	return ModuleSize;
}

int32 GetModuleTimeStamp( const struct mach_header* Header )
{
	struct load_command *CurrentCommand = (struct load_command *)( (char *)Header + sizeof(struct mach_header) );
	
	// Check header
	if( Header->magic != MH_MAGIC )
		return 0;
	
	for( int32 i = 0; i < Header->ncmds; i++ )
	{
		if( CurrentCommand->cmd == LC_LOAD_DYLIB ) //LC_ID_DYLIB )
		{
			struct dylib_command *DylibCommand = (struct dylib_command *) CurrentCommand;
			return DylibCommand->dylib.timestamp;
		}
		
		CurrentCommand = (struct load_command *)( (char *)CurrentCommand + CurrentCommand->cmdsize );
	}
	
	return 0;
}


void FApplePlatformStackWalk::CaptureStackBackTrace( uint64* BackTrace, uint32 MaxDepth, void* Context )
{
	// Make sure we have place to store the information before we go through the process of raising
	// an exception and handling it.
	if( BackTrace == NULL || MaxDepth == 0 )
	{
		return;
	}

	backtrace((void**)BackTrace, MaxDepth);
}

bool FApplePlatformStackWalk::ProgramCounterToHumanReadableString( int32 CurrentCallDepth, uint64 ProgramCounter, ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, EVerbosityFlags VerbosityFlags, FGenericCrashContext* Context )
{
	Dl_info DylibInfo;
	int32 Result = dladdr((const void*)ProgramCounter, &DylibInfo);
	if (Result == 0)
	{
		return false;
	}

	FProgramCounterSymbolInfo SymbolInfo;
	ProgramCounterToSymbolInfo( ProgramCounter, SymbolInfo );
	
	// Write out function name.
	FCStringAnsi::Strcat(HumanReadableString, HumanReadableStringSize, SymbolInfo.FunctionName);

	if (VerbosityFlags & VF_DISPLAY_FILENAME)
	{
		ANSICHAR FileNameLine[MAX_SPRINTF];
		
		if(SymbolInfo.LineNumber == 0)
		{
			// No line number. Print out the logical address instead.
			FCStringAnsi::Sprintf(FileNameLine, "Address = 0x%-8x (filename not found) ", ProgramCounter);
		}
		else
		{
			// try to add source file and line number, too
			FCStringAnsi::Sprintf(FileNameLine, "[%s, line %d] ", SymbolInfo.Filename, SymbolInfo.LineNumber);
		}
		
		FCStringAnsi::Strcat(HumanReadableString, HumanReadableStringSize, FileNameLine);
	}

	if (VerbosityFlags & VF_DISPLAY_MODULE)
	{
		ANSICHAR ModuleName[MAX_SPRINTF];
		// Write out Module information if there is sufficient space.
		FCStringAnsi::Sprintf(ModuleName, "[in %s]", SymbolInfo.ModuleName);
		FCStringAnsi::Strcat(HumanReadableString, HumanReadableStringSize, ModuleName);
	}
	
	// For the crash reporting code this needs a Windows line ending, the caller is responsible for the '\n'
	FCStringAnsi::Strcat(HumanReadableString, HumanReadableStringSize, "\r");

	return true;
}

void FApplePlatformStackWalk::ProgramCounterToSymbolInfo( uint64 ProgramCounter, FProgramCounterSymbolInfo&  SymbolInfo )
{
	Dl_info DylibInfo;
	int32 Result = dladdr((const void*)ProgramCounter, &DylibInfo);
	if (Result == 0)
	{
		return;
	}

	int32 Status = 0;
	ANSICHAR* DemangledName = NULL;
	
	size_t DemangledNameLen = MAX_SPRINTF;
	ANSICHAR DemangledNameBuffer[MAX_SPRINTF]= {0};
	DemangledName = abi::__cxa_demangle(DylibInfo.dli_sname, DemangledNameBuffer, &DemangledNameLen, &Status);

	ANSICHAR FunctionName[MAX_SPRINTF];
	if (DemangledName)
	{
		// C++ function
		FCStringAnsi::Sprintf(SymbolInfo.FunctionName, "%s ", DemangledName);
	}
	else if (DylibInfo.dli_sname && strchr(DylibInfo.dli_sname, ']'))
	{
		// ObjC function
		FCStringAnsi::Sprintf(SymbolInfo.FunctionName, "%s ", DylibInfo.dli_sname);
	}
	else if(DylibInfo.dli_sname)
	{
		// C function
		FCStringAnsi::Sprintf(SymbolInfo.FunctionName, "%s() ", DylibInfo.dli_sname);
	}
	else
	{
		// Unknown!
		FCStringAnsi::Sprintf(SymbolInfo.FunctionName, "[Unknown]() ");
	}

	bool OK = false;
	
#if PLATFORM_MAC
	{
		static char FileName[65536];
		FMemory::MemSet(FileName, 0);
		
		// Use fork() & execl() as they are async-signal safe, ExecProcess can fail in Cocoa
		int32 ReturnCode = 0;
		int FileDesc[2];
		pipe(FileDesc);
		pid_t ForkPID = fork();
		if(ForkPID == 0)
		{
			// Child
			close(FileDesc[0]);
			dup2(FileDesc[1], STDOUT_FILENO);
			
			static char FBase[65];
			static char Address[65];
			FCStringAnsi::Sprintf(FBase, "%p", (void*)DylibInfo.dli_fbase);
			FCStringAnsi::Sprintf(Address, "%p", (void*)ProgramCounter);
			
			execl("/usr/bin/atos", "-d", "-l", FBase, "-o", DylibInfo.dli_fname, Address, NULL);
			close(FileDesc[1]);
		}
		else
		{
			// Parent
			int StatLoc = 0;
			pid_t WaitPID = waitpid(ForkPID, &StatLoc, 0);
			int Bytes = 0;
			close(FileDesc[1]);
			int32 FirstIndex = -1;
			int32 LastIndex = -1;
			char* Buffer = FileName;
			uint32 Len = ARRAY_COUNT(FileName) - 1;
			while ((Bytes = read(FileDesc[0], Buffer, Len)) > 0)
			{
				Buffer += Bytes;
				Len -= Bytes;
			}
			close(FileDesc[0]);
			ReturnCode = WEXITSTATUS(StatLoc);
		}
		
		if(ReturnCode == 0)
		{
			char* FirstBracket = FCStringAnsi::Strchr(FileName, '(');
			char* SecondBracket = FCStringAnsi::Strrchr(FileName, '(');
			if(FirstBracket && SecondBracket && FirstBracket != SecondBracket)
			{
				char* CloseBracket = SecondBracket;
				CloseBracket = FCStringAnsi::Strrchr(CloseBracket, ')');
				if(CloseBracket)
				{
					char* Colon = SecondBracket;
					Colon = FCStringAnsi::Strchr(Colon, ':');
					if(Colon)
					{
						*Colon = 0;
					}
					*SecondBracket = 0;
					
					FCStringAnsi::Strncpy(SymbolInfo.Filename, (SecondBracket+1), ARRAY_COUNT(SymbolInfo.Filename)-1);
					if(Colon)
					{
						// I'm going to assume that this is safe too
						SymbolInfo.LineNumber = FCStringAnsi::Atoi(Colon+1);
					}
					OK = true;
				}
			}
		}
	}
#endif
	
	if(!OK)
	{
		// No line number found.
		FCStringAnsi::Strcat(SymbolInfo.Filename, "Unknown");
		SymbolInfo.LineNumber = 0;
	}

	// Write out Module information.
	ANSICHAR* DylibPath = (ANSICHAR*)DylibInfo.dli_fname;
	ANSICHAR* DylibName = FCStringAnsi::Strrchr(DylibPath, '/');
	if(DylibName)
	{
		DylibName += 1;
	}
	else
	{
		DylibName = DylibPath;
	}
	FCStringAnsi::Strcpy(SymbolInfo.ModuleName, DylibName);
}

int32 FApplePlatformStackWalk::GetProcessModuleCount()
{
	return (int32)_dyld_image_count();
}

int32 FApplePlatformStackWalk::GetProcessModuleSignatures(FStackWalkModuleInfo *ModuleSignatures, const int32 ModuleSignaturesSize)
{
	int32 ModuleCount = GetProcessModuleCount();

	int32 SignatureIndex = 0;

	for (int32 ModuleIndex = 0; ModuleIndex < ModuleCount && SignatureIndex < ModuleSignaturesSize; ModuleIndex++)
	{
		const struct mach_header* Header = _dyld_get_image_header(ModuleIndex);
		const ANSICHAR* ImageName = _dyld_get_image_name(ModuleIndex);
		if (Header && ImageName)
		{
			FStackWalkModuleInfo Info;
			Info.BaseOfImage = (uint64)Header;
			FCString::Strcpy(Info.ImageName, ANSI_TO_TCHAR(ImageName));
			Info.ImageSize = GetModuleImageSize( Header );
			FCString::Strcpy(Info.LoadedImageName, ANSI_TO_TCHAR(ImageName));
			FCString::Strcpy(Info.ModuleName, ANSI_TO_TCHAR(ImageName));
			Info.PdbAge = 0;
			Info.PdbSig = 0;
			FMemory::Memzero(&Info.PdbSig70, sizeof(Info.PdbSig70));
			Info.TimeDateStamp = GetModuleTimeStamp( Header );

			ModuleSignatures[SignatureIndex] = Info;
			++SignatureIndex;
		}
	}
	
	return SignatureIndex;
}

void CreateExceptionInfoString(int32 Signal, struct __siginfo* Info)
{
	FString ErrorString = TEXT("Unhandled Exception: ");
	
#define HANDLE_CASE(a,b) case a: ErrorString += TEXT(#a #b); break;
	
	switch (Signal)
	{
		case SIGSEGV:
			ErrorString += TEXT("SIGSEGV segmentation violation at address ");
			ErrorString += FString::Printf(TEXT("0x%08x"), (uint32*)Info->si_addr);
			break;
		case SIGBUS:
			ErrorString += TEXT("SIGBUS bus error at address ");
			ErrorString += FString::Printf(TEXT("0x%08x"), (uint32*)Info->si_addr);
			break;
			
		HANDLE_CASE(SIGINT, "interrupt program")
		HANDLE_CASE(SIGQUIT, "quit program")
		HANDLE_CASE(SIGILL, "illegal instruction")
		HANDLE_CASE(SIGTRAP, "trace trap")
		HANDLE_CASE(SIGABRT, "abort() call")
		HANDLE_CASE(SIGFPE, "floating-point exception")
		HANDLE_CASE(SIGKILL, "kill program")
		HANDLE_CASE(SIGSYS, "non-existent system call invoked")
		HANDLE_CASE(SIGPIPE, "write on a pipe with no reader")
		HANDLE_CASE(SIGTERM, "software termination signal")
		HANDLE_CASE(SIGSTOP, "stop")

		default:
			ErrorString += FString::Printf(TEXT("0x%08x"), (uint32)Signal);
	}
	
#if WITH_EDITORONLY_DATA
	FCString::Strncpy(GErrorExceptionDescription, *ErrorString, FMath::Min(ErrorString.Len() + 1, (int32)ARRAY_COUNT(GErrorExceptionDescription)));
#endif
#undef HANDLE_CASE
}

void NewReportEnsure( const TCHAR* ErrorMessage )
{
}

int32 ReportCrash(ucontext_t *Context, int32 Signal, struct __siginfo* Info)
{
	static bool GAlreadyCreatedMinidump = false;
	// Only create a minidump the first time this function is called.
	// (Can be called the first time from the RenderThread, then a second time from the MainThread.)
	if ( GAlreadyCreatedMinidump == false )
	{
		GAlreadyCreatedMinidump = true;

		const SIZE_T StackTraceSize = 65535;
		ANSICHAR* StackTrace = (ANSICHAR*) FMemory::Malloc( StackTraceSize );
		StackTrace[0] = 0;
		// Walk the stack and dump it to the allocated memory.
		FPlatformStackWalk::StackWalkAndDump( StackTrace, StackTraceSize, 0, Context );
#if WITH_EDITORONLY_DATA
        FCString::Strncat( GErrorHist, ANSI_TO_TCHAR(StackTrace), ARRAY_COUNT(GErrorHist) - 1 );
		CreateExceptionInfoString(Signal, Info);
#endif
		FMemory::Free( StackTrace );
	}

	return 0;
}
