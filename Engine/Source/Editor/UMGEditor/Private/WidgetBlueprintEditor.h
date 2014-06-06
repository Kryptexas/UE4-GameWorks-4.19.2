// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/Kismet/Public/BlueprintEditor.h"

#include "PreviewScene.h"

class ISequencer;

/**
 * widget blueprint editor (extends Blueprint editor)
 */
class FWidgetBlueprintEditor : public FBlueprintEditor
{
public:
	FWidgetBlueprintEditor();

	virtual ~FWidgetBlueprintEditor();

	void InitWidgetBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode);

	virtual void Tick(float DeltaTime) OVERRIDE;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged) OVERRIDE;

	/** FGCObjectInterface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	class UWidgetBlueprint* GetWidgetBlueprintObj() const;

	UUserWidget* GetPreview() const;

	/** @return The sequencer used to create widget animations */
	TSharedPtr<ISequencer>& GetSequencer();

	void SelectWidgets(TArray<UWidget*> Widgets);

private:
	void OnBlueprintChanged(UBlueprint* InBlueprint);

	void DestroyPreview();
	void UpdatePreview(UBlueprint* InBlueprint, bool bInForceFullUpdate);

	/**
	 * Gets the default movie scene which is used when there is no 
	 * animation data present on the widget blueprint
	 */
	UMovieScene* GetDefaultMovieScene();

private:
	FPreviewScene PreviewScene;

	/** Sequencer for creating and previewing widget animations */
	TSharedPtr<ISequencer> Sequencer;

	/** The Blueprint associated with the current preview */
	UBlueprint* PreviewBlueprint;

	/** Default movie scene for new animations */
	UMovieScene* DefaultMovieScene;

	TArray<UWidget*> SelectedPreviewWidgets;

	mutable TWeakObjectPtr<UUserWidget> PreviewWidgetActorPtr;
};
