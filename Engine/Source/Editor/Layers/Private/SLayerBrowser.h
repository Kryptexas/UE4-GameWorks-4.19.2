// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ModuleManager.h"
#include "Editor/SceneOutliner/Public/SceneOutlinerModule.h"

#define LOCTEXT_NAMESPACE "LayerBrowser"

typedef TTextFilter< const TSharedPtr< FLayerViewModel >& > LayerTextFilter;

namespace ELayerBrowserMode
{
	enum Type
	{
		Layers,
		LayerContents,

		Count
	};
}


/**
 *	
 */
class SLayerBrowser : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SLayerBrowser ) {}
	SLATE_END_ARGS()

	~SLayerBrowser()
	{
		LayerCollectionViewModel->OnLayersChanged().RemoveAll( this );
		LayerCollectionViewModel->OnSelectionChanged().RemoveAll( this );
		LayerCollectionViewModel->OnRenameRequested().RemoveAll( this );
		LayerCollectionViewModel->RemoveFilter( SearchBoxLayerFilter.ToSharedRef() );
	}

	/**
	 * Construct this widget.  Called by the SNew() Slate macro.
	 *
	 * @param	InArgs				Declaration used by the SNew() macro to construct this widget
	 * @param	InViewModel			The UI logic not specific to slate
	 */
	void Construct( const FArguments& InArgs )
	{
		//@todo Create a proper ViewModel [7/27/2012 Justin.Sargent]
		Mode = ELayerBrowserMode::Layers;

		LayerCollectionViewModel = FLayerCollectionViewModel::Create( GEditor->Layers.ToSharedRef(), GEditor );
		SelectedLayersFilter = MakeShareable( new FActorsAssignedToSpecificLayersFilter() );
		SelectedLayerViewModel = FLayerViewModel::Create( NULL, GEditor->Layers.ToSharedRef(), GEditor ); //We'll set the datasource for this viewmodel later
		
		SearchBoxLayerFilter = MakeShareable( new LayerTextFilter( LayerTextFilter::FItemToStringArray::CreateSP( this, &SLayerBrowser::TransformLayerToString ) ) );
		
		LayerCollectionViewModel->AddFilter( SearchBoxLayerFilter.ToSharedRef() );
		LayerCollectionViewModel->OnLayersChanged().AddSP( this, &SLayerBrowser::OnLayersChanged );
		LayerCollectionViewModel->OnSelectionChanged().AddSP( this, &SLayerBrowser::UpdateSelectedLayer );
		LayerCollectionViewModel->OnRenameRequested().AddSP( this, &SLayerBrowser::OnRenameRequested );

		//////////////////////////////////////////////////////////////////////////
		//	Layers View Section
		SAssignNew( LayersSection, SBorder )
		.Padding( 5 )
		.BorderImage( FEditorStyle::GetBrush( "NoBrush" ) )
		.Content()
		[
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SSearchBox )
				.ToolTipText( LOCTEXT("FilterSearchToolTip", "Type here to search layers").ToString() )
				.HintText( LOCTEXT( "FilterSearchHint", "Search Layers" ) )
				.OnTextChanged( SearchBoxLayerFilter.Get(), &LayerTextFilter::SetRawFilterText )
			]

			+SVerticalBox::Slot()
			.FillHeight( 1.0f )
			[
				SAssignNew( LayersView, SLayersView, LayerCollectionViewModel.ToSharedRef() )
				.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
				.ConstructContextMenu( FOnContextMenuOpening::CreateSP( this, &SLayerBrowser::ConstructLayerContextMenu ) )
				.HighlightText( SearchBoxLayerFilter.Get(), &LayerTextFilter::GetRawFilterText )
			]
		];

		//////////////////////////////////////////////////////////////////////////
		//	Layer Contents Header
		SAssignNew( LayerContentsHeader, SBorder )
		.BorderImage( FEditorStyle::GetBrush("LayerBrowser.LayerContentsQuickbarBackground") )
		.Visibility( TAttribute< EVisibility >( this, &SLayerBrowser::GetLayerContentsHeaderVisibility ) )
		.Content()
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( FMargin( 0, 0, 2, 0 ))
			[
				SAssignNew( ToggleModeButton, SButton )
				.ContentPadding( FMargin( 2, 0, 2, 0 ) ) 
				.ButtonStyle( FEditorStyle::Get(), "LayerBrowserButton" )
				.OnClicked( this, &SLayerBrowser::ToggleLayerContents )
				.ForegroundColor( FSlateColor::UseForeground() )
				.VAlign( VAlign_Center )
				.HAlign( HAlign_Center )
				.Content()
				[
					SNew( SHorizontalBox )
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign( HAlign_Center )
					.VAlign( VAlign_Center )
					.Padding(0, 1, 3, 1 )
					[
						SNew(SImage)
						.Image( this, &SLayerBrowser::GetToggleModeButtonImageBrush )
						.ColorAndOpacity( this, &SLayerBrowser::GetInvertedForegroundIfHovered )
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign( HAlign_Center )
					.VAlign( VAlign_Center )
					[
						SNew( STextBlock )
						.Text( LOCTEXT("ContentsLabel", "See Contents").ToString() )
						.Visibility( this, &SLayerBrowser::IsVisibleIfModeIs, ELayerBrowserMode::Layers )
						.ColorAndOpacity( this, &SLayerBrowser::GetInvertedForegroundIfHovered )
					]
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign( VAlign_Center )
			[
				SNew( STextBlock )
				.Text( this, &SLayerBrowser::GetLayerContentsHeaderText )
				.Visibility( this, &SLayerBrowser::IsVisibleIfModeIs, ELayerBrowserMode::LayerContents )
			]

			+SHorizontalBox::Slot()
			.HAlign( HAlign_Right )
			.FillWidth( 1.0f )
			[
				SNew( SLayerStats, SelectedLayerViewModel.ToSharedRef() )
			]
		];


		//////////////////////////////////////////////////////////////////////////
		//	Layer Contents Section
		FSceneOutlinerModule& SceneOutlinerModule = FModuleManager::LoadModuleChecked< FSceneOutlinerModule >( "SceneOutliner" );
		FSceneOutlinerInitializationOptions InitOptions;
		{
			InitOptions.Mode = ESceneOutlinerMode::ActorBrowsing;

			// We hide the header row to keep the UI compact.
			InitOptions.bShowHeaderRow = false;
			InitOptions.bShowParentTree = false;
			InitOptions.CustomDelete = FCustomSceneOutlinerDeleteDelegate::CreateSP( this, &SLayerBrowser::RemoveActorsFromSelectedLayer );
			InitOptions.CustomColumnFactory = FCreateSceneOutlinerColumnDelegate::CreateSP( this, &SLayerBrowser::CreateCustomLayerColumn );

			InitOptions.ActorFilters->Add( SelectedLayersFilter );
		}

		SAssignNew( LayerContentsSection, SBorder )
		.Padding( 5 )
		.BorderImage( FEditorStyle::GetBrush( "NoBrush" ) )
		.Content()
		[
			SceneOutlinerModule.CreateSceneOutliner( InitOptions, FOnContextMenuOpening(), FOnActorPicked() )
		];


		//////////////////////////////////////////////////////////////////////////
		//	Layer Browser
		ChildSlot
		[
			SAssignNew( ContentAreaBox, SVerticalBox )
		];

		SetupLayersMode();
	}


protected:

	/** Overridden from SWidget: Called when a key is pressed down */
	FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
	{
		if( InKeyboardEvent.GetKey() == EKeys::Escape && Mode == ELayerBrowserMode::LayerContents )
		{
			SetupLayersMode();			
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	/**
	 * Called during drag and drop when the drag leaves a widget.
	 *
	 * @param DragDropEvent   The drag and drop event.
	 */
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		if( !DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() ) )
		{
			return;
		}

		TSharedPtr< FActorDragDropGraphEdOp > DragActorOp = StaticCastSharedPtr< FActorDragDropGraphEdOp >( DragDropEvent.GetOperation() );	
		DragActorOp->SetToolTip( FActorDragDropGraphEdOp::ToolTip_Default );
	}

	/**
	 * Called during drag and drop when the the mouse is being dragged over a widget.
	 *
	 * @param MyGeometry      The geometry of the widget receiving the event.
	 * @param DragDropEvent   The drag and drop event.
	 *
	 * @return A reply that indicated whether this event was handled.
	 */
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		if( !SelectedLayerViewModel->GetDataSource().IsValid() || !DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() ) )
		{
			return FReply::Unhandled();
		}

		const FGeometry LayerContentsHeaderGeometry = SWidget::FindChildGeometry( MyGeometry, LayerContentsHeader.ToSharedRef() );
		bool bValidDrop = LayerContentsHeaderGeometry.IsUnderLocation( DragDropEvent.GetScreenSpacePosition() );

		if( !bValidDrop && Mode == ELayerBrowserMode::LayerContents )
		{
			const FGeometry LayerContentsSectionGeometry = SWidget::FindChildGeometry( MyGeometry, LayerContentsSection.ToSharedRef() );
			bValidDrop = LayerContentsSectionGeometry.IsUnderLocation( DragDropEvent.GetScreenSpacePosition() );
		}

		if( !bValidDrop )
		{
			return FReply::Unhandled();
		}

		TSharedPtr< FActorDragDropGraphEdOp > DragActorOp = StaticCastSharedPtr< FActorDragDropGraphEdOp >( DragDropEvent.GetOperation() );	

		if ( !DragActorOp.IsValid() || DragActorOp->Actors.Num() == 0 )
		{
			return FReply::Unhandled();
		}

		bool bCanAssign = false;
		FString Message;
		if( DragActorOp->Actors.Num() > 1 )
		{
			bCanAssign = SelectedLayerViewModel->CanAssignActors( DragActorOp->Actors, OUT Message );
		}
		else
		{
			bCanAssign = SelectedLayerViewModel->CanAssignActor( DragActorOp->Actors[ 0 ], OUT Message );
		}

		if ( bCanAssign )
		{
			DragActorOp->SetToolTip( FActorDragDropGraphEdOp::ToolTip_CompatibleGeneric, Message );
		}
		else
		{
			DragActorOp->SetToolTip( FActorDragDropGraphEdOp::ToolTip_IncompatibleGeneric, Message );
		}

		// We leave the event unhandled so the children of the ListView get a chance to grab the drag/drop
		return FReply::Unhandled();
	}

	/**
	 * Called when the user is dropping something onto a widget; terminates drag and drop.
	 *
	 * @param MyGeometry      The geometry of the widget receiving the event.
	 * @param DragDropEvent   The drag and drop event.
	 *
	 * @return A reply that indicated whether this event was handled.
	 */
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		if ( !SelectedLayerViewModel->GetDataSource().IsValid() || !DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() ) )	
		{
			return FReply::Unhandled();
		}

		const FGeometry LayerContentsHeaderGeometry = SWidget::FindChildGeometry( MyGeometry, LayerContentsHeader.ToSharedRef() );
		bool bValidDrop = LayerContentsHeaderGeometry.IsUnderLocation( DragDropEvent.GetScreenSpacePosition() );

		if( !bValidDrop && Mode == ELayerBrowserMode::LayerContents )
		{
			const FGeometry LayerContentsSectionGeometry = SWidget::FindChildGeometry( MyGeometry, LayerContentsSection.ToSharedRef() );
			bValidDrop = LayerContentsSectionGeometry.IsUnderLocation( DragDropEvent.GetScreenSpacePosition() );
		}

		if( !bValidDrop )
		{
			return FReply::Unhandled();
		}

		TSharedPtr< FActorDragDropGraphEdOp > DragActorOp = StaticCastSharedPtr< FActorDragDropGraphEdOp >( DragDropEvent.GetOperation() );		
		SelectedLayerViewModel->AddActors( DragActorOp->Actors );

		return FReply::Handled();
	}


