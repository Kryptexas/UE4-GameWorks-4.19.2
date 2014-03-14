// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MatineeModule.h"
#include "MatineeOptions.h"
#include "MatineeTransaction.h"

/*-----------------------------------------------------------------------------
	FMatineeTransaction
-----------------------------------------------------------------------------*/
void FMatineeTransaction::SaveObject( UObject* Object )
{
	check(Object);

	if( Object->IsA( AMatineeActor::StaticClass() ) ||  
		Object->IsA( UInterpData::StaticClass() ) ||
		Object->IsA( UInterpGroup::StaticClass() ) ||
		Object->IsA( UInterpTrack::StaticClass() ) ||
		Object->IsA( UInterpGroupInst::StaticClass() ) ||
		Object->IsA( UInterpTrackInst::StaticClass() ) ||
		Object->IsA( UMatineeOptions::StaticClass() ) )
	{
		// Save the object.
		new( Records )FObjectRecord( this, Object, NULL, 0, 0, 0, 0, NULL, NULL );
	}
}

void FMatineeTransaction::SaveArray( UObject* Object, FScriptArray* Array, int32 Index, int32 Count, int32 Oper, int32 ElementSize, STRUCT_AR Serializer, STRUCT_DTOR Destructor )
{
	// Never want this.
}