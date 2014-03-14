// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DocumentationModulePrivatePCH.h"
#include "SDocumentationToolTip.h"
#include "DocumentationLink.h"

void SDocumentationToolTip::Construct( const FArguments& InArgs )
{
	TextContent = InArgs._Text;
	StyleInfo = FEditorStyle::GetWidgetStyle<FTextBlockStyle>(InArgs._Style);
	ColorAndOpacity = InArgs._ColorAndOpacity;
	DocumentationLink = InArgs._DocumentationLink;
	IsDisplayingDocumentationLink = false;

	if ( !DocumentationLink.IsEmpty() )
	{
		// All in-editor udn documents must live under the Shared/ folder
		ensure( InArgs._DocumentationLink.StartsWith( TEXT("Shared/") ) );
	}

	ExcerptName = InArgs._ExcerptName;
	IsShowingFullTip = false;

	if( InArgs._Content.Widget != SNullWidget::NullWidget )
	{
		// Widget content argument takes precedence
		// overrides the text content.
		OverrideContent = InArgs._Content.Widget;
	}

	ConstructSimpleTipContent();

	ChildSlot
	[
		SAssignNew( WidgetContent, SBorder )
		.BorderImage( FEditorStyle::GetBrush( "NoBorder" ) )
		.Padding( 0 )
		[
			SimpleTipContent.ToSharedRef()
		]
	];
}

void SDocumentationToolTip::ConstructSimpleTipContent()
{
	TSharedPtr< SVerticalBox > VerticalBox;
	if ( !OverrideContent.IsValid() )
	{
		SAssignNew( SimpleTipContent, SBox )
		[
			SAssignNew( VerticalBox, SVerticalBox )
			+SVerticalBox::Slot()
			.FillHeight( 1.0f )
			[
				SNew( STextBlock )
				.Text( TextContent )
				.TextStyle( &StyleInfo )
				.ColorAndOpacity( ColorAndOpacity )
				.WrapTextAt( 400 )
			]
		];
	}
	else
	{
		SAssignNew( SimpleTipContent, SBox )
		[
			SAssignNew( VerticalBox, SVerticalBox )
			+SVerticalBox::Slot()
			.FillHeight( 1.0f )
			[
				SNew( STextBlock )
				.Text( TextContent )
				.TextStyle( &StyleInfo )
				.ColorAndOpacity( ColorAndOpacity )
				.WrapTextAt( 400 )
			]
		];
	}

	if ( !DocumentationLink.IsEmpty() )
	{
		IsDisplayingDocumentationLink = GEditor->EditorUserSettings->bDisplayDocumentationLink;

		if ( GEditor->EditorUserSettings->bDisplayDocumentationLink )
		{
			FString OptionalExcerptName;
			if ( !ExcerptName.IsEmpty() )
			{ 
				OptionalExcerptName = FString( TEXT(" [") ) + ExcerptName + TEXT("]");
			}

			VerticalBox->AddSlot()
				.AutoHeight()
				.HAlign( HAlign_Right )
				[
					SNew( STextBlock )
					.ColorAndOpacity( FSlateColor::UseSubduedForeground() )
					.Text( DocumentationLink + OptionalExcerptName )
					.TextStyle( &StyleInfo )
				];
		}

		if ( !DocumentationPage.IsValid() )
		{
			DocumentationPage = IDocumentation::Get()->GetPage( DocumentationLink, NULL );
		}

		if ( DocumentationPage->HasExcerpt( ExcerptName ) )
		{
			FText MacShortcut = NSLOCTEXT("SToolTip", "MacRichTooltipShortcut", "(Cmd + Alt)");
			FText WinShortcut = NSLOCTEXT("SToolTip", "WinRichTooltipShortcut", "(Ctrl + Alt)");

			FText KeyboardShortcut;
#if PLATFORM_MAC
			KeyboardShortcut = MacShortcut;
#else
			KeyboardShortcut = WinShortcut;
#endif

			VerticalBox->AddSlot()
				.AutoHeight()
				.HAlign( HAlign_Right )
				[
					SNew( STextBlock )
					.ColorAndOpacity( FSlateColor::UseSubduedForeground() )
					.Text( FText::Format( NSLOCTEXT( "SToolTip", "AdvancedToolTipMessage", "hold {0} for more" ), KeyboardShortcut) )
					.TextStyle( &StyleInfo )
				];
		}
		else
		{
			if ( GEditor->EditorUserSettings->bDisplayDocumentationLink && FSlateApplication::Get().SupportsSourceAccess() )
			{
				FString DocPath = FDocumentationLink::ToSourcePath( DocumentationLink, true );
				if ( DocPath.IsEmpty() )
				{
					DocPath = FPaths::ConvertRelativePathToFull( FDocumentationLink::ToSourcePath( DocumentationLink, false ) );
				}

				VerticalBox->AddSlot()
					.AutoHeight()
					.HAlign( HAlign_Right )
					[
						SNew( SHyperlink )
						.Text( NSLOCTEXT( "SToolTip", "EditDocumentationMessage", "create" ) )
						.OnNavigate( this, &SDocumentationToolTip::CreateExcerpt, DocPath, ExcerptName )
					];
			}
		}
	}
}

void SDocumentationToolTip::CreateExcerpt( FString FileSource, FString ExcerptName )
{
	bool NewFile = true;
	if (FPaths::FileExists(FileSource))
	{
		NewFile = false;
	}

	FArchive* FileWriter = IFileManager::Get().CreateFileWriter( *FileSource, EFileWrite::FILEWRITE_Append | EFileWrite::FILEWRITE_AllowRead | EFileWrite::FILEWRITE_EvenIfReadOnly );

	if ( NewFile )
	{
		FString UdnHeader;
		UdnHeader += "Availability:NoPublish";
		UdnHeader += LINE_TERMINATOR;
		UdnHeader += "Title:";
		UdnHeader += LINE_TERMINATOR;
		UdnHeader += "Crumbs:";
		UdnHeader += LINE_TERMINATOR;
		UdnHeader += "Description:";
		UdnHeader += LINE_TERMINATOR;

		FileWriter->Serialize( TCHAR_TO_ANSI( *UdnHeader ), UdnHeader.Len() );
	}

	FString NewExcerpt;
	NewExcerpt += LINE_TERMINATOR;
	NewExcerpt += "[EXCERPT:";
	NewExcerpt += ExcerptName;
	NewExcerpt += "]";
	NewExcerpt += LINE_TERMINATOR;

	NewExcerpt += TextContent.Get().ToString();
	NewExcerpt += LINE_TERMINATOR;

	NewExcerpt += "[/EXCERPT:";
	NewExcerpt += ExcerptName;
	NewExcerpt += "]";
	NewExcerpt += LINE_TERMINATOR;

	if ( !NewFile )
	{
		FileWriter->Seek( FMath::Max( FileWriter->TotalSize(), (int64)0 ) );
	}

	FileWriter->Serialize( TCHAR_TO_ANSI( *NewExcerpt ), NewExcerpt.Len() );

	FileWriter->Close();
	delete FileWriter;

	FSlateApplication::Get().GotoLineInSource( FileSource + "|" + FString::FromInt(0) );

	ReloadDocumentation();
}

