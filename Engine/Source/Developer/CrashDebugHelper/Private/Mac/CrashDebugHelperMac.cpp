// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashDebugHelperPrivatePCH.h"
#include "Database.h"
#include "ApplePlatformSymbolication.h"
#include <cxxabi.h>

#include "ISourceControlModule.h"

static int32 ParseReportVersion(TCHAR const* CrashLog, int32& OutVersion)
{
	int32 Found = 0;
	TCHAR const* VersionLine = FCStringWide::Strstr(CrashLog, TEXT("Report Version:"));
	if (VersionLine)
	{
		Found = swscanf(VersionLine, TEXT("%*ls %*ls %d"), &OutVersion);
	}
	return Found;
}

static int32 ParseVersion(TCHAR const* CrashLog, int32& OutMajor, int32& OutMinor, int32& OutBuild, int32& OutChangeList, FString& OutBranch)
{
	int32 Found = 0;
	TCHAR const* VersionLine = FCStringWide::Strstr(CrashLog, TEXT("Version:"));
	if (VersionLine)
	{
		TCHAR Branch[257] = {0};
		Found = swscanf(VersionLine, TEXT("%*s %d.%d.%d (%*d.%*d.%*d-%d+%256ls)"), &OutMajor, &OutMinor, &OutBuild, &OutChangeList, Branch);
		if(Found == 5)
		{
			TCHAR* BranchEnd = FCStringWide::Strchr(Branch, TEXT(')'));
			if(BranchEnd)
			{
				*BranchEnd = TEXT('\0');
			}
			OutBranch = Branch;
		}
	}
	return Found;
}

static int32 ParseOS(TCHAR const* CrashLog, int32& OutMajor, int32& OutMinor, int32& OutPatch, int32& OutBuild)
{
	int32 Found = 0;
	TCHAR const* VersionLine = FCStringWide::Strstr(CrashLog, TEXT("OS Version:"));
	if (VersionLine)
	{
		Found = swscanf(VersionLine, TEXT("%*s %*s Mac OS X %d.%d.%d (%xd)"), &OutMajor, &OutMinor, &OutPatch, &OutBuild);
	}
	return Found;
}

static bool ParseModel(TCHAR const* CrashLog, FString& OutModelDetails)
{
	bool bFound = false;
	TCHAR const* Line = FCStringWide::Strstr(CrashLog, TEXT("Model:"));
	if (Line)
	{
		Line += FCStringWide::Strlen(TEXT("Model: "));
		TCHAR const* End = FCStringWide::Strchr(Line, TEXT('\r'));
		if(!End)
		{
			End = FCStringWide::Strchr(Line, TEXT('\n'));
		}
		check(End);
		
		int32 Length = FMath::Min(256, (int32)((uintptr_t)(End - Line)));
		OutModelDetails.Append(Line, Length);
		
		bFound = true;
	}
	return bFound;
}

static int32 ParseGraphics(TCHAR const* CrashLog, FString& OutGPUDetails)
{
	bool bFound = false;
	TCHAR const* Line = FCStringWide::Strstr(CrashLog, TEXT("Graphics:"));
	int32 Output = 0;
	while (Line)
	{
		Line += FCStringWide::Strlen(TEXT("Graphics:"));
		TCHAR const* End = FCStringWide::Strchr(Line, TEXT('\r'));
		if(!End)
		{
			End = FCStringWide::Strchr(Line, TEXT('\n'));
		}
		check(End);
		
		int32 Length = FMath::Min((256 - Output), (int32)((uintptr_t)(End - Line)));
		OutGPUDetails.Append(Line, Length);
		
		Line = FCStringWide::Strstr(Line, TEXT("Graphics:"));
		
		bFound = true;
	}
	return bFound;
}

static int32 ParseError(TCHAR const* CrashLog, FString& OutErrorDetails)
{
	bool bFound = false;
	TCHAR const* Line = FCStringWide::Strstr(CrashLog, TEXT("Application Specific Information:"));
	if (Line)
	{
		Line = FCStringWide::Strchr(Line, TEXT('\n'));
		check(Line);
		Line += 1;
		TCHAR const* End = FCStringWide::Strchr(Line, TEXT('\r'));
		if(!End)
		{
			End = FCStringWide::Strchr(Line, TEXT('\n'));
		}
		check(End);
		
		int32 Length = FMath::Min(PATH_MAX, (int32)((uintptr_t)(End - Line)));
		OutErrorDetails.Append(Line, Length);
		
		bFound = true;
	}
	return bFound;
}

static int32 ParseCrashedThread(TCHAR const* CrashLog, int32& OutThreadNumber)
{
	int32 Found = 0;
	TCHAR const* Line = FCStringWide::Strstr(CrashLog, TEXT("Crashed Thread:"));
	if (Line)
	{
		Found = swscanf(Line, TEXT("%*s %*s %d"), &OutThreadNumber);
	}
	return Found;
}

static TCHAR const* FindThreadStack(TCHAR const* CrashLog, int32 const ThreadNumber)
{
	int32 Found = 0;
	FString Format = FString::Printf(TEXT("Thread %d"), ThreadNumber);
	TCHAR const* Line = FCStringWide::Strstr(CrashLog, *Format);
	if (Line)
	{
		Line = FCStringWide::Strchr(Line, TEXT('\n'));
		check(Line);
		Line += 1;
	}
	return Line;
}

static TCHAR const* FindCrashedThreadStack(TCHAR const* CrashLog)
{
	TCHAR const* Line = nullptr;
	int32 ThreadNumber = 0;
	int32 Found = ParseCrashedThread(CrashLog, ThreadNumber);
	if(Found)
	{
		Line = FindThreadStack(CrashLog, ThreadNumber);
	}
	return Line;
}

static int32 ParseThreadStackLine(TCHAR const* StackLine, FString& OutModuleName, uint64& OutProgramCounter, FString& OutFunctionName, FString& OutFileName, int32& OutLineNumber)
{
	TCHAR ModuleName[257];
	TCHAR FunctionName[1025];
	TCHAR FileName[257];
	
	int32 Found = swscanf(StackLine, TEXT("%*d %256ls 0x%lx %1024ls + %*d (%256ls:%d)"), ModuleName, &OutProgramCounter, FunctionName, FileName, &OutLineNumber);
	switch(Found)
	{
		case 5:
		case 4:
		{
			OutFileName = FileName;
		}
		case 3:
		{
			int32 Status = -1;
			ANSICHAR* DemangledName = abi::__cxa_demangle(TCHAR_TO_UTF8(FunctionName), nullptr, nullptr, &Status);
			if(DemangledName && Status == 0)
			{
				// C++ function
				OutFunctionName = FString::Printf(TEXT("%ls "), UTF8_TO_TCHAR(DemangledName));
			}
			else if (FCStringWide::Strlen(FunctionName) > 0 && FCStringWide::Strchr(FunctionName, ']'))
			{
				// ObjC function
				OutFunctionName = FString::Printf(TEXT("%ls "), FunctionName);
			}
			else if(FCStringWide::Strlen(FunctionName) > 0)
			{
				// C function
				OutFunctionName = FString::Printf(TEXT("%ls() "), FunctionName);
			}
		}
		case 2:
		case 1:
		{
			OutModuleName = ModuleName;
		}
		default:
		{
			break;
		}
	}
	
	return Found;
}

static int32 SymboliseStackInfo(TArray<FCrashModuleInfo> const& ModuleInfo, FString ModuleName, uint64 const ProgramCounter, FString& OutFunctionName, FString& OutFileName, int32& OutLineNumber)
{
	FProgramCounterSymbolInfo Info;
	int32 ValuesSymbolised = 0;
	
	FCrashModuleInfo Module;
	for (auto Iterator : ModuleInfo)
	{
		if(Iterator.Name.EndsWith(ModuleName))
		{
			Module = Iterator;
			break;
		}
	}

	if((Module.Name.Len() > 0) && FApplePlatformSymbolication::SymbolInfoForStrippedSymbol((ProgramCounter - Module.BaseOfImage), TCHAR_TO_UTF8(*Module.Name), TCHAR_TO_UTF8(*Module.Report), Info))
	{
		if(FCStringAnsi::Strlen(Info.FunctionName) > 0)
		{
			OutFunctionName = Info.FunctionName;
			ValuesSymbolised++;
		}
		if(ValuesSymbolised == 1 && FCStringAnsi::Strlen(Info.Filename) > 0)
		{
			OutFileName = Info.Filename;
			ValuesSymbolised++;
		}
		if(ValuesSymbolised == 2 && Info.LineNumber > 0)
		{
			OutLineNumber = Info.LineNumber;
			ValuesSymbolised++;
		}
	}
	
	return ValuesSymbolised;
}

