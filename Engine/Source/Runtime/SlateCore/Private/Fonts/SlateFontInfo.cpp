// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"


/* FSlateFontInfo structors
 *****************************************************************************/

FSlateFontInfo::FSlateFontInfo( )
	: FontName(NAME_None)
	, Size(0)
	, Hinting(EFontHinting::Default)
{ }


FSlateFontInfo::FSlateFontInfo( const FString& InFontName, uint16 InSize, EFontHinting InHinting )
	: FontName(*InFontName)
	, Size(InSize)
	, Hinting(InHinting)
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}


FSlateFontInfo::FSlateFontInfo( const FName& InFontName, uint16 InSize, EFontHinting InHinting )
	: FontName(InFontName)
	, Size(InSize)
	, Hinting(InHinting)
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}


FSlateFontInfo::FSlateFontInfo( const ANSICHAR* InFontName, uint16 InSize, EFontHinting InHinting )
	: FontName(InFontName)
	, Size(InSize)
	, Hinting(InHinting)
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}


FSlateFontInfo::FSlateFontInfo( const WIDECHAR* InFontName, uint16 InSize, EFontHinting InHinting )
	: FontName(InFontName)
	, Size(InSize)
	, Hinting(InHinting)
{
	//Useful for debugging style breakages
	//check( FPaths::FileExists( FontName.ToString() ) );
}
