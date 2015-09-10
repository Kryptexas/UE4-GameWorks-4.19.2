// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MaterialTrackEditor.h"
#include "MovieSceneMaterialTrack.h"
#include "MaterialParameterSection.h"
#include "MovieSceneMaterialParameterSection.h"

#define LOCTEXT_NAMESPACE "MaterialTrackEditor"

FMaterialTrackEditor::FMaterialTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer )
{
}

TSharedRef<ISequencerSection> FMaterialTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	UMovieSceneMaterialParameterSection* ParameterSection = Cast<UMovieSceneMaterialParameterSection>(&SectionObject);
	checkf( ParameterSection != nullptr, TEXT("Unsupported section type.") );
	return MakeShareable(new FMaterialParameterSection( *ParameterSection, ParameterSection->GetFName()));
}

TSharedPtr<SWidget> FMaterialTrackEditor::BuildOutlinerEditWidget( const FGuid& ObjectBinding, UMovieSceneTrack* Track )
{
	UMovieSceneMaterialTrack* MaterialTrack = Cast<UMovieSceneMaterialTrack>(Track);
	return
		SNew( SComboButton )
		.ButtonStyle( FEditorStyle::Get(), "FlatButton.Light" )
		.OnGetMenuContent( this, &FMaterialTrackEditor::OnGetAddParameterMenuContent, ObjectBinding, MaterialTrack )
		.ContentPadding( FMargin( 2, 0 ) )
		.ButtonContent()
		[
			SNew( SHorizontalBox )

			+ SHorizontalBox::Slot()
			.VAlign( VAlign_Center )
			.AutoWidth()
			[
				SNew( STextBlock )
				.Font( FEditorStyle::Get().GetFontStyle( "FontAwesome.8" ) )
				.Text( FText::FromString( FString( TEXT( "\xf067" ) ) ) /*fa-plus*/ )
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 4, 0, 0, 0 )
			[
				SNew( STextBlock )
				.Font( FEditorStyle::GetFontStyle( "Sequencer.AnimationOutliner.RegularFont" ) )
				.Text( LOCTEXT( "AddTrackButton", "Parameter" ) )
			]
		];
}

TSharedRef<SWidget> FMaterialTrackEditor::OnGetAddParameterMenuContent( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack )
{
	FMenuBuilder AddParameterMenuBuilder( true, nullptr );

	UMaterial* Material = GetMaterialForTrack( ObjectBinding, MaterialTrack );
	if ( Material != nullptr )
	{
		TArray<FName> ScalarParameterNames;
		TArray<FGuid> ScalarParmeterGuids;
		Material->GetAllScalarParameterNames( ScalarParameterNames, ScalarParmeterGuids );

		for ( const FName& ScalarParameterName : ScalarParameterNames )
		{
			FUIAction AddParameterMenuAction( FExecuteAction::CreateSP( this, &FMaterialTrackEditor::AddParameterSection, ObjectBinding, MaterialTrack, ScalarParameterName ) );
			AddParameterMenuBuilder.AddMenuEntry( FText::FromName( ScalarParameterName ), FText(), FSlateIcon(), AddParameterMenuAction );
		}
	}

	return AddParameterMenuBuilder.MakeWidget();
}

UMaterial* FMaterialTrackEditor::GetMaterialForTrack( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack )
{
	UMaterialInterface* MaterialInterface = GetMaterialInterfaceForTrack( ObjectBinding, MaterialTrack );
	if ( MaterialInterface != nullptr )
	{
		UMaterial* Material = Cast<UMaterial>( MaterialInterface );
		if ( Material != nullptr )
		{
			return Material;
		}
		else
		{
			UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>( MaterialInterface );
			if ( MaterialInstance != nullptr )
			{
				return MaterialInstance->GetMaterial();
			}
		}
	}
	return nullptr;
}

void FMaterialTrackEditor::AddParameterSection( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack, FName ParameterName )
{
	UMovieSceneSequence* MovieSceneSequence = GetMovieSceneSequence();
	float KeyTime = GetTimeForKey( MovieSceneSequence );

	UMaterial* Material = GetMaterialForTrack(ObjectBinding, MaterialTrack);
	if (Material != nullptr)
	{
		float ParameterValue;
		Material->GetScalarParameterValue(ParameterName, ParameterValue);
		MaterialTrack->AddScalarParameterKey(ParameterName, KeyTime, ParameterValue);
	}
	NotifyMovieSceneDataChanged();
}

FComponentMaterialTrackEditor::FComponentMaterialTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMaterialTrackEditor( InSequencer )
{
}

TSharedRef<FMovieSceneTrackEditor> FComponentMaterialTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer )
{
	return MakeShareable( new FComponentMaterialTrackEditor( OwningSequencer ) );
}

bool FComponentMaterialTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	return Type == UMovieSceneComponentMaterialTrack::StaticClass();
}

UMaterialInterface* FComponentMaterialTrackEditor::GetMaterialInterfaceForTrack( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack )
{
	UObject* ComponentObject = GetSequencer()->GetFocusedMovieSceneSequence()->FindObject( ObjectBinding );
	UPrimitiveComponent* Component = Cast<UPrimitiveComponent>( ComponentObject );
	UMovieSceneComponentMaterialTrack* ComponentMaterialTrack = Cast<UMovieSceneComponentMaterialTrack>( MaterialTrack );
	if ( Component != nullptr && ComponentMaterialTrack != nullptr )
	{
		return Component->GetMaterial( ComponentMaterialTrack->GetMaterialIndex() );
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE