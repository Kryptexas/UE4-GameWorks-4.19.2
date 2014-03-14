// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RepLayout.cpp: Unreal replication layout implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/RepLayout.h"
#include "Net/DataReplication.h"
#include "Net/NetworkProfiler.h"

static TAutoConsoleVariable<int32> CVarAllowPropertySkipping( TEXT( "net.AllowPropertySkipping" ), 1, TEXT( "Allow skipping of properties that haven't changed for other clients" ) );

static TAutoConsoleVariable<int32> CVarDoPropertyChecksum( TEXT( "net.DoPropertyChecksum" ), 0, TEXT( "" ) );

FAutoConsoleVariable CVarDoReplicationContextString( TEXT( "net.ContextDebug" ), 0, TEXT( "" ) );

#define ENABLE_PROPERTY_CHECKSUMS

//#define SANITY_CHECK_MERGES

#define USE_CUSTOM_COMPARE

//#define ENABLE_SUPER_CHECKSUMS

static const int32 UNMAPPED_FRAMES_THRESHOLD = 100;

#ifdef USE_CUSTOM_COMPARE
static FORCEINLINE bool CompareBool( const FRepLayoutCmd & Cmd, const void * A, const void * B )
{
	return Cmd.Property->Identical( A, B );
}

static FORCEINLINE bool CompareObject( const FRepLayoutCmd & Cmd, const void * A, const void * B )
{
	return Cmd.Property->Identical( A, B );
}

template< typename T >
bool CompareValue( const T * A, const T * B )
{
	return *A == *B;
}

template< typename T >
bool CompareValue( const void * A, const void * B )
{
	return CompareValue( (T*)A, (T*)B);
}

static FORCEINLINE bool PropertiesAreIdenticalNative( const FRepLayoutCmd & Cmd, const void * A, const void * B )
{
	switch ( Cmd.Type )
	{
		case REPCMD_PropertyBool:			return CompareBool( Cmd, A, B );
		case REPCMD_PropertyByte:			return CompareValue<uint8>( A, B );
		case REPCMD_PropertyFloat:			return CompareValue<float>( A, B );
		case REPCMD_PropertyInt:			return CompareValue<int32>( A, B );
		case REPCMD_PropertyName:			return CompareValue<FName>( A, B );
		case REPCMD_PropertyObject:			return CompareObject( Cmd, A, B );
		case REPCMD_PropertyUInt32:			return CompareValue<uint32>( A, B );
		case REPCMD_PropertyUInt64:			return CompareValue<uint64>( A, B );
		case REPCMD_PropertyVector:			return CompareValue<FVector>( A, B );
		case REPCMD_PropertyVector100:		return CompareValue<FVector_NetQuantize100>( A, B );
		case REPCMD_PropertyVectorQ:		return CompareValue<FVector_NetQuantize>( A, B );
		case REPCMD_PropertyVectorNormal:	return CompareValue<FVector_NetQuantizeNormal>( A, B );
		case REPCMD_PropertyVector10:		return CompareValue<FVector_NetQuantize10>( A, B );
		case REPCMD_PropertyPlane:			return CompareValue<FPlane>( A, B );
		case REPCMD_PropertyRotator:		return CompareValue<FRotator>( A, B );
		case REPCMD_PropertyNetId:			return CompareValue<FUniqueNetIdRepl>( A, B );
		case REPCMD_RepMovement:			return CompareValue<FRepMovement>( A, B );
		case REPCMD_PropertyString:			return CompareValue<FString>( A, B );
		case REPCMD_Property:				return Cmd.Property->Identical( A, B );
		default: 
			UE_LOG( LogNet, Fatal, TEXT( "PropertiesAreIdentical: Unsupported type! %i (%s)" ), Cmd.Type, *Cmd.Property->GetName() );
	}

	return false;
}

static FORCEINLINE bool PropertiesAreIdentical( const FRepLayoutCmd & Cmd, const void * A, const void * B )
{
	const bool bIsIdentical = PropertiesAreIdenticalNative( Cmd, A, B );
#if 0
	// Sanity check result
	if ( bIsIdentical != Cmd.Property->Identical( A, B ) )
	{
		UE_LOG( LogNet, Fatal, TEXT( "PropertiesAreIdentical: Result mismatch! (%s)" ), *Cmd.Property->GetFullName() );
	}
#endif
	return bIsIdentical;
}
#else
static FORCEINLINE bool PropertiesAreIdentical( const FRepLayoutCmd & Cmd, const void * A, const void * B )
{
	return Cmd.Property->Identical( A, B );
}
#endif

static FORCEINLINE void StoreProperty( const FRepLayoutCmd & Cmd, void * A, const void * B )
{
	Cmd.Property->CopySingleValue( A, B );
}

static FORCEINLINE void SerializeGenericChecksum( FArchive & Ar )
{
	uint32 Checksum = 0xABADF00D;
	Ar << Checksum;
	check( Checksum == 0xABADF00D );
}

static void SerializeReadWritePropertyChecksum( const FRepLayoutCmd & Cmd, const int32 CurCmdIndex, const uint8 * Data, FArchive & Ar, const bool bDiscard )
{
	if ( bDiscard )
	{
		uint32 MarkerChecksum = 0;
		uint32 PropertyChecksum = 0;
		Ar << MarkerChecksum;
		Ar << PropertyChecksum;
		return;
	}

	// Serialize various attributes that will mostly ensure we are working on the same property
	const uint32 NameHash = GetTypeHash( Cmd.Property->GetName() );

	uint32 MarkerChecksum = 0;

	// Evolve the checksum over several values that will uniquely identity where we are and should be
	MarkerChecksum = FCrc::MemCrc_DEPRECATED( &NameHash,		sizeof( NameHash ),		MarkerChecksum );
	MarkerChecksum = FCrc::MemCrc_DEPRECATED( &Cmd.Offset,		sizeof( Cmd.Offset ),	MarkerChecksum );
	MarkerChecksum = FCrc::MemCrc_DEPRECATED( &CurCmdIndex,	sizeof( CurCmdIndex ),	MarkerChecksum );

	const uint32 OriginalMarkerChecksum = MarkerChecksum;

	Ar << MarkerChecksum;

	if ( MarkerChecksum != OriginalMarkerChecksum )
	{
		// This is fatal, as it means we are out of sync to the point we can't recover
		UE_LOG( LogNet, Fatal, TEXT( "SerializeReadWritePropertyChecksum: Property checksum marker failed! [%s]" ), *Cmd.Property->GetFullName() );
	}

	if ( Cmd.Property->IsA( UObjectPropertyBase::StaticClass() ) )
	{
		// Can't handle checksums for objects right now
		// Need to resolve how to handle unmapped objects
		return;
	}

	// Now generate a checksum that guarantee that this property is in the exact state as the server
	// This will require NetSerializeItem to be deterministic, in and out
	// i.e, not only does NetSerializeItem need to write the same blob on the same input data, but
	//	it also needs to write the same blob it just read as well.
	FBitWriter Writer( 0, true );

	Cmd.Property->NetSerializeItem( Writer, NULL, const_cast< uint8 * >( Data ) );

	if ( Ar.IsSaving() )
	{
		// If this is the server, do a read, and then another write so that we do exactly what the client will do, which will better ensure determinism 

		// We do this to force InitializeValue, DestroyValue etc to work on a single item
		const int32 OriginalDim = Cmd.Property->ArrayDim;
		Cmd.Property->ArrayDim = 1;

		TArray< uint8 > TempPropMemory;
		TempPropMemory.AddZeroed( Cmd.Property->ElementSize + 4 );
		uint32 * Guard = (uint32*)&TempPropMemory[TempPropMemory.Num() - 4];
		const uint32 TAG_VALUE = 0xABADF00D;
		*Guard = TAG_VALUE;
		Cmd.Property->InitializeValue( TempPropMemory.GetTypedData() );
		check( *Guard == TAG_VALUE );

		// Read it back in and then write it out to produce what the client will produce
		FBitReader Reader( Writer.GetData(), Writer.GetNumBits() );
		Cmd.Property->NetSerializeItem( Reader, NULL, TempPropMemory.GetTypedData() );
		check( Reader.AtEnd() && !Reader.IsError() );
		check( *Guard == TAG_VALUE );

		// Write it back out for a final time
		Writer.Reset();

		Cmd.Property->NetSerializeItem( Writer, NULL, TempPropMemory.GetTypedData() );
		check( *Guard == TAG_VALUE );

		// Destroy temp memory
		Cmd.Property->DestroyValue( TempPropMemory.GetTypedData() );

		// Restore the static array size
		Cmd.Property->ArrayDim = OriginalDim;

		check( *Guard == TAG_VALUE );
	}

	uint32 PropertyChecksum = FCrc::MemCrc_DEPRECATED( Writer.GetData(), Writer.GetNumBytes() );

	const uint32 OriginalPropertyChecksum = PropertyChecksum;

	Ar << PropertyChecksum;

	if ( PropertyChecksum != OriginalPropertyChecksum )
	{
		// This is a warning, because for some reason, float rounding issues in the quantization functions cause this to return false positives
		UE_LOG( LogNet, Warning, TEXT( "Property checksum failed! [%s]" ), *Cmd.Property->GetFullName() );
	}
}

uint16 FRepLayout::CompareProperties_r(
	const int32				CmdStart,
	const int32				CmdEnd,
	const uint8 * RESTRICT	CompareData, 
	const uint8 * RESTRICT	Data,
	TArray< uint16 > &		Changed,
	uint16					Handle
) const
{
	for ( int32 CmdIndex = CmdStart; CmdIndex < CmdEnd; CmdIndex++ )
	{
		const FRepLayoutCmd & Cmd = Cmds[CmdIndex];

		check( Cmd.Type != REPCMD_Return );

		Handle++;

		if ( Cmd.Type == REPCMD_DynamicArray )
		{
			// Once we hit an array, start using a stack based approach
			CompareProperties_Array_r( CompareData ? CompareData + Cmd.Offset : NULL, (const uint8*)Data + Cmd.Offset, Changed, CmdIndex, Handle );
			CmdIndex = Cmd.EndCmd - 1;		// The -1 to handle the ++ in the for loop
			continue;
		}

		if ( CompareData == NULL || !PropertiesAreIdentical( Cmd, (const void*)( CompareData + Cmd.Offset ), (const void*)( Data + Cmd.Offset ) ) )
		{
			Changed.Add( Handle );
		}
	}

	return Handle;
}

void FRepLayout::CompareProperties_Array_r( 
	const uint8 * RESTRICT	CompareData, 
	const uint8 * RESTRICT	Data,
	TArray< uint16 > &		Changed,
	const uint16			CmdIndex,
	const uint16			Handle
) const
{
	const FRepLayoutCmd & Cmd = Cmds[CmdIndex];

	FScriptArray * CompareArray = (FScriptArray *)CompareData;
	FScriptArray * Array = (FScriptArray *)Data;

	const uint16 ArrayNum			= Array->Num();
	const uint16 CompareArrayNum	= CompareArray ? CompareArray->Num() : 0;

	TArray< uint16 > ChangedLocal;

	uint16 LocalHandle = 0;

	Data = (uint8*)Array->GetData();
	CompareData = CompareData ? (uint8*)CompareArray->GetData() : (uint8*)NULL;

	for ( int32 i = 0; i < ArrayNum; i++ )
	{
		const int32 ElementOffset = i * Cmd.ElementSize;
		const uint8 * LocalCompareData = ( i < CompareArrayNum ) ? ( CompareData + ElementOffset ) : NULL;

		LocalHandle = CompareProperties_r( CmdIndex + 1, Cmd.EndCmd - 1, LocalCompareData, Data + ElementOffset, ChangedLocal, LocalHandle );
	}

	if ( ChangedLocal.Num() )
	{
		Changed.Add( Handle );
		Changed.Add( ChangedLocal.Num() );		// This is so we can jump over the array if we need to
		Changed.Append( ChangedLocal );
		Changed.Add( 0 );
	}
	else if ( ArrayNum != CompareArrayNum )
	{
		// If nothing below us changed, we either shrunk, or we grew and our inner was an array that didn't have any elements
		check( ArrayNum < CompareArrayNum || Cmds[CmdIndex + 1].Type == REPCMD_DynamicArray );

		// Array got smaller, send the array handle to force array size change
		Changed.Add( Handle );
		Changed.Add( 0 );
		Changed.Add( 0 );
	}
}

