// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Tools for animation tracks
 */
class FAnimationTrackEditor : public FMovieSceneTrackEditor
{
public:
	/**
	 * Constructor
	 *
	 * @param InSequencer	The sequencer instance to be used by this tool
	 */
	FAnimationTrackEditor( TSharedRef<ISequencer> InSequencer );
	~FAnimationTrackEditor();

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
	virtual void AddKey(const FGuid& ObjectGuid, UObject* AdditionalAsset) OVERRIDE;
	virtual bool HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid) OVERRIDE;
	virtual void BuildObjectBindingContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass) OVERRIDE;

private:
	/** Delegate for AnimatablePropertyChanged in AddKey */
	void AddKeyInternal(float KeyTime, const TArray<UObject*> Objects, class UAnimSequence* AnimSequence);

	/** Gets a skeleton from an object guid in the movie scene */
	class USkeleton* AcquireSkeletonFromObjectGuid(const FGuid& Guid);
};



/** Class for animation sections, handles drawing of all waveform previews */
class FAnimationSection : public ISequencerSection, public TSharedFromThis<FAnimationSection>
{
public:
	FAnimationSection( UMovieSceneSection& InSection );
	~FAnimationSection();

	/** ISequencerSection interface */
	virtual UMovieSceneSection* GetSectionObject() OVERRIDE;
	virtual FString GetDisplayName() const OVERRIDE;
	virtual FString GetSectionTitle() const OVERRIDE;
	virtual float GetSectionHeight() const OVERRIDE;
	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const OVERRIDE {}
	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const OVERRIDE;
	virtual void Tick( const FGeometry& AllottedGeometry, const FGeometry& ParentGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

private:
	/** The section we are visualizing */
	UMovieSceneSection& Section;
};
