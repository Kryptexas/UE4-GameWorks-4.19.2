// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObjectGlobals.h"
#include "Package.h"
#include "UObject/SoftObjectPtr.h"
#include "SubclassOf.h"
#include "PersonaPreviewSceneController.h"
#include "PersonaPreviewSceneDescription.generated.h"

class UPreviewMeshCollection;
class USkeletalMesh;
class UDataAsset;
class UPreviewMeshCollection;

UCLASS()
class UPersonaPreviewSceneDescription : public UObject
{
public:
	GENERATED_BODY()

	// The method by which the preview is animated
	UPROPERTY(EditAnywhere, BlueprintReadOnly, NoClear, Category = "Animation")
	class TSubclassOf<UPersonaPreviewSceneController> PreviewController;

	UPROPERTY()
	UPersonaPreviewSceneController* PreviewControllerInstance;

	UPROPERTY()
	TArray<UPersonaPreviewSceneController*> PreviewControllerInstances;

	/** The preview mesh to use */
	UPROPERTY(EditAnywhere, Category = "Mesh", meta=(DisplayThumbnail=true))
	TSoftObjectPtr<USkeletalMesh> PreviewMesh;

	UPROPERTY(EditAnywhere, Category = "Additional Meshes")
	TSoftObjectPtr<UDataAsset> AdditionalMeshes;

	UPROPERTY()
	UPreviewMeshCollection* DefaultAdditionalMeshes;

	// Sets the current preview controller for the scene (handles uninitializing and initializing controllers)
	bool SetPreviewController(UClass* PreviewControllerClass, class IPersonaPreviewScene* PreviewScene)
	{
		if (!PreviewControllerClass->HasAnyClassFlags(CLASS_Abstract) &&
			PreviewControllerClass->IsChildOf(UPersonaPreviewSceneController::StaticClass()) &&
			(!PreviewControllerInstance || PreviewControllerClass != PreviewControllerInstance->GetClass()))
		{
			PreviewController = PreviewControllerClass;
			if (PreviewControllerInstance)
			{
				PreviewControllerInstance->UninitializeView(this, PreviewScene);
			}
			PreviewControllerInstance = GetControllerForClass(PreviewControllerClass);
			PreviewControllerInstance->InitializeView(this, PreviewScene);
			return true;
		}
		return false;
	}

private:
	// Gets created controller for the requested class (or creates one if none exists)
	UPersonaPreviewSceneController* GetControllerForClass(UClass* PreviewControllerClass)
	{
		for (UPersonaPreviewSceneController* Controller : PreviewControllerInstances)
		{
			if (Controller->GetClass() == PreviewControllerClass)
			{
				return Controller;
			}
		}

		UPersonaPreviewSceneController* NewController = NewObject<UPersonaPreviewSceneController>(GetTransientPackage(), PreviewControllerClass);
		PreviewControllerInstances.Add(NewController);
		return NewController;
	}
};
