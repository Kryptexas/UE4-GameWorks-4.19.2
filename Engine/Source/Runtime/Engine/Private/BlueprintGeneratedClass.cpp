// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintUtilities.h"

#if WITH_EDITOR
#include "BlueprintEditorUtils.h"
#include "KismetEditorUtilities.h"
#endif //WITH_EDITOR


UBlueprintGeneratedClass::UBlueprintGeneratedClass(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	NumReplicatedProperties = 0;
}

void UBlueprintGeneratedClass::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// Default__BlueprintGeneratedClass uses its own AddReferencedObjects function.
		ClassAddReferencedObjects = &UBlueprintGeneratedClass::AddReferencedObjects;
	}
}

void UBlueprintGeneratedClass::PostLoad()
{
	Super::PostLoad();

	// Make sure the class CDO has been generated
	UObject* ClassCDO = GetDefaultObject();

	// Go through the CDO of the class, and make sure we don't have any legacy components that aren't instanced hanging on.
	TArray<UObject*> SubObjects;
	GetObjectsWithOuter(ClassCDO, SubObjects);

	for( auto SubObjIt = SubObjects.CreateIterator(); SubObjIt; ++SubObjIt )
	{
		UObject* CurrObj = *SubObjIt;
		if( !CurrObj->IsDefaultSubobject() && !CurrObj->IsRooted() )
		{
			CurrObj->MarkPendingKill();
		}
	}

	if (GetLinkerUE4Version() < VER_UE4_CLASS_NOTPLACEABLE_ADDED)
	{
		// Make sure the placeable flag is correct for all blueprints
		if (CastChecked<UBlueprint>(ClassGeneratedBy)->BlueprintType != BPTYPE_MacroLibrary)
		{
			ClassFlags &= ~CLASS_NotPlaceable;
		}
	}
}

UClass* UBlueprintGeneratedClass::GetAuthoritativeClass()
{
	UBlueprint* GeneratingBP = CastChecked<UBlueprint>(ClassGeneratedBy);

	check(GeneratingBP);

	return (GeneratingBP->GeneratedClass != NULL) ? GeneratingBP->GeneratedClass : this;
}

#if WITH_EDITOR
void UBlueprintGeneratedClass::ConditionalRecompileClass()
{
	if( UBlueprint* GeneratingBP = Cast<UBlueprint>(ClassGeneratedBy) )
	{
		if( !FBlueprintEditorUtils::IsDataOnlyBlueprint(GeneratingBP) 
			&& !FBlueprintEditorUtils::IsInterfaceBlueprint(GeneratingBP)
			&& (GeneratingBP->SkeletonGeneratedClass != this) )
		{
			const bool bWasRegenerating = GeneratingBP->bIsRegeneratingOnLoad;
			GeneratingBP->bIsRegeneratingOnLoad = true;

			// Make sure that nodes are up to date, so that we get any updated blueprint signatures
			FBlueprintEditorUtils::RefreshExternalBlueprintDependencyNodes(GeneratingBP);
			
			if(GeneratingBP->Status != BS_Error)
			{
				FKismetEditorUtilities::RecompileBlueprintBytecode(GeneratingBP);
			}
						
			GeneratingBP->bIsRegeneratingOnLoad = bWasRegenerating;
		}
	}
}
#endif //WITH_EDITOR

bool UBlueprintGeneratedClass::IsFunctionImplementedInBlueprint(FName InFunctionName) const
{
	UFunction* Function = FindFunctionByName(InFunctionName);
	return Function && Function->GetOuter() && Function->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass());
}

UDynamicBlueprintBinding* UBlueprintGeneratedClass::GetDynamicBindingObject(UClass* Class) const
{
	UDynamicBlueprintBinding* DynamicBindingObject = NULL;
	for (int32 Index = 0; Index < DynamicBindingObjects.Num(); ++Index)
	{
		if (DynamicBindingObjects[Index]->GetClass() == Class)
		{
			DynamicBindingObject = DynamicBindingObjects[Index];
			break;
		}
	}
	return DynamicBindingObject;
}


