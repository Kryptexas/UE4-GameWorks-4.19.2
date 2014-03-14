// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PluginsEditorPrivatePCH.h"
#include "SPluginListTile.h"
#include "PluginListItem.h"
#include "SPluginsEditor.h"
#include "SPluginList.h"
#include "PluginStyle.h"

#define LOCTEXT_NAMESPACE "PluginListTile"


void SPluginListTile::Construct( const FArguments& Args, const TSharedRef< SPluginList > Owner, const TSharedRef< class FPluginListItem >& Item )
{
	OwnerWeak = Owner;
	ItemData = Item;

	const float PaddingAmount = FPluginStyle::Get()->GetFloat( "PluginTile.Padding" );
	const float ThumbnailImageSize = FPluginStyle::Get()->GetFloat( "PluginTile.ThumbnailImageSize" );

	// @todo plugedit: Also display whether plugin is editor-only, runtime-only, developer or a combination?
	//		-> Maybe a filter for this too?  (show only editor plugins, etc.)
	// @todo plugedit: Indicate whether plugin has content?  Filter to show only content plugins, and vice-versa?


	// Plugin thumbnail image
	if( !Item->PluginStatus.Icon128FilePath.IsEmpty() )
	{
		const FName BrushName( *Item->PluginStatus.Icon128FilePath );
		const FIntPoint Size = FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource( BrushName );
		if( Size.X > 0 && Size.Y > 0 )
		{
			PluginIconDynamicImageBrush = MakeShareable( new FSlateDynamicImageBrush( BrushName, FVector2D( Size.X, Size.Y ) ) );
		}
	}
	else
	{
		// Plugin is missing a thumbnail image
		// @todo plugedit: Should display default image or just omit the thumbnail
	}

	
	// Setup widget for the 'Created by' text.  Depending on how the user configured their plugin, we may display a plain text label, a hyperlink to
	// the company's web site, or nothing at all!
	TSharedPtr<SWidget> CreatedByWidget;
	if( !Item->PluginStatus.CreatedBy.IsEmpty() && !Item->PluginStatus.CreatedByURL.IsEmpty() )
	{
		struct Local
		{
			static void NavigateToURL( FString URL )
			{
				FPlatformProcess::LaunchURL( *URL, NULL, NULL );
			}
		};

		// Clickable 'Created by' URL text
		CreatedByWidget = SNew( SHyperlink )
			.Text( Item->PluginStatus.CreatedBy )
			.ToolTipText( FText::Format( LOCTEXT("NavigateToCreatedByURL", "Launch a web browser to visit {0}"), FText::FromString( Item->PluginStatus.CreatedByURL ) ).ToString() )
			.OnNavigate_Static( &Local::NavigateToURL, Item->PluginStatus.CreatedByURL );

	}
	else if( !Item->PluginStatus.CreatedBy.IsEmpty() )
	{
		// Plain text for 'Created by' (no URL specified)
		CreatedByWidget = 
			SNew( STextBlock )
			.Text( Item->PluginStatus.CreatedBy );
	}
	else
	{
		// No 'Created by' text was supplied
		CreatedByWidget = SNew( SSpacer );
	}


	ChildSlot.Widget = 
		SNew( SBorder )
		.BorderImage( FEditorStyle::GetBrush( "NoBorder" ) )
		.Padding( PaddingAmount )
		[
			SNew( SBorder )
			.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
			.Padding( PaddingAmount )
			[
				SNew( SHorizontalBox )

				// Thumbnail image
				+SHorizontalBox::Slot()
				.Padding( PaddingAmount )
				.AutoWidth()
				[
					SNew( SBox )
					.WidthOverride( ThumbnailImageSize )
					.HeightOverride( ThumbnailImageSize )
					[
						SNew( SImage )
						.Image( PluginIconDynamicImageBrush.IsValid() ? PluginIconDynamicImageBrush.Get() : NULL )
					]
				]

				+SHorizontalBox::Slot()
				[
					SNew( SVerticalBox )

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew( SHorizontalBox )

						// Friendly name
						+SHorizontalBox::Slot()
						.Padding( PaddingAmount )
						[
							SNew( STextBlock )
							.Text( Item->PluginStatus.FriendlyName )
							.HighlightText_Raw( &Owner->GetOwner().GetPluginTextFilter(), &FPluginTextFilter::GetRawFilterText )
							.TextStyle( FPluginStyle::Get(), "PluginTile.NameText" )
						]

						// Version
						+SHorizontalBox::Slot()
						.HAlign( HAlign_Right )
						.Padding( PaddingAmount )
						.AutoWidth()
						[
							SNew( SHorizontalBox )
							+SHorizontalBox::Slot()
							.AutoWidth()
							.Padding( 0.0f, 0.0f, 0.0f, 1.0f )		// Lower padding to align font with version number base
							[
								SNew( SHorizontalBox )

								// Beta version icon
								+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign( VAlign_Bottom )
								.Padding( 0.0f, 0.0f, 3.0f, 2.0f )
								[
									SNew( SImage )
									.Visibility( Item->PluginStatus.bIsBetaVersion ? EVisibility::Visible : EVisibility::Collapsed )
									.Image( FPluginStyle::Get()->GetBrush("PluginTile.BetaWarning") )
								]
								// Version label
								+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign( VAlign_Bottom )
								[
									SNew( STextBlock )
									.Text( Item->PluginStatus.bIsBetaVersion ?
										LOCTEXT( "PluginBetaVersionLabel", "BETA Version " ) :
										LOCTEXT( "PluginVersionLabel", "Version " ) )
								]
							]

							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign( VAlign_Bottom )
							.Padding( 0.0f, 0.0f, 2.0f, 0.0f )	// Extra padding from the right edge
							[
								SNew( STextBlock )
								.Text( Item->PluginStatus.VersionName )
								.TextStyle( FPluginStyle::Get(), "PluginTile.VersionNumberText" )
							]
						]
					]
			
					+SVerticalBox::Slot()
					[
						SNew( SVerticalBox )
				
						// Description
						+SVerticalBox::Slot()
						.Padding( PaddingAmount )
						[
							SNew( STextBlock )
							.Text( Item->PluginStatus.Description )
							.AutoWrapText( true )
						]

						+SVerticalBox::Slot()
						.Padding( PaddingAmount )
						.AutoHeight()
						[
							SNew( SHorizontalBox )

							// Enable checkbox
							+SHorizontalBox::Slot()
							.Padding( PaddingAmount )
							.AutoWidth()
							[
								SNew(SCheckBox)
								.OnCheckStateChanged(this, &SPluginListTile::OnEnablePluginCheckboxChanged)
								.IsChecked( this, &SPluginListTile::IsPluginEnabled )
								.ToolTipText( LOCTEXT( "EnableDisableButtonToolTip", "Toggles whether this plugin is enabled for your current project.  You may need to restart the program for this change to take effect." ) )
								.Content()
								[
									SNew(STextBlock) .Text( LOCTEXT("EnablePluginCheckbox", "Enabled") )
								]
							]

							// Uninstall button
							+SHorizontalBox::Slot()
							.Padding( PaddingAmount )
							.AutoWidth()
							[
								SNew( SButton )
								.Text( LOCTEXT( "PluginUninstallButtonLabel", "Uninstall..." ) )

								// Don't show 'Uninstall' for built-in plugins
								.Visibility( EVisibility::Collapsed )	
								// @todo plugins: Not supported yet
									// .Visibility( Item->PluginStatus.bIsBuiltIn ? EVisibility::Collapsed : EVisibility::Visible )

								.OnClicked( this, &SPluginListTile::OnUninstallButtonClicked )
								.IsEnabled( false )		// @todo plugedit: Not supported yet
								.ToolTipText( LOCTEXT( "UninstallButtonToolTip", "Allows you to uninstall this plugin.  This will delete the plugin from your project entirely.  You may need to restart the program for this change to take effect." ) )
							]

							// Created by
							+SHorizontalBox::Slot()
							.Padding( PaddingAmount )
							.HAlign( HAlign_Right )
							[
								SNew( SHorizontalBox )

								+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign( VAlign_Bottom )
								.Padding( 0, 0, 4, 0 )		// Extra padding before the URL string
								[
									SNew( STextBlock )
									.Visibility( Item->PluginStatus.CreatedBy.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible )
									.Text( LOCTEXT( "PluginCreatedByLabel", "Created by" ) )
									.Text( Item->PluginStatus.CreatedBy )							
								]

								+SHorizontalBox::Slot()
								.VAlign( VAlign_Bottom )
								.AutoWidth()
								[
									CreatedByWidget.ToSharedRef()
								]
							]
						]
					]
				]
			]
		]
	;
}


