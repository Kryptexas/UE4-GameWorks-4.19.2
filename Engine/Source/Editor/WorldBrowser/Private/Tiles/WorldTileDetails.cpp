// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "WorldTileModel.h"
#include "WorldTileDetails.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"

/////////////////////////////////////////////////////
// FStreamingLevelDetails

UClass* FTileStreamingLevelDetails::StreamingMode2Class(EStreamingLevelMode::Type InMode)
{
	switch (InMode)
	{
	case EStreamingLevelMode::AlwaysLoaded:
		return ULevelStreamingAlwaysLoaded::StaticClass();
	case EStreamingLevelMode::Kismet:
		return ULevelStreamingKismet::StaticClass();
	case EStreamingLevelMode::Bounds:
		return ULevelStreamingBounds::StaticClass();
	default:
		check(false); // unsupported mode
	}
	return NULL;
}

EStreamingLevelMode::Type FTileStreamingLevelDetails::StreamingObject2Mode(ULevelStreaming* InLevelStreaming)
{
	if (InLevelStreaming->IsA(ULevelStreamingAlwaysLoaded::StaticClass()))
	{
		return EStreamingLevelMode::AlwaysLoaded; 
	}

	if (InLevelStreaming->IsA(ULevelStreamingKismet::StaticClass()))
	{
		return EStreamingLevelMode::Kismet; 
	}

	if (InLevelStreaming->IsA(ULevelStreamingBounds::StaticClass()))
	{
		return EStreamingLevelMode::Bounds; 
	}

	check(false); // unsupported class
	return EStreamingLevelMode::AlwaysLoaded;
}

/////////////////////////////////////////////////////
// FTileLODEntryDetails
FTileLODEntryDetails::FTileLODEntryDetails()
	// Initialize properties with default values from FWorldTileLODInfo
	: LODIndex(0)
	, Distance(FWorldTileLODInfo().RelativeStreamingDistance)
	, DetailsPercentage(FWorldTileLODInfo().GenDetailsPercentage)
	, MaxDeviation(FWorldTileLODInfo().GenMaxDeviation)
{
}

/////////////////////////////////////////////////////
// UWorldTileDetails
UWorldTileDetails::UWorldTileDetails(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	LOD1.LODIndex = 0;
	LOD2.LODIndex = 1;
	LOD3.LODIndex = 2;
	LOD4.LODIndex = 3;
}

static void SyncLODSettings(FTileLODEntryDetails& To, const FWorldTileLODInfo& From)
{
	To.Distance				= From.RelativeStreamingDistance;
	To.DetailsPercentage	= From.GenDetailsPercentage;
	To.MaxDeviation			= From.GenMaxDeviation;
}

static void SyncLODSettings(FWorldTileLODInfo& To, const FTileLODEntryDetails& From)
{
	To.RelativeStreamingDistance	= From.Distance;			
	To.GenDetailsPercentage			= From.DetailsPercentage;
	To.GenMaxDeviation				= From.MaxDeviation;		
}

void UWorldTileDetails::SetInfo(const FWorldTileInfo& Info)
{
	ParentPackageName	= FName(*Info.ParentTilePackageName);
	Position			= Info.Position;
	AbsolutePosition	= Info.AbsolutePosition;
	Layer				= Info.Layer;
	Bounds				= Info.Bounds;
	bAlwaysLoaded		= Info.bAlwaysLoaded;
	ZOrder				= Info.ZOrder;

	// Sync LOD settings
	NumLOD				= Info.LODList.Num();
	FTileLODEntryDetails* LODEntries[WORLDTILE_LOD_MAX_INDEX] = {&LOD1, &LOD2, &LOD3, &LOD4};
	for (int32 LODIdx = 0; LODIdx < Info.LODList.Num(); ++LODIdx)
	{
		SyncLODSettings(*LODEntries[LODIdx], Info.LODList[LODIdx]);
	}
}
	
FWorldTileInfo UWorldTileDetails::GetInfo() const
{
	FWorldTileInfo Info;
	
	Info.ParentTilePackageName	= ParentPackageName.ToString();
	Info.Position				= Position;
	Info.AbsolutePosition		= AbsolutePosition;
	Info.Layer					= Layer;
	Info.Bounds					= Bounds;
	Info.bAlwaysLoaded			= bAlwaysLoaded;
	Info.ZOrder					= ZOrder;

	// Sync LOD settings
	Info.LODList.SetNum(FMath::Clamp(NumLOD, 0, WORLDTILE_LOD_MAX_INDEX));
	const FTileLODEntryDetails* LODEntries[WORLDTILE_LOD_MAX_INDEX] = {&LOD1, &LOD2, &LOD3, &LOD4};
	for (int32 LODIdx = 0; LODIdx < Info.LODList.Num(); ++LODIdx)
	{
		SyncLODSettings(Info.LODList[LODIdx], *LODEntries[LODIdx]);
	}
	
	return Info;
}

void UWorldTileDetails::SyncStreamingLevels(const FWorldTileModel& TileModel)
{
	// Clean list if tile streaming details, 
	// will be refiled with an actual information in a loop bellow
	StreamingLevels.Empty();
	
	const ULevel* Level = TileModel.GetLevelObject();
	if (Level)
	{
		auto WorldStreamingLevels = CastChecked<UWorld>(Level->GetOuter())->StreamingLevels;
		for (auto It = WorldStreamingLevels.CreateConstIterator(); It; ++It)
		{
			ULevelStreaming* LevelStreaming = (*It);
			
			FTileStreamingLevelDetails Details;
			Details.StreamingMode	= FTileStreamingLevelDetails::StreamingObject2Mode(LevelStreaming);
			Details.PackageName		= LevelStreaming->PackageName;

			StreamingLevels.Add(Details);
		}
	}
}

void UWorldTileDetails::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != NULL) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, Position)
		|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, Position))
	{
		PositionChangedEvent.Broadcast();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, ParentPackageName))
	{
		ParentPackageNameChangedEvent.Broadcast();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, bAlwaysLoaded))
	{
		AlwaysLoadedChangedEvent.Broadcast();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, StreamingLevels)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, StreamingLevels))
	{
		StreamingLevelsChangedEvent.Broadcast();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, NumLOD))
	{
		LODSettingsChangedEvent.Broadcast();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, LOD1)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, LOD1))
	{
		LODSettingsChangedEvent.Broadcast();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, LOD2)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, LOD2))
	{
		LODSettingsChangedEvent.Broadcast();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, LOD3)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, LOD3))
	{
		LODSettingsChangedEvent.Broadcast();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, LOD4)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, LOD4))
	{
		LODSettingsChangedEvent.Broadcast();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UWorldTileDetails, ZOrder))
	{
		ZOrderChangedEvent.Broadcast();
	}
}


#undef LOCTEXT_NAMESPACE
