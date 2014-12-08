// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintUtilities.h"
#include "LatentActions.h"
#include "DeferRegisterComponents.h"
#include "ComponentInstanceDataCache.h"
#include "Engine/LevelScriptActor.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h"
#endif
#include "Engine/SimpleConstructionScript.h"

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
		UActorComponent* Component = PreviouslyAttachedComponents[i];
		if (Component)
		{
			bool bDestroyComponent = false;
			if (Component->bCreatedByConstructionScript)
			{
				bDestroyComponent = true;
			}
			else
			{
				UActorComponent* OuterComponent = Component->GetTypedOuter<UActorComponent>();
				while (OuterComponent)
				{
					if (OuterComponent->bCreatedByConstructionScript)
					{
						bDestroyComponent = true;
						break;
					}
					OuterComponent = OuterComponent->GetTypedOuter<UActorComponent>();
				}
			}

			if (bDestroyComponent)
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
}

void AActor::RerunConstructionScripts()
{
	FEditorScriptExecutionGuard ScriptGuard;
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
		// Set global flag to let system know we are reconstructing blueprint instances
		TGuardValue<bool> GuardTemplateNameFlag(GIsReconstructingBlueprintInstances, true);

		// Temporarily suspend the undo buffer; we don't need to record reconstructed component objects into the current transaction
		ITransaction* CurrentTransaction = GUndo;
		GUndo = NULL;
		
		// Create cache to store component data across rerunning construction scripts
#if WITH_EDITOR
		FActorTransactionAnnotation* ActorTransactionAnnotation = CurrentTransactionAnnotation.Get();
#endif
		FComponentInstanceDataCache* InstanceDataCache;
		
		FTransform OldTransform = FTransform::Identity;
		FName  SocketName;
		AActor* Parent = NULL;
		USceneComponent* ParentComponent = NULL;

		bool bUseRootComponentProperties = true;

		// Struct to store info about attached actors
		struct FAttachedActorInfo
		{
			AActor* AttachedActor;
			FName AttachedToSocket;
			bool bSetRelativeTransform;
			FTransform RelativeTransform;
		};

		// Save info about attached actors
		TArray<FAttachedActorInfo> AttachedActorInfos;

#if WITH_EDITOR
		if (ActorTransactionAnnotation)
		{
			InstanceDataCache = &ActorTransactionAnnotation->ComponentInstanceData;

			if (ActorTransactionAnnotation->bRootComponentDataCached)
			{
				OldTransform = ActorTransactionAnnotation->RootComponentData.Transform;
				Parent = ActorTransactionAnnotation->RootComponentData.AttachedParentInfo.Actor.Get();
				if (Parent)
				{
					DetachRootComponentFromParent();
					SocketName = ActorTransactionAnnotation->RootComponentData.AttachedParentInfo.SocketName;
				}

				for (const auto& CachedAttachInfo : ActorTransactionAnnotation->RootComponentData.AttachedToInfo)
				{
					AActor* AttachedActor = CachedAttachInfo.Actor.Get();
					if (AttachedActor)
					{
						FAttachedActorInfo Info;
						Info.AttachedActor = AttachedActor;
						Info.AttachedToSocket = CachedAttachInfo.SocketName;
						Info.bSetRelativeTransform = true;
						Info.RelativeTransform = CachedAttachInfo.RelativeTransform;
						AttachedActorInfos.Add(Info);

						AttachedActor->DetachRootComponentFromParent();
					}
				}

				bUseRootComponentProperties = false;
			}
		}
		else
#endif
		{
			InstanceDataCache = new FComponentInstanceDataCache(this);

			// If there are attached objects detach them and store the socket names
			TArray<AActor*> AttachedActors;
			GetAttachedActors(AttachedActors);

			for (AActor* AttachedActor : AttachedActors)
			{
				USceneComponent* EachRoot = AttachedActor->GetRootComponent();
				// If the component we are attached to is about to go away...
				if (EachRoot && EachRoot->AttachParent && EachRoot->AttachParent->bCreatedByConstructionScript)
				{
					// Save info about actor to reattach
					FAttachedActorInfo Info;
					Info.AttachedActor = AttachedActor;
					Info.AttachedToSocket = EachRoot->AttachSocketName;
					Info.bSetRelativeTransform = false;
					AttachedActorInfos.Add(Info);

					// Now detach it
					AttachedActor->Modify();
					EachRoot->DetachFromParent(true);
				}
			}
		}

		if (bUseRootComponentProperties && RootComponent != nullptr)
		{
			// Do not need to detach if root component is not going away
			if (RootComponent->AttachParent != NULL && RootComponent->bCreatedByConstructionScript)
			{
				Parent = RootComponent->AttachParent->GetOwner();
				// Root component should never be attached to another component in the same actor!
				if (Parent == this)
				{
					UE_LOG(LogActor, Warning, TEXT("RerunConstructionScripts: RootComponent (%s) attached to another component in this Actor (%s)."), *RootComponent->GetPathName(), *Parent->GetPathName());
					Parent = NULL;
				}
				ParentComponent = RootComponent->AttachParent;
				SocketName = RootComponent->AttachSocketName;
				//detach it to remove any scaling 
				RootComponent->DetachFromParent(true);
			}

			OldTransform = RootComponent->ComponentToWorld;
			OldTransform.SetTranslation(RootComponent->GetComponentLocation()); // take into account any custom location
		}

		// Destroy existing components
		DestroyConstructedComponents();

		// Reset random streams
		ResetPropertiesForConstruction();

		// Exchange net roles before running construction scripts
		UWorld *OwningWorld = GetWorld();
		if (OwningWorld && !OwningWorld->IsServer())
		{
			ExchangeNetRoles(true);
		}

		// Run the construction scripts
		ExecuteConstruction(OldTransform, InstanceDataCache);

		if(Parent)
		{
			USceneComponent* ChildRoot = GetRootComponent();
			if (ParentComponent == NULL)
			{
				ParentComponent = Parent->GetRootComponent();
			}
			if (ChildRoot != NULL && ParentComponent != NULL)
			{
				ChildRoot->AttachTo(ParentComponent, SocketName, EAttachLocation::KeepWorldPosition);
			}
		}

		// If we had attached children reattach them now - unless they are already attached
		for(FAttachedActorInfo& Info : AttachedActorInfos)
		{
			// If this actor is no longer attached to anything, reattach
			if (!Info.AttachedActor->IsPendingKill() && Info.AttachedActor->GetAttachParentActor() == NULL)
			{
				USceneComponent* ChildRoot = Info.AttachedActor->GetRootComponent();
				if (ChildRoot && ChildRoot->AttachParent != RootComponent)
				{
					ChildRoot->AttachTo(RootComponent, Info.AttachedToSocket, EAttachLocation::KeepWorldPosition);
					if (Info.bSetRelativeTransform)
					{
						ChildRoot->SetRelativeTransform(Info.RelativeTransform);
					}
					ChildRoot->UpdateComponentToWorld();
				}
			}
		}

		// Restore the undo buffer
		GUndo = CurrentTransaction;

#if WITH_EDITOR
		if (ActorTransactionAnnotation)
		{
			CurrentTransactionAnnotation = NULL;
		}
		else
#endif
		{
			delete InstanceDataCache;
		}

	}
}

