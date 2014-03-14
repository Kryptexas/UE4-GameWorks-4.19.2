// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintUtilities.h"
#include "LatentActions.h"
#include "EngineLevelScriptClasses.h"
#include "DeferRegisterStaticComponents.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h"
#endif

DEFINE_LOG_CATEGORY(LogBlueprintUserMessages);

//////////////////////////////////////////////////////////////////////////
// AActor Blueprint Stuff

static TArray<FRandomStream*> FindRandomStreams(AActor* InActor)
{
	check(InActor);
	TArray<FRandomStream*> OutStreams;
	UScriptStruct* RandomStreamStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("RandomStream"));
	for( TFieldIterator<UStructProperty> It(InActor->GetClass()) ; It ; ++It )
	{
		UStructProperty* StructProp = *It;
		if( StructProp->Struct == RandomStreamStruct )
		{
			FRandomStream* StreamPtr = StructProp->ContainerPtrToValuePtr<FRandomStream>(InActor);
			OutStreams.Add(StreamPtr);
		}
	}
	return OutStreams;
}

#if WITH_EDITOR
void AActor::SeedAllRandomStreams()
{
	TArray<FRandomStream*> Streams = FindRandomStreams(this);
	for(int32 i=0; i<Streams.Num(); i++)
	{
		Streams[i]->GenerateNewSeed();
	}
}
#endif //WITH_EDITOR

void AActor::ResetPropertiesForConstruction()
{
	// Get class CDO
	AActor* Default = GetClass()->GetDefaultObject<AActor>();
	// RandomStream struct name to compare against
	const FName RandomStreamName(TEXT("RandomStream"));

	// We don't want to reset references to world object
	const bool bIsLevelScriptActor = IsA(ALevelScriptActor::StaticClass());

	// Iterate over properties
	for( TFieldIterator<UProperty> It(GetClass()) ; It ; ++It )
	{
		UProperty* Prop = *It;
		UStructProperty* StructProp = Cast<UStructProperty>(Prop);
		UClass* PropClass = CastChecked<UClass>(Prop->GetOuter()); // get the class that added this property

		// First see if it is a random stream, if so reset before running construction script
		if( (StructProp != NULL) && (StructProp->Struct != NULL) && (StructProp->Struct->GetFName() == RandomStreamName) )
		{
			FRandomStream* StreamPtr =  StructProp->ContainerPtrToValuePtr<FRandomStream>(this);
			StreamPtr->Reset();
		}
		// If it is a blueprint added variable that is not editable per-instance, reset to default before running construction script
		else if( !bIsLevelScriptActor 
				&& Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance)
				&& PropClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint) 
				&& !Prop->IsA(UDelegateProperty::StaticClass()) 
				&& !Prop->IsA(UMulticastDelegateProperty::StaticClass()) )
		{
			Prop->CopyCompleteValue_InContainer(this, Default);
		}
	}
}

//* Destroys the constructed components.
void AActor::DestroyConstructedComponents()
{
	// Remove all existing components
	TArray<UActorComponent*> PreviouslyAttachedComponents;
	GetComponents(PreviouslyAttachedComponents);
	for (int32 i = 0; i < PreviouslyAttachedComponents.Num(); i++)
	{
		UActorComponent*& Component = PreviouslyAttachedComponents[i];
		if (Component && Component->bCreatedByConstructionScript)
		{
			if (Component == RootComponent)
			{
				RootComponent = NULL;
			}

			Component->DestroyComponent();

			// Rename component to avoid naming conflicts in the case where we rerun the SCS and name the new components the same way.
			FName const NewBaseName( *(FString::Printf(TEXT("TRASH_%s"), *Component->GetClass()->GetName())) );
			FName const NewObjectName = MakeUniqueObjectName(this, GetClass(), NewBaseName);
			Component->Rename(*NewObjectName.ToString(), this, REN_ForceNoResetLoaders);
		}
	}
}

void AActor::RerunConstructionScripts()
{
	// don't allow (re)running construction scripts on dying actors
	bool bAllowReconstruction = !IsPendingKill() && !HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);