bool FRepLayout::CompareProperties( 
	FRepState * RESTRICT				RepState, 
	const uint8 * RESTRICT				CompareData,
	const uint8 * RESTRICT				Data, 
	TArray< FRepChangedParent > &		OutChangedParents,
	const TArray< uint16 > &			PropertyList ) const
{
	bool PropertyChanged = false;

	const uint16 * RESTRICT FirstProp	= PropertyList.GetTypedData();
	const uint16 * RESTRICT LastProp	= FirstProp + PropertyList.Num();

	for ( const uint16 * RESTRICT pLifeProp = FirstProp; pLifeProp < LastProp; ++pLifeProp )
	{
//#if USE_NETWORK_PROFILER 
		//const uint32 PropertyStartTime = GNetworkProfiler.IsTrackingEnabled() ? FPlatformTime::Cycles() : 0;
//#endif

		const FRepParentCmd & ParentCmd = Parents[*pLifeProp];

		// We store changed properties on each parent, so we can build a final sorted change list later
		TArray< uint16 > & Changed = OutChangedParents[*pLifeProp].Changed;

		check( Changed.Num() == 0 );

		// Loop over the block of child properties that are children of this parent
		for ( int32 i = ParentCmd.CmdStart; i < ParentCmd.CmdEnd; i++ )
		{
			const FRepLayoutCmd & Cmd = Cmds[i];

			check( Cmd.Type != REPCMD_Return );		// REPCMD_Return's are markers we shouldn't hit

			if ( Cmd.Type == REPCMD_DynamicArray )
			{
				// Once we hit an array, start using a recursive based approach
				CompareProperties_Array_r( CompareData + Cmd.Offset, Data + Cmd.Offset, Changed, i, Cmd.RelativeHandle );
				i = Cmd.EndCmd - 1;		// Jump past properties under array, we've already checked them (the -1 because of the ++ in the for loop)
				continue;
			}

			if ( !PropertiesAreIdentical( Cmd, (void*)( CompareData + Cmd.Offset ), (const void*)( Data + Cmd.Offset ) ) )
			{
				// Add this properties handle to the change list
				Changed.Add( Cmd.RelativeHandle );
			}
		}

		if ( Changed.Num() > 0 )
		{
			// Something changed on this parent property
			PropertyChanged = true;
		}

		//NETWORK_PROFILER( GNetworkProfiler.TrackReplicateProperty( ParentCmd.Property, true, false, FPlatformTime::Cycles() - PropertyStartTime, ParentCmd.Property->ElementSize * 8, 0 ) );
	}

	return PropertyChanged;
}

void FRepLayout::LogChangeListMismatches( 
	const uint8 * 						Data, 
	UActorChannel *						OwningChannel, 
	const TArray< uint16 > &			PropertyList,
	FRepState *							RepState1, 
	FRepState * 						RepState2, 
	const TArray< FRepChangedParent > & ChangedParents1,
	const TArray< FRepChangedParent > & ChangedParents2 ) const
{
	FRepChangedPropertyTracker *	ChangeTracker	= RepState1->RepChangedPropertyTracker.Get();
	UObject *						Object			= (UObject*)Data;
	const UNetDriver *				NetDriver		= OwningChannel->Connection->Driver;

	UE_LOG( LogNet, Warning, TEXT( "LogChangeListMismatches: %s" ), *Object->GetName() );

	UE_LOG( LogNet, Warning, TEXT( "  %i, %i, %i, %i" ), NetDriver->ReplicationFrame, ChangeTracker->LastReplicationFrame, ChangeTracker->LastReplicationGroupFrame, RepState1->LastReplicationFrame );

	for ( int32 i = 0; i < PropertyList.Num(); i++ )
	{
		const int32 Index		= PropertyList[i];
		const int32 Changed1	= ChangedParents1[Index].Changed.Num();
		const int32 Changed2	= ChangedParents2[Index].Changed.Num();

		if ( Changed1 || Changed2 )
		{
			FString Changed1Str = Changed1 ? TEXT( "TRUE" ) : TEXT( "FALSE" );
			FString Changed2Str = Changed2 ? TEXT( "TRUE" ) : TEXT( "FALSE" );
			FString ExtraStr	= Changed1 != Changed2 ? TEXT( "<--- MISMATCH!" ) : TEXT( "<--- SAME" );

			UE_LOG( LogNet, Warning, TEXT( "    Property changed: %s (%s / %s) %s" ), *Parents[Index].Property->GetName(), *Changed1Str, *Changed2Str, *ExtraStr );

			FString StringValue1;
			FString StringValue2;
			FString StringValue3;

			Parents[Index].Property->ExportText_InContainer( Parents[Index].ArrayIndex, StringValue1, Data, NULL, NULL, PPF_DebugDump );
			Parents[Index].Property->ExportText_InContainer( Parents[Index].ArrayIndex, StringValue2, RepState1->StaticBuffer.GetTypedData(), NULL, NULL, PPF_DebugDump );
			Parents[Index].Property->ExportText_InContainer( Parents[Index].ArrayIndex, StringValue3, RepState2->StaticBuffer.GetTypedData(), NULL, NULL, PPF_DebugDump );

			UE_LOG( LogNet, Warning, TEXT( "      Values: %s, %s, %s" ), *StringValue1, *StringValue2, *StringValue3 );
		}
	}
}

static bool ChangedParentsHasChanged( const TArray< uint16 > & PropertyList, const TArray< FRepChangedParent > & ChangedParents )
{
	for ( int32 i = 0; i < PropertyList.Num(); i++ )
	{
		if ( ChangedParents[PropertyList[i]].Changed.Num() )
		{
			return true;
		}
	}

	return false;
}

void FRepLayout::SanityCheckShadowStateAgainstChangeList( 
	FRepState *							RepState, 
	const uint8 *						Data, 
	UActorChannel *						OwningChannel, 
	const TArray< uint16 > &			PropertyList,
	FRepState *							OtherRepState,
	const TArray< FRepChangedParent > & OtherChangedParents ) const
{
	const uint8 *	CompareData	= RepState->StaticBuffer.GetTypedData();
	UObject *		Object		= (UObject*)Data;

	TArray< FRepChangedParent > LocalChangedParents;
	LocalChangedParents.SetNum( OtherChangedParents.Num() );

	// Do an actual check to see which properties are different from our internal shadow state, and make sure the passed in change list matches
	const bool Result = CompareProperties( RepState, CompareData, Data, LocalChangedParents, PropertyList );

	if ( Result != ChangedParentsHasChanged( PropertyList, OtherChangedParents ) )
	{
		LogChangeListMismatches( Data, OwningChannel, PropertyList, RepState, OtherRepState, LocalChangedParents, OtherChangedParents );
		UE_LOG( LogNet, Fatal, TEXT( "ReplicateProperties: Compare result mismatch: %s" ), *Object->GetName() );
	}

	for ( int32 i = 0; i < PropertyList.Num(); i++ )
	{
		const int32 Index = PropertyList[i];

		if ( OtherChangedParents[Index].Changed.Num() != LocalChangedParents[Index].Changed.Num() )
		{
			LogChangeListMismatches( Data, OwningChannel, PropertyList, RepState, OtherRepState, LocalChangedParents, OtherChangedParents );
			UE_LOG( LogNet, Fatal, TEXT( "ReplicateProperties: Compare count mismatch: %s" ), *Object->GetName() );
		}

		for ( int32 j = 0; j < OtherChangedParents[Index].Changed.Num(); j++ )
		{
			if ( OtherChangedParents[Index].Changed[j] != LocalChangedParents[Index].Changed[j] )
			{
				LogChangeListMismatches( Data, OwningChannel, PropertyList, RepState, OtherRepState, LocalChangedParents, OtherChangedParents );
				UE_LOG( LogNet, Fatal, TEXT( "ReplicateProperties: Compare changelist value mismatch: %s" ), *Object->GetName() );
			}
		}
	}
}

bool FRepLayout::GenerateChangelist( FRepState * RepState, const uint8 * Data, TArray< FRepChangedParent > & OutChangedParents ) const
{
	OutChangedParents.Empty();

	OutChangedParents.SetNum( Parents.Num() );

	return CompareProperties( RepState, RepState->StaticBuffer.GetTypedData(), Data, OutChangedParents, UnconditionalLifetime );
}

void FRepLayout::ValidateChangelist(
	FRepState *							RepState, 
	const uint8 * 						Data, 
	UActorChannel *						OwningChannel,
	FRepState *							OtherRepState,
	const TArray< FRepChangedParent > &	OtherChangedParents ) const
{
	SanityCheckShadowStateAgainstChangeList( RepState, Data, OwningChannel, UnconditionalLifetime, OtherRepState, OtherChangedParents );
}


