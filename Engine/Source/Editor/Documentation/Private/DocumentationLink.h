// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class FDocumentationLink
{
public: 

	static FString GetUrlRoot()
	{
		FString Url;
		FUnrealEdMisc::Get().GetURL( TEXT("UDNDocsURL"), Url, true );

		if ( !Url.EndsWith( TEXT( "/" ) ) )
		{
			Url += TEXT( "/" );
		}

		return Url;
	}

	static FString GetHomeUrl()
	{
		FString Url;
		FUnrealEdMisc::Get().GetURL( TEXT("UDNURL"), Url, true );

		Url.ReplaceInline( TEXT( "/INT/" ), *FString::Printf( TEXT( "/%s/" ), *(FInternationalization::GetCurrentCulture()->GetUnrealLegacyThreeLetterISOLanguageName()) ) );
		return Url;
	}

	static FString ToUrl( const FString& Link )
	{
		FString Path;
		FString Anchor;
		SplitLink( Link, Path, Anchor );

		const FString PartialPath = FString::Printf( TEXT( "%s%s/index.html" ), *(FInternationalization::GetCurrentCulture()->GetUnrealLegacyThreeLetterISOLanguageName()), *Path );
		
		return GetUrlRoot() + PartialPath + Anchor;
	}

	static FString ToFilePath( const FString& Link )
	{
		FString Path;
		FString Anchor;
		SplitLink( Link, Path, Anchor );

		const FString PartialPath = FString::Printf( TEXT( "%s%s/index.html" ), *(FInternationalization::GetCurrentCulture()->GetUnrealLegacyThreeLetterISOLanguageName()), *Path );
		return FString::Printf( TEXT( "%sDocumentation/HTML/%s" ), *FPaths::ConvertRelativePathToFull( FPaths::EngineDir() ), *PartialPath );
	}

	static FString ToFileUrl( const FString& Link )
	{
		FString Path;
		FString Anchor;
		SplitLink( Link, Path, Anchor );

		return FString::Printf( TEXT( "file:///%s%s" ), *ToFilePath( Link ), *Anchor );
	}

	static FString ToSourcePath( const FString& Link, bool MustExist = true )
	{
		FString Path;
		FString Anchor;
		SplitLink( Link, Path, Anchor );

		const FString FullDirectoryPath = FPaths::EngineDir() + TEXT( "Documentation/Source" ) + Path + "/";

		if ( MustExist )
		{
			const FString WildCard = FString::Printf( TEXT( "%s*.%s.udn" ), *FullDirectoryPath, *( FInternationalization::GetCurrentCulture()->GetUnrealLegacyThreeLetterISOLanguageName() ) );

			TArray<FString> Filenames;
			IFileManager::Get().FindFiles( Filenames, *WildCard, true, false );

			if ( Filenames.Num() > 0 )
			{
				return FullDirectoryPath + Filenames[0];
			}

			return FString();
		}
		else
		{
			FString Category = FPaths::GetBaseFilename( Link );
			return FString::Printf( TEXT( "%s%s.%s.udn" ), *FullDirectoryPath, *Category, *( FInternationalization::GetCurrentCulture()->GetUnrealLegacyThreeLetterISOLanguageName() ) );
		}
	}

	static void SplitLink( const FString& Link, /*OUT*/ FString& Path, /*OUT*/ FString& Anchor )
	{
		FString CleanedLink = Link;
		CleanedLink.Trim();
		CleanedLink.TrimTrailing();

		if ( CleanedLink == TEXT("%ROOT%") )
		{
			Path.Empty();
			Anchor.Empty();
		}
		else
		{
			if ( !CleanedLink.Split( TEXT("#"), &Path, &Anchor ) )
			{
				Path = CleanedLink;
			}
			else if ( !Anchor.IsEmpty() )
			{
				Anchor = FString( TEXT("#") ) + Anchor;
			}

			if ( Anchor.EndsWith( TEXT("/") ) )
			{
				Anchor = Anchor.Left( Anchor.Len() - 1 );
			}

			if ( Path.EndsWith( TEXT("/") ) )
			{
				Path = Path.Left( Path.Len() - 1 );
			}

			if ( !Path.IsEmpty() && !Path.StartsWith( TEXT("/") ) )
			{
				Path = FString( TEXT("/") ) + Path;
			}
		}
	}
};
