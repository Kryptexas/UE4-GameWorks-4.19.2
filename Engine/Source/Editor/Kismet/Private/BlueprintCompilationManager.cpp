// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilationManager.h"
#include "BlueprintEditorUtils.h"
#include "Engine/Engine.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#include "TickableEditorObject.h"
#include "ProfilingDebugging/ScopedTimers.h"

class FFixupBytecodeReferences : public FArchiveUObject
{
public:
	FFixupBytecodeReferences(UObject* InObject)
	{
		ArIsObjectReferenceCollector = true;
		
		InObject->Serialize(*this);
	}

	FArchive& operator<<( UObject*& Obj )
	{
		if (Obj != NULL)
		{
			if(UClass* RelatedClass = Cast<UClass>(Obj))
			{
				UClass* NewClass = RelatedClass->GetAuthoritativeClass();
				ensure(NewClass);
				if(NewClass != RelatedClass)
				{
					Obj = NewClass;
				}
			}
			else if(UField* AsField = Cast<UField>(Obj))
			{
				UClass* OwningClass = AsField->GetOwnerClass();
				if(OwningClass)
				{
					UClass* NewClass = OwningClass->GetAuthoritativeClass();
					ensure(NewClass);
					if(NewClass != OwningClass)
					{
						// drill into new class finding equivalent object:
						TArray<FName> Names;
						UObject* Iter = Obj;
						while(Iter != OwningClass && Iter)
						{
							CA_SUPPRESS(6011); // warning C6011: Dereferencing NULL pointer 'CurrentPin'.
							Names.Add(Iter->GetFName());
							Iter = Iter->GetOuter();
						}

						UObject* Owner = NewClass;
						UObject* Match = nullptr;
						for(int32 I = Names.Num() - 1; I >= 0; --I)
						{
							UObject* Next = StaticFindObjectFast( UObject::StaticClass(), Owner, Names[I]);
							if( Next )
							{
								if(I == 0)
								{
									Match = Next;
								}
								else
								{
									Owner = Match;
								}
							}
							else
							{
								break;
							}
						}
						
						if(Match)
						{
							Obj = Match;
						}
					}
				}
			}
		}
		return *this;
	}
};

struct FBlueprintCompilationManagerImpl : public FGCObject, FTickableEditorObject
{
	virtual ~FBlueprintCompilationManagerImpl() {}

	// FGCObject:
	virtual void AddReferencedObjects(FReferenceCollector& Collector);

	// FTickableEditorObject
	virtual void Tick(float DeltaTime) override { FlushCompilationQueueImpl(); }
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FBlueprintCompilationManagerImpl, STATGROUP_Tickables); }

	void QueueForCompilation(UBlueprint* BPToCompile);
	void FlushCompilationQueueImpl();
	bool HasBlueprintsToCompile();

	TArray<UBlueprint*> QueuedBPs;
};

void FBlueprintCompilationManagerImpl::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(QueuedBPs);
}

void FBlueprintCompilationManagerImpl::QueueForCompilation(UBlueprint* BPToCompile)
{
	QueuedBPs.Add(BPToCompile);
}

static double GTimeCompiling = 0.f;
static double GTimeReinstancing = 0.f;

