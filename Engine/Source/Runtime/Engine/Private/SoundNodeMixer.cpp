// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"

/*-----------------------------------------------------------------------------
	USoundNodeMixer implementation.
-----------------------------------------------------------------------------*/
USoundNodeMixer::USoundNodeMixer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void USoundNodeMixer::ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	FSoundParseParameters UpdatedParams = ParseParams;
	for( int32 ChildNodeIndex = 0; ChildNodeIndex < ChildNodes.Num(); ++ChildNodeIndex )
	{
		if( ChildNodes[ ChildNodeIndex ] )
		{
			UpdatedParams.Volume = ParseParams.Volume * InputVolume[ ChildNodeIndex ];

			ChildNodes[ ChildNodeIndex ]->ParseNodes( AudioDevice, GetNodeWaveInstanceHash(NodeWaveInstanceHash, ChildNodes[ChildNodeIndex], ChildNodeIndex), ActiveSound, UpdatedParams, WaveInstances );
		}
	}
}

void USoundNodeMixer::CreateStartingConnectors()
{
	// Mixers default with two connectors.
	InsertChildNode( ChildNodes.Num() );
	InsertChildNode( ChildNodes.Num() );
}

void USoundNodeMixer::InsertChildNode( int32 Index )
{
	Super::InsertChildNode( Index );
	InputVolume.InsertUninitialized( Index );
	InputVolume[ Index ] = 1.0f;
}

void USoundNodeMixer::RemoveChildNode( int32 Index )
{
	Super::RemoveChildNode( Index );
	InputVolume.RemoveAt( Index );
}

#if WITH_EDITOR
void USoundNodeMixer::SetChildNodes(TArray<USoundNode*>& InChildNodes)
{
	Super::SetChildNodes(InChildNodes);

	if (InputVolume.Num() < ChildNodes.Num())
	{
		while (InputVolume.Num() < ChildNodes.Num())
		{
			int32 NewIndex = InputVolume.Num();
			InputVolume.InsertUninitialized( NewIndex );
			InputVolume[ NewIndex ] = 1.0f;
		}
	}
	else if (InputVolume.Num() > ChildNodes.Num())
	{
		const int32 NumToRemove = InputVolume.Num() - ChildNodes.Num();
		InputVolume.RemoveAt(InputVolume.Num() - NumToRemove, NumToRemove);
	}
}
#endif //WITH_EDITOR

FString USoundNodeMixer::GetUniqueString() const
{
	FString Unique = TEXT( "Mixer" );

	for( int32 Index = 0; Index < InputVolume.Num(); Index++ )
	{
		Unique += FString::Printf( TEXT( " %g" ), InputVolume[ Index ] );
	}

	Unique += TEXT( "/" );
	return( Unique );
}
