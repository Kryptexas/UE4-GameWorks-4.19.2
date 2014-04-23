// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetBlueprintGeneratedClass

UWidgetBlueprintGeneratedClass::UWidgetBlueprintGeneratedClass(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UWidgetBlueprintGeneratedClass::CreateComponentsForActor(AActor* Actor) const
{
	Super::CreateComponentsForActor(Actor);

	// Duplicate the graph, keeping track of what was duplicated
	TMap<UObject*, UObject*> DuplicatedObjectList;

	TArray<USlateWrapperComponent*> ClonedWidgets;

	// Iterate over each timeline template
	for ( const USlateWrapperComponent* Template : WidgetTemplates )
	{
		FObjectDuplicationParameters Parameters(const_cast<USlateWrapperComponent*>( Template ), Actor);
		Parameters.CreatedObjects = &DuplicatedObjectList;

		USlateWrapperComponent* ClonedWidget = CastChecked<USlateWrapperComponent>(StaticDuplicateObjectEx(Parameters));
		ClonedWidgets.Add(ClonedWidget);
	}

	// Replace references to old classes and instances on this object with the corresponding new ones
	for ( USlateWrapperComponent* Widget : ClonedWidgets )
	{
		FArchiveReplaceObjectRef<UObject> ReplaceInCDOAr(Widget, DuplicatedObjectList, /*bNullPrivateRefs=*/ false, /*bIgnoreOuterRef=*/ false, /*bIgnoreArchetypeRef=*/ false);
	}

	// Iterate over each timeline template
	for ( USlateWrapperComponent* Widget : ClonedWidgets )
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



		//USlateWrapperComponent* Widget = ConstructObject<USlateWrapperComponent>(Template->GetClass(), Actor, NewName);
		Widget->bCreatedByConstructionScript = true; // Indicate it comes from a blueprint so it gets cleared when we rerun construction scripts
		Actor->SerializedComponents.Add(Widget); // Add to array so it gets saved
		Widget->SetNetAddressable();	// This component has a stable name that can be referenced for replication

		// Find property with the same name as the template and assign the new Timeline to it
		UClass* ActorClass = Actor->GetClass();
		FName thing = Widget->GetClass()->GetFName();
		//UTimelineTemplate::TimelineTemplateNameToVariableName(Template->GetFName())
		UObjectPropertyBase* Prop = FindField<UObjectPropertyBase>(ActorClass, thing);
		if ( Prop )
		{
			Prop->SetObjectPropertyValue_InContainer(Actor, Widget);
		}

		Widget->RegisterComponent();
	}
}

#undef LOCTEXT_NAMESPACE