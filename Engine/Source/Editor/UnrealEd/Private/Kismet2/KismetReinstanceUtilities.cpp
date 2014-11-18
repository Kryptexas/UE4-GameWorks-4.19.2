// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "BlueprintEditorUtils.h"
#include "Layers/ILayers.h"
#include "ComponentInstanceDataCache.h"
#include "Engine/DynamicBlueprintBinding.h"

DEFINE_STAT(EKismetReinstancerStats_ReplaceInstancesOfClass);
DEFINE_STAT(EKismetReinstancerStats_FindReferencers);
DEFINE_STAT(EKismetReinstancerStats_ReplaceReferences);
DEFINE_STAT(EKismetReinstancerStats_UpdateBytecodeReferences);
DEFINE_STAT(EKismetReinstancerStats_RecompileChildClasses);
DEFINE_STAT(EKismetReinstancerStats_ReplaceClassNoReinsancing);

/////////////////////////////////////////////////////////////////////////////////
// FBlueprintCompileReinstancer

FBlueprintCompileReinstancer::FBlueprintCompileReinstancer(UClass* InClassToReinstance, bool bIsBytecodeOnly, bool bSkipGC)
	: ClassToReinstance(InClassToReinstance)
	, DuplicatedClass(NULL)
	, OriginalCDO(NULL)
	, bHasReinstanced(false)
	, bSkipGarbageCollection(bSkipGC)
	, ClassToReinstanceDefaultValuesCRC(0)
{
	if( InClassToReinstance != NULL )
	{
		SaveClassFieldMapping(InClassToReinstance);

		// Remember the initial CDO for the class being resinstanced
		OriginalCDO = ClassToReinstance->GetDefaultObject();

		// Duplicate the class we're reinstancing into the transient package.  We'll re-class all objects we find to point to this new class
		GIsDuplicatingClassForReinstancing = true;
		ClassToReinstance->ClassFlags |= CLASS_NewerVersionExists;
		const FName RenistanceName = MakeUniqueObjectName(GetTransientPackage(), ClassToReinstance->GetClass(), *FString::Printf(TEXT("REINST_%s"), *ClassToReinstance->GetName()));
		DuplicatedClass = (UClass*)StaticDuplicateObject(ClassToReinstance, GetTransientPackage(), *RenistanceName.ToString(), ~RF_Transactional); 
		// Add the class to the root set, so that it doesn't get prematurely garbage collected
		DuplicatedClass->AddToRoot();
		ClassToReinstance->ClassFlags &= ~CLASS_NewerVersionExists;
		GIsDuplicatingClassForReinstancing = false;

		// Bind and link the duplicate class, so that it has the proper duplicate property offsets
		DuplicatedClass->Bind();
		DuplicatedClass->StaticLink(true);

		// Copy over the ComponentNametoDefaultObjectMap, which tells CopyPropertiesForUnrelatedObjects which components are instanced and which aren't

		GIsDuplicatingClassForReinstancing = true;
		UObject* OldCDO = ClassToReinstance->GetDefaultObject();
		DuplicatedClass->ClassDefaultObject = (UObject*)StaticDuplicateObject(OldCDO, GetTransientPackage(), *DuplicatedClass->GetDefaultObjectName().ToString());
		GIsDuplicatingClassForReinstancing = false;

		DuplicatedClass->ClassDefaultObject->SetFlags(RF_ClassDefaultObject);
		DuplicatedClass->ClassDefaultObject->SetClass(DuplicatedClass);

		OldCDO->SetClass(DuplicatedClass);

		if( !bIsBytecodeOnly )
		{
			TArray<UObject*> ObjectsToChange;
			const bool bIncludeDerivedClasses = false;
			GetObjectsOfClass(ClassToReinstance, ObjectsToChange, bIncludeDerivedClasses);
			for ( auto ObjIt = ObjectsToChange.CreateConstIterator(); ObjIt; ++ObjIt )
			{
				(*ObjIt)->SetClass(DuplicatedClass);
			}

			TArray<UClass*> ChildrenOfClass;
			GetDerivedClasses(ClassToReinstance, ChildrenOfClass);
			for ( auto ClassIt = ChildrenOfClass.CreateConstIterator(); ClassIt; ++ClassIt )
			{
				UClass* ChildClass = *ClassIt;
				UBlueprint* ChildBP = Cast<UBlueprint>(ChildClass->ClassGeneratedBy);
				if (ChildBP)
				{
					if (ChildBP->HasAnyFlags(RF_BeingRegenerated))
					{
						if (ChildClass->GetSuperClass() == ClassToReinstance)
						{
							ReparentChild(ChildClass);
						}
					}
					// If this is a direct child, change the parent and relink so the property chain is valid for reinstancing
					else if( !ChildBP->HasAnyFlags(RF_NeedLoad) )
					{
						if( ChildClass->GetSuperClass() == ClassToReinstance )
						{
							ReparentChild(ChildBP);
						}

						Children.AddUnique(ChildBP);
					}
					else
					{
						// If this is a child that caused the load of their parent, relink to the REINST class so that we can still serialize in the CDO, but do not add to later processing
						ReparentChild(ChildClass);
					}
				}
			}
		}

		// Pull the blueprint that generated this reinstance target, and gather the blueprints that are dependent on it
		UBlueprint* GeneratingBP = Cast<UBlueprint>(ClassToReinstance->ClassGeneratedBy);
		check(GeneratingBP || GIsAutomationTesting);
		if(GeneratingBP)
		{
			ClassToReinstanceDefaultValuesCRC = GeneratingBP->CrcPreviousCompiledCDO;
			FBlueprintEditorUtils::GetDependentBlueprints(GeneratingBP, Dependencies);
		}
	}
}