void FBlueprintCompilationManagerImpl::FlushCompilationQueueImpl()
{
	bool bDidSomething = QueuedBPs.Num() != 0;

	TArray<UBlueprint*> QueuedBPsLocal = MoveTemp(QueuedBPs);
	
	// filter out data only and interface blueprints:
	for(int32 I = 0; I < QueuedBPsLocal.Num(); ++I )
	{
		bool bSkipCompile = false;
		if( FBlueprintEditorUtils::IsDataOnlyBlueprint(QueuedBPsLocal[I]) )
		{
			const UClass* ParentClass = QueuedBPsLocal[I]->ParentClass;
			if (ParentClass && ParentClass->HasAllClassFlags(CLASS_Native))
			{
				bSkipCompile = true;
			}
			else if(FStructUtils::TheSameLayout(QueuedBPsLocal[I]->GeneratedClass, QueuedBPsLocal[I]->GeneratedClass->GetSuperStruct()))
			{
				bSkipCompile = true;
			}
		}
		else if (FBlueprintEditorUtils::IsInterfaceBlueprint(QueuedBPsLocal[I]))
		{
			bSkipCompile = true;
		}

		if(bSkipCompile)
		{
			QueuedBPsLocal.RemoveAtSwap(I);
			--I;
		}
	}

	// sort into sane compilation order. Not needed if we introduce compilation phases:
	QueuedBPsLocal.Sort(
		[](UBlueprint& A, UBlueprint& B)
	{
		int32 DepthA = 0;
		int32 DepthB = 0;
		UStruct* Iter = *(A.GeneratedClass) ? A.GeneratedClass->GetSuperStruct() : nullptr;
		while (Iter)
		{
			++DepthA;
			Iter = Iter->GetSuperStruct();
		}

		Iter = *(B.GeneratedClass) ? B.GeneratedClass->GetSuperStruct() : nullptr;
		while (Iter)
		{
			++DepthB;
			Iter = Iter->GetSuperStruct();
		}

		if (DepthA == DepthB)
		{
			return A.GetFName() < B.GetFName(); 
		}
		return DepthA < DepthB;
	}
	);

	// purge null graphs, could be done only on load
	for (UBlueprint* BP : QueuedBPsLocal)
	{
		FScopedDurationTimer ReinstTimer(GTimeCompiling);
		UPackage* Package = Cast<UPackage>(BP->GetOutermost());
		bool bIsPackageDirty = Package ? Package->IsDirty() : false;

		FBlueprintEditorUtils::PurgeNullGraphs(BP);

		if( Package )
		{
			Package->SetDirtyFlag(bIsPackageDirty);
		}
	}

	// recompile skeleton
	for (UBlueprint* BP : QueuedBPsLocal)
	{
		FScopedDurationTimer ReinstTimer(GTimeCompiling);
		UPackage* Package = Cast<UPackage>(BP->GetOutermost());
		bool bIsPackageDirty = Package ? Package->IsDirty() : false;
		
		BP->bIsRegeneratingOnLoad = true;
		FKismetEditorUtilities::GenerateBlueprintSkeleton(BP, true);
		BP->bIsRegeneratingOnLoad = false;
		
		if( Package )
		{
			Package->SetDirtyFlag(bIsPackageDirty);
		}
	}

	// reconstruct nodes
	for (UBlueprint* BP : QueuedBPsLocal)
	{
		FScopedDurationTimer ReinstTimer(GTimeCompiling);
		UPackage* Package = Cast<UPackage>(BP->GetOutermost());
		bool bIsPackageDirty = Package ? Package->IsDirty() : false;

		FBlueprintEditorUtils::ReconstructAllNodes(BP);
		FBlueprintEditorUtils::ReplaceDeprecatedNodes(BP);

		// we are regenerated, tag ourself as such so that
		// old logic to 'fix' circular dependencies doesn't
		// cause redundant regeneration (e.g. bForceRegenNodes
		// in ExpandTunnelsAndMacros):
		BP->bHasBeenRegenerated = true;
		
		if( Package )
		{
			Package->SetDirtyFlag(bIsPackageDirty);
		}
	}
	
	// reinstance every blueprint that is queued, note that this means classes in the hierarchy that are *not* being 
	// compiled will be parented to REINST versions of the class, so type checks (IsA, etc) involving those types
	// will be incoherent!
	TArray<TSharedPtr<FBlueprintCompileReinstancer>> Reinstancers;
	TMap<UBlueprint*, UObject*> OldCDOs;
	{
		FScopedDurationTimer ReinstTimer(GTimeCompiling);
		for (UBlueprint* BP : QueuedBPsLocal)
		{
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;

			OldCDOs.Add(BP, BP->GeneratedClass->ClassDefaultObject);
			Reinstancers.Emplace(MakeShareable( new FBlueprintCompileReinstancer(BP->GeneratedClass)));
			
			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}
	}
	
	// recompile every blueprint
	for (UBlueprint* BP : QueuedBPsLocal)
	{
		FScopedDurationTimer ReinstTimer(GTimeCompiling);
		UPackage* Package = Cast<UPackage>(BP->GetOutermost());
		bool bIsPackageDirty = Package ? Package->IsDirty() : false;

		ensure(BP->GeneratedClass->ClassDefaultObject == nullptr || 
			BP->GeneratedClass->ClassDefaultObject->GetClass() != BP->GeneratedClass);
		// default value propagation occurrs below:
		BP->GeneratedClass->ClassDefaultObject = nullptr;
		BP->bIsRegeneratingOnLoad = true;
		FKismetEditorUtilities::CompileBlueprint(
			BP, 
			EBlueprintCompileOptions::IsRegeneratingOnLoad|
			EBlueprintCompileOptions::SkeletonUpToDate|
			EBlueprintCompileOptions::SkipReinstancing
		);
		BP->bIsRegeneratingOnLoad = false;
		ensure(BP->GeneratedClass->ClassDefaultObject->GetClass() == *(BP->GeneratedClass));
		
		// get CDO up to date, this allows child instances to compile and immediately
		// generate up to date CDOs, but may cost us in reinstancing (have to reinstance
		// subobjects):
		UObject** OldCDO = OldCDOs.Find(BP);
		if(OldCDO && *OldCDO)
		{
			UEngine::FCopyPropertiesForUnrelatedObjectsParams Params;
			Params.bAggressiveDefaultSubobjectReplacement = false;//true;
			Params.bDoDelta = false;
			Params.bCopyDeprecatedProperties = true;
			Params.bSkipCompilerGeneratedDefaults = true;
			UEngine::CopyPropertiesForUnrelatedObjects(*OldCDO, BP->GeneratedClass->ClassDefaultObject, Params);
		}
		
		if( Package )
		{
			Package->SetDirtyFlag(bIsPackageDirty);
		}
	}
	
	// recompile blueprint bytecode, this is making up for various parts of compile being very monolithic
	{
		FScopedDurationTimer ReinstTimer(GTimeCompiling);
		for (UBlueprint* BP : QueuedBPsLocal)
		{
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;
			
			BP->bIsRegeneratingOnLoad = true;
			FKismetEditorUtilities::RecompileBlueprintBytecode(BP, nullptr, EBlueprintBytecodeRecompileOptions::BatchCompile | EBlueprintBytecodeRecompileOptions::SkipReinstancing);
			BP->bIsRegeneratingOnLoad = false;

			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}
	}

	// and now we can finish the reinstancing operation, moving old classes to new classes:
	{
		FScopedDurationTimer ReinstTimer(GTimeReinstancing);
		TMap< UClass*, UClass* > OldToNewClassMapToReinstance;
		for (TSharedPtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers)
		{
			OldToNewClassMapToReinstance.Add(  Reinstancer->DuplicatedClass, Reinstancer->ClassToReinstance );

			Reinstancer->UpdateBytecodeReferences();

			if (UBlueprintGeneratedClass* BPGClass = CastChecked<UBlueprintGeneratedClass>(Reinstancer->ClassToReinstance))
			{
				BPGClass->UpdateCustomPropertyListForPostConstruction();
			}
		}
			
		FBlueprintCompileReinstancer::BatchReplaceInstancesOfClass(OldToNewClassMapToReinstance);
	
		// make sure no junk in bytecode, this can happen only for blueprints that were in QueuedBPsLocal because
		// the reinstancer can detect all other references (see UpdateBytecodeReferences):
		for (UBlueprint* BP : QueuedBPsLocal)
		{
			for( TFieldIterator<UFunction> FuncIter(BP->GeneratedClass, EFieldIteratorFlags::ExcludeSuper); FuncIter; ++FuncIter )
			{
				UFunction* CurrentFunction = *FuncIter;
				if( CurrentFunction->Script.Num() > 0 )
				{
					FFixupBytecodeReferences ValidateAr(CurrentFunction);
				}
			}
		}
	}

	if(bDidSomething)
	{
		UE_LOG(LogBlueprint, Warning, TEXT("Time Compiling: %f, Time Reinstancing: %f"),  GTimeCompiling, GTimeReinstancing);
		//GTimeCompiling = 0.0;
		//GTimeReinstancing = 0.0;
	}
	ensure(QueuedBPs.Num() == 0);
}

