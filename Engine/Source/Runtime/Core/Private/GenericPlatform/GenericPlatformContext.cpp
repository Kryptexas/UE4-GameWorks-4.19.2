// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "GenericPlatform/GenericPlatformContext.h"
#include "Runtime/Launch/Resources/Version.h"

const ANSICHAR* FGenericCrashContext::CrashContextRuntimeXMLNameA = "CrashContext.runtime-xml";
const TCHAR* FGenericCrashContext::CrashContextRuntimeXMLNameW = TEXT( "CrashContext.runtime-xml" );

namespace
{
	static FString CrashDescriptionCommonBuffer;
}

FGenericCrashContext::FGenericCrashContext() :
CommonBuffer( CrashDescriptionCommonBuffer )
{
	CommonBuffer.Reserve( 32768 );
}

void FGenericCrashContext::SerializeContentToBuffer()
{
	AddHeader();

	// Add common crash properties.
	AddCrashProperty( TEXT( "GameName" ), *FString::Printf( TEXT( "UE4-%s" ), FApp::GetGameName() ) );
	AddCrashProperty( TEXT( "PlatformName" ), *FString( FPlatformProperties::PlatformName() ) );
	AddCrashProperty( TEXT( "EngineMode" ), FPlatformMisc::GetEngineMode() );
	AddCrashProperty( TEXT( "EngineVersion" ), ENGINE_VERSION_STRING );
	AddCrashProperty( TEXT( "CommandLine" ), FCommandLine::Get() );
	AddCrashProperty( TEXT( "BaseDir" ), FPlatformProcess::BaseDir() );
	AddCrashProperty( TEXT( "LanguageLCID" ), FInternationalization::Get().GetCurrentCulture()->GetLCID() );

	// Get the user name only for non-UE4 releases.
	if( !FRocketSupport::IsRocket() )
	{
		// Remove periods from internal user names to match AutoReporter user names
		// The name prefix is read by CrashRepository.AddNewCrash in the website code

		AddCrashProperty( TEXT( "UserName" ), *FString( FPlatformProcess::UserName() ).Replace( TEXT( "." ), TEXT( "" ) ) );
	}

	AddCrashProperty( TEXT( "MachineId" ), *FPlatformMisc::GetMachineId().ToString( EGuidFormats::Digits ) );
	AddCrashProperty( TEXT( "EpicAccountId" ), *FPlatformMisc::GetEpicAccountId() );

	AddCrashProperty( TEXT( "CallStack" ), TEXT( "" ) );
	AddCrashProperty( TEXT( "SourceContext" ), TEXT( "" ) );
	AddCrashProperty( TEXT( "UserDescription" ), TEXT( "" ) );
	AddCrashProperty( TEXT( "ErrorMessage" ), (const TCHAR*)GErrorMessage );

	AddCrashProperty( TEXT( "TimeOfCrash" ), FDateTime::UtcNow().GetTicks() );

	// Add misc stats.
	AddCrashProperty( TEXT( "Misc.NumberOfCores" ), FPlatformMisc::NumberOfCores() );
	AddCrashProperty( TEXT( "Misc.NumberOfCoresIncludingHyperthreads" ), FPlatformMisc::NumberOfCoresIncludingHyperthreads() );
	AddCrashProperty( TEXT( "Misc.Is64bitOperatingSystem" ), (int32)FPlatformMisc::Is64bitOperatingSystem() );

	AddCrashProperty( TEXT( "Misc.CPUVendor" ), *FPlatformMisc::GetCPUVendor() );
	AddCrashProperty( TEXT( "Misc.CPUBrand" ), *FPlatformMisc::GetCPUBrand() );
	AddCrashProperty( TEXT( "Misc.PrimaryGPUBrand" ), *FPlatformMisc::GetPrimaryGPUBrand() );

	FString OSVersion, OSSubVersion;
	FPlatformMisc::GetOSVersions( OSVersion, OSSubVersion );
	AddCrashProperty( TEXT( "Misc.OSVersion" ), *FString::Printf( TEXT( "%s, %s" ), *OSVersion, *OSSubVersion ) );

	uint64 AppDiskTotalNumberOfBytes = 0;
	uint64 AppDiskNumberOfFreeBytes = 0;
	FPlatformMisc::GetDiskTotalAndFreeSpace( FPlatformProcess::BaseDir(), AppDiskTotalNumberOfBytes, AppDiskNumberOfFreeBytes );
	AddCrashProperty( TEXT( "Misc.AppDiskTotalNumberOfBytes" ), AppDiskTotalNumberOfBytes );
	AddCrashProperty( TEXT( "Misc.AppDiskNumberOfFreeBytes" ), AppDiskNumberOfFreeBytes );

	// Add memory stats.
	const FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();

	AddCrashProperty( TEXT( "MemoryStats.TotalPhysical" ), (uint64)MemStats.TotalPhysical );
	AddCrashProperty( TEXT( "MemoryStats.TotalVirtual" ), (uint64)MemStats.TotalVirtual );
	AddCrashProperty( TEXT( "MemoryStats.PageSize" ), (uint64)MemStats.PageSize );
	AddCrashProperty( TEXT( "MemoryStats.TotalPhysicalGB" ), MemStats.TotalPhysicalGB );
	AddCrashProperty( TEXT( "MemoryStats.AvailablePhysical" ), (uint64)MemStats.AvailablePhysical );
	AddCrashProperty( TEXT( "MemoryStats.AvailableVirtual" ), (uint64)MemStats.AvailableVirtual );
	AddCrashProperty( TEXT( "MemoryStats.UsedPhysical" ), (uint64)MemStats.UsedPhysical );
	AddCrashProperty( TEXT( "MemoryStats.PeakUsedPhysical" ), (uint64)MemStats.PeakUsedPhysical );
	AddCrashProperty( TEXT( "MemoryStats.UsedVirtual" ), (uint64)MemStats.UsedVirtual );
	AddCrashProperty( TEXT( "MemoryStats.PeakUsedVirtual" ), (uint64)MemStats.PeakUsedVirtual );
	AddCrashProperty( TEXT( "MemoryStats.bIsOOM" ), (int32)FPlatformMemory::bIsOOM );

	// @TODO yrx 2014-09-10 
	//Architecture
	//CrashedModuleName
	//LoadedModules
	AddFooter();

	// Add platform specific properties.
	CommonBuffer += TEXT( "<PlatformSpecificProperties>" ) LINE_TERMINATOR;
	AddPlatformSpecificProperties();
	CommonBuffer += TEXT( "</PlatformSpecificProperties>" ) LINE_TERMINATOR;
}

void FGenericCrashContext::SerializeAsXML( const TCHAR* Filename )
{
	SerializeContentToBuffer();
	FFileHelper::SaveStringToFile( CommonBuffer, Filename, FFileHelper::EEncodingOptions::ForceUTF8 );
}

void FGenericCrashContext::AddPlatformSpecificProperties()
{
	// Nothing really to do here. Can be overridden by the platform code.
	// @see FWindowsPlatformCrashContext::AddPlatformSpecificProperties
}

void FGenericCrashContext::AddHeader()
{
	CommonBuffer += TEXT( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" ) LINE_TERMINATOR;
	CommonBuffer += TEXT( "<RuntimeCrashDescription>" ) LINE_TERMINATOR;

	AddCrashProperty( TEXT( "Version" ), (int32)ECrashDescVersions::VER_2_AddedNewProperties );
}

void FGenericCrashContext::AddFooter()
{
	CommonBuffer += TEXT( "</RuntimeCrashDescription>" ) LINE_TERMINATOR;
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
