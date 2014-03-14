// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SceneOutlinerPrivatePCH.h"
#include "SourceCodeNavigation.h"

#define LOCTEXT_NAMESPACE "SceneOutlinerActorInfoColumn"

TArray< TSharedPtr< ECustomColumnMode::Type > > FSceneOutlinerActorInfoColumn::ModeOptions;

FSceneOutlinerActorInfoColumn::FSceneOutlinerActorInfoColumn( const TWeakPtr< ISceneOutliner >& InSceneOutlinerWeak, ECustomColumnMode::Type InDefaultCustomColumnMode )
	: SceneOutlinerWeak( InSceneOutlinerWeak )
	, CurrentMode( InDefaultCustomColumnMode )
{

}


FName FSceneOutlinerActorInfoColumn::GetColumnID()
{
	return FName( "ActorInfo" );
}


SHeaderRow::FColumn::FArguments FSceneOutlinerActorInfoColumn::ConstructHeaderRowColumn()
{
	if( ModeOptions.Num() == 0 )
	{
		for( auto CurModeIndex = 0; CurModeIndex < ECustomColumnMode::Count; ++CurModeIndex )
		{
			ModeOptions.Add( MakeShareable( new ECustomColumnMode::Type( ( ECustomColumnMode::Type )CurModeIndex ) ) );
		}
	}

	/** Customizable actor data column */
	return SHeaderRow::Column( GetColumnID() )
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew( STextBlock )
				.Text( LOCTEXT("TreeColumn_CustomColumn", "Info") )
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				// @todo outliner: Alignment and style issues
				SNew( SComboBox< TSharedPtr< ECustomColumnMode::Type > > )
				.ContentPadding( FMargin( 0 ) )
				.ToolTipText( LOCTEXT("CustomColumnModeComboBox_ToolTip", "Choose what type of information to display in this column") )
				.OptionsSource( &ModeOptions )
				.OnGenerateWidget( this, &FSceneOutlinerActorInfoColumn::MakeComboButtonItemWidget )
				.OnSelectionChanged( this, &FSceneOutlinerActorInfoColumn::OnModeChanged )
				[
					SNew(STextBlock)
					.Text( this, &FSceneOutlinerActorInfoColumn::GetSelectedMode )
				]
				// Synchronize the initial custom column mode selection
				.InitiallySelectedItem( ModeOptions[ CurrentMode ] )
			]
		];
}


TSharedRef< SWidget > FSceneOutlinerActorInfoColumn::ConstructClassHyperlink( const TWeakObjectPtr< AActor >&  InActor )
{
	const AActor* Actor = InActor.Get();
	if ( Actor == nullptr )
	{
		return SNullWidget::NullWidget;
	}

	UClass* ObjectClass = Actor->GetClass();

	if ( UBlueprint* Blueprint = Cast<UBlueprint>( ObjectClass->ClassGeneratedBy ) )
	{
		TWeakObjectPtr<UBlueprint> BlueprintPtr = Blueprint;

		struct Local
		{
			static void OnEditBlueprintClicked( TWeakObjectPtr<UBlueprint> InBlueprint, TWeakObjectPtr<AActor> InAsset )
			{
				if ( UBlueprint* Blueprint = InBlueprint.Get() )
				{
					// Set the object being debugged if given an actor reference (if we don't do this before we edit the object the editor wont know we are debugging something)
					if ( UObject* Asset = InAsset.Get() )
					{
						check( Asset->GetClass()->ClassGeneratedBy == Blueprint );
						Blueprint->SetObjectBeingDebugged( Asset );
					}
					// Open the blueprint
					GEditor->EditObject( Blueprint );
				}
			}
		};

		return SNew( SHyperlink )
			.Visibility( this, &FSceneOutlinerActorInfoColumn::GetColumnDataVisibility, true )
			.Style( FEditorStyle::Get(), "HoverOnlyHyperlink" )
			.TextStyle( FEditorStyle::Get(), "SceneOutliner.EditBlueprintHyperlinkStyle" )
			.OnNavigate_Static( &Local::OnEditBlueprintClicked, BlueprintPtr, InActor )
			//.Text( FText::FromString( Blueprint->GetName() ) )
			.Text( FText::FromString( ObjectClass->GetName() ) )
			.ToolTipText( LOCTEXT( "EditBlueprint_ToolTip", "Click to edit the blueprint" ) );
	}
	else if ( FSourceCodeNavigation::IsCompilerAvailable() )
	{
		FString ClassHeaderPath;
		if ( FSourceCodeNavigation::FindClassHeaderPath( ObjectClass, ClassHeaderPath ) && IFileManager::Get().FileSize( *ClassHeaderPath ) != INDEX_NONE )
		{
			struct Local
			{
				static void OnEditCodeClicked( FString ClassHeaderPath )
				{
					FString AbsoluteHeaderPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead( *ClassHeaderPath );
					FSourceCodeNavigation::OpenSourceFile( AbsoluteHeaderPath );
				}
			};

			return SNew( SHyperlink )
				.Visibility( this, &FSceneOutlinerActorInfoColumn::GetColumnDataVisibility, true )
				.Style( FEditorStyle::Get(), "HoverOnlyHyperlink" )
				.TextStyle( FEditorStyle::Get(), "SceneOutliner.GoToCodeHyperlinkStyle" )
				.OnNavigate_Static( &Local::OnEditCodeClicked, ClassHeaderPath )
				//.Text( FText::FromString( FPaths::GetCleanFilename( *ClassHeaderPath ) ) )
				.Text( FText::FromString( ObjectClass->GetName() ) )
				.ToolTipText( LOCTEXT( "GoToCode_ToolTip", "Click to open this source file in a text editor" ) );
		}
	}

	return SNew( STextBlock )
		.Text( ObjectClass->GetName() )
		.HighlightText( SceneOutlinerWeak.Pin().Get(), &ISceneOutliner::GetFilterHighlightText )
		.ColorAndOpacity( FSlateColor::UseSubduedForeground() )
		.Visibility( this, &FSceneOutlinerActorInfoColumn::GetColumnDataVisibility, true );
}

const TSharedRef< SWidget > FSceneOutlinerActorInfoColumn::ConstructRowWidget( const TWeakObjectPtr< AActor >&  Actor )
{
	TSharedRef<SHorizontalBox> InfoBox = SNew( SHorizontalBox );

	//.Visibility( [&] () { return CurrentMode != ECustomColumnMode::ActorClass ? EVisibility::Visible : EVisibility::Collapsed; } );

	InfoBox->AddSlot()
	.AutoWidth()
	.Padding( 0.0f, 3.0f, 0.0f, 0.0f )
	[
		SNew( SHorizontalBox )

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( STextBlock )

			// Bind a delegate for custom text for this item's row
			.Text( this, &FSceneOutlinerActorInfoColumn::GetTextForActor, Actor )
			// Bind our filter text as the highlight string for the text block, so that when the user
			// starts typing search criteria, this text highlights
			.HighlightText( SceneOutlinerWeak.Pin().Get(), &ISceneOutliner::GetFilterHighlightText )
			.ColorAndOpacity( FSlateColor::UseSubduedForeground() )
			.Visibility( this, &FSceneOutlinerActorInfoColumn::GetColumnDataVisibility, false )
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			ConstructClassHyperlink( Actor )
		]
	];

	return InfoBox;
}

bool FSceneOutlinerActorInfoColumn::ProvidesSearchStrings()
{
	return false;
}

void FSceneOutlinerActorInfoColumn::PopulateActorSearchStrings( const AActor* const Actor, OUT TArray< FString >& OutSearchStrings ) const
{
	switch( CurrentMode )
	{
	case ECustomColumnMode::None:
		break;

	case ECustomColumnMode::Layer:
		{
			for( int32 LayerIndex=0; LayerIndex < Actor->Layers.Num(); ++LayerIndex)
			{
				OutSearchStrings.Add( Actor->Layers[ LayerIndex ].ToString() );
			}
		}
		break;

	case ECustomColumnMode::Level:
		{
			OutSearchStrings.Add( Actor->GetOutermost()->GetName() );
		}
		break;

	case ECustomColumnMode::Socket:
		{
			OutSearchStrings.Add( Actor->GetAttachParentSocketName().ToString() );
		}
		break;

	case ECustomColumnMode::InternalName:
		{
			OutSearchStrings.Add( Actor->GetFName().ToString() );
		}
		break;

	case ECustomColumnMode::UncachedLights:
		break;

	}

	// Covers ECustomColumnMode::ActorClass 
	// We always want this to be a search string
	FString ActorClassName;
	Actor->GetClass()->GetName( ActorClassName );
	OutSearchStrings.Add( FString::Printf( TEXT( "(%s)" ), *ActorClassName) );
}

bool FSceneOutlinerActorInfoColumn::SupportsSorting() const
{
	return CurrentMode != ECustomColumnMode::None;
}

void FSceneOutlinerActorInfoColumn::SortItems(TArray<TSharedPtr<SceneOutliner::TOutlinerTreeItem>>& RootItems, const EColumnSortMode::Type SortMode) const
{
	if (SortMode == EColumnSortMode::Ascending)
	{
		RootItems.Sort(
			[=](TSharedPtr<SceneOutliner::TOutlinerTreeItem> A, TSharedPtr<SceneOutliner::TOutlinerTreeItem> B)
			{
				return GetTextForActor(A->Actor) < GetTextForActor(B->Actor);
			}
		);
	}
	else if (SortMode == EColumnSortMode::Descending)
	{
		RootItems.Sort(
			[=](TSharedPtr<SceneOutliner::TOutlinerTreeItem> A, TSharedPtr<SceneOutliner::TOutlinerTreeItem> B)
			{
				return GetTextForActor(A->Actor) > GetTextForActor(B->Actor);
			}
		);
	}
}

void FSceneOutlinerActorInfoColumn::OnModeChanged( TSharedPtr< ECustomColumnMode::Type > NewSelection, ESelectInfo::Type /*SelectInfo*/ )
{
	CurrentMode = *NewSelection;

	// Refresh and refilter the list
	SceneOutlinerWeak.Pin()->Refresh();
}

EVisibility FSceneOutlinerActorInfoColumn::GetColumnDataVisibility( bool bIsClassHyperlink ) const
{
	if ( CurrentMode == ECustomColumnMode::ActorClass )
	{
		return bIsClassHyperlink ? EVisibility::Visible : EVisibility::Collapsed;
	}
	else
	{
		return bIsClassHyperlink ? EVisibility::Collapsed : EVisibility::Visible;
	}
}

FString FSceneOutlinerActorInfoColumn::GetTextForActor( const TWeakObjectPtr< AActor > TreeItem ) const
{
	FString CustomColumnText;
	AActor* Actor = TreeItem.Get();

	if( Actor == NULL )
	{
		return CustomColumnText;
	}

	switch( CurrentMode )
	{
	case ECustomColumnMode::ActorClass:
		CustomColumnText = Actor->GetClass()->GetName();
		break;

	case ECustomColumnMode::Layer:
		{
			FString ResultString = TEXT("");

			for( auto LayerIt = Actor->Layers.CreateConstIterator(); LayerIt; ++LayerIt )
			{
				if( ResultString.Len() )
				{
					ResultString += TEXT(",");
				}

				ResultString += LayerIt->ToString();
			}

			CustomColumnText = ResultString;
		}
		break;

	case ECustomColumnMode::Level:
		CustomColumnText = FPackageName::GetShortName( Actor->GetOutermost()->GetName() );
		break;

	case ECustomColumnMode::Socket:
		{
			CustomColumnText = Actor->GetAttachParentSocketName().ToString();
		}
		break;

	case ECustomColumnMode::InternalName:
		CustomColumnText = Actor->GetFName().ToString();
		break;

	case ECustomColumnMode::UncachedLights:
		CustomColumnText = FString::Printf(TEXT("%7d"), Actor->GetNumUncachedStaticLightingInteractions());
		break;
	};

	return CustomColumnText;
}

