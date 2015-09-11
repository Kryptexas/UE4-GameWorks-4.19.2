// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class UMovieSceneMaterialTrack;

/**
 * Track editor for material parameters.
 */
class FMaterialTrackEditor : public FMovieSceneTrackEditor
{
public:
	FMaterialTrackEditor( TSharedRef<ISequencer> InSequencer );

	virtual ~FMaterialTrackEditor() { }

	/* FMovieSceneTrackEditor interface */
	virtual TSharedRef<ISequencerSection> MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track ) override;
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget( const FGuid& ObjectBinding, UMovieSceneTrack* Track ) override;

protected:
	/** Gets a material interface for a specific object binding and material track. */
	virtual UMaterialInterface* GetMaterialInterfaceForTrack( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack ) = 0;

private:
	/** Provides the contents of the add parameter menu. */
	TSharedRef<SWidget> OnGetAddParameterMenuContent( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack );

	/** Gets a material for a specific object binding and track */
	UMaterial* GetMaterialForTrack( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack );

	/** Adds a scalar parameter and initial key to a material track.
	 * @param ObjectBinding The object binding which owns the material track.
	 * @param MaterialTrack The track to Add the section to.
	 * @param ParameterName The name of the parameter to add an initial key for.
	 */
	void AddScalarParameter( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack, FName ParameterName );

	/** Adds a vector parameter and initial key to a material track.
	* @param ObjectBinding The object binding which owns the material track.
	* @param MaterialTrack The track to Add the section to.
	* @param ParameterName The name of the parameter to add an initial key for.
	*/
	void AddVectorParameter( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack, FName ParameterName );
};

/**
 * A specialized material track editor for component materials
 */
class FComponentMaterialTrackEditor : public FMaterialTrackEditor
{
public:
	FComponentMaterialTrackEditor( TSharedRef<ISequencer> InSequencer );

	static TSharedRef<FMovieSceneTrackEditor> CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer );

	/** FMovieSceneTrackEditor Interface */
	virtual bool SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const override;

protected:
	/** FMaterialtrackEditor interface */
	virtual UMaterialInterface* GetMaterialInterfaceForTrack( FGuid ObjectBinding, UMovieSceneMaterialTrack* MaterialTrack ) override;
};