// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "BlueprintEditorUtils.h"
#include "Layers/ILayers.h"

/////////////////////////////////////////////////////////////////////////////////
// FBlueprintCompileReinstancer

FBlueprintCompileReinstancer::FBlueprintCompileReinstancer(UClass* InClassToReinstance, bool bIsBytecodeOnly, bool bSkipGC)
	: DuplicatedClass(NULL)
	, bHasReinstanced(false)
	, ClassToReinstance(InClassToReinstance)
	, OriginalCDO(NULL)
	, bSkipGarbageCollection(bSkipGC)
{
	if( InClassToReinstance != NULL )
	{
		SaveClassFieldMapping();

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
				if( ChildBP && !ChildBP->HasAnyFlags(RF_BeingRegenerated))
				{
					// If this is a direct child, change the parent and relink so the property chain is valid for reinstancing
					if( !ChildBP->HasAnyFlags(RF_NeedLoad) )
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
			TArray<UObject*> AllBlueprints;
			GetObjectsOfClass(UBlueprint::StaticClass(), AllBlueprints, true);
			for( auto BPIt = AllBlueprints.CreateIterator(); BPIt; ++BPIt )
			{
				UBlueprint* CurrentBP = Cast<UBlueprint>(*BPIt);
				if( FBlueprintEditorUtils::IsBlueprintDependentOn(CurrentBP, GeneratingBP) )
				{
					Dependencies.Add(CurrentBP);
				}
			}
		}
	}
}

void FBlueprintCompileReinstancer::SaveClassFieldMapping()
{
	check(ClassToReinstance);

	for(UProperty* Prop = ClassToReinstance->PropertyLink; Prop && (Prop->GetOuter() == ClassToReinstance); Prop = Prop->PropertyLinkNext)
	{
		PropertyMap.Add(Prop->GetFName(), Prop);
	}

	for(auto Function : TFieldRange<UFunction>(ClassToReinstance, EFieldIteratorFlags::ExcludeSuper))
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
}

FBlueprintCompileReinstancer::~FBlueprintCompileReinstancer()
{
	// Remove the duplicated class from the root set, because it is no longer useful
	if( DuplicatedClass )
	{
		DuplicatedClass->RemoveFromRoot();
	}
}

void FBlueprintCompileReinstancer::ReinstanceObjects()
{
	// Make sure we only reinstance classes once!
	if( bHasReinstanced )
	{
		return;
	}
	bHasReinstanced = true;

	if( ClassToReinstance && DuplicatedClass )
	{
		ReplaceInstancesOfClass(DuplicatedClass, ClassToReinstance, OriginalCDO);
	
		// Reparent all dependent blueprints, and recompile to ensure that they get reinstanced with the new memory layout
		for( auto ChildBP = Children.CreateIterator(); ChildBP; ++ChildBP)
		{
			UBlueprint* BP = *ChildBP;
			if( BP->ParentClass == ClassToReinstance || BP->ParentClass == DuplicatedClass)
			{
				ReparentChild(BP);
			}
			
			FKismetEditorUtilities::CompileBlueprint(BP, false, bSkipGarbageCollection);
		}
	}
}