private:
	
	void RemoveActorsFromSelectedLayer( const TArray< TWeakObjectPtr< AActor > >& Actors )
	{
		SelectedLayerViewModel->RemoveActors( Actors );
	}

	TSharedRef< ISceneOutlinerColumn > CreateCustomLayerColumn( const TWeakPtr< ISceneOutliner >& SceneOutliner ) const
	{
		return MakeShareable( new FSceneOutlinerLayerContentsColumn( SelectedLayerViewModel.ToSharedRef() ) );
	}

	FSlateColor GetInvertedForegroundIfHovered() const
	{
		return ( ToggleModeButton.IsValid() && ( ToggleModeButton->IsHovered() || ToggleModeButton->IsPressed() ) ) ? FEditorStyle::GetSlateColor( "InvertedForeground" ) : FSlateColor::UseForeground();
	}

	const FSlateBrush* GetToggleModeButtonImageBrush() const
	{
		return ( Mode == ELayerBrowserMode::Layers ) ? FEditorStyle::GetBrush( "LayerBrowser.ExploreLayerContents" ) : FEditorStyle::GetBrush( "LayerBrowser.ReturnToLayersList" );
	}

	FString GetLayerContentsHeaderText() const
	{
		return FString::Printf( *LOCTEXT("SelectedContentsLabel", "%s Contents").ToString(), *SelectedLayerViewModel->GetName() );
	}

	/**	Returns the visibility of the See Contents label */
	EVisibility IsVisibleIfModeIs( ELayerBrowserMode::Type DesiredMode ) const
	{
		return ( Mode == DesiredMode ) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	EVisibility GetLayerContentsHeaderVisibility() const
	{
		return ( SelectedLayerViewModel->GetDataSource().IsValid() ) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	FReply ToggleLayerContents()
	{
		switch( Mode )
		{
		default:
		case ELayerBrowserMode::Layers:
			SetupLayerContentsMode();
			break;

		case ELayerBrowserMode::LayerContents:
			SetupLayersMode();
			break;
		}

		return FReply::Handled();
	}

	void SetupLayersMode() 
	{
		ContentAreaBox->ClearChildren();
		ContentAreaBox->AddSlot()
		.FillHeight( 1.0f )
		[
			LayersSection.ToSharedRef()
		];

		ContentAreaBox->AddSlot()
		.AutoHeight()
		.VAlign( VAlign_Bottom )
		.MaxHeight( 23 )
		[
			LayerContentsHeader.ToSharedRef()
		];

		Mode = ELayerBrowserMode::Layers;
	}

	void SetupLayerContentsMode() 
	{
		ContentAreaBox->ClearChildren();
		ContentAreaBox->AddSlot()
		.AutoHeight()
		.VAlign( VAlign_Top )
		.MaxHeight( 23 )
		[
			LayerContentsHeader.ToSharedRef()
		];

		ContentAreaBox->AddSlot()
		.AutoHeight()
		.FillHeight( 1.0f )
		[
			LayerContentsSection.ToSharedRef()
		];

		Mode = ELayerBrowserMode::LayerContents;
	}

	void TransformLayerToString( const TSharedPtr< FLayerViewModel >& Layer, OUT TArray< FString >& OutSearchStrings ) const
	{
		if( !Layer.IsValid() )
		{
			return;
		}

		OutSearchStrings.Add( Layer->GetName() );
	}

	void UpdateLayerContentsFilter()
	{
		TArray< FName > LayerNames;
		LayerCollectionViewModel->GetSelectedLayerNames( LayerNames );
		SelectedLayersFilter->SetLayers( LayerNames );
	}

	void UpdateSelectedLayer()
	{
		UpdateLayerContentsFilter();

		if( LayerCollectionViewModel->GetSelectedLayers().Num() == 1 )
		{
			const auto Layers = LayerCollectionViewModel->GetSelectedLayers();
			check( Layers.Num() == 1 );
			SelectedLayerViewModel->SetDataSource( Layers[ 0 ]->GetDataSource() );
		}
		else
		{
			SelectedLayerViewModel->SetDataSource( NULL );
		}
	}

	void OnLayersChanged( const ELayersAction::Type Action, const TWeakObjectPtr< ULayer >& ChangedLayer, const FName& ChangedProperty )
	{
		if( Action != ELayersAction::Reset && Action != ELayersAction::Delete )
		{
			if( !ChangedLayer.IsValid() || SelectedLayerViewModel->GetDataSource() == ChangedLayer )
			{
				UpdateLayerContentsFilter();
			}

			return;
		}

		UpdateSelectedLayer();

		if( Mode == ELayerBrowserMode::LayerContents && !SelectedLayerViewModel->GetDataSource().IsValid() )
		{
			SetupLayersMode();
		}
	}
	
	/** Callback when layers want to be renamed */
	void OnRenameRequested()
	{
		LayersView->RequestRenameOnSelectedLayer();
	}

	TSharedPtr< SWidget > ConstructLayerContextMenu()
	{
		return SNew( SLayersCommandsMenu, LayerCollectionViewModel.ToSharedRef() );
	}


private:

	/** */
	TSharedPtr< SButton > ToggleModeButton;

	/**	 */
	TSharedPtr< SVerticalBox > ContentAreaBox;

	/**	 */
	TSharedPtr< SBorder > LayersSection;

	/**	 */
	TSharedPtr< SBorder > LayerContentsSection;

	/**	 */
	TSharedPtr< SBorder > LayerContentsHeader;

	/**	 */
	TSharedPtr< LayerTextFilter > SearchBoxLayerFilter;

	/**	 */
	TSharedPtr< FActorsAssignedToSpecificLayersFilter > SelectedLayersFilter;

	/**	 */
	TSharedPtr< FLayerCollectionViewModel > LayerCollectionViewModel;

	/**	 */
	ELayerBrowserMode::Type Mode;

	/**	 */
	TSharedPtr< FLayerViewModel > SelectedLayerViewModel;

	/** The layer view widget, displays all the layers in the level */
	TSharedPtr< SLayersView > LayersView;
};

#undef LOCTEXT_NAMESPACE
