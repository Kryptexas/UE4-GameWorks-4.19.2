// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "GenericPlatform/GenericPlatformContext.h"
#include "Misc/App.h"
#include "EngineVersion.h"
#include "EngineBuildSettings.h"

const ANSICHAR* FGenericCrashContext::CrashContextRuntimeXMLNameA = "CrashContext.runtime-xml";
const TCHAR* FGenericCrashContext::CrashContextRuntimeXMLNameW = TEXT( "CrashContext.runtime-xml" );
bool FGenericCrashContext::bIsInitialized = false;
FPlatformMemoryStats FGenericCrashContext::CrashMemoryStats = FPlatformMemoryStats();
namespace NCachedCrashContextProperties
{
	static bool bIsInternalBuild;
	static bool bIsPerforceBuild;
	static bool bIsSourceDistribution;
	static bool bIsUE4Release;
	static FString ExecutableName;
	static FString PlatformName;
	static FString PlatformNameIni;
	static FString BaseDir;
	static FString RootDir;
	static FString EpicAccountId;
	static FString MachineIdStr;
	static FString OsVersion;
	static FString OsSubVersion;
	static int32 NumberOfCores;
	static int32 NumberOfCoresIncludingHyperthreads;
	static FString CPUVendor;
	static FString CPUBrand;
	static FString PrimaryGPUBrand;
	static FString UserName;
	static FString DefaultLocale;
	static int32 CrashDumpMode;

	static FString CrashGUID;
}

void FGenericCrashContext::Initialize()
{
	NCachedCrashContextProperties::bIsInternalBuild = FEngineBuildSettings::IsInternalBuild();
	NCachedCrashContextProperties::bIsPerforceBuild = FEngineBuildSettings::IsPerforceBuild();
	NCachedCrashContextProperties::bIsSourceDistribution = FEngineBuildSettings::IsSourceDistribution();
	NCachedCrashContextProperties::bIsUE4Release = FApp::IsEngineInstalled();

	NCachedCrashContextProperties::ExecutableName = FPlatformProcess::ExecutableName();
	NCachedCrashContextProperties::PlatformName = FPlatformProperties::PlatformName();
	NCachedCrashContextProperties::PlatformNameIni = FPlatformProperties::IniPlatformName();
	NCachedCrashContextProperties::BaseDir = FPlatformProcess::BaseDir();
	NCachedCrashContextProperties::RootDir = FPlatformMisc::RootDir();
	NCachedCrashContextProperties::EpicAccountId = FPlatformMisc::GetEpicAccountId();
	NCachedCrashContextProperties::MachineIdStr = FPlatformMisc::GetMachineId().ToString(EGuidFormats::Digits);
	FPlatformMisc::GetOSVersions(NCachedCrashContextProperties::OsVersion, NCachedCrashContextProperties::OsSubVersion);
	NCachedCrashContextProperties::NumberOfCores = FPlatformMisc::NumberOfCores();
	NCachedCrashContextProperties::NumberOfCoresIncludingHyperthreads = FPlatformMisc::NumberOfCoresIncludingHyperthreads();

	NCachedCrashContextProperties::CPUVendor = FPlatformMisc::GetCPUVendor();
	NCachedCrashContextProperties::CPUBrand = FPlatformMisc::GetCPUBrand();
	NCachedCrashContextProperties::PrimaryGPUBrand = FPlatformMisc::GetPrimaryGPUBrand();
	NCachedCrashContextProperties::UserName = FPlatformProcess::UserName();
	NCachedCrashContextProperties::DefaultLocale = FPlatformMisc::GetDefaultLocale();

	// Using the -fullcrashdump parameter will cause full memory minidumps to be created for crashes
	NCachedCrashContextProperties::CrashDumpMode = (int32)ECrashDumpMode::Default;
	if (FCommandLine::IsInitialized())
	{
		const TCHAR* CmdLine = FCommandLine::Get();
		if (FParse::Param( CmdLine, TEXT("fullcrashdump") ))
		{
			NCachedCrashContextProperties::CrashDumpMode = (int32)ECrashDumpMode::FullDump;
		}
	}

	const FGuid Guid = FGuid::NewGuid();
	NCachedCrashContextProperties::CrashGUID = FString::Printf(TEXT("UE4CC-%s-%s"), *NCachedCrashContextProperties::PlatformNameIni, *Guid.ToString(EGuidFormats::Digits));

	bIsInitialized = true;
}

