// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneObjectManager.h"
#include "UMGMovieSceneObjectManager.generated.h"


class FWidgetBlueprintEditor;
class UObject;
class UWidgetAnimation;


/**
 * Movie scene object manager used by Sequencer.
 */
UCLASS(BlueprintType, MinimalAPI)
class UUMGMovieSceneObjectManager
	: public UObject
	, public IMovieSceneObjectManager
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Get the current widget animation.
	 *
	 * @return Widget animation object, or nullptr if not valid.
	 * @see HasValidWidgetAnimation
	 */
	UWidgetAnimation* GetWidgetAnimation()
	{
		return WidgetAnimation;
	}

	/** Check whether the current animation is valid.
	 *
	 * @return true if the animation is valid, false otherwise.
	 * @see GetWidgetAnimation
	 */
	bool HasValidWidgetAnimation() const;

	/** Rebuild the mappings to live preview objects. */
	void InitPreviewObjects();

	/**
	 * Bind the object manager to the given widget BP editor.
	 *
	 * @param InWidgetBlueprintEditor The widget BP editor to bind to.
	 * @param InAnimation The current animation to set.
	 */
	// @todo sequencer: gmp: call this from some appropriate place
	void BindWidgetBlueprintEditor(FWidgetBlueprintEditor& InWidgetBlueprintEditor, UWidgetAnimation& InAnimation);

	/** Unbind from the widget BP editor. */
	// @todo sequencer: gmp: call this from some appropriate place
	void UnbindWidgetBlueprintEditor();

public:

	// IMovieSceneObjectManager interface

	virtual bool AllowsSpawnableObjects() const override;
	virtual void BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject) override;
	virtual bool CanPossessObject(UObject& Object) const override;
	virtual void DestroyAllSpawnedObjects(UMovieScene& MovieScene) override { }
	virtual FGuid FindObjectId(const UMovieScene& MovieScene, UObject& Object) const override;
	virtual UObject* FindObject(const FGuid& ObjectId) const override;
	virtual void SpawnOrDestroyObjects(UMovieScene* MovieScene, bool DestroyAll) override { }
	virtual bool TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const override;
	virtual void UnbindPossessableObjects(const FGuid& ObjectId) override;

private:

	/** Handles widget preview updates in the current widget BP editor. */
	void HandleWidgetPreviewUpdated();

private:

	/** Mapping of preview objects to sequencer GUIDs */
	TMap<TWeakObjectPtr<UObject>, FGuid> PreviewObjectToIds;
	TMultiMap<FGuid, TWeakObjectPtr<UObject>> IdToPreviewObjects;
	TMultiMap<FGuid, TWeakObjectPtr<UObject>> IdToSlotContentPreviewObjects;
	TMap<TWeakObjectPtr<UObject>, FGuid> SlotContentPreviewObjectToIds;

	/** The current widget animation. */
	UPROPERTY(transient)
	UWidgetAnimation* WidgetAnimation;

	/** Pointer to the widget BP editor. */
	FWidgetBlueprintEditor* WidgetBlueprintEditor;
};