bool FRepLayout::ReplicateProperties( 
	FRepState * RESTRICT		RepState, 
	const uint8 * RESTRICT		Data, 
	UClass *					ObjectClass,
	UActorChannel *				OwningChannel,
	FOutBunch &					Writer, 
	const FReplicationFlags &	RepFlags,
	int32 &						LastIndex, 
	bool &						bContentBlockWritten ) const
{
	SCOPE_CYCLE_COUNTER( STAT_NetReplicateDynamicPropTime );

	check( ObjectClass == Owner );

	UObject *						Object			= (UObject*)Data;
	const UNetDriver *				NetDriver		= OwningChannel->Connection->Driver;
	FRepChangedPropertyTracker *	ChangeTracker	= RepState->RepChangedPropertyTracker.Get();
	const uint8 *					CompareData		= RepState->StaticBuffer.GetTypedData();

	// Rebuild conditional properties if needed
	if ( RepState->RepFlags.Value != RepFlags.Value || RepState->ActiveStatusChanged != ChangeTracker->ActiveStatusChanged )
	{
		RebuildConditionalProperties( RepState, *ChangeTracker, RepFlags );

		RepState->RepFlags.Value		= RepFlags.Value;
		RepState->ActiveStatusChanged	= ChangeTracker->ActiveStatusChanged;
	}

	bool PropertyChanged = false;

#ifdef ENABLE_SUPER_CHECKSUMS
	const bool bIsAllAcked = AllAcked( RepState );

	if ( bIsAllAcked || !RepState->OpenAckedCalled )
#endif
	{
		const int32	AllowSkipping = CVarAllowPropertySkipping.GetValueOnGameThread();
		
		const bool bCanSkip =	AllowSkipping > 0 && 
								RepState->LastReplicationFrame != 0 &&
								ChangeTracker->LastReplicationFrame == NetDriver->ReplicationFrame &&
								ChangeTracker->LastReplicationGroupFrame == RepState->LastReplicationFrame;

		if ( bCanSkip )
		{
			INC_DWORD_STAT_BY( STAT_NetSkippedDynamicProps, UnconditionalLifetime.Num() );

			if ( AllowSkipping == 2 )
			{
				// Sanity check results
				check( ChangeTracker->UnconditionalPropChanged == ChangedParentsHasChanged( UnconditionalLifetime, ChangeTracker->Parents ) );

				SanityCheckShadowStateAgainstChangeList( RepState, Data, OwningChannel, UnconditionalLifetime, ChangeTracker->LastRepState, ChangeTracker->Parents );
			}
		}
		else
		{
			// FRepState group changed, force this group to compare again this frame
			// This happens either once a frame, which is normal, or multiple times a frame 
			// when multiple connections of the same actor aren't updated at the same time
			ChangeTracker->LastReplicationFrame			= NetDriver->ReplicationFrame;
			ChangeTracker->LastReplicationGroupFrame	= RepState->LastReplicationFrame;
			ChangeTracker->LastRepState					= RepState;

			// Reset changed list if anything changed last time
			if ( ChangeTracker->UnconditionalPropChanged )
			{
				for ( int32 i = UnconditionalLifetime.Num() - 1; i >= 0; i-- )
				{
					ChangeTracker->Parents[UnconditionalLifetime[i]].Changed.Empty();
				}
			}

			// Loop over all unconditional lifetime properties
			ChangeTracker->UnconditionalPropChanged = CompareProperties( RepState, CompareData, Data, ChangeTracker->Parents, UnconditionalLifetime );
		}

		// Remember the last frame this FRepState was replicated, so we can note above when the FRepState replication group changes
		RepState->LastReplicationFrame = NetDriver->ReplicationFrame;

		if ( ChangeTracker->UnconditionalPropChanged )
		{
			PropertyChanged	= true;
		}

		// Loop over all the conditional properties
		if ( CompareProperties( RepState, CompareData, Data, ChangeTracker->Parents, RepState->ConditionalLifetime ) )
		{
			PropertyChanged = true;
		}
	}
#ifdef ENABLE_SUPER_CHECKSUMS
	else
	{
		// If we didn't compare this frame, make sure to reset out replication frame
		// This is to force a compare next time it comes up
		RepState->LastReplicationFrame = 0;		
	}
#endif

	// PreOpenAckHistory are all the properties sent before we got our first open ack
	const bool bFlushPreOpenAckHistory = RepState->OpenAckedCalled && RepState->PreOpenAckHistory.Num() > 0;

	if ( PropertyChanged || RepState->Unmapped.Num() > 0 || RepState->NumNaks > 0 || bFlushPreOpenAckHistory )
	{
		// Use the first inactive history item to build this change list on
		check( RepState->HistoryEnd - RepState->HistoryStart < FRepState::MAX_CHANGE_HISTORY );
		const int32 HistoryIndex = RepState->HistoryEnd % FRepState::MAX_CHANGE_HISTORY;

		FRepChangedHistory & NewHistoryItem = RepState->ChangeHistory[ HistoryIndex ];

		RepState->HistoryEnd++;

		TArray<uint16> & Changed = NewHistoryItem.Changed;

		check( Changed.Num() == 0 );		// Make sure this history item is actually inactive

		if ( PropertyChanged )
		{
			// Initialize the history item change list with the parent change lists
			// We do it in the order of the parents so that the final change list will be fully sorted
			for ( int32 i = 0; i < Parents.Num(); i++ )
			{
				if ( ChangeTracker->Parents[i].Changed.Num() > 0 )
				{
					Changed.Append( ChangeTracker->Parents[i].Changed );

					if ( Parents[i].Flags & PARENT_IsConditional )
					{
						// Reset properties that don't share information across connections
						ChangeTracker->Parents[i].Changed.Empty();
					}
				}
			}

			Changed.Add( 0 );

#ifdef SANITY_CHECK_MERGES
			SanityCheckChangeList( Data, Changed );
#endif
		}

		// Update the history, and merge in any nak'd change lists
		UpdateChangelistHistory( RepState, ObjectClass, Data, OwningChannel->Connection->OutAckPacketId, &Changed );

		// Merge in unmapped properties to be resent
		if ( RepState->Unmapped.Num() > 0 )
		{
			TArray< uint16 > Temp = Changed;
			Changed.Empty();
			MergeDirtyList( (void*)Data, Temp, RepState->Unmapped, Changed );

#ifdef SANITY_CHECK_MERGES
			SanityCheckChangeList( Data, Changed );
#endif

			RepState->Unmapped.Empty();
		}

		// Merge in the PreOpenAckHistory (unreliable properties sent before the bunch was initially acked)
		if ( bFlushPreOpenAckHistory )
		{
			for ( int32 i = 0; i < RepState->PreOpenAckHistory.Num(); i++ )
			{
				TArray< uint16 > Temp = Changed;
				Changed.Empty();
				MergeDirtyList( (void*)Data, Temp, RepState->PreOpenAckHistory[i].Changed, Changed );
			}
			RepState->PreOpenAckHistory.Empty();
		}

		// At this point we should have a non empty change list
		check( Changed.Num() > 0 );

#ifdef SANITY_CHECK_MERGES
		SanityCheckChangeList( Data, Changed );
#endif

		// For RepLayout properties, we hijack the first non custom property, and use that to identify these properties
		WritePropertyHeader( (UObject*)Data, ObjectClass, OwningChannel, Parents[FirstNonCustomParent].Property, Writer, 0, LastIndex, bContentBlockWritten );

		// Send the final merged change list
		SendProperties( RepState, Data, ObjectClass, OwningChannel, Writer, Changed, LastIndex, bContentBlockWritten );

#ifdef ENABLE_SUPER_CHECKSUMS
		Writer.WriteBit( bIsAllAcked ? 1 : 0 );

		if ( bIsAllAcked )
		{
			ValidateWithChecksum( RepState->StaticBuffer.GetTypedData(), Writer, false );
		}
#endif
		
		if ( RepState->Unmapped.Num() )
		{
			OwningChannel->bActorMustStayDirty = true;
			RepState->UnmappedFrames++;
			if ( RepState->UnmappedFrames >= UNMAPPED_FRAMES_THRESHOLD )
			{
				UE_LOG( LogNet, Warning, TEXT( "FRepLayout::ReplicateProperties: Properties have been unmapped longer than normal: %s" ), *ObjectClass->GetName() );
				RepState->UnmappedFrames = 0;
			}
		}
		else
		{
			RepState->UnmappedFrames = 0;
		}

		return true;
	}

	// Nothing changed and there are no nak's, so just do normal housekeeping and remove acked history items
	UpdateChangelistHistory( RepState, ObjectClass, Data, OwningChannel->Connection->OutAckPacketId, NULL );

	return false;
}

void FRepLayout::UpdateChangelistHistory( FRepState * RepState, UClass * ObjectClass, const uint8 * RESTRICT Data, const int32 AckPacketId, TArray< uint16 > * OutMerged ) const
{
	check( RepState->HistoryEnd >= RepState->HistoryStart );

	const int32 HistoryCount	= RepState->HistoryEnd - RepState->HistoryStart;
	const bool DumpHistory		= HistoryCount == FRepState::MAX_CHANGE_HISTORY;

	// If our buffer is currently full, forcibly send the entire history
	if ( DumpHistory )
	{
		UE_LOG( LogNet, Warning, TEXT( "FRepLayout::UpdateChangelistHistory: History overflow, forcing history dump %s" ), *ObjectClass->GetName() );
	}

	for ( int32 i = RepState->HistoryStart; i < RepState->HistoryEnd; i++ )
	{
		const int32 HistoryIndex = i % FRepState::MAX_CHANGE_HISTORY;

		FRepChangedHistory & HistoryItem = RepState->ChangeHistory[ HistoryIndex ];

		if ( HistoryItem.OutPacketIdRange.First == INDEX_NONE )
		{
			continue;		//  Hasn't been initialized in PostReplicate yet
		}

		check( HistoryItem.Changed.Num() > 0 );		// All active history items should contain a change list

		if ( AckPacketId >= HistoryItem.OutPacketIdRange.Last || HistoryItem.Resend || DumpHistory )
		{
			if ( HistoryItem.Resend || DumpHistory )
			{
				// Merge in nak'd change lists
				check( OutMerged != NULL );
				TArray< uint16 > Temp = *OutMerged;
				OutMerged->Empty();
				MergeDirtyList( (void*)Data, Temp, HistoryItem.Changed, *OutMerged );
				HistoryItem.Changed.Empty();

#ifdef SANITY_CHECK_MERGES
				SanityCheckChangeList( Data, *OutMerged );
#endif

				if ( HistoryItem.Resend )
				{
					HistoryItem.Resend = false;
					RepState->NumNaks--;
				}
			}

			HistoryItem.Changed.Empty();
			HistoryItem.OutPacketIdRange = FPacketIdRange();
			RepState->HistoryStart++;
		}
	}

	// Remove any tiling in the history markers to keep them from wrapping over time
	const int32 NewHistoryCount	= RepState->HistoryEnd - RepState->HistoryStart;

	RepState->HistoryStart	= RepState->HistoryStart % FRepState::MAX_CHANGE_HISTORY;
	RepState->HistoryEnd	= RepState->HistoryStart + NewHistoryCount;

	check( RepState->NumNaks == 0 );	// Make sure we processed all the naks properly
}

void FRepLayout::OpenAcked( FRepState * RepState ) const
{
	check( RepState != NULL );
	RepState->OpenAckedCalled = true;
}

void FRepLayout::PostReplicate( FRepState * RepState, FPacketIdRange & PacketRange, bool bReliable ) const
{
	for ( int32 i = RepState->HistoryStart; i < RepState->HistoryEnd; i++ )
	{
		const int32 HistoryIndex = i % FRepState::MAX_CHANGE_HISTORY;

		FRepChangedHistory & HistoryItem = RepState->ChangeHistory[ HistoryIndex ];

		if ( HistoryItem.OutPacketIdRange.First == INDEX_NONE )
		{
			check( HistoryItem.Changed.Num() > 0 );
			check( !HistoryItem.Resend );

			HistoryItem.OutPacketIdRange = PacketRange;

			if ( !bReliable && !RepState->OpenAckedCalled )
			{
				RepState->PreOpenAckHistory.Add( HistoryItem );
			}
		}
	}
}

void FRepLayout::ReceivedNak( FRepState * RepState, int32 NakPacketId ) const
{
	if ( RepState == NULL )
	{
		return;		// I'm not 100% certain why this happens, the only think I can think of is this is a bNetTemporary?
	}

	for ( int32 i = RepState->HistoryStart; i < RepState->HistoryEnd; i++ )
	{
		const int32 HistoryIndex = i % FRepState::MAX_CHANGE_HISTORY;

		FRepChangedHistory & HistoryItem = RepState->ChangeHistory[ HistoryIndex ];

		if ( !HistoryItem.Resend && HistoryItem.OutPacketIdRange.InRange( NakPacketId ) )
		{
			check( HistoryItem.Changed.Num() > 0 );
			HistoryItem.Resend = true;
			RepState->NumNaks++;
		}
	}
}

bool FRepLayout::AllAcked( FRepState * RepState ) const
{	
	if ( RepState->HistoryStart != RepState->HistoryEnd )
	{
		// We have change lists that haven't been acked
		return false;
	}

	if ( RepState->Unmapped.Num() > 0 )
	{
		// We have unmapped properties that need to be resent
		return false;
	}

	if ( RepState->NumNaks > 0 )
	{
		return false;
	}

	if ( !RepState->OpenAckedCalled )
	{
		return false;
	}

	if ( RepState->PreOpenAckHistory.Num() > 0 )
	{
		return false;
	}

	return true;
}

bool FRepLayout::ReadyForDormancy( FRepState * RepState ) const
{
	if ( RepState == NULL )
	{
		return false;
	}

	return AllAcked( RepState );
}

static FORCEINLINE void WritePropertyHandle( FNetBitWriter & Writer, uint16 Handle, bool bDoChecksum )
{
	uint32 LocalHandle = Handle;
	Writer.SerializeIntPacked( LocalHandle );

#ifdef ENABLE_PROPERTY_CHECKSUMS
	if ( bDoChecksum )
	{
		SerializeGenericChecksum( Writer );
	}
#endif
}

static bool ShouldSendProperty( FRepWriterState & WriterState, uint16 Handle )
{
	if ( Handle == WriterState.Changed[WriterState.CurrentChanged] )
	{
		// Write out the handle
		WritePropertyHandle( WriterState.Writer, Handle, WriterState.bDoChecksum );

		// Now load the next expected handle (we should have at least one)
		WriterState.CurrentChanged++;

		return true;
	}

	return false;
}

void FRepLayout::SendProperties_DynamicArray_r( 
	FRepState *	RESTRICT	RepState, 
	FRepWriterState &		WriterState,
	const int32				CmdIndex, 
	const uint8 * RESTRICT	StoredData, 
	const uint8 * RESTRICT	Data, 
	TArray< uint16 > &		Unmapped,
	uint16					Handle ) const
{
	const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

	FScriptArray * Array = (FScriptArray *)Data;
	FScriptArray * StoredArray = (FScriptArray *)StoredData;

	// Write array num
	uint16 ArrayNum = Array->Num();
	WriterState.Writer << ArrayNum;

	// Make the shadow state match the actual state at the time of send
	FScriptArrayHelper StoredArrayHelper( (UArrayProperty *)Cmd.Property, StoredData );
	StoredArrayHelper.Resize( ArrayNum );

	// Read the jump offset
	// We won't need to actually jump over anything because we expect the change list to be pruned once we get here
	// But we can use it to verify we read the correct amount.
	const int32 ArrayChangedCount = WriterState.Changed[WriterState.CurrentChanged++];

	const int32 OldChangedIndex = WriterState.CurrentChanged;

	Data = (uint8*)Array->GetData();
	StoredData = (uint8*)StoredArray->GetData();

	TArray< uint16 > LocalUnmapped;

	uint16 LocalHandle = 0;

	for ( int32 i = 0; i < Array->Num(); i++ )
	{
		const int32 ElementOffset = i * Cmd.ElementSize;
		LocalHandle = SendProperties_r( RepState, WriterState, CmdIndex + 1, Cmd.EndCmd - 1, StoredData + ElementOffset, Data + ElementOffset, LocalUnmapped, LocalHandle );
	}

	check( WriterState.CurrentChanged - OldChangedIndex == ArrayChangedCount );	// Make sure we read correct amount
	check( WriterState.Changed[WriterState.CurrentChanged] == 0 );				// Make sure we are at the end

	WriterState.CurrentChanged++;

	WritePropertyHandle( WriterState.Writer, 0, WriterState.bDoChecksum );		// Signify end of dynamic array

	if ( LocalUnmapped.Num() > 0 )
	{
		// We have unmapped properties, save them
		Unmapped.Add( Handle );
		Unmapped.Add( LocalUnmapped.Num() );	// So we can jump over this array if needed
		Unmapped.Append( LocalUnmapped );
		Unmapped.Add( 0 );
	}
}

uint16 FRepLayout::SendProperties_r( 
	FRepState *	RESTRICT	RepState, 
	FRepWriterState &		WriterState,
	const int32				CmdStart, 
	const int32				CmdEnd, 
	const uint8 * RESTRICT	StoredData, 
	const uint8 * RESTRICT	Data, 
	TArray< uint16 > &		Unmapped,
	uint16					Handle ) const
{
	for ( int32 CmdIndex = CmdStart; CmdIndex < CmdEnd; CmdIndex++ )
	{
		const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

		check( Cmd.Type != REPCMD_Return );

		Handle++;

		if ( Cmd.Type == REPCMD_DynamicArray )
		{
			if ( ShouldSendProperty( WriterState, Handle ) )
			{
				SendProperties_DynamicArray_r( RepState, WriterState, CmdIndex, StoredData + Cmd.Offset, Data + Cmd.Offset, Unmapped, Handle );
			}
			CmdIndex = Cmd.EndCmd - 1;	// Jump past children of this array (-1 for the ++ in the for loop)
			continue;
		}

		if ( ShouldSendProperty( WriterState, Handle ) )
		{

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDoReplicationContextString->GetInt() > 0)
			{
				WriterState.Writer.PackageMap->SetDebugContextString( FString::Printf(TEXT("%s - %s"), *Owner->GetPathName(), *Cmd.Property->GetPathName() ) );
			}
#endif

			const int32 NumStartBits = WriterState.Writer.GetNumBits();
			
			// This property changed, so send it
			WriterState.Writer.PackageMap->ResetUnAckedObject();	// Set this to false so floats, ints, etc don't trigger it
			bool bMapped = Cmd.Property->NetSerializeItem( WriterState.Writer, WriterState.Writer.PackageMap, (void*)( Data + Cmd.Offset ) );

			const int32 NumEndBits = WriterState.Writer.GetNumBits();

			const FRepParentCmd & Parent = Parents[Cmd.ParentIndex];

			NETWORK_PROFILER( GNetworkProfiler.TrackReplicateProperty( Parent.Property, false, false, 0, 0, NumEndBits - NumStartBits ) );

			// If this property is unmapped, force it to resend
			if ( !bMapped || WriterState.Writer.PackageMap->SerializedUnAckedObject() )
			{
				Unmapped.Add( Handle );

				if ( RepState->UnmappedFrames == UNMAPPED_FRAMES_THRESHOLD - 1 )
				{
					UE_LOG( LogNet, Warning, TEXT( "  FRepLayout::SendProperties_r: Property has been unmapped longer than normal: %s" ), *Cmd.Property->GetName() );
				}
			}

			// Make the shadow state match the actual state at the time of send
			StoreProperty( Cmd, (void*)( StoredData + Cmd.Offset ), (const void*)( Data + Cmd.Offset ) );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (CVarDoReplicationContextString->GetInt() > 0)
			{
				WriterState.Writer.PackageMap->ClearDebugContextString();
			}
#endif

#ifdef ENABLE_PROPERTY_CHECKSUMS
			if ( WriterState.bDoChecksum )
			{
				SerializeReadWritePropertyChecksum( Cmd, CmdIndex, Data + Cmd.Offset, WriterState.Writer, false );
			}
#endif
		}
	}

	return Handle;
}

void FRepLayout::WritePropertyHeader( 
	UObject *			Object,
	UClass *			ObjectClass,
	UActorChannel *		OwningChannel,
	UProperty *			Property, 
	FOutBunch &			Bunch, 
	int32				ArrayIndex, 
	int32 &				LastArrayIndex, 
	bool &				bContentBlockWritten ) const
{
	UNetConnection * Connection = OwningChannel->Connection;

	// Get class network info cache.
	FClassNetCache * ClassCache = Connection->PackageMap->GetClassNetCache( ObjectClass );
	check( ClassCache );

	if ( !bContentBlockWritten )
	{
		OwningChannel->BeginContentBlock( Object, Bunch );
		bContentBlockWritten = true;
	}

	// Get the network friend property index to replicate
	// Swap Role/RemoteRole while we're at it
	FFieldNetCache * FieldCache
		=	Property->GetFName() == NAME_Role
		?	ClassCache->GetFromField( Connection->Driver->RemoteRoleProperty )
		:	Property->GetFName() == NAME_RemoteRole
		?	ClassCache->GetFromField( Connection->Driver->RoleProperty )
		:	ClassCache->GetFromField( Property );

	checkSlow( FieldCache );

	// Send property name and optional array index.
	check( FieldCache->FieldNetIndex <= ClassCache->GetMaxIndex() );

	Bunch.WriteIntWrapped( FieldCache->FieldNetIndex, ClassCache->GetMaxIndex() + 1 );

	NET_CHECKSUM( Bunch );

	if ( Property->ArrayDim != 1 )
	{
		// Serialize index as delta from previous index to increase chance we'll only use 1 byte
		uint32 idx = static_cast<uint32>( ArrayIndex - LastArrayIndex );
		Bunch.SerializeIntPacked(idx);
		LastArrayIndex = ArrayIndex;
	}
}

void FRepLayout::SendProperties( 
	FRepState *	RESTRICT	RepState, 
	const uint8 * RESTRICT	Data, 
	UClass *				ObjectClass,
	UActorChannel	*		OwningChannel,
	FOutBunch &				Writer, 
	TArray< uint16 > &		Changed, 
	int32 &					LastIndex, 
	bool &					bContentBlockWritten ) const
{
	check( RepState->Unmapped.Num() == 0 );

#ifdef ENABLE_PROPERTY_CHECKSUMS
	const bool bDoChecksum = CVarDoPropertyChecksum.GetValueOnGameThread() == 1;
#else
	const bool bDoChecksum = false;
#endif

	FRepWriterState WriterState( Writer, Changed, bDoChecksum );

#ifdef ENABLE_PROPERTY_CHECKSUMS
	Writer.WriteBit( bDoChecksum ? 1 : 0 );
#endif

	SendProperties_r( RepState, WriterState, 0, Cmds.Num() - 1, RepState->StaticBuffer.GetTypedData(), Data, RepState->Unmapped, 0 );

	WritePropertyHandle( Writer, 0, bDoChecksum );

	if ( RepState->Unmapped.Num() > 0 )
	{
		RepState->Unmapped.Add( 0 );
	}
}

static void ReadNextHandle( FRepReaderState & ReaderState )
{
	ReaderState.Bunch.SerializeIntPacked( ReaderState.WaitingHandle );

#ifdef ENABLE_PROPERTY_CHECKSUMS
	if ( ReaderState.bDoChecksum )
	{
		SerializeGenericChecksum( ReaderState.Bunch );
	}
#endif
}

static bool ShouldReadProperty( FRepReaderState & ReaderState )
{
	ReaderState.CurrentHandle++;

	if ( ReaderState.CurrentHandle == ReaderState.WaitingHandle )
	{
		check( ReaderState.WaitingHandle != 0 );
		return true;
	}

	return false;
}

bool FRepLayout::ReadProperty( FRepReaderState & ReaderState, const FRepLayoutCmd & Cmd, const int32 CmdIndex, uint8 * RESTRICT StoredData, uint8 * RESTRICT Data, const bool bDiscard ) const
{
	if ( !ShouldReadProperty( ReaderState ) )
	{
		// This property didn't change, nothing to read
		return true;
	}

	if ( bDiscard )
	{
		FMemMark Mark( FMemStack::Get() );
		uint8 * DiscardBuffer = NewZeroed<uint8>( FMemStack::Get(), Cmd.ElementSize );
		const int32 OldArrayDim = Cmd.Property->ArrayDim;
		Cmd.Property->ArrayDim = 1;		// Force this to 1, so that InitializeValue/DestroyValue work on a single item
		Cmd.Property->InitializeValue( DiscardBuffer );
		Cmd.Property->NetSerializeItem( ReaderState.Bunch, ReaderState.Bunch.PackageMap, DiscardBuffer );
		Cmd.Property->DestroyValue( DiscardBuffer );
		Cmd.Property->ArrayDim = OldArrayDim;
		Mark.Pop();

		// Read the next property handle
		ReadNextHandle( ReaderState );
		return true;
	}

	const FRepParentCmd & Parent = Parents[Cmd.ParentIndex];

	// This swaps Role/RemoteRole as we write it
	const FRepLayoutCmd & SwappedCmd = Parent.RoleSwapIndex != -1 ? Cmds[Parents[Parent.RoleSwapIndex].CmdStart] : Cmd;

	if ( Parent.Property->HasAnyPropertyFlags( CPF_RepNotify ) )
	{
		// Copy current value over so we can check to see if it changed
		StoreProperty( Cmd, StoredData + Cmd.Offset, Data + SwappedCmd.Offset );

		// Read the property
		Cmd.Property->NetSerializeItem( ReaderState.Bunch, ReaderState.Bunch.PackageMap, Data + SwappedCmd.Offset );

		// Check to see if this property changed
		if ( !PropertiesAreIdentical( Cmd, StoredData + Cmd.Offset, Data + SwappedCmd.Offset ) )
		{
			ReaderState.RepState->RepNotifies.AddUnique( Parent.Property );
		}
	} 
	else
	{
		Cmd.Property->NetSerializeItem( ReaderState.Bunch, ReaderState.Bunch.PackageMap, Data + SwappedCmd.Offset );
	}

#ifdef ENABLE_PROPERTY_CHECKSUMS
	if ( ReaderState.bDoChecksum )
	{
		SerializeReadWritePropertyChecksum( Cmd, CmdIndex, Data + SwappedCmd.Offset, ReaderState.Bunch, false );
	}
#endif

	// Store the property we just received FIXME: Can't do this because of rep notifies. This needs to be done for super checksums though, FIXME!!!
	//StoreProperty( Cmd, StoredData + Cmd.Offset, Data + SwappedCmd.Offset );

	// Read the next property handle
	ReadNextHandle( ReaderState );

	return true;
}

bool FRepLayout::ReceiveProperties_AnyArray_r( 
	FRepReaderState &	ReaderState, 
	const int32			ArrayNum, 
	const int32			ElementSize, 
	const int32			CmdIndex, 
	uint8 * RESTRICT	StoredData, 
	uint8 *	RESTRICT	Data,
	const bool			bDiscard ) const
{
	const int32 CmdStart	= CmdIndex + 1;
	const int32 CmdEnd		= Cmds[CmdIndex].EndCmd - 1;

	for ( int32 i = 0; i < ArrayNum; i++ )
	{
		const int32 ElementOffset = i * ElementSize;

		if ( !ReceiveProperties_r( ReaderState, CmdStart, CmdEnd, StoredData + ElementOffset, Data + ElementOffset, bDiscard ) )
		{
			return false;
		}
	}

	return true;
}