FGenericCrashContext::FGenericCrashContext()
{
	CommonBuffer.Reserve( 32768 );
}

void FGenericCrashContext::SerializeContentToBuffer()
{
	// Must conform against:
	// https://www.securecoding.cert.org/confluence/display/seccode/SIG30-C.+Call+only+asynchronous-safe+functions+within+signal+handlers
	AddHeader();

	BeginSection( TEXT( "RuntimeProperties" ) );
	AddCrashProperty( TEXT( "Version" ), (int32)ECrashDescVersions::VER_2_AddedNewProperties );
	// ProcessId will be used by the crash report client to get more information about the crashed process.
	// It should be enough to open the process, get the memory stats, a list of loaded modules etc.
	// It means that the crashed process needs to be alive as long as the crash report client is working on the process.
	// Proposal, not implemented yet.
	AddCrashProperty( TEXT( "ProcessId" ), FPlatformProcess::GetCurrentProcessId() );
	AddCrashProperty( TEXT( "IsInternalBuild" ), (int32)NCachedCrashContextProperties::bIsInternalBuild );
	AddCrashProperty( TEXT( "IsPerforceBuild" ), (int32)NCachedCrashContextProperties::bIsPerforceBuild );
	AddCrashProperty( TEXT( "IsSourceDistribution" ), (int32)NCachedCrashContextProperties::bIsSourceDistribution );

	// Added 'editor/game open time till crash' 

	// Add common crash properties.
	AddCrashProperty( TEXT( "GameName" ), FApp::GetGameName() );
	AddCrashProperty(TEXT("ExecutableName"), *NCachedCrashContextProperties::ExecutableName);
	AddCrashProperty( TEXT( "BuildConfiguration" ), EBuildConfigurations::ToString( FApp::GetBuildConfiguration() ) );

	AddCrashProperty(TEXT("PlatformName"), *NCachedCrashContextProperties::PlatformName);
	AddCrashProperty(TEXT("PlatformNameIni"), *NCachedCrashContextProperties::PlatformNameIni);
	AddCrashProperty( TEXT( "EngineMode" ), FPlatformMisc::GetEngineMode() );
	AddCrashProperty( TEXT( "EngineVersion" ), *GEngineVersion.ToString() );
	AddCrashProperty( TEXT( "CommandLine" ), FCommandLine::IsInitialized() ? FCommandLine::Get() : TEXT("") );
	AddCrashProperty( TEXT( "LanguageLCID" ), FInternationalization::Get().GetCurrentCulture()->GetLCID() );
	AddCrashProperty(TEXT("DefaultLocale"), *NCachedCrashContextProperties::DefaultLocale);

	AddCrashProperty( TEXT( "IsUE4Release" ), (int32)NCachedCrashContextProperties::bIsUE4Release);

	// Remove periods from user names to match AutoReporter user names
	// The name prefix is read by CrashRepository.AddNewCrash in the website code
	const bool bSendUserName = NCachedCrashContextProperties::bIsInternalBuild;
	AddCrashProperty( TEXT( "UserName" ), bSendUserName ? *NCachedCrashContextProperties::UserName.Replace( TEXT( "." ), TEXT( "" ) ) : TEXT( "" ) );

	AddCrashProperty(TEXT("BaseDir"), *NCachedCrashContextProperties::BaseDir);
	AddCrashProperty(TEXT("RootDir"), *NCachedCrashContextProperties::RootDir);
	AddCrashProperty(TEXT("MachineId"), *NCachedCrashContextProperties::MachineIdStr);
	AddCrashProperty(TEXT("EpicAccountId"), *NCachedCrashContextProperties::EpicAccountId);

	AddCrashProperty( TEXT( "CallStack" ), TEXT( "" ) );
	AddCrashProperty( TEXT( "SourceContext" ), TEXT( "" ) );
	AddCrashProperty( TEXT( "UserDescription" ), TEXT( "" ) );
	AddCrashProperty( TEXT( "ErrorMessage" ), (const TCHAR*)GErrorMessage ); // GErrorMessage may be broken.

	// Add misc stats.
	AddCrashProperty(TEXT("MiscNumberOfCores"), NCachedCrashContextProperties::NumberOfCores);
	AddCrashProperty(TEXT("MiscNumberOfCoresIncludingHyperthreads"), NCachedCrashContextProperties::NumberOfCoresIncludingHyperthreads);
	AddCrashProperty( TEXT( "MiscIs64bitOperatingSystem" ), (int32)FPlatformMisc::Is64bitOperatingSystem() );

	AddCrashProperty(TEXT("MiscCPUVendor"), *NCachedCrashContextProperties::CPUVendor);
	AddCrashProperty(TEXT("MiscCPUBrand"), *NCachedCrashContextProperties::CPUBrand);
	AddCrashProperty(TEXT("MiscPrimaryGPUBrand"), *NCachedCrashContextProperties::PrimaryGPUBrand);
	AddCrashProperty(TEXT("MiscOSVersionMajor"), *NCachedCrashContextProperties::OsVersion);
	AddCrashProperty(TEXT("MiscOSVersionMinor"), *NCachedCrashContextProperties::OsSubVersion);

	AddCrashProperty( TEXT( "CrashDumpMode" ), NCachedCrashContextProperties::CrashDumpMode );


	// @TODO yrx 2014-10-08 Move to the crash report client.
	/*if( CanUseUnsafeAPI() )
	{
		uint64 AppDiskTotalNumberOfBytes = 0;
		uint64 AppDiskNumberOfFreeBytes = 0;
		FPlatformMisc::GetDiskTotalAndFreeSpace( FPlatformProcess::BaseDir(), AppDiskTotalNumberOfBytes, AppDiskNumberOfFreeBytes );
		AddCrashProperty( TEXT( "MiscAppDiskTotalNumberOfBytes" ), AppDiskTotalNumberOfBytes );
		AddCrashProperty( TEXT( "MiscAppDiskNumberOfFreeBytes" ), AppDiskNumberOfFreeBytes );
	}*/

	// FPlatformMemory::GetConstants is called in the GCreateMalloc, so we can assume it is always valid.
	{
		// Add memory stats.
		const FPlatformMemoryConstants& MemConstants = FPlatformMemory::GetConstants();

		AddCrashProperty( TEXT( "MemoryStatsTotalPhysical" ), (uint64)MemConstants.TotalPhysical );
		AddCrashProperty( TEXT( "MemoryStatsTotalVirtual" ), (uint64)MemConstants.TotalVirtual );
		AddCrashProperty( TEXT( "MemoryStatsPageSize" ), (uint64)MemConstants.PageSize );
		AddCrashProperty( TEXT( "MemoryStatsTotalPhysicalGB" ), MemConstants.TotalPhysicalGB );
	}

	AddCrashProperty( TEXT( "MemoryStatsAvailablePhysical" ), (uint64)CrashMemoryStats.AvailablePhysical );
	AddCrashProperty( TEXT( "MemoryStatsAvailableVirtual" ), (uint64)CrashMemoryStats.AvailableVirtual );
	AddCrashProperty( TEXT( "MemoryStatsUsedPhysical" ), (uint64)CrashMemoryStats.UsedPhysical );
	AddCrashProperty( TEXT( "MemoryStatsPeakUsedPhysical" ), (uint64)CrashMemoryStats.PeakUsedPhysical );
	AddCrashProperty( TEXT( "MemoryStatsUsedVirtual" ), (uint64)CrashMemoryStats.UsedVirtual );
	AddCrashProperty( TEXT( "MemoryStatsPeakUsedVirtual" ), (uint64)CrashMemoryStats.PeakUsedVirtual );
	AddCrashProperty( TEXT( "MemoryStatsbIsOOM" ), (int32)FPlatformMemory::bIsOOM );
	AddCrashProperty( TEXT( "MemoryStatsOOMAllocationSize"), (uint64)FPlatformMemory::OOMAllocationSize );
	AddCrashProperty( TEXT( "MemoryStatsOOMAllocationAlignment"), (int32)FPlatformMemory::OOMAllocationAlignment );

	//Architecture
	//CrashedModuleName
	//LoadedModules
	EndSection( TEXT( "RuntimeProperties" ) );
	
	// Add platform specific properties.
	BeginSection( TEXT("PlatformProperties") );
	AddPlatformSpecificProperties();
	EndSection( TEXT( "PlatformProperties" ) );

	AddFooter();
}

const FString& FGenericCrashContext::GetUniqueCrashName()
{
	return NCachedCrashContextProperties::CrashGUID;
}

const bool FGenericCrashContext::IsFullCrashDump()
{
	return (NCachedCrashContextProperties::CrashDumpMode == (int32)ECrashDumpMode::FullDump);
}

void FGenericCrashContext::SerializeAsXML( const TCHAR* Filename )
{
	SerializeContentToBuffer();
	// Use OS build-in functionality instead.
	FFileHelper::SaveStringToFile( CommonBuffer, Filename, FFileHelper::EEncodingOptions::AutoDetect );
}

void FGenericCrashContext::AddCrashProperty( const TCHAR* PropertyName, const TCHAR* PropertyValue )
{
	CommonBuffer += TEXT( "<" );
	CommonBuffer += PropertyName;
	CommonBuffer += TEXT( ">" );

	CommonBuffer += PropertyValue;

	CommonBuffer += TEXT( "</" );
	CommonBuffer += PropertyName;
	CommonBuffer += TEXT( ">" );
	CommonBuffer += LINE_TERMINATOR;
}

void FGenericCrashContext::AddPlatformSpecificProperties()
{
	// Nothing really to do here. Can be overridden by the platform code.
	// @see FWindowsPlatformCrashContext::AddPlatformSpecificProperties
}

