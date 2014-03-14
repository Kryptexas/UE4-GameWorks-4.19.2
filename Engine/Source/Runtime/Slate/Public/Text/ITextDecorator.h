// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

struct SLATE_API FTextRunParseResults
{
	FTextRunParseResults( FString InName, const FTextRange& InOriginalRange)
		: Name( InName )
		, OriginalRange( InOriginalRange )
		, MetaData()
	{

	}

	FTextRunParseResults( FString InName, const FTextRange& InOriginalRange, const FTextRange& InContentRange)
		: Name( InName )
		, OriginalRange( InOriginalRange )
		, ContentRange( InContentRange )
		, MetaData()
	{

	}

	FString Name;
	FTextRange OriginalRange;
	FTextRange ContentRange;
	TMap< FString, FTextRange > MetaData; 
};

struct SLATE_API FTextLineParseResults
{
public:

	FTextLineParseResults()
		: Range( )
		, Runs()
	{

	}

	FTextLineParseResults(const FTextRange& InRange)
		: Range( InRange )
		, Runs()
	{

	}

	FTextRange Range;
	TArray< FTextRunParseResults > Runs;
};

struct SLATE_API FTextRunInfo
{
	FTextRunInfo( FString InName, const FText& InContent )
		: Name( InName )
		, Content( InContent )
		, MetaData()
	{

	}

	FString Name;
	FText Content;
	TMap< FString, FString > MetaData;
};

class SLATE_API ITextDecorator
{
public:

	virtual bool Supports( const FTextRunParseResults& RunInfo, const FString& Text ) const = 0;

	virtual TSharedRef< ISlateRun > Create( const FTextRunParseResults& RunInfo, const FString& OriginalText, const TSharedRef< FString >& ModelText, const ISlateStyle* Style ) = 0;
};