void UBlueprintGeneratedClass::BindDynamicDelegates(AActor* InInstance)
{
	if(!InInstance->IsA(this))
	{
		UE_LOG(LogBlueprint, Warning, TEXT("BindComponentDelegates: '%s' is not an instance of '%s'."), *InInstance->GetName(), *GetName());
		return;
	}

	for (int32 Index = 0; Index < DynamicBindingObjects.Num(); ++Index)
	{
		DynamicBindingObjects[Index]->BindDynamicDelegates(InInstance);
	}

	// call on super class, if it's a BlueprintGeneratedClass
	UBlueprintGeneratedClass* BGClass = Cast<UBlueprintGeneratedClass>(SuperStruct);
	if(BGClass != NULL)
	{
		BGClass->BindDynamicDelegates(InInstance);
	}
}

bool UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(const UClass* InClass, TArray<const UBlueprintGeneratedClass*>& OutBPGClasses)
{
	OutBPGClasses.Empty();
	bool bNoErrors = true;
	while(const UBlueprintGeneratedClass* BPGClass = Cast<const UBlueprintGeneratedClass>(InClass))
	{
#if WITH_EDITORONLY_DATA
		const UBlueprint* BP = Cast<const UBlueprint>(BPGClass->ClassGeneratedBy);
		bNoErrors &= (NULL != BP) && (BP->Status != BS_Error);
#endif
		OutBPGClasses.Add(BPGClass);
		InClass = BPGClass->GetSuperClass();
	}
	return bNoErrors;
}

UActorComponent* UBlueprintGeneratedClass::FindComponentTemplateByName(const FName& TemplateName)
{
	for(int32 i=0; i<ComponentTemplates.Num(); i++)
	{
		UActorComponent* Template = ComponentTemplates[i];
		if(Template != NULL && Template->GetFName() == TemplateName)
		{
			return Template;
		}
	}

	return NULL;
}

