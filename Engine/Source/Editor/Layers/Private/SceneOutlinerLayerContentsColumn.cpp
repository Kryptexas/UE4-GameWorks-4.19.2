// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "LayersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SceneOutlinerLayerContentsColumn"


FSceneOutlinerLayerContentsColumn::FSceneOutlinerLayerContentsColumn( const TSharedRef< FLayerViewModel >& InViewModel )
	: ViewModel( InViewModel )
{

}


FName FSceneOutlinerLayerContentsColumn::GetColumnID()
{
	return FName( "LayerContents" );
}


SHeaderRow::FColumn::FArguments FSceneOutlinerLayerContentsColumn::ConstructHeaderRowColumn()
{
	return SHeaderRow::Column( GetColumnID() )
		.FillWidth( 48.0f )
		[
			SNew( SSpacer )
		];
}


const TSharedRef< SWidget > FSceneOutlinerLayerContentsColumn::ConstructRowWidget( const TWeakObjectPtr< AActor >&  InActor )
{
	return 	SNew( SButton )
		.HAlign( HAlign_Center )
		.VAlign( VAlign_Center )
		.ButtonStyle( FEditorStyle::Get(), "LayerBrowserButton" )
		.ContentPadding( 0 )
		.OnClicked( this, &FSceneOutlinerLayerContentsColumn::OnRemoveFromLayerClicked, InActor )
		.ToolTipText( LOCTEXT("RemoveFromLayerButtonText", "Remove from Layer").ToString() )
		[
			SNew( SImage )
			.Image( FEditorStyle::GetBrush( TEXT( "LayerBrowser.Actor.RemoveFromLayer" ) ) )
		]
	;
}


void FSceneOutlinerLayerContentsColumn::PopulateActorSearchStrings( const AActor* const InActor, OUT TArray< FString >& OutSearchStrings ) const
{
	
}


bool FSceneOutlinerLayerContentsColumn::ProvidesSearchStrings()
{
	return false;
}


FReply FSceneOutlinerLayerContentsColumn::OnRemoveFromLayerClicked( const TWeakObjectPtr< AActor > Actor )
{
	ViewModel->RemoveActor( Actor );
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE