// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"

/*-----------------------------------------------------------------------------
    USoundNodeRandom implementation.
-----------------------------------------------------------------------------*/
USoundNodeRandom::USoundNodeRandom(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bRandomizeWithoutReplacement = true;
	NumRandomUsed = 0;
}

void USoundNodeRandom::FixWeightsArray()
{
	// If weights and children got out of sync, we fix it first.
	if( Weights.Num() < ChildNodes.Num() )
	{
		Weights.AddZeroed( ChildNodes.Num() - Weights.Num() );
	}
	else if( Weights.Num() > ChildNodes.Num() )
	{
		const int32 NumToRemove = Weights.Num() - ChildNodes.Num();
		Weights.RemoveAt( Weights.Num() - NumToRemove, NumToRemove );
	}
}

void USoundNodeRandom::FixHasBeenUsedArray()
{
	// If HasBeenUsed and children got out of sync, we fix it first.
	if( HasBeenUsed.Num() < ChildNodes.Num() )
	{
		HasBeenUsed.AddZeroed( ChildNodes.Num() - HasBeenUsed.Num() );
	}
	else if( HasBeenUsed.Num() > ChildNodes.Num() )
	{
		const int32 NumToRemove = HasBeenUsed.Num() - ChildNodes.Num();
		HasBeenUsed.RemoveAt( HasBeenUsed.Num() - NumToRemove, NumToRemove );
	}
}

void USoundNodeRandom::PostLoad()
{
	Super::PostLoad();
	if (!GIsEditor && PreselectAtLevelLoad > 0)
	{
		while (ChildNodes.Num() > PreselectAtLevelLoad)
		{
			RemoveChildNode(FMath::Rand() % ChildNodes.Num());
		}
	}
}

void USoundNodeRandom::ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	RETRIEVE_SOUNDNODE_PAYLOAD( sizeof( int32 ) );
	DECLARE_SOUNDNODE_ELEMENT( int32, NodeIndex );

	if( bRandomizeWithoutReplacement == true )
	{
		FixHasBeenUsedArray();  // for now prob need this until resave packages has occurred
	}

	// Pick a random child node and save the index.
	if( *RequiresInitialization )
	{
		NodeIndex = 0;
		float WeightSum = 0.0f;

		// only calculate the weights that have not been used and use that set for the random choice
		for( int32 i = 0; i < Weights.Num(); ++i )
		{
			if( ( bRandomizeWithoutReplacement == false ) ||  ( HasBeenUsed[ i ] != true ) )
			{
				WeightSum += Weights[ i ];
			}
		}

		float Weight = FMath::FRand() * WeightSum;
		for( int32 i = 0; i < ChildNodes.Num() && i < Weights.Num(); ++i )
		{
			if( bRandomizeWithoutReplacement && ( Weights[ i ] >= Weight ) && ( HasBeenUsed[ i ] != true ) )
			{
				HasBeenUsed[ i ] = true;
				// we played a sound so increment how many sounds we have played
				++NumRandomUsed;

				NodeIndex = i;
				break;

			}
			else if( ( bRandomizeWithoutReplacement == false ) && ( Weights[ i ] >= Weight ) )
			{
				NodeIndex = i;
				break;
			}
			else
			{
				Weight -= Weights[ i ];
			}
		}

		*RequiresInitialization = 0;
	}

	// check to see if we have used up our random sounds
	if( bRandomizeWithoutReplacement && ( HasBeenUsed.Num() > 0 ) && ( NumRandomUsed >= HasBeenUsed.Num() )	)
	{
		// reset all of the children nodes
		for( int32 i = 0; i < HasBeenUsed.Num(); ++i )
		{
			HasBeenUsed[ i ] = false;
		}

		// set the node that has JUST played to be true so we don't repeat it
		if (HasBeenUsed.Num() > NodeIndex)
		{
			HasBeenUsed[ NodeIndex ] = true;
		}
		NumRandomUsed = 1;
	}

	// "play" the sound node that was selected
	if( NodeIndex < ChildNodes.Num() && ChildNodes[ NodeIndex ] )
	{
		ChildNodes[ NodeIndex ]->ParseNodes( AudioDevice, GetNodeWaveInstanceHash(NodeWaveInstanceHash, ChildNodes[NodeIndex], NodeIndex), ActiveSound, ParseParams, WaveInstances );
	}
}

void USoundNodeRandom::CreateStartingConnectors()
{
	// Random Sound Nodes default with two connectors.
	InsertChildNode( ChildNodes.Num() );
	InsertChildNode( ChildNodes.Num() );
}

void USoundNodeRandom::InsertChildNode( int32 Index )
{
	FixWeightsArray();
	FixHasBeenUsedArray();

	check( Index >= 0 && Index <= Weights.Num() );
	check( ChildNodes.Num() == Weights.Num() );

	Weights.InsertUninitialized( Index );
	Weights[Index] = 1.0f;

	HasBeenUsed.InsertUninitialized( Index );
	HasBeenUsed[ Index ] = false;

	Super::InsertChildNode( Index );
}

void USoundNodeRandom::RemoveChildNode( int32 Index )
{
	FixWeightsArray();
	FixHasBeenUsedArray();

	check( Index >= 0 && Index < Weights.Num() );
	check( ChildNodes.Num() == Weights.Num() );

	Weights.RemoveAt( Index );
	HasBeenUsed.RemoveAt( Index );

	Super::RemoveChildNode( Index );
}

#if WITH_EDITOR
void USoundNodeRandom::SetChildNodes(TArray<USoundNode*>& InChildNodes)
{
	Super::SetChildNodes(InChildNodes);

	if (Weights.Num() < ChildNodes.Num())
	{
		while (Weights.Num() < ChildNodes.Num())
		{
			int32 NewIndex = Weights.Num();
			Weights.InsertUninitialized(NewIndex);
			Weights[ NewIndex ] = 1.0f;
		}
	}
	else if (Weights.Num() > ChildNodes.Num())
	{
		const int32 NumToRemove = Weights.Num() - ChildNodes.Num();
		Weights.RemoveAt(Weights.Num() - NumToRemove, NumToRemove);
	}

	if (HasBeenUsed.Num() < ChildNodes.Num())
	{
		while (HasBeenUsed.Num() < ChildNodes.Num())
		{
			int32 NewIndex = HasBeenUsed.Num();
			HasBeenUsed.InsertUninitialized(NewIndex);
			HasBeenUsed[ NewIndex ] = false;
		}
	}
	else if (HasBeenUsed.Num() > ChildNodes.Num())
	{
		const int32 NumToRemove = HasBeenUsed.Num() - ChildNodes.Num();
		HasBeenUsed.RemoveAt(HasBeenUsed.Num() - NumToRemove, NumToRemove);
	}

}
#endif //WITH_EDITOR

FString USoundNodeRandom::GetUniqueString() const
{
	FString Unique = TEXT( "Random" );

	Unique += bRandomizeWithoutReplacement ? TEXT( " True" ) : TEXT( " False" );

	Unique += TEXT( "/" );
	return( Unique );
}
