// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "MovieScene.h"
#include "UMGMovieSceneObjectManager.h"
#include "Animation/WidgetAnimation.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditor.h"


#define LOCTEXT_NAMESPACE "UUMGBindingManager"


/* UUMGBindingManager structors
 *****************************************************************************/

UUMGMovieSceneObjectManager::UUMGMovieSceneObjectManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WidgetAnimation(nullptr)
	, WidgetBlueprintEditor(nullptr)
{ }


/* UUMGBindingManager interface
 *****************************************************************************/

void UUMGMovieSceneObjectManager::InitPreviewObjects()
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
			IdToPreviewObjects.Add(Binding.AnimationId, FoundObject);
			PreviewObjectToIds.Add(FoundObject, Binding.AnimationId);
		}
		else
		{
			IdToSlotContentPreviewObjects.Add(Binding.AnimationId, FoundSlot->Content);
			SlotContentPreviewObjectToIds.Add(FoundSlot->Content, Binding.AnimationId);
		}
	}
}


void UUMGMovieSceneObjectManager::BindWidgetBlueprintEditor(FWidgetBlueprintEditor& InWidgetBlueprintEditor, UWidgetAnimation& InAnimation)
{
	WidgetAnimation = &InAnimation;
	WidgetBlueprintEditor = &InWidgetBlueprintEditor;
	WidgetBlueprintEditor->GetOnWidgetPreviewUpdated().AddUObject(this, &UUMGMovieSceneObjectManager::HandleWidgetPreviewUpdated);
}


void UUMGMovieSceneObjectManager::UnbindWidgetBlueprintEditor()
{
	if (WidgetBlueprintEditor != nullptr)
	{
		WidgetBlueprintEditor->GetOnWidgetPreviewUpdated().RemoveAll(this);
	}
}



/* IMovieSceneObjectManager interface
 *****************************************************************************/

bool UUMGMovieSceneObjectManager::AllowsSpawnableObjects() const
{
	return false;
}


void UUMGMovieSceneObjectManager::BindPossessableObject(const FGuid& ObjectId, UObject& PossessedObject)
{
	if (WidgetAnimation == nullptr)
	{
		return;
	}

	UPanelSlot* PossessedSlot = Cast<UPanelSlot>(&PossessedObject);

	if ((PossessedSlot != nullptr) && (PossessedSlot->Content != nullptr))
	{
		SlotContentPreviewObjectToIds.Add(PossessedSlot->Content, ObjectId);
		IdToSlotContentPreviewObjects.Add(ObjectId, PossessedSlot->Content);

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
		PreviewObjectToIds.Add(&PossessedObject, ObjectId);
		IdToPreviewObjects.Add(ObjectId, &PossessedObject);

		FWidgetAnimationBinding NewBinding;
		{
			NewBinding.AnimationId = ObjectId;
			NewBinding.WidgetName = PossessedObject.GetFName();
		}

		WidgetAnimation->AnimationBindings.Add(NewBinding);
	}
}


bool UUMGMovieSceneObjectManager::CanPossessObject(UObject& Object) const
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


FGuid UUMGMovieSceneObjectManager::FindObjectId(const UMovieScene& MovieScene, UObject& Object) const
{
	UPanelSlot* Slot = Cast<UPanelSlot>(&Object);

	if (Slot != nullptr)
	{
		// slot guids are tracked by their content.
		return PreviewObjectToIds.FindRef(Slot->Content);
	}

	return PreviewObjectToIds.FindRef(&Object);
}


UObject* UUMGMovieSceneObjectManager::FindObject(const FGuid& ObjectId) const
{
	TWeakObjectPtr<UObject> PreviewObject = IdToPreviewObjects.FindRef(ObjectId);

	if (PreviewObject.IsValid())
	{
		return PreviewObject.Get();
	}

	TWeakObjectPtr<UObject> SlotContentPreviewObject = IdToSlotContentPreviewObjects.FindRef(ObjectId);

	if (SlotContentPreviewObject.IsValid())
	{
		return SlotContentPreviewObject.Get();
	}
	
	return nullptr;
}


bool UUMGMovieSceneObjectManager::TryGetObjectDisplayName(const FGuid& ObjectId, FText& OutDisplayName) const
{
	// TODO: This gets called every frame for every bound object and could
	// be a potential performance issue for a really complicated animation.

	TWeakObjectPtr<UObject> PreviewObject = IdToPreviewObjects.FindRef(ObjectId);

	if (PreviewObject.IsValid())
	{
		OutDisplayName = FText::FromString(PreviewObject.Get()->GetName());
		return true;
	}

	TWeakObjectPtr<UObject> SlotContentPreviewObject = IdToSlotContentPreviewObjects.FindRef(ObjectId);

	if (SlotContentPreviewObject.IsValid())
	{
		UWidget* SlotContent = Cast<UWidget>(SlotContentPreviewObject.Get());
		FText PanelName = SlotContent->Slot != nullptr && SlotContent->Slot->Parent != nullptr
			? FText::FromString(SlotContent->Slot->Parent->GetName())
			: LOCTEXT("InvalidPanel", "Invalid Panel");
		FText ContentName = FText::FromString(SlotContent->GetName());
		OutDisplayName = FText::Format(LOCTEXT("SlotObject", "{0} ({1} Slot)"), ContentName, PanelName);

		return true;
	}

	return false;
}


void UUMGMovieSceneObjectManager::UnbindPossessableObjects(const FGuid& ObjectId)
{
	// unbind widgets
	TArray<TWeakObjectPtr<UObject>> PreviewObjects;
	IdToPreviewObjects.MultiFind(ObjectId, PreviewObjects);

	for (TWeakObjectPtr<UObject>& PreviewObject : PreviewObjects)
	{
		PreviewObjectToIds.Remove(PreviewObject);
	}

	IdToPreviewObjects.Remove(ObjectId);

	// unbind slots
	TArray<TWeakObjectPtr<UObject>> SlotContentPreviewObjects;
	IdToSlotContentPreviewObjects.MultiFind(ObjectId, SlotContentPreviewObjects);

	for (TWeakObjectPtr<UObject>& SlotContentPreviewObject : SlotContentPreviewObjects)
	{
		SlotContentPreviewObjectToIds.Remove(SlotContentPreviewObject);
	}

	IdToSlotContentPreviewObjects.Remove(ObjectId);

	// remove animation bindings
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

void UUMGMovieSceneObjectManager::HandleWidgetPreviewUpdated()
{
	PreviewObjectToIds.Empty();
	IdToPreviewObjects.Empty();
	IdToSlotContentPreviewObjects.Empty();
	SlotContentPreviewObjectToIds.Empty();

	InitPreviewObjects();

	if (WidgetBlueprintEditor != nullptr)
	{
		WidgetBlueprintEditor->GetSequencer()->UpdateRuntimeInstances();
	}
}


#undef LOCTEXT_NAMESPACE