ESlateCheckBoxState::Type SPluginListTile::IsPluginEnabled() const
{
	return ItemData->PluginStatus.bIsEnabled ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}


void SPluginListTile::OnEnablePluginCheckboxChanged(ESlateCheckBoxState::Type NewCheckedState)
{
	const bool bNewEnabledState = (NewCheckedState == ESlateCheckBoxState::Checked);

	if (bNewEnabledState && ItemData->PluginStatus.bIsBetaVersion)
	{
		FText WarningMessage = FText::Format(
			LOCTEXT("Warning_EnablingBetaPlugin", "Plugin '{0}' is a beta version and might be unstable or removed without notice. Please use with caution. Are you sure you want to enable the plugin?"),
			FText::FromString(ItemData->PluginStatus.FriendlyName));

		if (EAppReturnType::No == FMessageDialog::Open(EAppMsgType::YesNo, WarningMessage))
		{
			// User chose to keep beta plugin disabled
			return;
		}
	}

	ItemData->PluginStatus.bIsEnabled = bNewEnabledState;
	IPluginManager::Get().SetPluginEnabled(ItemData->PluginStatus.Name, bNewEnabledState);
}


FReply SPluginListTile::OnUninstallButtonClicked()
{
	// Not expecting to deal with engine plugins here.  They can't be unintalled
	if( ensure( !ItemData->PluginStatus.bIsBuiltIn ) )
	{
		// @todo plugedit: Make this work!
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE