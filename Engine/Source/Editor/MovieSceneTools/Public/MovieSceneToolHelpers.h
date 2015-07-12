// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencer.h"
#include "ISequencerSection.h"
#include "IKeyArea.h"

/**
 * A key area for float keys
 */
class MOVIESCENETOOLS_API FFloatCurveKeyArea : public IKeyArea
{
public:
	FFloatCurveKeyArea( FRichCurve* InCurve, UMovieSceneSection* InOwningSection )
		: Curve( InCurve )
		, OwningSection( InOwningSection )
	{}

	/** IKeyArea interface */
	virtual TArray<FKeyHandle> GetUnsortedKeyHandles() const override
	{
		TArray<FKeyHandle> OutKeyHandles;
		for (auto It(Curve->GetKeyHandleIterator()); It; ++It)
		{
			OutKeyHandles.Add(It.Key());
		}
		return OutKeyHandles;
	}

	virtual float GetKeyTime( FKeyHandle KeyHandle ) const override
	{
		return Curve->GetKeyTime( KeyHandle );
	}

	virtual FKeyHandle MoveKey( FKeyHandle KeyHandle, float DeltaPosition ) override
	{
		return Curve->SetKeyTime( KeyHandle, Curve->GetKeyTime( KeyHandle ) + DeltaPosition );
	}

	virtual void DeleteKey(FKeyHandle KeyHandle) override
	{
		if ( Curve->IsKeyHandleValid( KeyHandle ) )
		{
			Curve->DeleteKey( KeyHandle );
		}
	}

	virtual void AddKeyUnique(float Time) override;

	virtual FRichCurve* GetRichCurve() override { return Curve; }

	virtual UMovieSceneSection* GetOwningSection() override { return OwningSection; }

	virtual bool CanCreateKeyEditor() override
	{
		return true;
	}

	virtual TSharedRef<SWidget> CreateKeyEditor(ISequencer* Sequencer) override;

private:
	/** The curve which provides the keys for this key area. */
	FRichCurve* Curve;
	/** The section that owns this key area. */
	UMovieSceneSection* OwningSection;
};



/** A key area for integral keys */
class MOVIESCENETOOLS_API FIntegralKeyArea : public IKeyArea
{
public:
	FIntegralKeyArea(FIntegralCurve& InCurve, UMovieSceneSection* InOwningSection)
		: Curve(InCurve)
		, OwningSection(InOwningSection)
	{}

	/** IKeyArea interface */
	virtual TArray<FKeyHandle> GetUnsortedKeyHandles() const override
	{
		TArray<FKeyHandle> OutKeyHandles;
		for (auto It(Curve.GetKeyHandleIterator()); It; ++It)
		{
			OutKeyHandles.Add(It.Key());
		}
		return OutKeyHandles;
	}

	virtual float GetKeyTime( FKeyHandle KeyHandle ) const override
	{
		return Curve.GetKeyTime( KeyHandle );
	}

	virtual FKeyHandle MoveKey( FKeyHandle KeyHandle, float DeltaPosition ) override
	{
		return Curve.SetKeyTime( KeyHandle, Curve.GetKeyTime( KeyHandle ) + DeltaPosition );
	}

	virtual void DeleteKey(FKeyHandle KeyHandle) override
	{
		Curve.DeleteKey(KeyHandle);
	}

	virtual void AddKeyUnique(float Time) override;

	virtual FRichCurve* GetRichCurve() override { return nullptr; };

	virtual UMovieSceneSection* GetOwningSection() override { return OwningSection; }

	virtual bool CanCreateKeyEditor() override
	{
		return true;
	}

	virtual TSharedRef<SWidget> CreateKeyEditor(ISequencer* Sequencer) override;

protected:
	/** Curve with keys in this area */
	FIntegralCurve& Curve;
	/** The section that owns this key area. */
	UMovieSceneSection* OwningSection;
};

/** A key area for displaying and editing intragral curves representing enums. */
class FEnumKeyArea : public FIntegralKeyArea
{
public:
	FEnumKeyArea(FIntegralCurve& InCurve, UMovieSceneSection* InOwningSection, UEnum* InEnum)
		: FIntegralKeyArea(InCurve, InOwningSection)
		, Enum(InEnum)
	{}

	virtual bool CanCreateKeyEditor() override 
	{
		return true;
	}

	virtual TSharedRef<SWidget> CreateKeyEditor(ISequencer* Sequencer) override;

private:
	/** The enum which provides available integral values for this key area. */
	UEnum* Enum;
};

/** A key area for displaying and editing integral curves representing bools. */
class FBoolKeyArea : public FIntegralKeyArea
{
public:
	FBoolKeyArea(FIntegralCurve& InCurve, UMovieSceneSection* InOwningSection)
		: FIntegralKeyArea(InCurve, InOwningSection)
	{}

	virtual bool CanCreateKeyEditor() override 
	{
		return true;
	}

	virtual TSharedRef<SWidget> CreateKeyEditor(ISequencer* Sequencer) override;
};