#if WITH_EDITOR
	if(bAllowReconstruction && GIsEditor)
	{
		// Generate the blueprint hierarchy for this actor
		TArray<UBlueprint*> ParentBPStack;
		bAllowReconstruction = UBlueprint::GetBlueprintHierarchyFromClass(GetClass(), ParentBPStack);
		if(bAllowReconstruction)
		{
			for(int i = ParentBPStack.Num() - 1; i > 0 && bAllowReconstruction; --i)
			{
				const UBlueprint* ParentBP = ParentBPStack[i];
				if(ParentBP && ParentBP->bBeingCompiled)
				{
					// don't allow (re)running construction scripts if a parent BP is being compiled
					bAllowReconstruction = false;
				}
			}
		}
	}
#endif
	if(bAllowReconstruction)
	{
		// Temporarily suspend the undo buffer; we don't need to record reconstructed component objects into the current transaction
		ITransaction* CurrentTransaction = GUndo;
		GUndo = NULL;
		
		// Create cache to store component data across rerunning construction scripts
		FComponentInstanceDataCache InstanceDataCache(this);

		// If there are attached objects detach them and store the socket names
		TArray<AActor*> AttachedActors;
		GetAttachedActors(AttachedActors);

		// Struct to store info about attached actors
		struct FAttachedActorInfo
		{
			AActor* AttachedActor;
			FName AttachedToSocket;
		};

		// Save info about attached actors
		TArray<FAttachedActorInfo> AttachedActorInfos;
		for( AActor* AttachedActor : AttachedActors)
		{
			USceneComponent* EachRoot = AttachedActor->GetRootComponent();
			// If the component we are attached to is about to go away...
			if( EachRoot && EachRoot->AttachParent && EachRoot->AttachParent->bCreatedByConstructionScript )
			{
				// Save info about actor to reattach
				FAttachedActorInfo Info;
				Info.AttachedActor = AttachedActor;
				Info.AttachedToSocket = EachRoot->AttachSocketName;
				AttachedActorInfos.Add(Info);

				// Now detach it
				AttachedActor->Modify();
				EachRoot->DetachFromParent(true);					
			}
		}

		// Save off original pose of the actor
		FTransform OldTransform = FTransform::Identity;
		FName  SocketName;
		AActor* Parent = NULL;
		if (RootComponent != NULL)
		{
			// Do not need to detach if root component is not going away
			if(RootComponent->AttachParent != NULL && RootComponent->bCreatedByConstructionScript)
			{
				Parent = RootComponent->AttachParent->GetOwner();
				// Root component should never be attached to another component in the same actor!
				if(Parent == this)
				{
					UE_LOG(LogActor, Warning, TEXT("RerunConstructionScripts: RootComponent (%s) attached to another component in this Actor (%s)."), *RootComponent->GetPathName(), *Parent->GetPathName());
					Parent = NULL;
				}

				SocketName = RootComponent->AttachSocketName;
				//detach it to remove any scaling 
				RootComponent->DetachFromParent(true);
			}
			OldTransform = RootComponent->ComponentToWorld;
		}

		// Destroy existing components
		DestroyConstructedComponents();

		// Reset random streams
		ResetPropertiesForConstruction();

		// Run the construction scripts
		OnConstruction(OldTransform);

		if(Parent)
		{
			USceneComponent* ChildRoot =	GetRootComponent();
			USceneComponent* ParentRoot =	Parent->GetRootComponent();
			if(ChildRoot != NULL && ParentRoot != NULL)
			{
				ChildRoot->AttachTo( ParentRoot, SocketName, EAttachLocation::KeepWorldPosition );
			}
		}

		// Apply per-instance data.
		InstanceDataCache.ApplyToActor(this);

		// If we had attached children reattach them now - unless they are already attached
		for(FAttachedActorInfo& Info : AttachedActorInfos)
		{
			// If this actor is no longer attached to anything, reattach
			if (Info.AttachedActor->GetAttachParentActor() == NULL)
			{
				USceneComponent* ChildRoot = Info.AttachedActor->GetRootComponent();
				if (ChildRoot && ChildRoot->AttachParent != RootComponent)
				{
					ChildRoot->AttachTo(RootComponent, Info.AttachedToSocket, EAttachLocation::KeepWorldPosition);
					ChildRoot->UpdateComponentToWorld();
				}
			}
		}

		// Restore the undo buffer
		GUndo = CurrentTransaction;
	}
}