void FBlueprintCompileReinstancer::UpdateBytecodeReferences()
{
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
	const bool bLogConversions = false; // for debugging

	// Map of old objects to new objects
	TMap<UObject*, UObject*> OldToNewInstanceMap;
	TMap<UClass*, UClass*> OldToNewClassMap;
	OldToNewClassMap.Add(OldClass, NewClass);

	TArray<UObject*> ObjectsToReplace;
	for( TObjectIterator<UObject> It; It; ++It )
	{
		UObject* Obj = *It;
		if( Obj && (Obj->GetClass() == OldClass) && (Obj != OldClass->ClassDefaultObject) )
		{
			ObjectsToReplace.Add(Obj);
		}
	}

	bool bSelectionChanged = false;
	
	USelection* SelectedActors = GEditor->GetSelectedActors();
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
				AActor* AttachParentActor = NULL;
				TArray<AActor*> OldAttachChildren;
				FName   AttachSocketName;

				// Create cache to store component data across rerunning construction scripts
				FComponentInstanceDataCache InstanceDataCache(OldActor);

				if (USceneComponent* OldRootComponent = OldActor->GetRootComponent())
				{
					if(OldRootComponent->AttachParent != NULL)
					{
						AttachParentActor = OldRootComponent->AttachParent->GetOwner();
						check(AttachParentActor != OldActor); // Root component should never be attached to another component in the same actor!

						AttachSocketName = OldRootComponent->AttachSocketName;

						//detach it to remove any scaling 
						OldRootComponent->DetachFromParent(true);
					}

					for (auto AttachIt = OldRootComponent->AttachChildren.CreateConstIterator(); AttachIt; ++AttachIt)
					{
						OldAttachChildren.Add((*AttachIt)->GetOwner());
					}

					// Save off transform
					WorldTransform = OldActor->GetRootComponent()->ComponentToWorld;
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
				AActor* NewActor = World->SpawnActor(SpawnClass, LocationPtr, RotationPtr, SpawnInfo );

				check(NewActor);
				NewObject = NewActor;

				OldActor->DestroyConstructedComponents(); // don't want to serialize components from the old actor

				NewActor->UnregisterAllComponents(); // Unregister any native components, might have cached state based on properties we are going to overwrite

				UEditorEngine::CopyPropertiesForUnrelatedObjects(OldActor, NewActor);

				NewActor->ResetPropertiesForConstruction(); // reset properties/streams

				NewActor->RegisterAllComponents(); // Register native components

				// Run the construction script, which will use the properties we just copied over
				NewActor->OnConstruction(WorldTransform);

				//Attach the new instance to original parent
				if(AttachParentActor)
				{
					GEditor->ParentActors(AttachParentActor, NewActor, AttachSocketName);
				}

				for (auto AttachIt = OldAttachChildren.CreateIterator(); AttachIt; ++AttachIt)
				{
					// Cannot attach to a socket on a blueprint
					GEditor->ParentActors(NewActor, *AttachIt, FName());
				}

				// Apply cached data to new instance
				InstanceDataCache.ApplyToActor(NewActor);

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
					AnimTree->InitializeAnimation();
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

	UObject* OldCDO = OldClass->GetDefaultObject();
	UObject* NewCDO = NewClass->GetDefaultObject();

	// Now replace any pointers to the old archetypes/instances with pointers to the new one
	TArray<UObject*> SourceObjects;
	OldToNewInstanceMap.GenerateKeyArray(SourceObjects);

	// Add in the old class/CDO to this pass, so class/CDO references are fixed up
	SourceObjects.Add(OldClass);
	SourceObjects.Add(OldCDO);
	if (OriginalCDO)
	{
		SourceObjects.Add(OriginalCDO);
	}

	// Find everything that references these objects
	TFindObjectReferencers<UObject> Referencers(SourceObjects, NULL, false);
	TSet<UObject *> Targets;
	for (TFindObjectReferencers<UObject>::TIterator It(Referencers); It; ++It)
	{
		Targets.Add(It.Value());
	}

	// Add the old->new class/CDO mapping into the fixup map
	OldToNewInstanceMap.Add(OldClass, NewClass);
	OldToNewInstanceMap.Add(OldCDO, NewCDO);
	if (OriginalCDO)
	{
		OldToNewInstanceMap.Add(OriginalCDO, NewCDO);
	}

	for (TSet<UObject *>::TIterator It(Targets); It; ++It)
	{
		UObject* Obj = *It;
		if (!ObjectsToReplace.Contains(Obj)) // Don't bother trying to fix old objects, this would break them
		{
			FArchiveReplaceObjectRef<UObject> ReplaceAr(Obj, OldToNewInstanceMap, /*bNullPrivateRefs=*/ false, /*bIgnoreOuterRef=*/ false, /*bIgnoreArchetypeRef=*/ false);
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
	check(ChildClass);
	check(ChildClass->GetSuperClass() == ClassToReinstance || ChildClass->GetSuperClass() == DuplicatedClass);

	ChildClass->AssembleReferenceTokenStream();
	ChildClass->SetSuperStruct(DuplicatedClass);
	ChildClass->Bind();
	ChildClass->StaticLink(true);
}