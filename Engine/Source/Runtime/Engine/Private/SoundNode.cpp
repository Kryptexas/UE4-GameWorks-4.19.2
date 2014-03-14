// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"
#if WITH_EDITORONLY_DATA
#include "UnrealEd.h"
#endif

/*-----------------------------------------------------------------------------
	USoundNode implementation.
-----------------------------------------------------------------------------*/
USoundNode::USoundNode(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

#if WITH_EDITORONLY_DATA
void USoundNode::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (!Ar.IsCooking() && Ar.UE4Ver() >= VER_UE4_SOUND_CUE_GRAPH_EDITOR)
	{
		Ar << GraphNode;
	}
}

void USoundNode::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	USoundNode* This = CastChecked<USoundNode>(InThis);

	Collector.AddReferencedObject(This->GraphNode, This);

	Super::AddReferencedObjects(InThis, Collector);
}
#endif //WITH_EDITORONLY_DATA

void USoundNode::ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	for( int32 i = 0; i < ChildNodes.Num() && i < GetMaxChildNodes(); ++i )
	{
		if( ChildNodes[ i ] )
		{
			ChildNodes[ i ]->ParseNodes( AudioDevice, GetNodeWaveInstanceHash(NodeWaveInstanceHash, ChildNodes[i], i), ActiveSound, ParseParams, WaveInstances );
		}
	}
}

void USoundNode::GetAllNodes( TArray<USoundNode*>& SoundNodes )
{
	SoundNodes.Add( this );
	for( int32 i = 0; i < ChildNodes.Num(); ++i )
	{
		if( ChildNodes[ i ] )
		{
			ChildNodes[ i ]->GetAllNodes( SoundNodes );
		}
	}
}

void USoundNode::CreateStartingConnectors()
{
	InsertChildNode( ChildNodes.Num() );
}

void USoundNode::InsertChildNode( int32 Index )
{
	check( Index >= 0 && Index <= ChildNodes.Num() );
	int32 MaxChildNodes = GetMaxChildNodes();
	if (MaxChildNodes > ChildNodes.Num())
	{
		ChildNodes.InsertZeroed( Index );
#if WITH_EDITORONLY_DATA
		GraphNode->AddInputPin();
#endif //WITH_EDITORONLY_DATA
	}
}

void USoundNode::RemoveChildNode( int32 Index )
{
	check( Index >= 0 && Index < ChildNodes.Num() );
	int32 MinChildNodes = GetMinChildNodes();
	if (ChildNodes.Num() > MinChildNodes )
	{
		ChildNodes.RemoveAt( Index );
	}
}

#if WITH_EDITOR
void USoundNode::SetChildNodes(TArray<USoundNode*>& InChildNodes)
{
	int32 MaxChildNodes = GetMaxChildNodes();
	int32 MinChildNodes = GetMinChildNodes();
	if (MaxChildNodes >= InChildNodes.Num() && InChildNodes.Num() >= MinChildNodes)
	{
		ChildNodes = InChildNodes;
	}
}
#endif //WITH_EDITOR

float USoundNode::GetDuration()
{
	// Iterate over children and return maximum length of any of them
	float MaxDuration = 0.0f;
	for( int32 i = 0; i < ChildNodes.Num(); i++ )
	{
		if( ChildNodes[ i ] )
		{
			MaxDuration = FMath::Max( ChildNodes[ i ]->GetDuration(), MaxDuration );
		}
	}
	return MaxDuration;
}

#if WITH_EDITOR
void USoundNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	MarkPackageDirty();
}
#endif // WITH_EDITOR

FString USoundNode::GetUniqueString() const
{
	return TEXT("ERROR" );
}

#if WITH_EDITOR
void USoundNode::PlaceNode( int32 NodeColumn, int32 NodeRow, int32 RowCount )
{
	GraphNode->NodePosX = (-150 * NodeColumn) - 100;
	GraphNode->NodePosY = (100 * NodeRow) - (50 * RowCount);
}
#endif //WITH_EDITOR