bool FRepLayout::ReceiveProperties_DynamicArray_r( 
	FRepReaderState &		ReaderState, 
	const FRepLayoutCmd &	Cmd, 
	const int32				CmdIndex, 
	uint8 * RESTRICT		StoredData, 
	uint8 * RESTRICT		Data,
	const bool				bDiscard ) const
{
	if ( !ShouldReadProperty( ReaderState ) )
	{
		// This array didn't change, nothing to read
		return true;
	}

	// Read array size
	uint16 ArrayNum = 0;
	ReaderState.Bunch << ArrayNum;

	// Read the next property handle
	ReadNextHandle( ReaderState );

	if ( !bDiscard )
	{
		FScriptArray * Array = (FScriptArray *)Data;
		FScriptArray * StoredArray = (FScriptArray *)StoredData;

		const FRepParentCmd & Parent = Parents[Cmd.ParentIndex];

		if ( Array->Num() != ArrayNum && Parent.Property->HasAnyPropertyFlags( CPF_RepNotify ) )
		{
			ReaderState.RepState->RepNotifies.AddUnique( Parent.Property );
		}

		// Set the array to the correct size (if we aren't discarding)
		FScriptArrayHelper ArrayHelper( (UArrayProperty *)Cmd.Property, Data );
		ArrayHelper.Resize( ArrayNum );

		FScriptArrayHelper StoredArrayHelper( (UArrayProperty *)Cmd.Property, StoredData );
		StoredArrayHelper.Resize( ArrayNum );

		Data		= (uint8*)Array->GetData();
		StoredData	= (uint8*)StoredArray->GetData();
	}

	const uint16 OldHandle = ReaderState.CurrentHandle;

	// Array children handles are always relative to their immediate parent
	ReaderState.CurrentHandle = 0;

	// Read any changed properties into the array
	if ( !ReceiveProperties_AnyArray_r( ReaderState, ArrayNum, Cmd.ElementSize, CmdIndex, StoredData, Data, bDiscard ) )
	{
		return false;
	}

	ReaderState.CurrentHandle = OldHandle;

	// We should be waiting on the NULL terminator handle at this point
	check( ReaderState.WaitingHandle == 0 );
	ReadNextHandle( ReaderState );

	return true;
}

bool FRepLayout::ReceiveProperties_r( 
	FRepReaderState &	ReaderState, 
	const int32			CmdStart, 
	const int32			CmdEnd, 
	uint8 * RESTRICT	StoredData, 
	uint8 * RESTRICT	Data,
	const bool			bDiscard ) const
{
	for ( int32 CmdIndex = CmdStart; CmdIndex < CmdEnd; CmdIndex++ )
	{
		const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

		check( Cmd.Type != REPCMD_Return );

		if ( Cmd.Type == REPCMD_DynamicArray )
		{
			if ( !ReceiveProperties_DynamicArray_r( ReaderState, Cmd, CmdIndex, StoredData + Cmd.Offset, Data + Cmd.Offset, bDiscard ) )
			{
				return false;
			}
			CmdIndex = Cmd.EndCmd - 1;	// Jump past children of this array ( -1 for the ++ in the for loop)
			continue;
		}

		if ( !ReadProperty( ReaderState, Cmd, CmdIndex, StoredData, Data, bDiscard ) )
		{
			return false;
		}
	}

	return true;
}

bool FRepLayout::ReceiveProperties( UClass * InObjectClass, FRepState * RESTRICT RepState, void * RESTRICT Data, FNetBitReader & InBunch, bool bDiscard ) const
{
	check( InObjectClass == Owner );

#ifdef ENABLE_PROPERTY_CHECKSUMS
	const bool bDoChecksum = InBunch.ReadBit() ? true : false;
#else
	const bool bDoChecksum = false;
#endif

	FRepReaderState ReaderState( InBunch, RepState, bDoChecksum );

	// Read first handle
	ReadNextHandle( ReaderState );

	// Read all properties
	if ( !ReceiveProperties_r( ReaderState, 0, Cmds.Num() - 1, RepState->StaticBuffer.GetTypedData(), (uint8*)Data, bDiscard ) )
	{
		return false;
	}

	// Make sure we're waiting on the last NULL terminator
	if ( ReaderState.WaitingHandle != 0 )
	{
		UE_LOG( LogNet, Warning, TEXT( "Read out of sync." ) );
		return false;
	}

#ifdef ENABLE_SUPER_CHECKSUMS
	if ( InBunch.ReadBit() == 1 )
	{
		ValidateWithChecksum( RepState->StaticBuffer.GetTypedData(), InBunch, bDiscard );
	}
#endif

	return true;
}

void FRepLayout::CallRepNotifies( FRepState * RepState, UObject * Object ) const
{
	if ( RepState->RepNotifies.Num() == 0 )
	{
		return;
	}

	for ( int32 i = 0; i < RepState->RepNotifies.Num(); i++ )
	{
		UProperty * RepProperty = RepState->RepNotifies[i];

		UFunction * RepNotifyFunc = Object->FindFunctionChecked( RepProperty->RepNotifyFunc );

		check( RepNotifyFunc->NumParms <= 1 );	// 2 parms not supported yet

		if ( RepNotifyFunc->NumParms == 0 )
		{
			Object->ProcessEvent( RepNotifyFunc, NULL );
		}
		else if (RepNotifyFunc->NumParms == 1 )
		{
			Object->ProcessEvent( RepNotifyFunc, RepProperty->ContainerPtrToValuePtr<uint8>( RepState->StaticBuffer.GetTypedData() ) );
		}

		// Store the property we just received
		//StoreProperty( Cmd, StoredData + Cmd.Offset, Data + SwappedCmd.Offset );
	}

	RepState->RepNotifies.Empty();
}

void FRepLayout::ValidateWithChecksum_DynamicArray_r( const FRepLayoutCmd & Cmd, const int32 CmdIndex, const uint8 * RESTRICT Data, FArchive & Ar, const bool bDiscard ) const
{
	if ( bDiscard )
	{
		uint16 ArrayNum		= 0;
		uint16 ElementSize	= 0;
		Ar << ArrayNum;
		Ar << ElementSize;

		for ( int32 i = 0; i < ArrayNum; i++ )
		{
			ValidateWithChecksum_r( CmdIndex + 1, Cmd.EndCmd - 1, NULL, Ar, bDiscard );
		}
		return;
	}

	FScriptArray * Array = (FScriptArray *)Data;

	uint16 ArrayNum		= Array->Num();
	uint16 ElementSize	= Cmd.ElementSize;

	Ar << ArrayNum;
	Ar << ElementSize;

	if ( ArrayNum != Array->Num() )
	{
		UE_LOG( LogNet, Fatal, TEXT( "ValidateWithChecksum_AnyArray_r: Array sizes different! %s %i / %i" ), *Cmd.Property->GetFullName(), ArrayNum, Array->Num() );
	}

	if ( ElementSize != Cmd.ElementSize )
	{
		UE_LOG( LogNet, Fatal, TEXT( "ValidateWithChecksum_AnyArray_r: Array element sizes different! %s %i / %i" ), *Cmd.Property->GetFullName(), ElementSize, Cmd.ElementSize );
	}

	uint8 * LocalData = (uint8*)Array->GetData();

	for ( int32 i = 0; i < ArrayNum; i++ )
	{
		ValidateWithChecksum_r( CmdIndex + 1, Cmd.EndCmd - 1, LocalData + i * ElementSize, Ar, bDiscard );
	}
}

void FRepLayout::ValidateWithChecksum_r( 
	const int32				CmdStart, 
	const int32				CmdEnd, 
	const uint8 * RESTRICT	Data, 
	FArchive &				Ar,
	const bool				bDiscard ) const
{
	for ( int32 CmdIndex = CmdStart; CmdIndex < CmdEnd; CmdIndex++ )
	{
		const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

		check( Cmd.Type != REPCMD_Return );

		if ( Cmd.Type == REPCMD_DynamicArray )
		{
			ValidateWithChecksum_DynamicArray_r( Cmd, CmdIndex, Data + Cmd.Offset, Ar, bDiscard );
			CmdIndex = Cmd.EndCmd - 1;	// Jump past children of this array (-1 for ++ in for loop)
			continue;
		}

		SerializeReadWritePropertyChecksum( Cmd, CmdIndex - 1, Data + Cmd.Offset, Ar, bDiscard );
	}
}

void FRepLayout::ValidateWithChecksum( const void * RESTRICT Data, FArchive & Ar, const bool bDiscard ) const
{
	ValidateWithChecksum_r( 0, Cmds.Num() - 1, (const uint8*)Data, Ar, bDiscard );
}

uint32 FRepLayout::GenerateChecksum( const FRepState * RepState ) const
{
	FBitWriter Writer( 1024, true );
	ValidateWithChecksum_r( 0, Cmds.Num() - 1, (const uint8*)RepState->StaticBuffer.GetTypedData(), Writer, false );

	return FCrc::MemCrc32( Writer.GetData(), Writer.GetNumBytes(), 0 );
}

void FRepLayout::MergeDirtyList_AnyArray_r( FMergeDirtyListState & MergeState, const FRepLayoutCmd & Cmd, const int32 ArrayNum, const int32 ElementSize, const int32 CmdIndex, const uint8 * RESTRICT Data ) const
{
	for ( int32 i = 0; i < ArrayNum; i++ )
	{
		MergeDirtyList_r( MergeState, CmdIndex + 1, Cmd.EndCmd - 1, Data + i * ElementSize );
	}
}

void FRepLayout::MergeDirtyList_DynamicArray_r( FMergeDirtyListState & MergeState, const FRepLayoutCmd & Cmd, const int32 CmdIndex, const uint8 * RESTRICT Data ) const
{
	// At least one of the list should be valid to be here
	check( MergeState.DirtyValid1 || MergeState.DirtyValid2 );

	MergeState.Handle++;

	const bool Dirty1Matches = MergeState.DirtyValid1 && MergeState.DirtyList1[MergeState.DirtyListIndex1] == MergeState.Handle;
	const bool Dirty2Matches = MergeState.DirtyValid2 && MergeState.DirtyList2[MergeState.DirtyListIndex2] == MergeState.Handle;

	if ( !Dirty1Matches && !Dirty2Matches )
	{
		// Neither match, we're done with this branch
		return;
	}

	// This will be a new merged dirty entry
	MergeState.MergedDirtyList.Add( MergeState.Handle );

	const int32 RememberIndex = MergeState.MergedDirtyList.AddUninitialized();
	const int32 OldMergedListSize = MergeState.MergedDirtyList.Num();

	// Advance the matching dirty lists
	if ( Dirty1Matches )
	{
		MergeState.DirtyListIndex1++;
	}

	if ( Dirty2Matches )
	{
		MergeState.DirtyListIndex2++;
	}

	// Remember valid dirty lists
	const bool OldDirtyValid1 = MergeState.DirtyValid1;
	const bool OldDirtyValid2 = MergeState.DirtyValid2;

	// Update which lists are still valid from this point
	MergeState.DirtyValid1 = Dirty1Matches;
	MergeState.DirtyValid2 = Dirty2Matches;

	const int32 JumpToIndex1 = Dirty1Matches ? MergeState.DirtyList1[MergeState.DirtyListIndex1++] : -1;
	const int32 JumpToIndex2 = Dirty2Matches ? MergeState.DirtyList2[MergeState.DirtyListIndex2++] : -1;

	const int32 OldDirtyListIndex1 = MergeState.DirtyListIndex1;
	const int32 OldDirtyListIndex2 = MergeState.DirtyListIndex2;

	FScriptArray * Array = (FScriptArray *)Data;

	const int32 OldHandle = MergeState.Handle;
	MergeState.Handle = 0;
	MergeDirtyList_AnyArray_r( MergeState, Cmd, Array->Num(), Cmd.ElementSize, CmdIndex, (uint8*)Array->GetData() );
	MergeState.Handle = OldHandle;

	if ( Dirty1Matches )
	{
		check( MergeState.DirtyListIndex1 - OldDirtyListIndex1 <= JumpToIndex1 );
		MergeState.DirtyListIndex1 = OldDirtyListIndex1 + JumpToIndex1;
		check( MergeState.DirtyList1[MergeState.DirtyListIndex1] == 0 );
		MergeState.DirtyListIndex1++;
	}

	if ( Dirty2Matches )
	{
		check( MergeState.DirtyListIndex2 - OldDirtyListIndex2 <= JumpToIndex2 );
		MergeState.DirtyListIndex2 = OldDirtyListIndex2 + JumpToIndex2;
		check( MergeState.DirtyList2[MergeState.DirtyListIndex2] == 0 );
		MergeState.DirtyListIndex2++;
	}

	MergeState.DirtyValid1 = OldDirtyValid1;
	MergeState.DirtyValid2 = OldDirtyValid2;

	// Patch in the jump offset
	MergeState.MergedDirtyList[RememberIndex] = MergeState.MergedDirtyList.Num() - OldMergedListSize;

	// Add the array terminator
	MergeState.MergedDirtyList.Add( 0 );	
}