void FBlueprintCompileReinstancer::SaveClassFieldMapping(UClass* InClassToReinstance)
{
	check(InClassToReinstance);

	for (UProperty* Prop = InClassToReinstance->PropertyLink; Prop && (Prop->GetOuter() == InClassToReinstance); Prop = Prop->PropertyLinkNext)
	{
		PropertyMap.Add(Prop->GetFName(), Prop);
	}

	for (auto Function : TFieldRange<UFunction>(InClassToReinstance, EFieldIteratorFlags::ExcludeSuper))
	{
		FunctionMap.Add(Function->GetFName(),Function);
	}
}

void FBlueprintCompileReinstancer::GenerateFieldMappings(TMap<UObject*, UObject*>& FieldMapping)
{
	check(ClassToReinstance);

	FieldMapping.Empty();

	for (auto& Prop : PropertyMap)
	{
		FieldMapping.Add(Prop.Value, FindField<UProperty>(ClassToReinstance, *Prop.Key.ToString()));
	}

	for (auto& Func : FunctionMap)
	{
		UFunction* NewFunction = ClassToReinstance->FindFunctionByName(Func.Key, EIncludeSuperFlag::ExcludeSuper);
		FieldMapping.Add(Func.Value, NewFunction);
	}

	UObject* NewCDO = ClassToReinstance->GetDefaultObject();
	FieldMapping.Add(OriginalCDO, NewCDO);
}

FBlueprintCompileReinstancer::~FBlueprintCompileReinstancer()
{
	// Remove the duplicated class from the root set, because it is no longer useful
	if( DuplicatedClass )
	{
		DuplicatedClass->RemoveFromRoot();
	}
}

