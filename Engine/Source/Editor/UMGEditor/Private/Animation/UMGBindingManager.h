// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneBindingManager.h"
#include "UMGBindingManager.generated.h"


class FWidgetBlueprintEditor;
class UObject;
class UWidgetAnimation;


/**
 * Movie scene binding manager used by Sequencer.
 */
UCLASS(BlueprintType, MinimalAPI)
class UUMGBindingManager
	: public UObject
	, public IMovieSceneBindingManager
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
	 * Bind the binding manager the given widget BP editor.
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

	// IMovieSceneBindingManager interface

	virtual bool AllowsSpawnableObjects() const override;
	virtual void BindPossessableObject(const FMovieSceneObjectId& ObjectId, UObject& PossessedObject) override;
	virtual bool CanPossessObject( UObject& Object ) const override;
	virtual void DestroyAllSpawnedObjects() override { }
	virtual FMovieSceneObjectId FindIdForObject(const UMovieScene& MovieScene, UObject& Object) const override;
	virtual void GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, TArray<UObject*>& OutRuntimeObjects) const override;
	virtual void RemoveMovieSceneInstance(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance) override { }
	virtual void SpawnOrDestroyObjectsForInstance(TSharedRef<FMovieSceneInstance> MovieSceneInstance, bool bDestroyAll) override { }
	virtual bool TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, FText& OutDisplayName) const override;
	virtual void UnbindPossessableObjects(const FMovieSceneObjectId& ObjectId) override;

private:

	/** Handles widget preview updates in the current widget BP editor. */
	void HandleWidgetPreviewUpdated();

private:

	/** Mapping of preview objects to sequencer guids */
	TMap<TWeakObjectPtr<UObject>, FMovieSceneObjectId> PreviewObjectToId;
	TMultiMap<FMovieSceneObjectId, TWeakObjectPtr<UObject>> ObjectIdToPreviewObjects;
	TMultiMap<FMovieSceneObjectId, TWeakObjectPtr<UObject>> ObjectIdToSlotContentPreviewObjects;
	TMap<TWeakObjectPtr<UObject>, FMovieSceneObjectId> SlotContentPreviewObjectToId;

	/** The current widget animation. */
	UPROPERTY(transient)
	UWidgetAnimation* WidgetAnimation;

	/** Pointer to the widget BP editor. */
	FWidgetBlueprintEditor* WidgetBlueprintEditor;
};