void FRepLayout::MergeDirtyList_r( FMergeDirtyListState & MergeState, const int32 CmdStart, const int32 CmdEnd, const uint8 * RESTRICT Data ) const
{
	for ( int32 CmdIndex = CmdStart; CmdIndex < CmdEnd; CmdIndex++ )
	{
		const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

		check( Cmd.Type != REPCMD_Return );

		if ( Cmd.Type == REPCMD_DynamicArray )
		{
			MergeDirtyList_DynamicArray_r( MergeState, Cmd, CmdIndex, Data + Cmd.Offset );
			CmdIndex = Cmd.EndCmd - 1;	// Jump past children of this array (-1 for ++ in for loop)
			continue;
		}

		MergeState.Handle++;

		const bool Dirty1Matches = MergeState.DirtyValid1 && MergeState.DirtyList1[MergeState.DirtyListIndex1] == MergeState.Handle;
		const bool Dirty2Matches = MergeState.DirtyValid2 && MergeState.DirtyList2[MergeState.DirtyListIndex2] == MergeState.Handle;

		if ( Dirty1Matches || Dirty2Matches )
		{
			// This will be a new merged dirty entry
			MergeState.MergedDirtyList.Add( MergeState.Handle );

			// Advance matching dirty indices
			if ( Dirty1Matches )
			{
				MergeState.DirtyListIndex1++;
			}

			if ( Dirty2Matches )
			{
				MergeState.DirtyListIndex2++;
			}
		}
	}
}

void FRepLayout::MergeDirtyList( const void * RESTRICT Data, const TArray< uint16 > & Dirty1, const TArray< uint16 > & Dirty2, TArray< uint16 > & MergedDirty ) const
{
	check( Dirty1.Num() > 0 || Dirty2.Num() > 0 );

	FMergeDirtyListState MergeState( Dirty1, Dirty2, MergedDirty );

	MergedDirty.Empty();

	// Even though one of these can be empty, we need to send the single one through, so we can prune it to the current shape of the tree
	MergeState.DirtyValid1 = Dirty1.Num() > 0;
	MergeState.DirtyValid2 = Dirty2.Num() > 0;

	MergeDirtyList_r( MergeState, 0, Cmds.Num() - 1, (const uint8*)Data );

	MergeState.MergedDirtyList.Add( 0 );
}

void FRepLayout::SanityCheckChangeList_DynamicArray_r( 
	const int32				CmdIndex, 
	const uint8 * RESTRICT	Data, 
	TArray< uint16 > &		Changed,
	int32 &					ChangedIndex ) const
{
	const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

	FScriptArray * Array = (FScriptArray *)Data;

	// Read the jump offset
	// We won't need to actually jump over anything because we expect the change list to be pruned once we get here
	// But we can use it to verify we read the correct amount.
	const int32 ArrayChangedCount = Changed[ChangedIndex++];

	const int32 OldChangedIndex = ChangedIndex;

	Data = (uint8*)Array->GetData();

	uint16 LocalHandle = 0;

	for ( int32 i = 0; i < Array->Num(); i++ )
	{
		LocalHandle = SanityCheckChangeList_r( CmdIndex + 1, Cmd.EndCmd - 1, Data + i * Cmd.ElementSize, Changed, ChangedIndex, LocalHandle );
	}

	check( ChangedIndex - OldChangedIndex == ArrayChangedCount );	// Make sure we read correct amount
	check( Changed[ChangedIndex] == 0 );							// Make sure we are at the end

	ChangedIndex++;
}

uint16 FRepLayout::SanityCheckChangeList_r( 
	const int32				CmdStart, 
	const int32				CmdEnd, 
	const uint8 * RESTRICT	Data, 
	TArray< uint16 > &		Changed,
	int32 &					ChangedIndex,
	uint16					Handle 
	) const
{
	for ( int32 CmdIndex = CmdStart; CmdIndex < CmdEnd; CmdIndex++ )
	{
		const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

		check( Cmd.Type != REPCMD_Return );

		Handle++;

		if ( Cmd.Type == REPCMD_DynamicArray )
		{
			if ( Handle == Changed[ChangedIndex] )
			{
				const int32 LastChangedArrayHandle = Changed[ChangedIndex];
				ChangedIndex++;
				SanityCheckChangeList_DynamicArray_r( CmdIndex, Data + Cmd.Offset, Changed, ChangedIndex );
				check( Changed[ChangedIndex] == 0 || Changed[ChangedIndex] > LastChangedArrayHandle );
			}
			CmdIndex = Cmd.EndCmd - 1;	// Jump past children of this array (the -1 because of the ++ in the for loop)
			continue;
		}

		if ( Handle == Changed[ChangedIndex] )
		{
			const int32 LastChangedArrayHandle = Changed[ChangedIndex];
			ChangedIndex++;
			check( Changed[ChangedIndex] == 0 || Changed[ChangedIndex] > LastChangedArrayHandle );
		}
	}

	return Handle;
}

void FRepLayout::SanityCheckChangeList( const uint8 * RESTRICT Data, TArray< uint16 > & Changed ) const
{
	int32 ChangedIndex = 0;
	SanityCheckChangeList_r( 0, Cmds.Num() - 1, Data, Changed, ChangedIndex, 0 );
	check( Changed[ChangedIndex] == 0 );
}

void FRepLayout::DiffProperties_DynamicArray_r( 
	FRepState *				RepState,
	const int32				CmdIndex, 
	const uint8 * RESTRICT	StoredData, 
	const uint8 * RESTRICT	Data,
	const bool				bSync,
	bool &					bOutDifferent ) const
{
	const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

	FScriptArray * Array = (FScriptArray *)Data;
	FScriptArray * StoredArray = (FScriptArray *)StoredData;

	if ( Array->Num() != StoredArray->Num() )
	{
		bOutDifferent = true;

		if ( !bSync )
		{			
			UE_LOG( LogNet, Warning, TEXT( "DiffProperties_DynamicArray_r: Array sizes different: %s %i / %i" ), *Cmd.Property->GetFullName(), Array->Num(), StoredArray->Num() );
			return;
		}

		if ( !( Parents[Cmd.ParentIndex].Flags & PARENT_IsLifetime ) )
		{
			// Currently, only lifetime properties init from their defaults
			return;
		}

		// Make the shadow state match the actual state
		FScriptArrayHelper StoredArrayHelper( (UArrayProperty *)Cmd.Property, StoredData );
		StoredArrayHelper.Resize( Array->Num() );
	}

	Data		= (uint8*)Array->GetData();
	StoredData	= (uint8*)StoredArray->GetData();

	for ( int32 i = 0; i < Array->Num(); i++ )
	{
		const int32 ElementOffset = i * Cmd.ElementSize;
		DiffProperties_r( RepState, CmdIndex + 1, Cmd.EndCmd - 1, StoredData + ElementOffset, Data + ElementOffset, bSync, bOutDifferent );
	}
}

void FRepLayout::DiffProperties_r( 
	FRepState *				RepState,
	const int32				CmdStart, 
	const int32				CmdEnd, 
	const uint8 * RESTRICT	StoredData, 
	const uint8 * RESTRICT	Data,
	const bool				bSync,
	bool &					bOutDifferent ) const
{
	for ( int32 CmdIndex = CmdStart; CmdIndex < CmdEnd; CmdIndex++ )
	{
		const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

		check( Cmd.Type != REPCMD_Return );

		if ( Cmd.Type == REPCMD_DynamicArray )
		{
			DiffProperties_DynamicArray_r( RepState, CmdIndex, StoredData + Cmd.Offset, Data + Cmd.Offset, bSync, bOutDifferent );
			CmdIndex = Cmd.EndCmd - 1;	// Jump past children of this array (-1 for the ++ in the for loop)
			continue;
		}

		const FRepParentCmd & Parent = Parents[Cmd.ParentIndex];

		const FRepLayoutCmd & SwappedCmd = Cmd;//Parent.RoleSwapIndex != -1 ? Cmds[Parents[Parent.RoleSwapIndex].CmdStart] : Cmd;

		// Make the shadow state match the actual state at the time of send
		if ( !PropertiesAreIdentical( Cmd, (const void*)( Data + SwappedCmd.Offset ), (const void*)( StoredData + Cmd.Offset ) ) )
		{
			bOutDifferent = true;

			if ( !bSync )
			{			
				UE_LOG( LogNet, Warning, TEXT( "DiffProperties_r: Property different: %s" ), *Cmd.Property->GetFullName() );
				continue;
			}

			if ( !( Parents[Cmd.ParentIndex].Flags & PARENT_IsLifetime ) )
			{
				// Currently, only lifetime properties init from their defaults
				continue;
			}

			StoreProperty( Cmd, (void*)( Data + SwappedCmd.Offset ), (const void*)( StoredData + Cmd.Offset ) );

			if ( Parent.Property->HasAnyPropertyFlags( CPF_RepNotify ) )
			{
				RepState->RepNotifies.AddUnique( Parent.Property );
			}
		}
	}
}

bool FRepLayout::DiffProperties( FRepState * RepState, const void * RESTRICT Data, const bool bSync ) const
{
	bool bDifferent = false;
	DiffProperties_r( RepState, 0, Cmds.Num() - 1, RepState->StaticBuffer.GetTypedData(), (uint8*)Data, bSync, bDifferent );
	return bDifferent;
}

void FRepLayout::AddPropertyCmd( UProperty * Property, int32 Offset, int32 RelativeHandle, int32 ParentIndex )
{
	const int32 Index = Cmds.AddZeroed();

	FRepLayoutCmd & Cmd = Cmds[Index];

	Cmd.Property		= Property;
	Cmd.Type			= REPCMD_Property;		// Initially set to generic type
	Cmd.Offset			= Offset;
	Cmd.ElementSize		= Property->ElementSize;
	Cmd.RelativeHandle	= RelativeHandle;
	Cmd.ParentIndex		= ParentIndex;

	// Try to special case to custom types we know about
	if ( Property->IsA( UStructProperty::StaticClass() ) )
	{
		UStructProperty * StructProp = Cast< UStructProperty >( Property );
		UScriptStruct * Struct = StructProp->Struct;
		if ( Struct->GetFName() == NAME_Vector )
		{
			Cmd.Type = REPCMD_PropertyVector;
		}
		else if ( Struct->GetFName() == NAME_Rotator )
		{
			Cmd.Type = REPCMD_PropertyRotator;
		}
		else if ( Struct->GetFName() == NAME_Plane )
		{
			Cmd.Type = REPCMD_PropertyPlane;
		}
		else if ( Struct->GetName() == TEXT( "Vector_NetQuantize100" ) )
		{
			Cmd.Type = REPCMD_PropertyVector100;
		}
		else if ( Struct->GetName() == TEXT( "Vector_NetQuantize10" ) )
		{
			Cmd.Type = REPCMD_PropertyVector10;
		}
		else if ( Struct->GetName() == TEXT( "Vector_NetQuantizeNormal" ) )
		{
			Cmd.Type = REPCMD_PropertyVectorNormal;
		}
		else if ( Struct->GetName() == TEXT( "Vector_NetQuantize" ) )
		{
			Cmd.Type = REPCMD_PropertyVectorQ;
		}
		else if ( Struct->GetName() == TEXT( "UniqueNetIdRepl" ) )
		{
			Cmd.Type = REPCMD_PropertyNetId;
		}
		else if ( Struct->GetName() == TEXT( "RepMovement" ) )
		{
			Cmd.Type = REPCMD_RepMovement;
		}
		else
		{
			UE_LOG( LogNet, Warning, TEXT( "AddPropertyCmd: Unsupported object type! [%s]" ), *Cmd.Property->GetFullName() );
		}
	}
	else if ( Property->IsA( UBoolProperty::StaticClass() ) )
	{
		Cmd.Type = REPCMD_PropertyBool;
	}
	else if ( Property->IsA( UFloatProperty::StaticClass() ) )
	{
		Cmd.Type = REPCMD_PropertyFloat;
	}
	else if ( Property->IsA( UIntProperty::StaticClass() ) )
	{
		Cmd.Type = REPCMD_PropertyInt;
	}
	else if ( Property->IsA( UByteProperty::StaticClass() ) )
	{
		Cmd.Type = REPCMD_PropertyByte;
	}
	else if ( Property->IsA( UObjectPropertyBase::StaticClass() ) )
	{
		Cmd.Type = REPCMD_PropertyObject;
	}
	else if ( Property->IsA( UNameProperty::StaticClass() ) )
	{
		Cmd.Type = REPCMD_PropertyName;
	}
	else if ( Property->IsA( UUInt32Property::StaticClass() ) )
	{
		Cmd.Type = REPCMD_PropertyUInt32;
	}
	else if ( Property->IsA( UUInt64Property::StaticClass() ) )
	{
		Cmd.Type = REPCMD_PropertyUInt64;
	}
	else if ( Property->IsA( UStrProperty::StaticClass() ) )
	{
		Cmd.Type = REPCMD_PropertyString;
	}
	else
	{
		UE_LOG( LogNet, Warning, TEXT( "AddPropertyCmd: Unsupported object type! [%s]" ), *Cmd.Property->GetFullName() );
	}
}

