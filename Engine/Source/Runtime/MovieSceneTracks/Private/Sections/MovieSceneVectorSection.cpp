// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneVectorSection.h"
#include "MovieSceneVectorTrack.h"


UMovieSceneVectorSection::UMovieSceneVectorSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


FVector4 UMovieSceneVectorSection::Eval( float Position, const FVector4& DefaultVector ) const
{
	return FVector4(
		Curves[0].Eval( Position, DefaultVector.X ),
		Curves[1].Eval( Position, DefaultVector.Y ),
		Curves[2].Eval( Position, DefaultVector.Z ),
		Curves[3].Eval( Position, DefaultVector.W ) );
}


void UMovieSceneVectorSection::AddKey( float Time, FName CurveName, const FVector4& Value, FKeyParams KeyParams )
{
	Modify();

	static FName X("X");
	static FName Y("Y");
	static FName Z("Z");
	static FName W("W");
	
	check(ChannelsUsed >= 2 && ChannelsUsed <= 4);

	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		if ( CurveName == NAME_None || 
			 (CurveName == X && i == 0) ||
			 (CurveName == Y && i == 1) ||
			 (CurveName == Z && i == 2) ||
			 (CurveName == W && i == 3) )
		{
			if (Curves[i].GetNumKeys() == 0 && !KeyParams.bAddKeyEvenIfUnchanged)
			{
				Curves[i].SetDefaultValue(Value[i]);
			}
			else
			{
				Curves[i].UpdateOrAddKey(Time, Value[i]);
			}
		}
	}
}


bool UMovieSceneVectorSection::NewKeyIsNewData(float Time, const FVector4& Value, FKeyParams KeyParams) const
{
	check(ChannelsUsed >= 2 && ChannelsUsed <= 4);

	bool bHasEmptyKeys = false;
	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		if (Curves[i].GetNumKeys() == 0)
		{
			bHasEmptyKeys = true;
			break;
		}
	}

	bool bNewData = false;
	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		float OriginalData = Curves[i].Eval(Time);
		// don't re-add keys if the data already matches
		if (!FMath::IsNearlyEqual(OriginalData, Value[i]))
		{
			bNewData = true;
			break;
		}
	}

	if ( bHasEmptyKeys || (KeyParams.bAutoKeying && bNewData) )
	{
		return true;
	}

	return false;
}


void UMovieSceneVectorSection::MoveSection( float DeltaTime, TSet<FKeyHandle>& KeyHandles )
{
	check(ChannelsUsed >= 2 && ChannelsUsed <= 4);

	Super::MoveSection( DeltaTime, KeyHandles );

	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		Curves[i].ShiftCurve(DeltaTime, KeyHandles);
	}
}


void UMovieSceneVectorSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{
	check(ChannelsUsed >= 2 && ChannelsUsed <= 4);
	Super::DilateSection(DilationFactor, Origin, KeyHandles);

	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		Curves[i].ScaleCurve(Origin, DilationFactor, KeyHandles);
	}
}


void UMovieSceneVectorSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		for (auto It(Curves[i].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Curves[i].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}
	}
}
