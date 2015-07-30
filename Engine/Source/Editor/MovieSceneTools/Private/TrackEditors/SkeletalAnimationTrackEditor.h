// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Tools for animation tracks
 */
class FSkeletalAnimationTrackEditor : public FMovieSceneTrackEditor
{
public:
	/**
	 * Constructor
	 *
	 * @param InSequencer	The sequencer instance to be used by this tool
	 */
	FSkeletalAnimationTrackEditor( TSharedRef<ISequencer> InSequencer );
	~FSkeletalAnimationTrackEditor();

	/**
	 * Creates an instance of this class.  Called by a sequencer 
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<FMovieSceneTrackEditor> CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer );

	/** FMovieSceneTrackEditor Interface */
	virtual bool SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track ) override;
	virtual void AddKey(const FGuid& ObjectGuid, UObject* AdditionalAsset) override;
	virtual bool HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid) override;
	virtual void BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass) override;

private:
	/** Animation sub menu */
	void BuildAnimationSubMenu(FMenuBuilder& MenuBuilder, FGuid ObjectBinding, USkeleton* Skeleton);

	/** Delegate for AnimatablePropertyChanged in AddKey */
	void AddKeyInternal(float KeyTime, const TArray<UObject*> Objects, class UAnimSequence* AnimSequence);

	/** Gets a skeleton from an object guid in the movie scene */
	class USkeleton* AcquireSkeletonFromObjectGuid(const FGuid& Guid);
};



/** Class for animation sections, handles drawing of all waveform previews */
class FSkeletalAnimationSection : public ISequencerSection, public TSharedFromThis<FSkeletalAnimationSection>
{
public:
	FSkeletalAnimationSection( UMovieSceneSection& InSection );
	~FSkeletalAnimationSection();

	/** ISequencerSection interface */
	virtual UMovieSceneSection* GetSectionObject() override;
	virtual FText GetDisplayName() const override;
	virtual FText GetSectionTitle() const override;
	virtual float GetSectionHeight() const override;
	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) const override {}
	virtual int32 OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const override;

private:
	/** The section we are visualizing */
	UMovieSceneSection& Section;
};
