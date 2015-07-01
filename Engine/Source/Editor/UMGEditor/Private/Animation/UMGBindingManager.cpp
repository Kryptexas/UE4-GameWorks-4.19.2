// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "MovieScene.h"
#include "MovieSceneInstance.h"
#include "UMGBindingManager.h"
#include "Animation/WidgetAnimation.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditor.h"


#define LOCTEXT_NAMESPACE "UUMGBindingManager"


/* UUMGBindingManager structors
 *****************************************************************************/

UUMGBindingManager::UUMGBindingManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WidgetAnimation(nullptr)
	, WidgetBlueprintEditor(nullptr)
{ }


/* UUMGBindingManager interface
 *****************************************************************************/

void UUMGBindingManager::InitPreviewObjects()
{
	if ((WidgetAnimation == nullptr) || (WidgetBlueprintEditor == nullptr))
	{
		return;
	}

	// Populate preview object to guid map
	UUserWidget* PreviewWidget = WidgetBlueprintEditor->GetPreview();

	if (PreviewWidget == nullptr)
	{
		return;
	}

	UWidgetTree* WidgetTree = PreviewWidget->WidgetTree;

	for (const FWidgetAnimationBinding& Binding : WidgetAnimation->AnimationBindings)
	{
		UObject* FoundObject = Binding.FindRuntimeObject(*WidgetTree);

		if (FoundObject == nullptr)
		{
			continue;
		}

		UPanelSlot* FoundSlot = Cast<UPanelSlot>(FoundObject);

		if (FoundSlot == nullptr)
		{
			ObjectIdToPreviewObjects.Add(Binding.AnimationId, FoundObject);
			PreviewObjectToId.Add(FoundObject, Binding.AnimationId);
		}
		else
		{
			ObjectIdToSlotContentPreviewObjects.Add(Binding.AnimationId, FoundSlot->Content);
			SlotContentPreviewObjectToId.Add(FoundSlot->Content, Binding.AnimationId);
		}
	}
}


void UUMGBindingManager::BindWidgetBlueprintEditor(FWidgetBlueprintEditor& InWidgetBlueprintEditor, UWidgetAnimation& InAnimation)
{
	WidgetAnimation = &InAnimation;
	WidgetBlueprintEditor = &InWidgetBlueprintEditor;
	WidgetBlueprintEditor->GetOnWidgetPreviewUpdated().AddUObject(this, &UUMGBindingManager::HandleWidgetPreviewUpdated);
}


void UUMGBindingManager::UnbindWidgetBlueprintEditor()
{
	if (WidgetBlueprintEditor != nullptr)
	{
		WidgetBlueprintEditor->GetOnWidgetPreviewUpdated().RemoveAll(this);
	}
}



/* IMovieSceneBindingManager interface
 *****************************************************************************/

bool UUMGBindingManager::AllowsSpawnableObjects() const
{
	return false;
}


void UUMGBindingManager::BindPossessableObject(const FMovieSceneObjectId& ObjectId, UObject& PossessedObject)
{
	if (WidgetAnimation == nullptr)
	{
		return;
	}

	UPanelSlot* PossessedSlot = Cast<UPanelSlot>(&PossessedObject);

	if ((PossessedSlot != nullptr) && (PossessedSlot->Content != nullptr))
	{
		SlotContentPreviewObjectToId.Add(PossessedSlot->Content, ObjectId);
		ObjectIdToSlotContentPreviewObjects.Add(ObjectId, PossessedSlot->Content);

		// Save the name of the widget containing the slots. This is the object
		// to look up that contains the slot itself (the thing we are animating).
		FWidgetAnimationBinding NewBinding;
		{
			NewBinding.AnimationId = ObjectId;
			NewBinding.SlotWidgetName = PossessedSlot->GetFName();
			NewBinding.WidgetName = PossessedSlot->Content->GetFName();
		}

		WidgetAnimation->AnimationBindings.Add(NewBinding);
	}
	else if (PossessedSlot == nullptr)
	{
		PreviewObjectToId.Add(&PossessedObject, ObjectId);
		ObjectIdToPreviewObjects.Add(ObjectId, &PossessedObject);

		FWidgetAnimationBinding NewBinding;
		{
			NewBinding.AnimationId = ObjectId;
			NewBinding.WidgetName = PossessedObject.GetFName();
		}

		WidgetAnimation->AnimationBindings.Add(NewBinding);
	}
}


bool UUMGBindingManager::CanPossessObject(UObject& Object) const
{
	if (WidgetBlueprintEditor == nullptr)
	{
		return false;
	}

	UPanelSlot* Slot = Cast<UPanelSlot>(&Object);

	if ((Slot != nullptr) && (Slot->Content == nullptr))
	{
		// can't possess empty slots.
		return false;
	}

	UWidgetBlueprint* WidgetBlueprint = WidgetBlueprintEditor->GetWidgetBlueprintObj();
	UUserWidget* PreviewWidget = WidgetBlueprintEditor->GetPreview();

	return (Object.IsA<UVisual>() && Object.IsIn(PreviewWidget));
}