void FRepLayout::AddArrayCmd( UArrayProperty * Property, int32 Offset, int32 RelativeHandle, int32 ParentIndex )
{
	const int32 Index = Cmds.AddZeroed();

	FRepLayoutCmd & Cmd = Cmds[Index];

	Cmd.Type			= REPCMD_DynamicArray;
	Cmd.Property		= Property;
	Cmd.Offset			= Offset;
	Cmd.ElementSize		= Property->Inner->ElementSize;
	Cmd.RelativeHandle	= RelativeHandle;
	Cmd.ParentIndex		= ParentIndex;
}

void FRepLayout::AddReturnCmd()
{
	const int32 Index = Cmds.AddZeroed();
	
	FRepLayoutCmd & Cmd = Cmds[Index];

	Cmd.Type = REPCMD_Return;
}

int32 FRepLayout::InitFromProperty_r( UProperty * Property, int32 Offset, int32 RelativeHandle, int32 ParentIndex )
{
	UArrayProperty * ArrayProp = Cast< UArrayProperty >( Property );

	if ( ArrayProp != NULL )
	{
		const int32 CmdStart = Cmds.Num();

		RelativeHandle++;

		AddArrayCmd( ArrayProp, Offset + ArrayProp->GetOffset_ForGC(), RelativeHandle, ParentIndex );

		InitFromProperty_r( ArrayProp->Inner, 0, 0, ParentIndex );
		
		AddReturnCmd();

		Cmds[CmdStart].EndCmd = Cmds.Num();		// Patch in the offset to jump over our array inner elements

		return RelativeHandle;
	}

	UStructProperty * StructProp = Cast< UStructProperty >( Property );

	if ( StructProp != NULL )
	{
		UScriptStruct * Struct = StructProp->Struct;

		if ( Struct->StructFlags & STRUCT_NetDeltaSerializeNative )
		{
			// Custom delta serializers handles outside of FRepLayout
			return RelativeHandle;
		}

		if ( Struct->StructFlags & STRUCT_NetSerializeNative )
		{
			RelativeHandle++;
			AddPropertyCmd( Property, Offset + Property->GetOffset_ForGC(), RelativeHandle, ParentIndex );
			return RelativeHandle;
		}

		TArray< UProperty * > NetProperties;		// Track properties so me can ensure they are sorted by offsets at the end

		for ( TFieldIterator<UProperty> It( Struct ); It; ++It )
		{
			if ( ( It->PropertyFlags & CPF_RepSkip ) )
			{
				continue;
			}

			NetProperties.Add( *It );
		}

		// Sort NetProperties by memory offset
		struct FCompareUFieldOffsets
		{
			FORCEINLINE bool operator()( UProperty & A, UProperty & B ) const
			{
				return A.GetOffset_ForGC() < B.GetOffset_ForGC();
			}
		};

		Sort( NetProperties.GetTypedData(), NetProperties.Num(), FCompareUFieldOffsets() );

		for ( int32 i = 0; i < NetProperties.Num(); i++ )
		{
			for ( int32 j = 0; j < NetProperties[i]->ArrayDim; j++ )
			{
				RelativeHandle = InitFromProperty_r( NetProperties[i], Offset + StructProp->GetOffset_ForGC() + j * NetProperties[i]->ElementSize, RelativeHandle, ParentIndex );
			}
		}
		return RelativeHandle;
	}

	// Add actual property
	RelativeHandle++;

	AddPropertyCmd( Property, Offset + Property->GetOffset_ForGC(), RelativeHandle, ParentIndex );

	return RelativeHandle;
}

uint16 FRepLayout::AddParentProperty( UProperty * Property, int32 ArrayIndex )
{
	return Parents.Add( FRepParentCmd( Property, ArrayIndex ) );
}

static bool IsCustomDeltaProperty( UProperty * Property )
{
	UStructProperty * StructProperty = Cast< UStructProperty >( Property );

	if ( StructProperty != NULL && StructProperty->Struct->StructFlags & STRUCT_NetDeltaSerializeNative )
	{
		return true;
	}

	return false;
}

void FRepLayout::InitFromObjectClass( UClass * InObjectClass )
{
	RoleIndex				= -1;
	RemoteRoleIndex			= -1;
	FirstNonCustomParent	= -1;

	int32 RelativeHandle	= 0;
	int32 LastOffset		= -1;

	Parents.Empty();

	for ( int32 i = 0; i < InObjectClass->ClassReps.Num(); i++ )
	{
		UProperty * Property	= InObjectClass->ClassReps[i].Property;
		const int32 ArrayIdx	= InObjectClass->ClassReps[i].Index;

		check( Property->PropertyFlags & CPF_Net );

		const int32 ParentHandle = AddParentProperty( Property, ArrayIdx );

		check( ParentHandle == i );
		check( Parents[i].Property->RepIndex + Parents[i].ArrayIndex == i );

		Parents[ParentHandle].CmdStart = Cmds.Num();
		RelativeHandle = InitFromProperty_r( Property, Property->ElementSize * ArrayIdx, RelativeHandle, ParentHandle );
		Parents[ParentHandle].CmdEnd = Cmds.Num();
		Parents[ParentHandle].Flags |= PARENT_IsConditional;

		if ( Parents[i].CmdEnd > Parents[i].CmdStart )
		{
			check( Cmds[Parents[i].CmdStart].Offset >= LastOffset );		// >= since bool's can be combined
			LastOffset = Cmds[Parents[i].CmdStart].Offset;
		}

		// Setup flags
		if ( IsCustomDeltaProperty( Property ) )
		{
			Parents[ParentHandle].Flags |= PARENT_IsCustomDelta;
		}

		if ( Property->GetPropertyFlags() & CPF_Config )
		{
			Parents[ParentHandle].Flags |= PARENT_IsConfig;
		}

		// Hijack the first non custom property for identifying this as a rep layout block
		if ( FirstNonCustomParent == -1 && Property->ArrayDim == 1 && ( Parents[ParentHandle].Flags & PARENT_IsCustomDelta ) == 0 )
		{
			FirstNonCustomParent = ParentHandle;
		}

		// Find Role/RemoteRole property indexes so we can swap them on the client
		if ( Property->GetFName() == NAME_Role )
		{
			check( RoleIndex == -1 );
			check( Parents[ParentHandle].CmdEnd == Parents[ParentHandle].CmdStart + 1 );
			RoleIndex = ParentHandle;
		}

		if ( Property->GetFName() == NAME_RemoteRole )
		{
			check( RemoteRoleIndex == -1 );
			check( Parents[ParentHandle].CmdEnd == Parents[ParentHandle].CmdStart + 1 );
			RemoteRoleIndex = ParentHandle;
		}
	}

	// Make sure it either found both, or didn't find either
	check( ( RoleIndex == -1 ) == ( RemoteRoleIndex == -1 ) );

	// This is so the receiving side can swap these as it receives them
	if ( RoleIndex != -1 )
	{
		Parents[RoleIndex].RoleSwapIndex = RemoteRoleIndex;
		Parents[RemoteRoleIndex].RoleSwapIndex = RoleIndex;
	}
	
	AddReturnCmd();

	// Initialize lifetime props
	TArray< FLifetimeProperty >	LifetimeProps;			// Properties that replicate for the lifetime of the channel

	UObject * Object = InObjectClass->GetDefaultObject();

	Object->GetLifetimeReplicatedProps( LifetimeProps );

	if ( LifetimeProps.Num() > 0 )
	{
		UnconditionalLifetime.Empty();
		ConditionalLifetime.Empty();

		// Fix Lifetime props to have the proper index to the parent
		for ( int32 i = 0; i < LifetimeProps.Num(); i++ )
		{
			if ( Parents[LifetimeProps[i].RepIndex].Flags & PARENT_IsCustomDelta )
			{
				continue;		// We don't handle custom properties in the FRepLayout class
			}

			Parents[LifetimeProps[i].RepIndex].Flags |= PARENT_IsLifetime;

			if ( LifetimeProps[i].RepIndex == RemoteRoleIndex )
			{
				// We handle remote role specially, since it can change between connections when downgraded
				// So we force it on the conditional list
				check( LifetimeProps[i].Condition == COND_None );
				LifetimeProps[i].Condition = COND_Custom;
				ConditionalLifetime.AddUnique( LifetimeProps[i] );
				continue;		
			}

			if ( LifetimeProps[i].Condition == COND_None )
			{
				// These properties are simple, and can benefit from property compare sharing
				UnconditionalLifetime.AddUnique( LifetimeProps[i].RepIndex );
				Parents[LifetimeProps[i].RepIndex].Flags &= ~PARENT_IsConditional;
				continue;
			}
			
			// The rest are conditional
			// These properties are eventually conditionally copied to the RepState
			ConditionalLifetime.AddUnique( LifetimeProps[i] );
		}
	}

	Owner = InObjectClass;
}

void FRepLayout::InitFromFunction( UFunction * InFunction )
{
	int32 RelativeHandle = 0;

	for ( TFieldIterator<UProperty> It( InFunction ); It && ( It->PropertyFlags & ( CPF_Parm | CPF_ReturnParm ) ) == CPF_Parm; ++It )
	{
		for ( int32 ArrayIdx = 0; ArrayIdx < It->ArrayDim; ++ArrayIdx )
		{
			const int32 ParentHandle = AddParentProperty( *It, ArrayIdx );
			Parents[ParentHandle].CmdStart = Cmds.Num();
			RelativeHandle = InitFromProperty_r( *It, It->ElementSize * ArrayIdx, RelativeHandle, ParentHandle );
			Parents[ParentHandle].CmdEnd = Cmds.Num();
		}
	}

	Owner = InFunction;
}

void FRepLayout::InitFromStruct( UStruct * InStruct )
{
	int32 RelativeHandle = 0;

	for ( TFieldIterator<UProperty> It( InStruct ); It; ++It )
	{
		if ( It->PropertyFlags & CPF_RepSkip )
		{
			continue;
		}
			
		for ( int32 ArrayIdx = 0; ArrayIdx < It->ArrayDim; ++ArrayIdx )
		{
			const int32 ParentHandle = AddParentProperty( *It, ArrayIdx );
			Parents[ParentHandle].CmdStart = Cmds.Num();
			RelativeHandle = InitFromProperty_r( *It, It->ElementSize * ArrayIdx, RelativeHandle, ParentHandle );
			Parents[ParentHandle].CmdEnd = Cmds.Num();
		}
	}

	Owner = InStruct;
}

