// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/Kismet/Public/BlueprintEditor.h"

#include "PreviewScene.h"

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
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged) OVERRIDE;

	class UWidgetBlueprint* GetWidgetBlueprintObj() const;

	UUserWidget* GetPreview() const;

private:
	void OnBlueprintChanged(UBlueprint* InBlueprint);

	void DestroyPreview();
	void UpdatePreview(UBlueprint* InBlueprint, bool bInForceFullUpdate);

private:
	FPreviewScene PreviewScene;

	/** The Blueprint associated with the current preview */
	UBlueprint* PreviewBlueprint;

	mutable TWeakObjectPtr<UUserWidget> PreviewWidgetActorPtr;
};