void FBlueprintCompileReinstancer::ReinstanceObjects(bool bAlwaysReinstance)
{
	BP_SCOPED_COMPILER_EVENT_NAME(TEXT("Reinstance Objects"));

	// Make sure we only reinstance classes once!
	if( bHasReinstanced )
	{
		return;
	}
	bHasReinstanced = true;

	if (ClassToReinstance && DuplicatedClass)
	{
		bool bShouldReinstance = true;
		if (!bAlwaysReinstance)
		{
			BP_SCOPED_COMPILER_EVENT_STAT(EKismetReinstancerStats_ReplaceClassNoReinsancing);

			auto BPClassA = Cast<const UBlueprintGeneratedClass>(DuplicatedClass);
			auto BPClassB = Cast<const UBlueprintGeneratedClass>(ClassToReinstance);
			auto BP = Cast<const UBlueprint>(ClassToReinstance->ClassGeneratedBy);

			static const FBoolConfigValueHelper ChangeDefaultValueWithoutReinstancing(TEXT("Kismet"), TEXT("bChangeDefaultValueWithoutReinstancing"), GEngineIni);
			const bool bTheSameDefaultValues = BP && ClassToReinstanceDefaultValuesCRC && (BP->CrcPreviousCompiledCDO == ClassToReinstanceDefaultValuesCRC);
			const bool bTheSame = (ChangeDefaultValueWithoutReinstancing || bTheSameDefaultValues) && BPClassA && BPClassB && FStructUtils::TheSameLayout(BPClassA, BPClassB, true);
			if (bTheSame)
			{
				UE_LOG(LogBlueprint, Log, TEXT("BlueprintCompileReinstancer: class '%s' is replaced without reinstancing (the optimized way)."), *GetPathNameSafe(ClassToReinstance));

				TArray<UObject*> ObjectsToReplace;
				GetObjectsOfClass(DuplicatedClass, ObjectsToReplace, false);

				const bool bIsActor = ClassToReinstance->IsChildOf<AActor>();
				const bool bIsAnimInstance = ClassToReinstance->IsChildOf<UAnimInstance>();
				for (auto Obj : ObjectsToReplace)
				{
					if (!Obj->IsTemplate() && !Obj->IsPendingKill())
					{
						Obj->SetClass(ClassToReinstance);
						if (bIsActor)
						{
							auto Actor = CastChecked<AActor>(Obj);
							Actor->ReregisterAllComponents();
							Actor->RerunConstructionScripts();
						}

						if (bIsAnimInstance)
						{
							// Initialising the anim instance isn't enough to correctly set up the skeletal mesh again in a
							// paused world, need to initialise the skeletal mesh component that contains the anim instance.
							if (USkeletalMeshComponent* SkelComponent = Cast<USkeletalMeshComponent>(Obj->GetOuter()))
							{
								SkelComponent->InitAnim(true);
							}
						}
					}
				}

				bShouldReinstance = false;
			}
		}

		if (bShouldReinstance)
		{
			ReplaceInstancesOfClass(DuplicatedClass, ClassToReinstance, OriginalCDO);
		}
	
		{ 
			BP_SCOPED_COMPILER_EVENT_STAT(EKismetReinstancerStats_RecompileChildClasses);

			// Reparent all dependent blueprints, and recompile to ensure that they get reinstanced with the new memory layout
			for( auto ChildBP = Children.CreateIterator(); ChildBP; ++ChildBP)
			{
				UBlueprint* BP = *ChildBP;
				if( BP->ParentClass == ClassToReinstance || BP->ParentClass == DuplicatedClass)
				{
					ReparentChild(BP);

					FKismetEditorUtilities::CompileBlueprint(BP, false, bSkipGarbageCollection);
				}
			}
		}
	}
}

void FBlueprintCompileReinstancer::UpdateBytecodeReferences()
{
	BP_SCOPED_COMPILER_EVENT_STAT(EKismetReinstancerStats_UpdateBytecodeReferences);

	if(ClassToReinstance != NULL)
	{
		TMap<UObject*, UObject*> FieldMappings;
		GenerateFieldMappings(FieldMappings);

		for( auto DependentBP = Dependencies.CreateIterator(); DependentBP; ++DependentBP )
		{
			UClass* BPClass = (*DependentBP)->GeneratedClass;

			// Skip cases where the class is junk, or haven't finished serializing in yet
			if( (BPClass == ClassToReinstance)
				|| (BPClass->GetOutermost() == GetTransientPackage()) 
				|| BPClass->HasAnyClassFlags(CLASS_NewerVersionExists)
				|| (BPClass->ClassGeneratedBy && BPClass->ClassGeneratedBy->HasAnyFlags(RF_NeedLoad|RF_BeingRegenerated)) )
			{
				continue;
			}

			// For each function defined in this blueprint, run through the bytecode, and update any refs from the old properties to the new
			for( TFieldIterator<UFunction> FuncIter(BPClass, EFieldIteratorFlags::ExcludeSuper); FuncIter; ++FuncIter )
			{
				UFunction* CurrentFunction = *FuncIter;
				if( CurrentFunction->Script.Num() > 0 )
				{
					FArchiveReplaceObjectRef<UObject> ReplaceAr(CurrentFunction, FieldMappings, /*bNullPrivateRefs=*/ false, /*bIgnoreOuterRef=*/ true, /*bIgnoreArchetypeRef=*/ true);
				}
			}
		}
	}
}