void AActor::ExecuteConstruction(const FTransform& Transform, const FComponentInstanceDataCache* InstanceDataCache, bool bIsDefaultTransform)
{
	check(!IsPendingKill());
	check(!HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed));

	// ensure that any existing native root component gets this new transform
	// we can skip this in the default case as the given transform will be the root component's transform
	if (RootComponent && !bIsDefaultTransform)
	{
		RootComponent->SetWorldTransform(Transform);
	}

	// Generate the parent blueprint hierarchy for this actor, so we can run all the construction scripts sequentially
	TArray<const UBlueprintGeneratedClass*> ParentBPClassStack;
	const bool bErrorFree = UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(GetClass(), ParentBPClassStack);

	// If this actor has a blueprint lineage, go ahead and run the construction scripts from least derived to most
	if( (ParentBPClassStack.Num() > 0)  )
	{
		if( bErrorFree )
		{
			// Prevent user from spawning actors in User Construction Script
			TGuardValue<bool> AutoRestoreISCS(GetWorld()->bIsRunningConstructionScript, true);
			for( int32 i = ParentBPClassStack.Num() - 1; i >= 0; i-- )
			{
				const UBlueprintGeneratedClass* CurrentBPGClass = ParentBPClassStack[i];
				check(CurrentBPGClass);
				if(CurrentBPGClass->SimpleConstructionScript)
				{
					CurrentBPGClass->SimpleConstructionScript->ExecuteScriptOnActor(this, Transform, bIsDefaultTransform);
				}
				// Now that the construction scripts have been run, we can create timelines and hook them up
				CurrentBPGClass->CreateComponentsForActor(this);
			}

			// If we passed in cached data, we apply it now, so that the UserConstructionScript can use the updated values
			if(InstanceDataCache)
			{
				InstanceDataCache->ApplyToActor(this);
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

			// Apply any cached data procedural components
			// @TODO Don't re-apply to components we already applied to above
			if (InstanceDataCache)
			{
				InstanceDataCache->ApplyToActor(this);
			}
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

	// Now run virtual notification
	OnConstruction(Transform);
}

void FDeferRegisterComponents::DeferComponentRegistration(AActor* Actor, UActorComponent* Component)
{
	TArray<UActorComponent*>* ActorComponentsToRegister = ComponentsToRegister.Find(Actor);
	if (!ActorComponentsToRegister)
	{
		ActorComponentsToRegister = &ComponentsToRegister.Add(Actor, TArray<UActorComponent*>());
	}
	
	ActorComponentsToRegister->Add(Component);
}

void FDeferRegisterComponents::RegisterComponents(AActor* Actor)
{
	TArray<UActorComponent*>* ActorComponentsToRegister = ComponentsToRegister.Find(Actor);
	if (ActorComponentsToRegister)
	{
		for (int32 ComponentIndex = 0; ComponentIndex < ActorComponentsToRegister->Num(); ++ComponentIndex)
		{
			UActorComponent* Component = (*ActorComponentsToRegister)[ComponentIndex];
			
			Component->RegisterComponent();
		}
		ComponentsToRegister.Remove(Actor);
	}
}

// FGCObject interface
void FDeferRegisterComponents::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (TMap<AActor*, TArray<UActorComponent*> >::TIterator It(ComponentsToRegister); It; ++It)
	{
		Collector.AddReferencedObject(It.Key());
		TArray<UActorComponent*>& Components = It.Value();
		for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ++ComponentIndex)
		{
			Collector.AddReferencedObject(Components[ComponentIndex]);
		}
	}
}

