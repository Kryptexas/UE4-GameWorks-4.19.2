// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateFontInfo.cpp: Implements the FSlateFontInfo structure.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* FSlateFontInfo structors
 *****************************************************************************/

FSlateFontInfo::FSlateFontInfo( )
	: FontName(NAME_None)
	, Size(0)
{ }


FSlateFontInfo::FSlateFontInfo( const FString& InFontName, uint16 InSize )
	: FontName(*InFontName)
	, Size(InSize)
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}


FSlateFontInfo::FSlateFontInfo( const FName& InFontName, uint16 InSize )
	: FontName(InFontName)
	, Size(InSize)
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}


FSlateFontInfo::FSlateFontInfo( const ANSICHAR* InFontName, uint16 InSize )
	: FontName(InFontName)
	, Size(InSize)
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}


FSlateFontInfo::FSlateFontInfo( const WIDECHAR* InFontName, uint16 InSize )
	: FontName(InFontName)
	, Size(InSize)
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}