static TCHAR const* FindModules(TCHAR const* CrashLog)
{
	TCHAR const* Line = FCStringWide::Strstr(CrashLog, TEXT("Binary Images:"));
	if (Line)
	{
		Line = FCStringWide::Strchr(Line, TEXT('\n'));
		check(Line);
		Line += 1;
	}
	return Line;
}

static int32 ParseModuleVersion(TCHAR const* Version, uint16& OutMajor, uint16& OutMinor, uint16& OutPatch, uint16& OutBuild)
{
	OutMajor = OutMinor = OutPatch = OutBuild = 0;
	int32 Found = swscanf(Version, TEXT("%hu.%hu.%hu"), &OutMajor, &OutMinor, &OutPatch);
	TCHAR const* CurrentStart = FCStringWide::Strchr(Version, TEXT('-'));
	if(CurrentStart)
	{
		int32 Components[3] = {0, 0, 0};
		int32 Result = swscanf(CurrentStart, TEXT("%*ls %d.%d.%d"), &Components[0], &Components[1], &Components[2]);
		OutBuild = (uint16)(Components[0] * 10000) + (Components[1] * 100) + (Components[2]);
		Found = 4;
	}
	return Found;
}

static bool ParseModuleLine(TCHAR const* ModuleLine, FCrashModuleInfo& OutModule)
{
	bool bOK = false;
	TCHAR ModuleName[257] = {0};
	uint64 ModuleBase = 0;
	uint64 ModuleEnd = 0;
	
	int32 Found = swscanf(ModuleLine, TEXT("%lx %*ls %lx %256ls"), &ModuleBase, &ModuleEnd, ModuleName);
	switch (Found)
	{
		case 3:
		{
			TCHAR const* VersionStart = FCStringWide::Strchr(ModuleLine, TEXT('('));
			TCHAR const* VersionEnd = FCStringWide::Strchr(ModuleLine, TEXT(')'));
			if(VersionStart && VersionEnd)
			{
				++VersionStart;
				Found += ParseModuleVersion(VersionStart, OutModule.Major, OutModule.Minor, OutModule.Patch, OutModule.Revision);
			}
			
			TCHAR const* UUIDStart = FCStringWide::Strchr(ModuleLine, TEXT('<'));
			TCHAR const* UUIDEnd = FCStringWide::Strchr(ModuleLine, TEXT('>'));
			if(UUIDStart && UUIDEnd)
			{
				++UUIDStart;
				int32 Length = FMath::Min(64, (int32)((uintptr_t)(UUIDEnd - UUIDStart)));
				OutModule.Report.Append(UUIDStart, Length);
				Found++;
			}
			
			TCHAR const* Path = FCStringWide::Strchr(ModuleLine, TEXT('/'));
			if(Path)
			{
				TCHAR const* End = FCStringWide::Strchr(Path, TEXT('\r'));
				if(!End)
				{
					End = FCStringWide::Strchr(Path, TEXT('\n'));
				}
				check(End);
				
				int32 Length = FMath::Min(PATH_MAX, (int32)((uintptr_t)(End - Path)));
				OutModule.Name.Append(Path, Length);
				Found++;
				bOK = true;
			}
		}
		case 2:
		{
			OutModule.SizeOfImage = (ModuleBase - ModuleEnd);
		}
		case 1:
		{
			OutModule.BaseOfImage = ModuleBase;
			break;
		}
		default:
		{
			break;
		}
	}
	return bOK;
}

static int64 StringSize( const ANSICHAR* Line )
{
	int64 Size = 0;
	if( Line != NULL )
	{
		while( *Line++ != 0 )
		{
			Size++;
		}
	}
	return Size;
}

static void WriteLine( FArchive* ReportFile, const ANSICHAR* Line = "" )
{
	if( Line != NULL )
	{
		int64 StringBytes = StringSize( Line );
		ReportFile->Serialize( ( void* )Line, StringBytes );
	}
	
	ReportFile->Serialize( TCHAR_TO_UTF8( LINE_TERMINATOR ), 1 );
}

FCrashDebugHelperMac::FCrashDebugHelperMac()
{
}

FCrashDebugHelperMac::~FCrashDebugHelperMac()
{
}

