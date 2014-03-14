// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DocumentationModulePrivatePCH.h"
#include "Documentation.h"
#include "SDocumentationAnchor.h"
#include "UDNParser.h"
#include "DocumentationPage.h"
#include "DocumentationLink.h"
#include "SDocumentationToolTip.h"
#include "IAnalyticsProvider.h"
#include "EngineAnalytics.h"

TSharedRef< IDocumentation > FDocumentation::Create() 
{
	return MakeShareable( new FDocumentation() );
}

FDocumentation::FDocumentation() 
{

}

FDocumentation::~FDocumentation() 
{

}

bool FDocumentation::OpenHome() const
{
	return Open( TEXT("%ROOT%") );
}

bool FDocumentation::OpenAPIHome() const
{
	FString APIPath = FPaths::Combine(*FPaths::EngineDir(), TEXT("Documentation/CHM/API.chm"));
	if( IFileManager::Get().FileSize( *APIPath ) != INDEX_NONE )
	{
		FString AbsoluteAPIPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*APIPath);
		FPlatformProcess::LaunchFileInDefaultExternalApplication(*AbsoluteAPIPath);
		return true;
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("Documentation", "CannotFindAPIReference", "Cannot open API reference; help file not found."));
		return false;
	}
}

bool FDocumentation::Open( const FString& Link ) const
{
	FString DocumentationUrl;

	if( !FParse::Param( FCommandLine::Get(), TEXT("testdocs") ) )
	{
		const FString OnDiskPath = FDocumentationLink::ToFilePath(Link);
		if (IFileManager::Get().FileSize(*OnDiskPath) != INDEX_NONE)
		{
			DocumentationUrl = FDocumentationLink::ToFileUrl(Link);
		}
	}

	if( DocumentationUrl.IsEmpty() )
	{
		DocumentationUrl = FDocumentationLink::ToUrl( Link );
	}

	if( !DocumentationUrl.IsEmpty() )
	{
		FPlatformProcess::LaunchURL( *DocumentationUrl, NULL, NULL );
	}

	if( !DocumentationUrl.IsEmpty() && FEngineAnalytics::IsAvailable() )
	{
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.Documentation"), TEXT("OpenedPage"), Link );
	}
	return !DocumentationUrl.IsEmpty();
}

TSharedRef< SWidget > FDocumentation::CreateAnchor( const FString& Link, const FString& PreviewLink, const FString& PreviewExcerptName ) const
{
	return SNew( SDocumentationAnchor, Link )
		.PreviewLink(PreviewLink)
		.PreviewExcerptName(PreviewExcerptName);
}

TSharedRef< IDocumentationPage > FDocumentation::GetPage( const FString& Link, const TSharedPtr< FParserConfiguration >& Config, const FDocumentationStyle& Style )
{
	TSharedPtr< IDocumentationPage > Page;
	const TWeakPtr< IDocumentationPage >* ExistingPagePtr = LoadedPages.Find( Link );

	if ( ExistingPagePtr != NULL )
	{
		const TSharedPtr< IDocumentationPage > ExistingPage = ExistingPagePtr->Pin();
		if ( ExistingPage.IsValid() )
		{
			Page = ExistingPage;
		}
	}

	if ( !Page.IsValid() )
	{
		Page = FDocumentationPage::Create( Link, FUDNParser::Create( Config, Style ) );
		LoadedPages.Add( Link, TWeakPtr< IDocumentationPage >( Page ) );
	}

	return Page.ToSharedRef();
}

bool FDocumentation::PageExists(const FString& Link) const
{
	const TWeakPtr< IDocumentationPage >* ExistingPagePtr = LoadedPages.Find(Link);
	if (ExistingPagePtr != NULL)
	{
		return true;
	}

	const FString SourcePath = FDocumentationLink::ToSourcePath(Link, true);
	return !SourcePath.IsEmpty();
}

TSharedRef< class SToolTip > FDocumentation::CreateToolTip( const TAttribute<FText>& Text, const TSharedPtr<SWidget>& OverrideContent, const FString& Link, const FString& ExcerptName ) const
{
	TSharedPtr< SDocumentationToolTip > DocToolTip;

	if ( !Text.IsBound() && Text.Get().IsEmpty() )
	{
		return SNew( SToolTip );
	}

	if ( OverrideContent.IsValid() )
	{
		SAssignNew( DocToolTip, SDocumentationToolTip )
		.DocumentationLink( Link )
		.ExcerptName( ExcerptName )
		[
			OverrideContent.ToSharedRef()
		];
	}
	else
	{
		SAssignNew( DocToolTip, SDocumentationToolTip )
		.Text( Text )
		.DocumentationLink( Link )
		.ExcerptName( ExcerptName );
	}
	
	return SNew( SToolTip )
		.IsInteractive( DocToolTip.ToSharedRef(), &SDocumentationToolTip::IsInteractive )
		[
			DocToolTip.ToSharedRef()
		];
}