void AActor::OnConstruction(const FTransform& Transform)
{
	check(!IsPendingKill());
	check(!HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed));


	// Generate the parent blueprint hierarchy for this actor, so we can run all the construction scripts sequentially
	TArray<const UBlueprintGeneratedClass*> ParentBPClassStack;
	const bool bErrorFree = UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(GetClass(), ParentBPClassStack);

	// If this actor has a blueprint lineage, go ahead and run the construction scripts from least derived to most
	if( (ParentBPClassStack.Num() > 0)  )
	{
		if( bErrorFree )
		{
			FEditorScriptExecutionGuard ScriptGuard;
			// Prevent user from spawning actors in User Construction Script
			TGuardValue<bool> AutoRestoreISCS(GetWorld()->bIsRunningConstructionScript, true);
			for( int32 i = ParentBPClassStack.Num() - 1; i >= 0; i-- )
			{
				const UBlueprintGeneratedClass* CurrentBPGClass = ParentBPClassStack[i];
				check(CurrentBPGClass);
				if(CurrentBPGClass->SimpleConstructionScript)
				{
					CurrentBPGClass->SimpleConstructionScript->ExecuteScriptOnActor(this, Transform);
				}
				// Now that the construction scripts have been run, we can create timelines and hook them up
				CurrentBPGClass->CreateTimelinesForActor(this);
			}

#if WITH_EDITOR
			bool bDoUserConstructionScript;
			GConfig->GetBool(TEXT("Kismet"), TEXT("bTurnOffEditorConstructionScript"), bDoUserConstructionScript, GEngineIni);
			if (!GIsEditor || !bDoUserConstructionScript)
#endif
			{
				// Then run the user script, which is responsible for calling its own super, if desired
				ProcessUserConstructionScript();
			}

			// Bind any delegates on components			
			((UBlueprintGeneratedClass*)GetClass())->BindDynamicDelegates(this); // We have a BP stack, we must have a UBlueprintGeneratedClass...
		}
		else
		{
			// Disaster recovery mode; create a dummy billboard component to retain the actor location
			// until the compile error can be fixed
			if (RootComponent == NULL)
			{
				UBillboardComponent* BillboardComponent = NewObject<UBillboardComponent>(this);
				BillboardComponent->SetFlags(RF_Transactional);
				BillboardComponent->bCreatedByConstructionScript = true;
#if WITH_EDITOR
				BillboardComponent->Sprite = (UTexture2D*)(StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("/Engine/EditorResources/BadBlueprintSprite.BadBlueprintSprite"), NULL, LOAD_None, NULL));
#endif
				BillboardComponent->SetRelativeTransform(Transform);

				SetRootComponent(BillboardComponent);
				FinishAndRegisterComponent(BillboardComponent);
			}
		}
	}
}


void FDeferRegisterStaticComponents::DeferStaticComponent(AActor* Actor, USceneComponent* Component, EComponentMobility::Type OriginalMobility)
{
	TArray<FDeferredComponentInfo>* ActorComponentsToRegister = ComponentsToRegister.Find(Actor);
	if (!ActorComponentsToRegister)
	{
		ActorComponentsToRegister = &ComponentsToRegister.Add(Actor, TArray<FDeferredComponentInfo>());
	}
	ActorComponentsToRegister->Add(FDeferredComponentInfo(Component, OriginalMobility));
	// Make sure Mobility is temporarily set to EComponentMobility::Movable.
	Component->Mobility = EComponentMobility::Movable;
}

void FDeferRegisterStaticComponents::RegisterComponents(AActor* Actor)
{
	TArray<FDeferredComponentInfo>* ActorComponentsToRegister = ComponentsToRegister.Find(Actor);
	if (ActorComponentsToRegister)
	{
		for (int32 ComponentIndex = 0; ComponentIndex < ActorComponentsToRegister->Num(); ++ComponentIndex)
		{
			const FDeferredComponentInfo& SavedInfo = (*ActorComponentsToRegister)[ComponentIndex];
			USceneComponent* Component = SavedInfo.Component;
			// Restore saved mobility just before registering this component.
			Component->Mobility = SavedInfo.SavedMobility;
			Component->RegisterComponent();
		}
		ComponentsToRegister.Remove(Actor);
	}
}

// FGCObject interface
void FDeferRegisterStaticComponents::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (TMap<AActor*, TArray<FDeferredComponentInfo> >::TIterator It(ComponentsToRegister); It; ++It)
	{
		Collector.AddReferencedObject(It.Key());
		TArray<FDeferredComponentInfo>& Components = It.Value();
		for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ++ComponentIndex)
		{
			Collector.AddReferencedObject(Components[ComponentIndex].Component);
		}
	}
}

/* Gets the FDeferRegisterStaticComponents singleton. */
FDeferRegisterStaticComponents& FDeferRegisterStaticComponents::Get()
{
	static FDeferRegisterStaticComponents Singleton;
	return Singleton;
}

void AActor::ProcessUserConstructionScript()
{
	// Set a flag that this actor is currently running UserConstructionScript.
	bRunningUserConstructionScript = true;
	UserConstructionScript();
	bRunningUserConstructionScript = false;

	// Register all deferred components for this actor.
	FDeferRegisterStaticComponents::Get().RegisterComponents(this);
}

void AActor::FinishAndRegisterComponent(UActorComponent* Component)
{
	Component->RegisterComponent();
	SerializedComponents.Add(Component);
}

UActorComponent* AActor::CreateComponentFromTemplate(UActorComponent* Template, const FString & InName)
{
	UActorComponent* NewActorComp = NULL;
	if(Template != NULL)
	{
		// Note we aren't copying the the RF_ArchetypeObject flag. Also note the result is non-transactional by default.
		NewActorComp = (UActorComponent*)StaticDuplicateObject(Template, this, *InName, RF_AllFlags & ~(RF_ArchetypeObject|RF_Transactional|RF_WasLoaded) );
		//NewActorComp = ConstructObject<UActorComponent>(Template->GetClass(), this, *InName, RF_NoFlags, Template);

		NewActorComp->bCreatedByConstructionScript = true;

		// Need to do this so component gets saved - Components array is not serialized
		SerializedComponents.Add(NewActorComp);
	}
	return NewActorComp;
}

UActorComponent* AActor::AddComponent(FName TemplateName, bool bManualAttachment, const FTransform& RelativeTransform, const UObject* ComponentTemplateContext)
{
	UActorComponent* Template = NULL;
	UBlueprintGeneratedClass* BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>((ComponentTemplateContext != NULL) ? ComponentTemplateContext->GetClass() : GetClass());
	while(BlueprintGeneratedClass != NULL)
	{
		Template = BlueprintGeneratedClass->FindComponentTemplateByName(TemplateName);
		if(NULL != Template)
		{
			break;
		}
		BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(BlueprintGeneratedClass->GetSuperClass());
	}

	UActorComponent* NewActorComp = CreateComponentFromTemplate(Template);
	if(NewActorComp != NULL)
	{
		// The user has the option of doing attachment manually where they have complete control or via the automatic rule
		// that the first component added becomes the root component, with subsequent components attached to the root.
		USceneComponent* NewSceneComp = Cast<USceneComponent>(NewActorComp);
		bool bDeferRegisterStaticComponent = false;
		EComponentMobility::Type OriginalMobility = EComponentMobility::Movable;

		if(NewSceneComp != NULL)
		{
			// Components with Mobility set to EComponentMobility::Static or EComponentMobility::Stationary can't be properly set up in UCS (all changes will be rejected
			// due to EComponentMobility::Static flag) so we're going to temporarily change the flag and defer the registration until UCS has finished.
			bDeferRegisterStaticComponent = bRunningUserConstructionScript && NewSceneComp->Mobility != EComponentMobility::Movable;
			OriginalMobility = NewSceneComp->Mobility;
			if (bDeferRegisterStaticComponent)
			{
				NewSceneComp->Mobility = EComponentMobility::Movable;
			}

			if (!bManualAttachment)
			{
				if (RootComponent == NULL)
				{
					RootComponent = NewSceneComp;
				}
				else
				{
					NewSceneComp->AttachTo(RootComponent);
				}
			}

			NewSceneComp->SetRelativeTransform(RelativeTransform);
		}

		// Call function to notify component it has been created
		NewActorComp->OnComponentCreated();

		if (bDeferRegisterStaticComponent)
		{
			// Defer registration until after UCS has completed.
			FDeferRegisterStaticComponents::Get().DeferStaticComponent(this, NewSceneComp, OriginalMobility);
		}
		else
		{
			// Register component, which will create physics/rendering state, now component is in correct position
			NewActorComp->RegisterComponent();
		}
	}

	return NewActorComp;
}



