// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsPlatformAtomics.cpp: Windows implementations of atomic functions
=============================================================================*/

#include "CorePrivate.h"

void FWindowsPlatformAtomics::HandleAtomicsFailure( const TCHAR* InFormat, ... )
{	
	TCHAR TempStr[1024];
	va_list Ptr;

	va_start( Ptr, InFormat );	
	FCString::GetVarArgs( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr) - 1, InFormat, Ptr );
	va_end( Ptr );

	UE_LOG(LogWindows, Log,  TempStr );
	check( 0 );
}
