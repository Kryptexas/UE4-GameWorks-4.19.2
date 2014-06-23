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
	DECLARE_MULTICAST_DELEGATE(FOnSelectedWidgetsChanged)

	/** Called after the widget preview has been updated */
	DECLARE_MULTICAST_DELEGATE(FOnWidgetPreviewUpdated)
public:
	FWidgetBlueprintEditor();

	virtual ~FWidgetBlueprintEditor();

	void InitWidgetBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode);

	virtual void Tick(float DeltaTime) override;
	virtual void NotifyPreChange(class FEditPropertyChain* PropertyAboutToChange) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged) override;

	/** FGCObjectInterface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;

	/** @return The widget blueprint currently being edited in this editor */
	class UWidgetBlueprint* GetWidgetBlueprintObj() const;

	/** @return The preview widget. */
	UUserWidget* GetPreview() const;

	/** @return The sequencer used to create widget animations */
	TSharedPtr<ISequencer>& GetSequencer();

	/** Sets the currently selected set of widgets */
	void SelectWidgets(const TSet<FWidgetReference>& Widgets);

	/** @return The selected set of widgets */
	const TSet<FWidgetReference>& GetSelectedWidgets() const;

	/** @return Notification for when the preview widget has been updated */
	FOnWidgetPreviewUpdated& GetOnWidgetPreviewUpdated() { return OnWidgetPreviewUpdated; }

public:
	/** Fires whenever the selected set of widgets changes */
	FOnSelectedWidgetsChanged OnSelectedWidgetsChanged;

	/** Command list for handling widget actions in the WidgetBlueprintEditor */
	TSharedPtr< FUICommandList > WidgetCommandList;

	/** Paste Metadata */
	FVector2D PasteDropLocation;

private:
	bool CanDeleteSelectedWidgets();
	void DeleteSelectedWidgets();

	bool CanCopySelectedWidgets();
	void CopySelectedWidgets();

	bool CanPasteWidgets();
	void PasteWidgets();

private:
	/** Updates the inspector to be viewing the currently selected set of widgets */
	void RefreshDetails();

	/** Called whenever the blueprint is structurally changed. */
	virtual void OnBlueprintChanged(UBlueprint* InBlueprint) override;

	/** Destroy the current preview GUI object */
	void DestroyPreview();

	/** Tick the current preview GUI object */
	void UpdatePreview(UBlueprint* InBlueprint, bool bInForceFullUpdate);

	/** Migrate a property change from the preview GUI to the template GUI. */
	void MigrateFromChain(FEditPropertyChain* PropertyThatChanged, bool bIsModify);

	/**
	 * Gets the default movie scene which is used when there is no 
	 * animation data present on the widget blueprint
	 */
	UMovieScene* GetDefaultMovieScene();

private:
	/** The preview scene that owns the preview GUI */
	FPreviewScene PreviewScene;

	/** Sequencer for creating and previewing widget animations */
	TSharedPtr<ISequencer> Sequencer;

	/** The Blueprint associated with the current preview */
	UBlueprint* PreviewBlueprint;

	/** Default movie scene for new animations */
	UMovieScene* DefaultMovieScene;

	/** The currently selected preview widgets in the preview GUI */
	TSet<FWidgetReference> SelectedWidgets;

	/** The preview GUI object */
	mutable TWeakObjectPtr<UUserWidget> PreviewWidgetActorPtr;
	
	/** Notification for when the preview widget has been updated  */
	FOnWidgetPreviewUpdated OnWidgetPreviewUpdated;
};