void FBlueprintCompileReinstancer::ReplaceInstancesOfClass(UClass* OldClass, UClass* NewClass, UObject*	OriginalCDO)
{
	USelection* SelectedActors;
	bool bSelectionChanged = false;
	TArray<UObject*> ObjectsToReplace;
	const bool bLogConversions = false; // for debugging

	TMap<FStringAssetReference, UObject*> ReinstancedObjectsWeakReferenceMap;

	// Map of old objects to new objects
	TMap<UObject*, UObject*> OldToNewInstanceMap;
	TMap<UClass*, UClass*> OldToNewClassMap;
	OldToNewClassMap.Add(OldClass, NewClass);

	// Set global flag to let system know we are reconstructing blueprint instances
	TGuardValue<bool> GuardTemplateNameFlag(GIsReconstructingBlueprintInstances, true);

	{ BP_SCOPED_COMPILER_EVENT_STAT(EKismetReinstancerStats_ReplaceInstancesOfClass);

		const bool bIncludeDerivedClasses = false;
		GetObjectsOfClass(OldClass, ObjectsToReplace, bIncludeDerivedClasses);
	
		SelectedActors = GEditor->GetSelectedActors();
		SelectedActors->BeginBatchSelectOperation();
		SelectedActors->Modify();

		// Then fix 'real' (non archetype) instances of the class
		for(int32 i=0; i<ObjectsToReplace.Num(); i++)
		{
			UObject* OldObject = ObjectsToReplace[i];
			if (!OldObject->IsTemplate() && !OldObject->IsPendingKill())
			{
				AActor* OldActor = Cast<AActor>(OldObject);
				UObject* NewObject = NULL;
				UObject* OldBlueprintDebugObject = NULL;
				UBlueprint* CorrespondingBlueprint = NULL;
				// If this object is being debugged, cache it off so we can preserve the 'object being debugged' association
				CorrespondingBlueprint = Cast<UBlueprint>(OldObject->GetClass()->ClassGeneratedBy);
				if ( CorrespondingBlueprint )
				{
					if (CorrespondingBlueprint->GetObjectBeingDebugged() == OldObject)
					{
						OldBlueprintDebugObject = OldObject;
					}
				}
				if (OldActor != NULL)
				{
					FTransform WorldTransform = FTransform::Identity;
					FVector* LocationPtr = NULL;
					FVector Location = FVector::ZeroVector;
					FRotator* RotationPtr = NULL;
					FRotator Rotation = FRotator::ZeroRotator;
					AActor* AttachParent = NULL;
					USceneComponent* AttachParentComp = NULL;
					FName   AttachSocketName;

					// Struct to store info about attached actors
					struct FAttachedActorInfo
					{
						AActor* AttachedActor;
						FName AttachedToSocket;
					};

					// Save info about attached actors
					TArray<FAttachedActorInfo> AttachedActorInfos;

					// Create cache to store component data across rerunning construction scripts
					FComponentInstanceDataCache InstanceDataCache(OldActor);

					// If there are attached objects detach them and store the socket names
					TArray<AActor*> AttachedActors;
					OldActor->GetAttachedActors(AttachedActors);
					for (AActor* AttachedActor : AttachedActors)
					{
						USceneComponent* AttachedActorRoot = AttachedActor->GetRootComponent();
						if (AttachedActorRoot && AttachedActorRoot->AttachParent)
						{
							// Save info about actor to reattach
							FAttachedActorInfo Info;
							Info.AttachedActor = AttachedActor;
							Info.AttachedToSocket = AttachedActorRoot->AttachSocketName;
							AttachedActorInfos.Add(Info);

							// Now detach it
							AttachedActorRoot->DetachFromParent(true);
						}
					}

					if (USceneComponent* OldRootComponent = OldActor->GetRootComponent())
					{
						if(OldRootComponent->AttachParent != NULL)
						{
							AttachParent = OldRootComponent->AttachParent->GetOwner();
							// Root component should never be attached to another component in the same actor!
							if (AttachParent == OldActor)
							{
								UE_LOG(LogBlueprint, Warning, TEXT("ReplaceInstancesOfClass: RootComponent (%s) attached to another component in this Actor (%s)."), *OldRootComponent->GetPathName(), *AttachParent->GetPathName());
								AttachParent = NULL;
							}

							AttachParentComp = OldRootComponent->AttachParent;
							AttachSocketName = OldRootComponent->AttachSocketName;

							//detach it to remove any scaling
							OldRootComponent->DetachFromParent(true);
						}

						// Save off transform
						WorldTransform = OldRootComponent->ComponentToWorld;
						WorldTransform.SetTranslation(OldRootComponent->GetComponentLocation()); // take into account any custom location
						Location = OldActor->GetActorLocation();
						LocationPtr = &Location;
						Rotation = OldActor->GetActorRotation();
						RotationPtr = &Rotation;
					}

					// If this actor was spawned from an Archetype, we spawn the new actor from the new version of that archetype
					UObject* OldArchetype = OldActor->GetArchetype();
					UWorld* World = OldActor->GetWorld();
					AActor* NewArchetype = Cast<AActor>(OldToNewInstanceMap.FindRef(OldArchetype));

					// Check that either this was an instance of the class directly, or we found a new archetype for it
					check(OldArchetype == OldClass->GetDefaultObject() || NewArchetype);

					// Spawn the new actor instance, in the same level as the original, but deferring running the construction script until we have transferred modified properties
					ULevel* ActorLevel = OldActor->GetLevel();				
					UClass** MappedClass = OldToNewClassMap.Find(OldActor->GetClass());
					UClass* SpawnClass = MappedClass ? *MappedClass : NewClass;

					FActorSpawnParameters SpawnInfo;
					SpawnInfo.OverrideLevel = ActorLevel;
					SpawnInfo.Template = NewArchetype;
					SpawnInfo.bNoCollisionFail = true;
					SpawnInfo.bDeferConstruction = true;

					// Temporarily remove the deprecated flag so we can respawn the Blueprint in the level
					bool bIsClassDeprecated = SpawnClass->HasAnyClassFlags(CLASS_Deprecated);
					SpawnClass->ClassFlags &= ~CLASS_Deprecated;

					AActor* NewActor = World->SpawnActor(SpawnClass, LocationPtr, RotationPtr, SpawnInfo );

					// Reassign the deprecated flag if it was previously assigned
					if(bIsClassDeprecated)
					{
						SpawnClass->ClassFlags |= CLASS_Deprecated;
					}

					check(NewActor);
					NewObject = NewActor;

					ReinstancedObjectsWeakReferenceMap.Add(OldObject, NewObject);

					OldActor->DestroyConstructedComponents(); // don't want to serialize components from the old actor
					// Unregister native components so we don't copy any sub-components they generate for themselves (like UCameraComponent does)
					OldActor->UnregisterAllComponents(); 

					NewActor->UnregisterAllComponents(); // Unregister any native components, might have cached state based on properties we are going to overwrite

					UEditorEngine::CopyPropertiesForUnrelatedObjects(OldActor, NewActor);

					NewActor->ResetPropertiesForConstruction(); // reset properties/streams

					NewActor->RegisterAllComponents(); // Register native components

					// Because this is an editor context it's important to use this execution guard:
					FEditorScriptExecutionGuard ScriptGuard;

					// Run the construction script, which will use the properties we just copied over
					NewActor->ExecuteConstruction(WorldTransform, &InstanceDataCache);

					if (USceneComponent* NewRootComponent = NewActor->GetRootComponent())
					{
						//Attach the new instance to original parent
						if(AttachParent != nullptr)
						{
							if(AttachParentComp == nullptr)
							{
								AttachParentComp = AttachParent->GetRootComponent();
							}

							if(AttachParentComp != nullptr)
							{
								NewRootComponent->AttachTo(AttachParentComp, AttachSocketName, EAttachLocation::KeepWorldPosition);
							}
						}

						// If we had attached children reattach them now - unless they are already attached
						for(FAttachedActorInfo& Info : AttachedActorInfos)
						{
							// If this actor is no longer attached to anything, reattach
							if (!Info.AttachedActor->IsPendingKill() && Info.AttachedActor->GetAttachParentActor() == nullptr)
							{
								USceneComponent* ChildRoot = Info.AttachedActor->GetRootComponent();
								if (ChildRoot && ChildRoot->AttachParent != NewRootComponent)
								{
									ChildRoot->AttachTo(NewRootComponent, Info.AttachedToSocket, EAttachLocation::KeepWorldPosition);
									ChildRoot->UpdateComponentToWorld();
								}
							}
						}
					}

					// Determine if the actor used to be selected.
					if (OldActor->IsSelected())
					{
						// Deselect the old actor and select the new actor.
						const bool bNotify = false;
						GEditor->SelectActor(OldActor, /*bInSelected=*/ false, /*bNotify=*/ bNotify);
						GEditor->SelectActor(NewActor, /*bInSelected=*/ true, /*bNotify=*/ bNotify);

						bSelectionChanged = true;
					}

					// Destroy actor and clear references.
					NewActor->Modify();
					if(GEditor->Layers.IsValid()) // ensure(NULL != GEditor->Layers) ?? While cooking the Layers is NULL.
					{
						GEditor->Layers->InitializeNewActorLayers( NewActor );

						GEditor->Layers->DisassociateActorFromLayers( OldActor );
					}
				
					World->EditorDestroyActor( OldActor, true );

					OldToNewInstanceMap.Add(OldActor, NewActor);

					if(bLogConversions)
					{
						UE_LOG(LogBlueprint, Log, TEXT("Converted instance '%s' to '%s'"), *OldActor->GetPathName(), *NewActor->GetPathName() );
					}
				}
				else
				{
					FName OldName(OldObject->GetFName());
					OldObject->Rename(NULL, OldObject->GetOuter(), REN_DoNotDirty | REN_DontCreateRedirectors);
					NewObject = ConstructObject<UObject>(NewClass, OldObject->GetOuter(), OldName);
					check(NewObject);

					UEditorEngine::CopyPropertiesForUnrelatedObjects(OldObject, NewObject);

					if (UAnimInstance* AnimTree = Cast<UAnimInstance>(NewObject))
					{
						// Initialising the anim instance isn't enough to correctly set up the skeletal mesh again in a
						// paused world, need to initialise the skeletal mesh component that contains the anim instance.
						if(USkeletalMeshComponent* SkelComponent = Cast<USkeletalMeshComponent>(AnimTree->GetOuter()))
						{
							SkelComponent->InitAnim(true);
						}
					}

					OldObject->RemoveFromRoot();
					OldObject->MarkPendingKill();

					OldToNewInstanceMap.Add(OldObject, NewObject);

					if(bLogConversions)
					{
						UE_LOG(LogBlueprint, Log, TEXT("Converted instance '%s' to '%s'"), *OldObject->GetPathName(), *NewObject->GetPathName() );
					}
				}

				// If this original object came from a blueprint and it was in the selected debug set, change the debugging to the new object.
				if( ( CorrespondingBlueprint) && ( OldBlueprintDebugObject ) && ( NewObject ) )
				{
					CorrespondingBlueprint->SetObjectBeingDebugged(NewObject);
				}
			}
		}
	}

	UObject* OldCDO = OldClass->GetDefaultObject();
	UObject* NewCDO = NewClass->GetDefaultObject();

	// Now replace any pointers to the old archetypes/instances with pointers to the new one
	TArray<UObject*> SourceObjects;
	TArray<UObject*> DstObjects;

	OldToNewInstanceMap.GenerateKeyArray(SourceObjects);
	OldToNewInstanceMap.GenerateValueArray(DstObjects); // Also look for references in new spawned objects.

	SourceObjects.Append(DstObjects);

	// Add in the old class/CDO to this pass, so class/CDO references are fixed up
	SourceObjects.Add(OldClass);
	SourceObjects.Add(OldCDO);
	if (OriginalCDO)
	{
		SourceObjects.Add(OriginalCDO);
	}

	// Find everything that references these objects
	TSet<UObject *> Targets;
	{ BP_SCOPED_COMPILER_EVENT_STAT(EKismetReinstancerStats_FindReferencers);

		TFindObjectReferencers<UObject> Referencers(SourceObjects, NULL, false);
		for (TFindObjectReferencers<UObject>::TIterator It(Referencers); It; ++It)
		{
			Targets.Add(It.Value());
		}
	}

	// Add the old->new class/CDO mapping into the fixup map
	OldToNewInstanceMap.Add(OldClass, NewClass);
	OldToNewInstanceMap.Add(OldCDO, NewCDO);
	if (OriginalCDO)
	{
		OldToNewInstanceMap.Add(OriginalCDO, NewCDO);
	}

	{ BP_SCOPED_COMPILER_EVENT_STAT(EKismetReinstancerStats_ReplaceReferences);

		for (TSet<UObject *>::TIterator It(Targets); It; ++It)
		{
			UObject* Obj = *It;
			if (!ObjectsToReplace.Contains(Obj)) // Don't bother trying to fix old objects, this would break them
			{
				// The class for finding and replacing weak references.
				// We can't relay on "standard" weak references replacement as
				// it depends on FStringAssetReference::ResolveObject, which
				// tries to find the object with the stored path. It is
				// impossible, cause above we deleted old actors (after
				// spawning new ones), so during objects traverse we have to
				// find FStringAssetReferences with the raw given path taken
				// before deletion of old actors and fix them.
				class ReferenceReplace : public FArchiveReplaceObjectRef<UObject>
				{
				public:
					ReferenceReplace(UObject* InSearchObject, const TMap<UObject*, UObject*>& InReplacementMap, TMap<FStringAssetReference, UObject*> WeakReferencesMap)
						: FArchiveReplaceObjectRef<UObject>(InSearchObject, InReplacementMap, false, false, false, true), WeakReferencesMap(WeakReferencesMap)
					{
						SerializeSearchObject();
					}

					FArchive& operator<<(FStringAssetReference& Ref) override
					{
						const UObject*const* PtrToObjPtr = WeakReferencesMap.Find(Ref);

						if (PtrToObjPtr != nullptr)
						{
							Ref = *PtrToObjPtr;
						}

						return *this;
					}

					FArchive& operator<<(FAssetPtr& Ref) override
					{
						return operator<<(Ref.GetUniqueID());
					}

				private:
					const TMap<FStringAssetReference, UObject*>& WeakReferencesMap;
				};

				ReferenceReplace ReplaceAr(Obj, OldToNewInstanceMap, ReinstancedObjectsWeakReferenceMap);
			}
		}
	}

	SelectedActors->EndBatchSelectOperation();
	if (bSelectionChanged)
	{
		GEditor->NoteSelectionChange();
	}
}

void FBlueprintCompileReinstancer::VerifyReplacement()
{
	TArray<UObject*> SourceObjects;

	// Find all instances of the old class
	for( TObjectIterator<UObject> it; it; ++it )
	{
		UObject* CurrentObj = *it;

		if( (CurrentObj->GetClass() == DuplicatedClass) )
		{
			SourceObjects.Add(CurrentObj);
		}
	}

	// For each instance, track down references
	if( SourceObjects.Num() > 0 )
	{
		TFindObjectReferencers<UObject> Referencers(SourceObjects, NULL, false);
		for (TFindObjectReferencers<UObject>::TIterator It(Referencers); It; ++It)
		{
			UObject* CurrentObject = It.Key();
			UObject* ReferencedObj = It.Value();
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("- Object %s is referencing %s ---"), *CurrentObject->GetName(), *ReferencedObj->GetName());
		}
	}
}
void FBlueprintCompileReinstancer::ReparentChild(UBlueprint* ChildBP)
{
	check(ChildBP);
	check(ChildBP->ParentClass == ClassToReinstance || ChildBP->ParentClass == DuplicatedClass);

	UClass* SkeletonClass = ChildBP->SkeletonGeneratedClass;
	UClass* GeneratedClass = ChildBP->GeneratedClass;

	if( SkeletonClass )
	{
		ReparentChild(SkeletonClass);
	}

	if( GeneratedClass )
	{
		ReparentChild(GeneratedClass);
	}
}

void FBlueprintCompileReinstancer::ReparentChild(UClass* ChildClass)
{
	check(ChildClass && ClassToReinstance && DuplicatedClass && ChildClass->GetSuperClass());
	bool bIsReallyAChild = ChildClass->GetSuperClass() == ClassToReinstance || ChildClass->GetSuperClass() == DuplicatedClass;
	const auto SuperClassBP = Cast<UBlueprint>(ChildClass->GetSuperClass()->ClassGeneratedBy);
	if (SuperClassBP && !bIsReallyAChild)
	{
		bIsReallyAChild |= (SuperClassBP->SkeletonGeneratedClass == ClassToReinstance) || (SuperClassBP->SkeletonGeneratedClass == DuplicatedClass);
		bIsReallyAChild |= (SuperClassBP->GeneratedClass == ClassToReinstance) || (SuperClassBP->GeneratedClass == DuplicatedClass);
	}
	check(bIsReallyAChild);

	ChildClass->AssembleReferenceTokenStream();
	ChildClass->SetSuperStruct(DuplicatedClass);
	ChildClass->Bind();
	ChildClass->StaticLink(true);
}
