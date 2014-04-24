// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetBlueprintGeneratedClass

UWidgetBlueprintGeneratedClass::UWidgetBlueprintGeneratedClass(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	WidgetTree = ConstructObject<UWidgetTree>(UWidgetTree::StaticClass(), this);
}

void UWidgetBlueprintGeneratedClass::CreateComponentsForActor(AActor* Actor) const
{
	Super::CreateComponentsForActor(Actor);

	// Duplicate the graph, keeping track of what was duplicated
	TMap<UObject*, UObject*> DuplicatedObjectList;

	FObjectDuplicationParameters Parameters(const_cast<UWidgetTree*>( WidgetTree ), Actor);
	Parameters.CreatedObjects = &DuplicatedObjectList;

	UWidgetTree* ClonedTree = CastChecked<UWidgetTree>(StaticDuplicateObjectEx(Parameters));

	AUserWidget* WidgetActor = CastChecked<AUserWidget>(Actor);

	int32 i = 0;
	for ( USlateWrapperComponent* Widget : ClonedTree->WidgetTemplates )
	{
		// Not fatal if NULL, but shouldn't happen
		if ( !ensure(Widget != NULL) )
		{
			continue;
		}

		FName NewName = *( FString::Printf(TEXT("WidgetComponent__%d"), Actor->SerializedComponents.Num()) );

		////Serialize object properties using write/read operations.
		//TArray<uint8> SavedProperties;
		//FObjectWriter Writer(ComponentTemplate, SavedProperties);
		//FObjectReader(ClonedComponent, SavedProperties);


		FString VariableName = Widget->GetClass()->GetName() + TEXT("_") + FString::FromInt(i);


		//USlateWrapperComponent* Widget = ConstructObject<USlateWrapperComponent>(Template->GetClass(), Actor, NewName);
		Widget->bCreatedByConstructionScript = true; // Indicate it comes from a blueprint so it gets cleared when we rerun construction scripts
		Actor->SerializedComponents.Add(Widget); // Add to array so it gets saved
		Widget->SetNetAddressable();	// This component has a stable name that can be referenced for replication

		// Find property with the same name as the template and assign the new Timeline to it
		UClass* ActorClass = Actor->GetClass();
		//UTimelineTemplate::TimelineTemplateNameToVariableName(Template->GetFName())
		UObjectPropertyBase* Prop = FindField<UObjectPropertyBase>(ActorClass, *VariableName);
		if ( Prop )
		{
			Prop->SetObjectPropertyValue_InContainer(Actor, Widget);
		}

		Widget->RegisterComponent();

#if WITH_EDITOR
		Widget->ConnectEditorData();
#endif
	}
}

#undef LOCTEXT_NAMESPACE