void UBlueprintGeneratedClass::CreateTimelinesForActor(AActor* Actor) const
{
	check(Actor != NULL);
	check(!Actor->IsTemplate());
	check(!Actor->IsPendingKill());

	// Iterate over each timeline template
	for(int32 i=0; i<Timelines.Num(); i++)
	{
		const UTimelineTemplate* TimelineTemplate = Timelines[i];

		// Not fatal if NULL, but shouldn't happen
		if(!ensure(TimelineTemplate != NULL))
		{
			continue;
		}

		FName NewName = *(FString::Printf(TEXT("TimelineComp__%d"), Actor->SerializedComponents.Num() ) );
		UTimelineComponent* NewTimeline = NewNamedObject<UTimelineComponent>(Actor, NewName);
		NewTimeline->bCreatedByConstructionScript = true; // Indicate it comes from a blueprint so it gets cleared when we rerun construction scripts
		Actor->SerializedComponents.Add(NewTimeline); // Add to array so it gets saved
		NewTimeline->SetNetAddressable();	// This component has a stable name that can be referenced for replication

		NewTimeline->SetPropertySetObject(Actor); // Set which object the timeline should drive properties on
		NewTimeline->SetDirectionPropertyName(TimelineTemplate->GetDirectionPropertyName());

		NewTimeline->SetTimelineLength(TimelineTemplate->TimelineLength); // copy length
		NewTimeline->SetTimelineLengthMode(TimelineTemplate->LengthMode);

		// Find property with the same name as the template and assign the new Timeline to it
		UClass* ActorClass = Actor->GetClass();
		UObjectPropertyBase* Prop = FindField<UObjectPropertyBase>( ActorClass, *UTimelineTemplate::TimelineTemplateNameToVariableName(TimelineTemplate->GetFName()) );
		if(Prop)
		{
			Prop->SetObjectPropertyValue_InContainer(Actor, NewTimeline);
		}

		// Event tracks
		// In the template there is a track for each function, but in the runtime Timeline each key has its own delegate, so we fold them together
		for(int32 TrackIdx=0; TrackIdx<TimelineTemplate->EventTracks.Num(); TrackIdx++)
		{
			const FTTEventTrack* EventTrackTemplate = &TimelineTemplate->EventTracks[TrackIdx];
			if(EventTrackTemplate->CurveKeys != NULL)
			{
				// Create delegate for all keys in this track
				FScriptDelegate EventDelegate;
				EventDelegate.SetObject(Actor);
				EventDelegate.SetFunctionName(TimelineTemplate->GetEventTrackFunctionName(TrackIdx));

				// Create an entry in Events for each key of this track
				for (auto It(EventTrackTemplate->CurveKeys->FloatCurve.GetKeyIterator()); It; ++It)
				{
					NewTimeline->AddEvent(It->Time, FOnTimelineEvent(EventDelegate));
				}
			}
		}

		// Float tracks
		for(int32 TrackIdx=0; TrackIdx<TimelineTemplate->FloatTracks.Num(); TrackIdx++)
		{
			const FTTFloatTrack* FloatTrackTemplate = &TimelineTemplate->FloatTracks[TrackIdx];
			if(FloatTrackTemplate->CurveFloat != NULL)
			{
				NewTimeline->AddInterpFloat(FloatTrackTemplate->CurveFloat, FOnTimelineFloat(), TimelineTemplate->GetTrackPropertyName(FloatTrackTemplate->TrackName));
			}
		}

		// Vector tracks
		for(int32 TrackIdx=0; TrackIdx<TimelineTemplate->VectorTracks.Num(); TrackIdx++)
		{
			const FTTVectorTrack* VectorTrackTemplate = &TimelineTemplate->VectorTracks[TrackIdx];
			if(VectorTrackTemplate->CurveVector != NULL)
			{
				NewTimeline->AddInterpVector(VectorTrackTemplate->CurveVector, FOnTimelineVector(), TimelineTemplate->GetTrackPropertyName(VectorTrackTemplate->TrackName));
			}
		}

		// Linear color tracks
		for(int32 TrackIdx=0; TrackIdx<TimelineTemplate->LinearColorTracks.Num(); TrackIdx++)
		{
			const FTTLinearColorTrack* LinearColorTrackTemplate = &TimelineTemplate->LinearColorTracks[TrackIdx];
			if(LinearColorTrackTemplate->CurveLinearColor != NULL)
			{
				NewTimeline->AddInterpLinearColor(LinearColorTrackTemplate->CurveLinearColor, FOnTimelineLinearColor(), TimelineTemplate->GetTrackPropertyName(LinearColorTrackTemplate->TrackName));
			}
		}

		// Set up delegate that gets called after all properties are updated
		FScriptDelegate UpdateDelegate;
		UpdateDelegate.SetObject(Actor);
		UpdateDelegate.SetFunctionName(TimelineTemplate->GetUpdateFunctionName());
		NewTimeline->SetTimelinePostUpdateFunc(FOnTimelineEvent(UpdateDelegate));

		// Set up finished delegate that gets called after all properties are updated
		FScriptDelegate FinishedDelegate;
		FinishedDelegate.SetObject(Actor);
		FinishedDelegate.SetFunctionName(TimelineTemplate->GetFinishedFunctionName());
		NewTimeline->SetTimelineFinishedFunc(FOnTimelineEvent(FinishedDelegate));

		NewTimeline->RegisterComponent();

		// Start playing now, if desired
		if(TimelineTemplate->bAutoPlay)
		{
			// Needed for autoplay timelines in cooked builds, since they won't have Activate() called via the Play call below
			NewTimeline->bAutoActivate = true;
			NewTimeline->Play();
		}

		// Set to loop, if desired
		if(TimelineTemplate->bLoop)
		{
			NewTimeline->SetLooping(true);
		}

		// Set replication, if desired
		if(TimelineTemplate->bReplicated)
		{
			NewTimeline->SetIsReplicated(true);
		}
	}
}