bool FCrashDebugHelperMac::ParseCrashDump(const FString& InCrashDumpName, FCrashDebugInfo& OutCrashDebugInfo)
{
	if (bInitialized == false)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("ParseCrashDump: CrashDebugHelper not initialized"));
		return false;
	}
	
	FString CrashDump;
	if ( FFileHelper::LoadFileToString( CrashDump, *InCrashDumpName ) )
	{
		// Only supports Apple crash report version 11
		int32 ReportVersion = 0;
		int32 Result = ParseReportVersion(*CrashDump, ReportVersion);
		if(Result == 1 && ReportVersion == 11)
		{
			int32 Major = 0;
			int32 Minor = 0;
			int32 Build = 0;
			int32 CLNumber = 0;
			FString Branch;
			int32 Result = ParseVersion(*CrashDump, Major, Minor, Build, CLNumber, Branch);
			if(Result >= 3)
			{
				if (Result < 5)
				{
					OutCrashDebugInfo.EngineVersion = Build;
				}
				else
				{
					OutCrashDebugInfo.EngineVersion = CLNumber;
				}
				if(Result == 5)
				{
					OutCrashDebugInfo.SourceControlLabel = Branch;
				}
				OutCrashDebugInfo.PlatformName = TEXT("Mac");
				OutCrashDebugInfo.CrashDumpName = InCrashDumpName;
				return true;
			}
		}
	}

	return false;
}

bool FCrashDebugHelperMac::SyncAndDebugCrashDump(const FString& InCrashDumpName)
{
	if (bInitialized == false)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("SyncAndDebugCrashDump: CrashDebugHelper not initialized"));
		return false;
	}

	FCrashDebugInfo CrashDebugInfo;
	
	// Parse the crash dump if it hasn't already been...
	if (ParseCrashDump(InCrashDumpName, CrashDebugInfo) == false)
	{
		UE_LOG(LogCrashDebugHelper, Warning, TEXT("SyncAndDebugCrashDump: Failed to parse crash dump %s"), *InCrashDumpName);
		return false;
	}

	return true;
}

bool FCrashDebugHelperMac::CreateMinidumpDiagnosticReport( const FString& InCrashDumpName )
{
	bool bOK = false;
	
	FString CrashDump;
	if ( FFileHelper::LoadFileToString( CrashDump, *InCrashDumpName ) )
	{
		int32 ReportVersion = 0;
		int32 Result = ParseReportVersion(*CrashDump, ReportVersion);
		if(Result == 1 && ReportVersion == 11)
		{
			FString DiagnosticsPath = FPaths::GetPath(InCrashDumpName) / TEXT("diagnostics.txt");
			
			FArchive* ReportFile = IFileManager::Get().CreateFileWriter( *DiagnosticsPath );
			if( ReportFile != NULL )
			{
				FString Error;
				FString ModulePath;
				FString ModuleName;
				FString FunctionName;
				FString FileName;
				FString Branch;
				FString Model;
				FString Gpu;
				FString Line;
				
				uint64 ProgramCounter = 0;
				int32 Major = 0;
				int32 Minor = 0;
				int32 Build = 0;
				int32 CLNumber = 0;
				int32 OSMajor = 0;
				int32 OSMinor = 0;
				int32 OSPatch = 0;
				int32 OSBuild = 0;
				int32 LineNumber = 0;;
				
				WriteLine( ReportFile, TCHAR_TO_UTF8( TEXT( "Generating report for minidump" ) ) );
				WriteLine( ReportFile );
				
				int32 Result = ParseVersion(*CrashDump, Major, Minor, Build, CLNumber, Branch);
				if(Result >= 3)
				{
					Line = FString::Printf( TEXT( "Application version %d.%d.%d" ), Major, Minor, Build );
					WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
				}
				
				if(Result >= 4)
				{
					Line = FString::Printf( TEXT( " ... built from changelist %d" ), CLNumber );
					WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
				}
				
				if(Result == 5 && Branch.Len() > 0)
				{
					Line = FString::Printf( TEXT( " ... based on label %s" ), *Branch );
					WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
				}
				WriteLine( ReportFile );
				
				Result = ParseOS(*CrashDump, OSMajor, OSMinor, OSPatch, OSBuild);
				check(Result == 4);
				Line = FString::Printf( TEXT( "OS version %d.%d.%d.%xd" ), OSMajor, OSMinor, OSPatch, OSBuild );
				WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
				
				ParseModel(*CrashDump, Model);
				ParseGraphics(*CrashDump, Gpu);
				
				Line = FString::Printf( TEXT( "Running %s %s" ), *Model, *Gpu );
				WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
				
				Result = ParseError(*CrashDump, Error);
				check(Result == 1);
				
				Line = FString::Printf( TEXT( "Exception was \"%s\"" ), *Error );
				WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
				WriteLine( ReportFile );
				
				// Parse modules now for symbolication - if we don't have the running process we need to symbolicate by UUID
				TArray<FCrashModuleInfo> Modules;
				TCHAR const* ModuleLine = FindModules(*CrashDump);
				while(ModuleLine)
				{
					FCrashModuleInfo Module;
					bool const bOK = ParseModuleLine(ModuleLine, Module);
					if(bOK)
					{
						Modules.Push(Module);
						
						ModuleLine = FCStringWide::Strchr(ModuleLine, TEXT('\n'));
						check(ModuleLine);
						ModuleLine += 1;
					}
					else
					{
						ModuleLine = nullptr;
					}
				}
				
				Line = FString::Printf( TEXT( "<CALLSTACK START>" ) );
				WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
				
				TCHAR const* ThreadStackLine = FindCrashedThreadStack(*CrashDump);
				while(ThreadStackLine)
				{
					Result = ParseThreadStackLine(ThreadStackLine, ModuleName, ProgramCounter, FunctionName, FileName, LineNumber);
					
					// If we got the modulename & program counter but didn't parse the filename & linenumber we can resymbolise
					if(Result > 1 && Result < 4)
					{
						// Attempt to resymbolise using CoreSymbolication
						Result += SymboliseStackInfo(Modules, ModuleName, ProgramCounter, FunctionName, FileName, LineNumber);
					}
					
					// Output in our format based on the fields we actually have
					switch (Result)
					{
						case 2:
							Line = FString::Printf( TEXT( "Unknown() Address = 0x%lx (filename not found) [in %s]" ), ProgramCounter, *ModuleName );
							WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
							
							ThreadStackLine = FCStringWide::Strchr(ThreadStackLine, TEXT('\n'));
							check(ThreadStackLine);
							ThreadStackLine += 1;
							break;
							
						case 3:
						case 4:
							Line = FString::Printf( TEXT( "%s Address = 0x%lx (filename not found) [in %s]" ), *FunctionName, ProgramCounter, *ModuleName );
							WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
							
							ThreadStackLine = FCStringWide::Strchr(ThreadStackLine, TEXT('\n'));
							check(ThreadStackLine);
							ThreadStackLine += 1;
							break;
							
						case 5:
						case 6: // Function name might be parsed twice
							Line = FString::Printf( TEXT( "%s Address = 0x%lx [%s, line %d] [in %s]" ), *FunctionName, ProgramCounter, *FileName, LineNumber, *ModuleName );
							WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
							
							ThreadStackLine = FCStringWide::Strchr(ThreadStackLine, TEXT('\n'));
							check(ThreadStackLine);
							ThreadStackLine += 1;
							break;
							
						default:
							ThreadStackLine = nullptr;
							break;
					}
				}
				
				Line = FString::Printf( TEXT( "<CALLSTACK END>" ) );
				WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
				WriteLine( ReportFile );
				
				Line = FString::Printf( TEXT( "%d loaded modules" ), Modules.Num() );
				WriteLine( ReportFile, TCHAR_TO_UTF8( *Line ) );
				
				for( int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++ )
				{
					FCrashModuleInfo& Module = Modules[ModuleIndex];
					
					FString ModuleDirectory = FPaths::GetPath(Module.Name);
					FString ModuleName = FPaths::GetBaseFilename( Module.Name, true ) + FPaths::GetExtension( Module.Name, true );
					
					FString ModuleDetail = FString::Printf( TEXT( "%40s" ), *ModuleName );
					FString Version = FString::Printf( TEXT( " (%d.%d.%d.%d)" ), Module.Major, Module.Minor, Module.Patch, Module.Revision );
					ModuleDetail += FString::Printf( TEXT( " %22s" ), *Version );
					ModuleDetail += FString::Printf( TEXT( " 0x%016x 0x%08x" ), Module.BaseOfImage, Module.SizeOfImage );
					ModuleDetail += FString::Printf( TEXT( " %s" ), *ModuleDirectory );
					
					WriteLine( ReportFile, TCHAR_TO_UTF8( *ModuleDetail ) );
				}
				
				WriteLine( ReportFile );
				
				Line = FString::Printf( TEXT( "Report end!" ) );
				WriteLine( ReportFile, TCHAR_TO_UTF8( *Line )  );
				
				ReportFile->Close();
				delete ReportFile;
				
				bOK = true;
			}
		}
	}
	
	return bOK;
}