FText FSceneOutlinerActorInfoColumn::GetSelectedMode() const
{
	return MakeComboText(CurrentMode);
}

FText FSceneOutlinerActorInfoColumn::MakeComboText( const ECustomColumnMode::Type& Mode ) const
{
	FText ModeName;

	switch( Mode )
	{
	case ECustomColumnMode::None:
		ModeName = LOCTEXT("CustomColumnMode_None", " - ");
		break;

	case ECustomColumnMode::ActorClass:
		ModeName = LOCTEXT("CustomColumnMode_ActorClass", "Type");
		break;

	case ECustomColumnMode::Level:
		ModeName = LOCTEXT("CustomColumnMode_Level", "Level");
		break;

	case ECustomColumnMode::Layer:
		ModeName = LOCTEXT("CustomColumnMode_Layer", "Layer");
		break;

	case ECustomColumnMode::Socket:
		ModeName = LOCTEXT("CustomColumnMode_Socket", "Socket");
		break;

	case ECustomColumnMode::InternalName:
		ModeName = LOCTEXT("CustomColumnMode_InternalName", "ID Name");
		break;

	case ECustomColumnMode::UncachedLights:
		ModeName = LOCTEXT("CustomColumnMode_UncachedLights", "#UncachedLights");
		break;

	default:
		ensure(0);
		break;
	}

	return ModeName;
}


FText FSceneOutlinerActorInfoColumn::MakeComboToolTipText( const ECustomColumnMode::Type& Mode )
{
	FText ToolTipText;

	switch( Mode )
	{
	case ECustomColumnMode::None:
		ToolTipText = LOCTEXT("CustomColumnModeToolTip_None", "Hides all extra actor info");
		break;

	case ECustomColumnMode::ActorClass:
		ToolTipText = LOCTEXT("CustomColumnModeToolTip_ActorClass", "Displays the name of each actor's type");
		break;

	case ECustomColumnMode::Level:
		ToolTipText = LOCTEXT("CustomColumnModeToolTip_Level", "Displays the level each actor is in, and allows you to search by level name");
		break;

	case ECustomColumnMode::Layer:
		ToolTipText = LOCTEXT("CustomColumnModeToolTip_Layer", "Displays the layer each actor is in, and allows you to search by layer name");
		break;

	case ECustomColumnMode::Socket:
		ToolTipText = LOCTEXT("CustomColumnModeToolTip_Socket", "Shows the socket the actor is attached to, and allows you to search by socket name");
		break;

	case ECustomColumnMode::InternalName:
		ToolTipText = LOCTEXT("CustomColumnModeToolTip_InternalName", "Shows the internal name of the actor (for diagnostics)");
		break;

	case ECustomColumnMode::UncachedLights:
		ToolTipText = LOCTEXT("CustomColumnModeToolTip_UncachedLights", "Shows the number of uncached static lights (missing in lightmap)");
		break;

	default:
		ensure(0);
		break;
	}

	return ToolTipText;
}


TSharedRef< SWidget > FSceneOutlinerActorInfoColumn::MakeComboButtonItemWidget( TSharedPtr< ECustomColumnMode::Type > Mode )
{
	return
		SNew( STextBlock )
		.Text( MakeComboText( *Mode ) )
		.ToolTipText( MakeComboToolTipText( *Mode ) );
}

#undef LOCTEXT_NAMESPACE