FMovieSceneObjectId UUMGBindingManager::FindIdForObject(const UMovieScene& MovieScene, UObject& Object) const
{
	UPanelSlot* Slot = Cast<UPanelSlot>(&Object);

	if (Slot != nullptr)
	{
		// slot guids are tracked by their content.
		return PreviewObjectToId.FindRef(Slot->Content);
	}

	return PreviewObjectToId.FindRef(&Object);
}


void UUMGBindingManager::GetRuntimeObjects(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, TArray<UObject*>& OutRuntimeObjects) const
{
	TArray<TWeakObjectPtr<UObject>> PreviewObjects;
	ObjectIdToPreviewObjects.MultiFind(ObjectId, PreviewObjects);

	for (TWeakObjectPtr<UObject>& PreviewObject : PreviewObjects)
	{
		OutRuntimeObjects.Add(PreviewObject.Get());
	}

	TArray<TWeakObjectPtr<UObject>> SlotContentPreviewObjects;
	ObjectIdToSlotContentPreviewObjects.MultiFind(ObjectId, SlotContentPreviewObjects);

	for (TWeakObjectPtr<UObject>& SlotContentPreviewObject : SlotContentPreviewObjects)
	{
		UWidget* ContentWidget = Cast<UWidget>(SlotContentPreviewObject.Get());
		OutRuntimeObjects.Add(ContentWidget->Slot);
	}
}


bool UUMGBindingManager::TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FMovieSceneObjectId& ObjectId, FText& OutDisplayName) const
{
	// TODO: This gets called every frame for every bound object and could be a potential performance issue for a really complicated animation.
	TArray<TWeakObjectPtr<UObject>> BindingObjects;
	ObjectIdToPreviewObjects.MultiFind(ObjectId, BindingObjects);
	
	TArray<TWeakObjectPtr<UObject>> SlotContentBindingObjects;
	ObjectIdToSlotContentPreviewObjects.MultiFind(ObjectId, SlotContentBindingObjects);
	
	if ((BindingObjects.Num() == 0) && (SlotContentBindingObjects.Num() == 0))
	{
		OutDisplayName = LOCTEXT("NoBoundObjects", "No bound objects");
	}
	else if (BindingObjects.Num() + SlotContentBindingObjects.Num() > 1)
	{
		OutDisplayName = LOCTEXT("Multiple bound objects", "Multilple bound objects");
	}
	else if (BindingObjects.Num() == 1)
	{
		OutDisplayName = FText::FromString(BindingObjects[0].Get()->GetName());
	}
	else // SlotContentBindingObjects.Num() == 1
	{
		UWidget* SlotContent = Cast<UWidget>(SlotContentBindingObjects[0].Get());
		FText PanelName = SlotContent->Slot != nullptr && SlotContent->Slot->Parent != nullptr
			? FText::FromString(SlotContent->Slot->Parent->GetName())
			: LOCTEXT("InvalidPanel", "Invalid Panel");
		FText ContentName = FText::FromString(SlotContent->GetName());
		OutDisplayName = FText::Format(LOCTEXT("SlotObject", "{0} ({1} Slot)"), ContentName, PanelName);
	}

	return true;
}


void UUMGBindingManager::UnbindPossessableObjects(const FMovieSceneObjectId& ObjectId)
{
	TArray<TWeakObjectPtr<UObject>> PreviewObjects;
	ObjectIdToPreviewObjects.MultiFind(ObjectId, PreviewObjects);

	for (TWeakObjectPtr<UObject>& PreviewObject : PreviewObjects)
	{
		PreviewObjectToId.Remove(PreviewObject);
	}

	ObjectIdToPreviewObjects.Remove(ObjectId);

	TArray<TWeakObjectPtr<UObject>> SlotContentPreviewObjects;
	ObjectIdToSlotContentPreviewObjects.MultiFind(ObjectId, SlotContentPreviewObjects);

	for (TWeakObjectPtr<UObject>& SlotContentPreviewObject : SlotContentPreviewObjects)
	{
		SlotContentPreviewObjectToId.Remove(SlotContentPreviewObject);
	}

	ObjectIdToSlotContentPreviewObjects.Remove(ObjectId);

	if ((WidgetAnimation != nullptr) && (WidgetBlueprintEditor != nullptr))
	{
		UWidgetBlueprint* WidgetBlueprint = WidgetBlueprintEditor->GetWidgetBlueprintObj();

		WidgetAnimation->Modify();
		WidgetAnimation->AnimationBindings.RemoveAll([&](const FWidgetAnimationBinding& Binding) {
			return Binding.AnimationId == ObjectId;
		});
	}
}


/* UUMGBindingManager event handlers
 *****************************************************************************/

void UUMGBindingManager::HandleWidgetPreviewUpdated()
{
	PreviewObjectToId.Empty();
	ObjectIdToPreviewObjects.Empty();
	ObjectIdToSlotContentPreviewObjects.Empty();
	SlotContentPreviewObjectToId.Empty();

	InitPreviewObjects();

	if (WidgetBlueprintEditor != nullptr)
	{
		WidgetBlueprintEditor->GetSequencer()->UpdateRuntimeInstances();
	}
}


#undef LOCTEXT_NAMESPACE