/* Gets the FDeferRegisterComponents singleton. */
FDeferRegisterComponents& FDeferRegisterComponents::Get()
{
	static FDeferRegisterComponents Singleton;
	return Singleton;
}

void AActor::ProcessUserConstructionScript()
{
	// Set a flag that this actor is currently running UserConstructionScript.
	bRunningUserConstructionScript = true;
	UserConstructionScript();
	bRunningUserConstructionScript = false;
}

void AActor::FinishAndRegisterComponent(UActorComponent* Component)
{
	Component->RegisterComponent();
	SerializedComponents.Add(Component);
}

UActorComponent* AActor::CreateComponentFromTemplate(UActorComponent* Template, const FString& InName)
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
	UActorComponent* Template = nullptr;
	UBlueprintGeneratedClass* BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>((ComponentTemplateContext != nullptr) ? ComponentTemplateContext->GetClass() : GetClass());
	while(BlueprintGeneratedClass != nullptr)
	{
		Template = BlueprintGeneratedClass->FindComponentTemplateByName(TemplateName);
		if(nullptr != Template)
		{
			break;
		}
		BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(BlueprintGeneratedClass->GetSuperClass());
	}

	UActorComponent* NewActorComp = CreateComponentFromTemplate(Template);
	if(NewActorComp != nullptr)
	{
		// The user has the option of doing attachment manually where they have complete control or via the automatic rule
		// that the first component added becomes the root component, with subsequent components attached to the root.
		USceneComponent* NewSceneComp = Cast<USceneComponent>(NewActorComp);
		if(NewSceneComp != nullptr)
		{
			if (!bManualAttachment)
			{
				if (RootComponent == nullptr)
				{
					RootComponent = NewSceneComp;
				}
				else
				{
					// The root component can't be more mobile than its children, so we check for that here and adjust as needed before attaching.
					if(RootComponent->Mobility > NewSceneComp->Mobility)
					{
						if(NewSceneComp->IsA<UStaticMeshComponent>())
						{
							// SMCs can't be stationary, so always set them to be movable
							NewSceneComp->SetMobility(EComponentMobility::Movable);
						}
						else
						{
							// Set the new component to be at least as mobile as the root
							NewSceneComp->SetMobility(RootComponent->Mobility);
						}
					}

					NewSceneComp->AttachTo(RootComponent);
				}
			}

			NewSceneComp->SetRelativeTransform(RelativeTransform);
		}

		// Call function to notify component it has been created
		NewActorComp->OnComponentCreated();

		// Register component, which will create physics/rendering state, now component is in correct position
		NewActorComp->RegisterComponent();
	}

	return NewActorComp;
}



