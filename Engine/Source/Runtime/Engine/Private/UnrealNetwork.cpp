// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "NetworkReplayStreaming.h"
#include "Net/UnrealNetwork.h"
#include "GeneralProjectSettings.h"

FNetworkVersion::FGetLocalNetworkVersionOverride FNetworkVersion::GetLocalNetworkVersionOverride;
FNetworkVersion::FIsNetworkCompatibleOverride FNetworkVersion::IsNetworkCompatibleOverride;

enum ENetworkVersionHistory
{
	HISTORY_INITIAL				= 1,
	HISTORY_INTERNAL_ACK		= 3				// We no longer save packet/channel sequence in stream. We can derive this for 100% reliable connections.
};

const uint32 FNetworkVersion::InternalProtocolVersion = HISTORY_INTERNAL_ACK;

uint32 FNetworkVersion::GetLocalNetworkVersion()
{
	if ( GetLocalNetworkVersionOverride.IsBound() )
	{
		const uint32 LocalNetworkVersion = GetLocalNetworkVersionOverride.Execute();

		UE_LOG( LogNet, Log, TEXT( "GetLocalNetworkVersionOverride: LocalNetworkVersion: %i" ), LocalNetworkVersion );

		return LocalNetworkVersion;
	}

	// Get the project name (NOT case sensitive)
	const FString ProjectName( FString( FApp::GetGameName() ).ToLower() );

	// Get the project version string (IS case sensitive!)
	const FString& ProjectVersion = Cast<UGeneralProjectSettings>(UGeneralProjectSettings::StaticClass()->GetDefaultObject())->ProjectVersion;

	// Start with engine version as seed, and then hash with project name + project version
	const uint32 VersionHash = FCrc::StrCrc32( *ProjectVersion, FCrc::StrCrc32( *ProjectName, GEngineNetVersion ) );

	// Hash with internal protocol version
	uint32 LocalNetworkVersion = FCrc::MemCrc32( &InternalProtocolVersion, sizeof( InternalProtocolVersion ), VersionHash );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if ( !GEngineVersion.IsPromotedBuild() )
	{
		// Further hash with machine id if this is a non promoted build
		const FString MachineId = FPlatformMisc::GetMachineId().ToString( EGuidFormats::Digits ).ToLower();

		const uint32 LocalNetworkVersionNonPromoted = FCrc::StrCrc32( *MachineId, LocalNetworkVersion );

		UE_LOG( LogNet, Log, TEXT( "GetLocalNetworkVersion: NON-PROMOTED: MachineId: %s, ProjectName: %s, ProjectVersion: %s, InternalProtocolVersion: %i, LocalNetworkVersionNonPromoted: %u" ), *MachineId, *ProjectName, *ProjectVersion, InternalProtocolVersion, LocalNetworkVersionNonPromoted );

		return LocalNetworkVersionNonPromoted;
	}
#endif

	UE_LOG( LogNet, Log, TEXT( "GetLocalNetworkVersion: GEngineNetVersion: %i, ProjectName: %s, ProjectVersion: %s, InternalProtocolVersion: %i, LocalNetworkVersion: %u" ), GEngineNetVersion, *ProjectName, *ProjectVersion, InternalProtocolVersion, LocalNetworkVersion );

	return LocalNetworkVersion;
}

bool FNetworkVersion::IsNetworkCompatible( const uint32 LocalNetworkVersion, const uint32 RemoteNetworkVersion )
{
	if ( IsNetworkCompatibleOverride.IsBound() )
	{
		return IsNetworkCompatibleOverride.Execute( LocalNetworkVersion, RemoteNetworkVersion );
	}

	return LocalNetworkVersion == RemoteNetworkVersion;
}

FNetworkReplayVersion FNetworkVersion::GetReplayVersion()
{
	return FNetworkReplayVersion( FApp::GetGameName(), GetLocalNetworkVersion(), GEngineVersion.GetChangelist() );
}


// ----------------------------------------------------------------