void SDocumentationToolTip::ConstructFullTipContent()
{
	TArray< FExcerpt > Excerpts;
	DocumentationPage->GetExcerpts( Excerpts );

	if ( Excerpts.Num() > 0 )
	{
		int32 ExcerptIndex = 0;
		if ( !ExcerptName.IsEmpty() )
		{
			for (int Index = 0; Index < Excerpts.Num(); Index++)
			{
				if ( Excerpts[ Index ].Name == ExcerptName )
				{
					ExcerptIndex = Index;
					break;
				}
			}
		}

		if ( !Excerpts[ ExcerptIndex ].Content.IsValid() )
		{
			DocumentationPage->GetExcerptContent( Excerpts[ ExcerptIndex ] );
		}

		if ( Excerpts[ ExcerptIndex ].Content.IsValid() )
		{
			TSharedPtr< SVerticalBox > Box;
			FullTipContent = 
				SAssignNew( Box, SVerticalBox )
				+SVerticalBox::Slot()
				.HAlign( HAlign_Center )
				.AutoHeight()
				[
					Excerpts[ ExcerptIndex ].Content.ToSharedRef()
				];

			FString* FullDocumentationLink = Excerpts[ ExcerptIndex ].Variables.Find( TEXT("ToolTipFullLink") );
			if ( FullDocumentationLink != NULL && !FullDocumentationLink->IsEmpty() )
			{
				struct Local
				{
					static void OpenLink( FString Link )
					{
						if ( !IDocumentation::Get()->Open( Link ) )
						{
							FNotificationInfo Info( NSLOCTEXT("SToolTip", "FailedToOpenLink", "Failed to Open Link") );
							FSlateNotificationManager::Get().AddNotification(Info);
						}
					}
				};

				Box->AddSlot()
				.HAlign( HAlign_Center )
				.AutoHeight()
				[
					SNew( SHyperlink )
						.Text( NSLOCTEXT( "SToolTip", "GoToFullDocsLinkMessage", "see full documentation" ) )
						.OnNavigate_Static( &Local::OpenLink, *FullDocumentationLink )
				];
			}

			if ( GEditor->EditorUserSettings->bDisplayDocumentationLink && FSlateApplication::Get().SupportsSourceAccess() )
			{
				struct Local
				{
					static void EditSource( FString Link )
					{
						FSlateApplication::Get().GotoLineInSource( Link );
					}
				};

				Box->AddSlot()
				.AutoHeight()
				.HAlign( HAlign_Right )
				[
					SNew( SHyperlink )
						.Text( NSLOCTEXT( "SToolTip", "EditDocumentationMessage", "edit" ) )
						.OnNavigate_Static( &Local::EditSource, FPaths::ConvertRelativePathToFull( FDocumentationLink::ToSourcePath( DocumentationLink ) ) + TEXT("|") + FString::FromInt( Excerpts[ ExcerptIndex ].LineNumber ) )
				];
			}
		}
	}
}

FReply SDocumentationToolTip::ReloadDocumentation()
{
	SimpleTipContent.Reset();
	FullTipContent.Reset();

	ConstructSimpleTipContent();

	if ( DocumentationPage.IsValid() )
	{
		DocumentationPage->Reload();

		if ( DocumentationPage->HasExcerpt( ExcerptName ) )
		{
			ConstructFullTipContent();
		}
	}

	return FReply::Handled();
}

void SDocumentationToolTip::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	const bool NeedsUpdate = IsDisplayingDocumentationLink != GEditor->EditorUserSettings->bDisplayDocumentationLink;

	if ( !IsShowingFullTip && ModifierKeys.IsAltDown() && ModifierKeys.IsControlDown() )
	{
		if ( !FullTipContent.IsValid() && DocumentationPage.IsValid() && DocumentationPage->HasExcerpt( ExcerptName ) )
		{
			ConstructFullTipContent();
		}
		else if ( GEditor->EditorUserSettings->bDisplayDocumentationLink )
		{
			ReloadDocumentation();
		}

		if ( FullTipContent.IsValid() )
		{
			WidgetContent->SetContent( FullTipContent.ToSharedRef() );
			IsShowingFullTip = true;
		}
	}
	else if ( ( IsShowingFullTip || NeedsUpdate )  && ( !ModifierKeys.IsAltDown() || !ModifierKeys.IsControlDown() ) )
	{
		if ( NeedsUpdate )
		{
			ReloadDocumentation();
			IsDisplayingDocumentationLink = GEditor->EditorUserSettings->bDisplayDocumentationLink;
		}

		WidgetContent->SetContent( SimpleTipContent.ToSharedRef() );
		IsShowingFullTip = false;
	}
}

bool SDocumentationToolTip::IsInteractive() const
{
	const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	return ( DocumentationPage.IsValid() && ModifierKeys.IsAltDown() && ModifierKeys.IsControlDown() );
}