bool FBlueprintCompilationManagerImpl::HasBlueprintsToCompile()
{
	return QueuedBPs.Num() != 0;
}

// Singleton boilperplate:
FBlueprintCompilationManagerImpl* BPCMImpl = nullptr;

void FBlueprintCompilationManager::Initialize()
{
	if(!BPCMImpl)
	{
		BPCMImpl = new FBlueprintCompilationManagerImpl();
	}
}

void FBlueprintCompilationManager::Shutdown()
{
	delete BPCMImpl;
	BPCMImpl = nullptr;
}

// Forward to impl:
void FBlueprintCompilationManager::FlushCompilationQueue()
{
	if(BPCMImpl)
	{
		BPCMImpl->FlushCompilationQueueImpl();
	}
}

void FBlueprintCompilationManager::QueueForCompilation(UBlueprint* BPToCompile)
{
	check(BPToCompile->GetLinker());
	BPCMImpl->QueueForCompilation(BPToCompile);
}
	
void FBlueprintCompilationManager::NotifyBlueprintLoaded(UBlueprint* BPLoaded)
{
	// Blueprints can be loaded before editor modules are on line:
	if(!BPCMImpl)
	{
		FBlueprintCompilationManager::Initialize();
	}
	check(BPLoaded->GetLinker());
	BPCMImpl->QueueForCompilation(BPLoaded);
}