void FRepLayout::SerializeProperties_DynamicArray_r( 
	FArchive &			Ar, 
	UPackageMap	*		Map,
	const int32			CmdIndex,
	uint8 *				Data,
	bool &				bHasUnmapped ) const
{
	const FRepLayoutCmd & Cmd = Cmds[ CmdIndex ];

	FScriptArray * Array = (FScriptArray *)Data;

	// Read array num
	uint16 ArrayNum = Array->Num();
	Ar << ArrayNum;

	const int MAX_ARRAY_SIZE = 2048;

	if ( ArrayNum > MAX_ARRAY_SIZE )
	{
		UE_LOG( LogNetTraffic, Error, TEXT( "SerializeProperties_DynamicArray_r: ArrayNum > MAX_ARRAY_SIZE (%s)" ), *Cmd.Property->GetName() );
		Ar.SetError();
		return;
	}

	if ( Ar.IsLoading() && ArrayNum != Array->Num() )
	{
		// If we are reading, size the array to the incoming value
		FScriptArrayHelper ArrayHelper( (UArrayProperty *)Cmd.Property, Data );
		ArrayHelper.Resize( ArrayNum );
	}

	Data = (uint8*)Array->GetData();

	for ( int32 i = 0; i < Array->Num() && !Ar.IsError(); i++ )
	{
		SerializeProperties_r( Ar, Map, CmdIndex + 1, Cmd.EndCmd - 1, Data + i * Cmd.ElementSize, bHasUnmapped );
	}
}

void FRepLayout::SerializeProperties_r( 
	FArchive &			Ar, 
	UPackageMap	*		Map,
	const int32			CmdStart, 
	const int32			CmdEnd,
	void *				Data,
	bool &				bHasUnmapped ) const
{
	for ( int32 CmdIndex = CmdStart; CmdIndex < CmdEnd && !Ar.IsError(); CmdIndex++ )
	{
		const FRepLayoutCmd & Cmd = Cmds[CmdIndex];

		check( Cmd.Type != REPCMD_Return );

		if ( Cmd.Type == REPCMD_DynamicArray )
		{
			SerializeProperties_DynamicArray_r( Ar, Map, CmdIndex, (uint8*)Data + Cmd.Offset, bHasUnmapped );
			CmdIndex = Cmd.EndCmd;
			continue;
		}

		Map->ResetHasSerializedCDO();
		
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDoReplicationContextString->GetInt() > 0)
		{
			Map->SetDebugContextString( FString::Printf(TEXT("%s - %s"), *Owner->GetPathName(), *Cmd.Property->GetPathName() ) );
		}
#endif

		if ( !Cmd.Property->NetSerializeItem( Ar, Map, (void*)( (uint8*)Data + Cmd.Offset ) ) )
		{
			bHasUnmapped = true;
		}

		if ( Map->HasSerializedCDO() && Ar.IsLoading() )
		{
			UE_LOG( LogNetTraffic, Warning, TEXT( "CDO serialized. Parameter %s." ), *Cmd.Property->GetName() );
			Ar.SetError();
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVarDoReplicationContextString->GetInt() > 0)
		{
			Map->ClearDebugContextString();
		}
#endif

	}
}

void FRepLayout::SendPropertiesForRPC( UObject * Object, UFunction * Function, UActorChannel * Channel, FNetBitWriter & Writer, void * Data ) const
{
	check( Function == Owner );

	for ( int32 i = 0; i < Parents.Num(); i++ )
	{
		if ( !Writer.PackageMap->SupportsObject( Parents[i].Property ) )
		{
			continue;
		}

		bool Send = true;

		if ( !Cast<UBoolProperty>( Parents[i].Property ) )
		{
			// check for a complete match, including arrays
			// (we're comparing against zero data here, since 
			// that's the default.)
			Send = false;

			if ( !Parents[i].Property->Identical_InContainer( Data, NULL, Parents[i].ArrayIndex ) )
			{
				Send = true;
			}

			Writer.WriteBit( Send ? 1 : 0 );
		}

		if ( Send )
		{
			bool bHasUnmapped = false;

			SerializeProperties_r( Writer, Writer.PackageMap, Parents[i].CmdStart, Parents[i].CmdEnd, Data, bHasUnmapped );

			if ( bHasUnmapped )
			{
				// RPC function is sending an unmapped object...
				UE_LOG( LogNetTraffic, Log, TEXT( "Actor[%d] %s RPC %s parameter %s was sent while unmapped! This call may not be correctly handled on the receiving end." ),
					Channel->ChIndex, *Object->GetName(), *Function->GetName(), *Parents[i].Property->GetName() );
			}
		}
	}
}

void FRepLayout::ReceivePropertiesForRPC( UObject * Object, UFunction * Function, UActorChannel * Channel, FNetBitReader & Reader, void * Data ) const
{
	check( Function == Owner );

	for ( int32 i = 0; i < Parents.Num(); i++ )
	{
		if ( !Reader.PackageMap->SupportsObject( Parents[i].Property ) )
		{
			continue;
		}

		if ( Cast<UBoolProperty>( Parents[i].Property ) || Reader.ReadBit() ) 
		{
			bool bHasUnmapped = false;

			SerializeProperties_r( Reader, Reader.PackageMap, Parents[i].CmdStart, Parents[i].CmdEnd, Data, bHasUnmapped );

			if ( Reader.IsError() )
			{
				return;
			}
			
			if ( bHasUnmapped )
			{
				UE_LOG( LogNetTraffic, Log, TEXT( "Unable to resolve RPC parameter. Object[%d] %s. Function %s. Parameter %s." ), 
					Channel->ChIndex, *Object->GetName(), *Function->GetName(), *Parents[i].Property->GetName() );
			}
		}
	}
}

void FRepLayout::SerializePropertiesForStruct( UStruct * Struct, FArchive & Ar, UPackageMap	* Map, void * Data, bool & bHasUnmapped ) const
{
	check( Struct == Owner );

	for ( int32 i = 0; i < Parents.Num(); i++ )
	{
		if ( Map != NULL && !Map->SupportsObject( Parents[i].Property ) )
		{
			continue;
		}

		SerializeProperties_r( Ar, Map, Parents[i].CmdStart, Parents[i].CmdEnd, Data, bHasUnmapped );

		if ( Ar.IsError() )
		{
			return;
		}
	}
}

void FRepLayout::RebuildConditionalProperties( FRepState * RESTRICT	RepState, const FRepChangedPropertyTracker & ChangedTracker, const FReplicationFlags & RepFlags ) const
{
	SCOPE_CYCLE_COUNTER( STAT_NetRebuildConditionalTime );

	//UE_LOG( LogNet, Warning, TEXT( "Rebuilding custom properties [%s]" ), *Owner->GetName() );
	
	// Setup condition map
	bool ConditionMap[COND_Max];

	const bool bIsInitial	= RepFlags.bNetInitial		? true : false;
	const bool bIsOwner		= RepFlags.bNetOwner		? true : false;
	const bool bIsSimulated	= RepFlags.bNetSimulated	? true : false;
	const bool bIsPhysics	= RepFlags.bRepPhysics		? true : false;

	ConditionMap[COND_None]					= true;
	ConditionMap[COND_InitialOnly]			= bIsInitial;

	ConditionMap[COND_OwnerOnly]			= bIsOwner;
	ConditionMap[COND_SkipOwner]			= !bIsOwner;

	ConditionMap[COND_SimulatedOnly]		= bIsSimulated;
	ConditionMap[COND_AutonomousOnly]		= !bIsSimulated;

	ConditionMap[COND_SimulatedOrPhysics]	= bIsSimulated || bIsPhysics;

	ConditionMap[COND_InitialOrOwner]		= bIsInitial || bIsOwner;

	ConditionMap[COND_Custom]				= true;

	RepState->ConditionalLifetime.Empty();

	for ( int32 i = 0; i < ConditionalLifetime.Num(); i++ )
	{
		if ( !ConditionMap[ConditionalLifetime[i].Condition] )
		{
			continue;
		}
		
		if ( !ChangedTracker.Parents[ConditionalLifetime[i].RepIndex].Active )
		{
			continue;
		}

		RepState->ConditionalLifetime.Add( ConditionalLifetime[i].RepIndex );
	}

	RepState->RepFlags = RepFlags;
}

void FRepLayout::InitChangedTracker( FRepChangedPropertyTracker * ChangedTracker ) const
{
	ChangedTracker->Parents.SetNum( Parents.Num() );

	for ( int32 i = 0; i < Parents.Num(); i++ )
	{
		ChangedTracker->Parents[i].IsConditional = ( Parents[i].Flags & PARENT_IsConditional ) ? 1 : 0;
	}
}

void FRepLayout::InitRepState( 
	FRepState *									RepState,
	UClass *									InObjectClass, 
	uint8 *										Src, 
	TSharedPtr< FRepChangedPropertyTracker > &	InRepChangedPropertyTracker ) const
{
	RepState->StaticBuffer.Empty();
	RepState->StaticBuffer.AddZeroed( InObjectClass->GetDefaultsCount() );

	// Construct the properties
	ConstructProperties( RepState );

	// Init the properties
	InitProperties( RepState, Src );
	
	RepState->RepChangedPropertyTracker = InRepChangedPropertyTracker;

	check( RepState->RepChangedPropertyTracker->Parents.Num() == Parents.Num() );

	// Start out the conditional props based on a default RepFlags struct
	// It will rebuild if it ever changes
	RebuildConditionalProperties( RepState, *InRepChangedPropertyTracker.Get(), FReplicationFlags() );
}

void FRepLayout::ConstructProperties( FRepState * RepState ) const
{
	uint8 * StoredData = RepState->StaticBuffer.GetTypedData();

	// Construct all items
	for ( int32 i = 0; i < Parents.Num(); i++ )
	{
		// Only construct the 0th element of static arrays (InitializeValue will handle the elements)
		if ( Parents[i].ArrayIndex == 0 )
		{
			PTRINT Offset = Parents[i].Property->ContainerPtrToValuePtr<uint8>( StoredData ) - StoredData;
			check( Offset >= 0 && Offset < RepState->StaticBuffer.Num() );

			Parents[i].Property->InitializeValue( StoredData + Offset );
		}
	}
}

void FRepLayout::InitProperties( FRepState * RepState, uint8 * Src ) const
{
	uint8 * StoredData = RepState->StaticBuffer.GetTypedData();

	// Init all items
	for ( int32 i = 0; i < Parents.Num(); i++ )
	{
		// Only copy the 0th element of static arrays (CopyCompleteValue will handle the elements)
		if ( Parents[i].ArrayIndex == 0 )
		{
			PTRINT Offset = Parents[i].Property->ContainerPtrToValuePtr<uint8>( StoredData ) - StoredData;
			check( Offset >= 0 && Offset < RepState->StaticBuffer.Num() );

			Parents[i].Property->CopyCompleteValue( StoredData + Offset, Src + Offset );
		}
	}
}

void FRepLayout::DestructProperties( FRepState * RepState ) const
{
	uint8 * StoredData = RepState->StaticBuffer.GetTypedData();

	// Destruct all items
	for ( int32 i = 0; i < Parents.Num(); i++ )
	{
		// Only copy the 0th element of static arrays (DestroyValue will handle the elements)
		if ( Parents[i].ArrayIndex == 0 )
		{
			PTRINT Offset = Parents[i].Property->ContainerPtrToValuePtr<uint8>( StoredData ) - StoredData;
			check( Offset >= 0 && Offset < RepState->StaticBuffer.Num() );

			Parents[i].Property->DestroyValue( StoredData + Offset );
		}
	}
}

void FRepLayout::GetLifetimeCustomDeltaProperties( TArray< int32 > & OutCustom )
{
	OutCustom.Empty();

	for ( int32 i = 0; i < Parents.Num(); i++ )
	{
		if ( Parents[i].Flags & PARENT_IsCustomDelta )
		{
			check( Parents[i].Property->RepIndex + Parents[i].ArrayIndex == i );

			OutCustom.AddUnique( i );
		}
	}
}

FRepState::~FRepState()
{
	if (RepLayout.IsValid() && StaticBuffer.Num() > 0)
	{	
		RepLayout->DestructProperties( this );
	}
}
