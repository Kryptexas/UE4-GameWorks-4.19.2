// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Tools for particle tracks
 */
class FParticleTrackEditor : public FMovieSceneTrackEditor, public TSharedFromThis<FParticleTrackEditor>
{
public:
	/**
	 * Constructor
	 *
	 * @param InSequencer	The sequencer instance to be used by this tool
	 */
	FParticleTrackEditor( TSharedRef<ISequencer> InSequencer );
	~FParticleTrackEditor();

	/**
	 * Creates an instance of this class.  Called by a sequencer 
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<FMovieSceneTrackEditor> CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer );

	/** FMovieSceneTrackEditor Interface */
	virtual bool SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const OVERRIDE;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track ) OVERRIDE;
	virtual void BuildObjectBindingContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass) OVERRIDE;
	
	void AddParticleKey(const FGuid ObjectGuid, bool bTrigger);

private:
	/** Delegate for AnimatablePropertyChanged in AddKey */
	virtual void AddKeyInternal( float KeyTime, const TArray<UObject*> Objects, bool bTrigger);
};



/** Class for particle sections */
class FParticleSection : public ISequencerSection, public TSharedFromThis<FParticleSection>
{
public:
	FParticleSection( UMovieSceneSection& InSection );
	~FParticleSection();

	/** ISequencerSection interface */
	virtual UMovieSceneSection* GetSectionObject() OVERRIDE;
	virtual FString GetDisplayName() const OVERRIDE;
	virtual FString GetSectionTitle() const OVERRIDE;
	virtual float GetSectionHeight() const OVERRIDE;
	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const OVERRIDE {}
	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const OVERRIDE;
	virtual bool SectionIsResizable() const OVERRIDE;

private:
	/** The section we are visualizing */
	UMovieSceneSection& Section;
};