void FGenericCrashContext::AddHeader()
{
	CommonBuffer += TEXT( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" ) LINE_TERMINATOR;
	BeginSection( TEXT("FGenericCrashContext") );
}

void FGenericCrashContext::AddFooter()
{
	EndSection( TEXT( "FGenericCrashContext" ) );
}

void FGenericCrashContext::BeginSection( const TCHAR* SectionName )
{
	CommonBuffer += TEXT( "<" );
	CommonBuffer += SectionName;
	CommonBuffer += TEXT( ">" );
	CommonBuffer += LINE_TERMINATOR;
}

void FGenericCrashContext::EndSection( const TCHAR* SectionName )
{
	CommonBuffer += TEXT( "</" );
	CommonBuffer += SectionName;
	CommonBuffer += TEXT( ">" );
	CommonBuffer += LINE_TERMINATOR;
}


FProgramCounterSymbolInfoEx::FProgramCounterSymbolInfoEx( FString InModuleName, FString InFunctionName, FString InFilename, uint32 InLineNumber, uint64 InSymbolDisplacement, uint64 InOffsetInModule, uint64 InProgramCounter ) :
	ModuleName( InModuleName ),
	FunctionName( InFunctionName ),
	Filename( InFilename ),
	LineNumber( InLineNumber ),
	SymbolDisplacement( InSymbolDisplacement ),
	OffsetInModule( InOffsetInModule ),
	ProgramCounter( InProgramCounter )
{

}