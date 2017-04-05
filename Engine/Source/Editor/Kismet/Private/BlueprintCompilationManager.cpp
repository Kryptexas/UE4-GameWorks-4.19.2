// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilationManager.h"

#include "BlueprintEditorUtils.h"
#include "Blueprint/BlueprintSupport.h"
#include "CompilerResultsLog.h"
#include "Components/TimelineComponent.h"
#include "Engine/Engine.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/TimelineTemplate.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#include "KismetCompiler.h"
#include "ProfilingDebugging/ScopedTimers.h"
#include "Serialization/ArchiveHasReferences.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "TickableEditorObject.h"
#include "UObject/ReferenceChainSearch.h"
#include "UObject/UObjectHash.h"
#include "WidgetBlueprint.h"

// Debugging switches:
#define VERIFY_NO_STALE_CLASS_REFERENCES 0
#define VERIFY_NO_BAD_SKELETON_REFERENCES 0

class FFixupBytecodeReferences : public FArchiveUObject
{
public:
	FFixupBytecodeReferences(UObject* InObject)
	{
		ArIsObjectReferenceCollector = true;
		
		InObject->Serialize(*this);
		class FArchiveProxyCollector : public FReferenceCollector
		{
			/** Archive we are a proxy for */
			FArchive& Archive;
		public:
			FArchiveProxyCollector(FArchive& InArchive)
				: Archive(InArchive)
			{
			}
			virtual void HandleObjectReference(UObject*& Object, const UObject* ReferencingObject, const UProperty* ReferencingProperty) override
			{
				Archive << Object;
			}
			virtual void HandleObjectReferences(UObject** InObjects, const int32 ObjectNum, const UObject* InReferencingObject, const UProperty* InReferencingProperty) override
			{
				for (int32 ObjectIndex = 0; ObjectIndex < ObjectNum; ++ObjectIndex)
				{
					UObject*& Object = InObjects[ObjectIndex];
					Archive << Object;
				}
			}
			virtual bool IsIgnoringArchetypeRef() const override
			{
				return false;
			}
			virtual bool IsIgnoringTransient() const override
			{
				return false;
			}
		} ArchiveProxyCollector(*this);
		
		InObject->GetClass()->CallAddReferencedObjects(InObject, ArchiveProxyCollector);
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
						while (Iter && Iter != OwningClass)
						{
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

// free function that we use to cross a module boundary (from CoreUObject to here)
void FlushReinstancingQueueImplWrapper();

struct FBlueprintCompilationManagerImpl : public FGCObject
{
	FBlueprintCompilationManagerImpl();
	virtual ~FBlueprintCompilationManagerImpl();

	// FGCObject:
	virtual void AddReferencedObjects(FReferenceCollector& Collector);

	void QueueForCompilation(UBlueprint* BPToCompile);
	void FlushCompilationQueueImpl(TArray<UObject*>* ObjLoaded);
	void FlushReinstancingQueueImpl();
	bool HasBlueprintsToCompile() const;
	bool IsGeneratedClassLayoutReady() const;
	static void ReinstanceBatch(TArray<TUniquePtr<FBlueprintCompileReinstancer>>& Reinstancers, TMap< UClass*, UClass* >& InOutOldToNewClassMap, TArray<UObject*>* ObjLoaded);
	static UClass* FastGenerateSkeletonClass(UBlueprint* BP, FKismetCompilerContext& CompilerContext);

	TArray<UBlueprint*> QueuedBPs;
	TMap<UClass*, UClass*> ClassesToReinstance;
	bool bGeneratedClassLayoutReady;
};

FBlueprintCompilationManagerImpl::FBlueprintCompilationManagerImpl()
{
	FBlueprintSupport::SetFlushReinstancingQueueFPtr(&FlushReinstancingQueueImplWrapper);
	bGeneratedClassLayoutReady = true;
}

FBlueprintCompilationManagerImpl::~FBlueprintCompilationManagerImpl() 
{ 
	FBlueprintSupport::SetFlushReinstancingQueueFPtr(nullptr); 
}

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

enum class ECompilationManagerFlags
{
	None = 0x0,

	SkeletonOnly				= 0x1,
};

ENUM_CLASS_FLAGS(ECompilationManagerFlags)

struct FCompilerData
{
	FCompilerData(UBlueprint* InBP, ECompilationManagerFlags InFlags)
	{
		check(InBP);
		BP = InBP;
		Flags = InFlags;
		ResultsLog = MakeUnique<FCompilerResultsLog>();
		ResultsLog->BeginEvent(TEXT("BlueprintCompilationManager Compile"));
		ResultsLog->SetSourcePath(InBP->GetPathName());
		Options.bRegenerateSkelton = false;
		Options.bReinstanceAndStubOnFailure = false;
		if( UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(BP))
		{
			Compiler = UWidgetBlueprint::GetCompilerForWidgetBP(WidgetBP, *ResultsLog, Options);
		}
		else
		{
			Compiler = FKismetCompilerContext::GetCompilerForBP(BP, *ResultsLog, Options);
		}
	}

	bool IsSkeletonOnly() const { return !!(Flags & ECompilationManagerFlags::SkeletonOnly); }

	UBlueprint* BP;
	TUniquePtr<FCompilerResultsLog> ResultsLog;
	TUniquePtr<FKismetCompilerContext> Compiler;
	FKismetCompilerOptions Options;

	ECompilationManagerFlags Flags;
};
	
void FBlueprintCompilationManagerImpl::FlushCompilationQueueImpl(TArray<UObject*>* ObjLoaded)
{
	ensure(bGeneratedClassLayoutReady);

	if( QueuedBPs.Num() == 0 )
	{
		return;
	}

	TArray<FCompilerData> CurrentlyCompilingBPs;
	TArray<TUniquePtr<FBlueprintCompileReinstancer>> Reinstancers;
	{ // begin GTimeCompiling scope 
		FScopedDurationTimer SetupTimer(GTimeCompiling); 
	
		for(int32 I = 0; I < QueuedBPs.Num(); ++I)
		{
			// filter out data only and interface blueprints:
			bool bSkipCompile = false;
			UBlueprint* QueuedBP = QueuedBPs[I];

			ensure(!(QueuedBP->GeneratedClass->ClassDefaultObject->HasAnyFlags(RF_NeedLoad)));
			if( FBlueprintEditorUtils::IsDataOnlyBlueprint(QueuedBP) )
			{
				const UClass* ParentClass = QueuedBP->ParentClass;
				if (ParentClass && ParentClass->HasAllClassFlags(CLASS_Native))
				{
					bSkipCompile = true;
				}
				else
				{
					const UClass* CurrentClass = QueuedBP->GeneratedClass;
					if(FStructUtils::TheSameLayout(CurrentClass, CurrentClass->GetSuperStruct()))
					{
						bSkipCompile = true;
					}
				}
			}

			if(bSkipCompile)
			{
				CurrentlyCompilingBPs.Add(FCompilerData(QueuedBP, ECompilationManagerFlags::SkeletonOnly));
				if (QueuedBP->IsGeneratedClassAuthoritative() && (QueuedBP->GeneratedClass != nullptr))
				{
					// set bIsRegeneratingOnLoad so that we don't reset loaders:
					QueuedBP->bIsRegeneratingOnLoad = true;
					FBlueprintEditorUtils::RemoveStaleFunctions(Cast<UBlueprintGeneratedClass>(QueuedBP->GeneratedClass), QueuedBP);
					QueuedBP->bIsRegeneratingOnLoad = false;
				}

				// No actual compilation work to be done, but try to conform the class and fix up anything that might need to be updated if the native base class has changed in any way
				FKismetEditorUtilities::ConformBlueprintFlagsAndComponents(QueuedBP);

				if (QueuedBP->GeneratedClass)
				{
					FBlueprintEditorUtils::RecreateClassMetaData(QueuedBP, QueuedBP->GeneratedClass, true);
				}

				QueuedBPs.RemoveAtSwap(I);
				--I;
			}
			else
			{
				CurrentlyCompilingBPs.Add(FCompilerData(QueuedBP, ECompilationManagerFlags::None));
			}
		}
	
		QueuedBPs.Empty();

		// sort into sane compilation order. Not needed if we introduce compilation phases:
		auto HierarchyDepthSortFn = [](const FCompilerData& CompilerDataA, const FCompilerData& CompilerDataB)
		{
			UBlueprint& A = *(CompilerDataA.BP);
			UBlueprint& B = *(CompilerDataB.BP);

			bool bAIsInterface = FBlueprintEditorUtils::IsInterfaceBlueprint(&A);
			bool bBIsInterface = FBlueprintEditorUtils::IsInterfaceBlueprint(&B);

			if(bAIsInterface && !bBIsInterface)
			{
				return true;
			}
			else if(bBIsInterface && !bAIsInterface)
			{
				return false;
			}

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
		};
		CurrentlyCompilingBPs.Sort( HierarchyDepthSortFn );

		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			if(CompilerData.IsSkeletonOnly())
			{
				continue;
			}

			UBlueprint* BP = CompilerData.BP;
		
			// even ValidateVariableNames will trigger compilation if 
			// it thinks the BP is not being compiled:
			BP->bBeingCompiled = true;
			CompilerData.Compiler->ValidateVariableNames();
			BP->bBeingCompiled = false;
		}

		// purge null graphs, could be done only on load
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;

			FBlueprintEditorUtils::PurgeNullGraphs(BP);

			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}

		// recompile skeleton
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;
		
			BP->SkeletonGeneratedClass = FastGenerateSkeletonClass(BP, *(CompilerData.Compiler) );

			if(CompilerData.IsSkeletonOnly())
			{
				// Flag data only blueprints as being up-to-date
				BP->Status = BS_UpToDate;
				BP->bHasBeenRegenerated = true;
				BP->GeneratedClass->ClearFunctionMapsCaches();
			}

			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}

		// reconstruct nodes
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			if(CompilerData.IsSkeletonOnly())
			{
				continue;
			}

			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;

			// Setting bBeingCompiled to make sure that FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified
			// doesn't run skeleton compile while the compilation manager is compiling things:
			BP->bBeingCompiled = true;
			FBlueprintEditorUtils::ReconstructAllNodes(BP);
			FBlueprintEditorUtils::ReplaceDeprecatedNodes(BP);
			BP->bBeingCompiled = false;

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
		TMap<UBlueprint*, UObject*> OldCDOs;
		{
			for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
			{
				// we including skeleton only compilation jobs for reinstancing because we need UpdateCustomPropertyListForPostConstruction
				// to happen (at the right time) for those generated classes as well. This means we *don't* need to reinstance if 
				// the parent is a native type (unless we hot reload, but that should not need to be handled here):
				if(CompilerData.IsSkeletonOnly() && CompilerData.BP->ParentClass->IsNative())
				{
					continue;
				}

				UBlueprint* BP = CompilerData.BP;
				UPackage* Package = Cast<UPackage>(BP->GetOutermost());
				bool bIsPackageDirty = Package ? Package->IsDirty() : false;

				OldCDOs.Add(BP, BP->GeneratedClass->ClassDefaultObject);
				Reinstancers.Emplace(TUniquePtr<FBlueprintCompileReinstancer>( 
					new FBlueprintCompileReinstancer(
						BP->GeneratedClass, 
						EBlueprintCompileReinstancerFlags::AutoInferSaveOnCompile|EBlueprintCompileReinstancerFlags::AvoidCDODuplication
					)
				));

				if( Package )
				{
					Package->SetDirtyFlag(bIsPackageDirty);
				}
			}
		}

		// Reinstancing done, lets fix up child->parent pointers:
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			UBlueprint* BP = CompilerData.BP;
			if(BP->GeneratedClass->GetSuperClass()->HasAnyClassFlags(CLASS_NewerVersionExists))
			{
				BP->GeneratedClass->SetSuperStruct(BP->GeneratedClass->GetSuperClass()->GetAuthoritativeClass());
				BP->GeneratedClass->Children = NULL;
				BP->GeneratedClass->Script.Empty();
				BP->GeneratedClass->MinAlignment = 0;
				BP->GeneratedClass->RefLink = NULL;
				BP->GeneratedClass->PropertyLink = NULL;
				BP->GeneratedClass->DestructorLink = NULL;
				BP->GeneratedClass->ScriptObjectReferences.Empty();
				BP->GeneratedClass->PropertyLink = NULL;
			}
		}

		// recompile every blueprint
		bGeneratedClassLayoutReady = false;
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			if(CompilerData.IsSkeletonOnly())
			{
				continue;
			}

			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;

			ensure(BP->GeneratedClass->ClassDefaultObject == nullptr || 
				BP->GeneratedClass->ClassDefaultObject->GetClass() != BP->GeneratedClass);
			// default value propagation occurrs below:
			BP->GeneratedClass->ClassDefaultObject = nullptr;
			BP->bIsRegeneratingOnLoad = true;
			BP->bBeingCompiled = true;
			// Reset the flag, so if the user tries to use PIE it will warn them if the BP did not compile
			BP->bDisplayCompilePIEWarning = true;
		
			FKismetCompilerContext& CompilerContext = *(CompilerData.Compiler);
			CompilerContext.CompileClassLayout(EInternalCompilerFlags::PostponeLocalsGenerationUntilPhaseTwo);
			BP->bIsRegeneratingOnLoad = false;

			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}
		bGeneratedClassLayoutReady = true;
	
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			if(CompilerData.IsSkeletonOnly())
			{
				continue;
			}

			UBlueprint* BP = CompilerData.BP;
			UPackage* Package = Cast<UPackage>(BP->GetOutermost());
			bool bIsPackageDirty = Package ? Package->IsDirty() : false;

			ensure(BP->GeneratedClass->ClassDefaultObject == nullptr || 
				BP->GeneratedClass->ClassDefaultObject->GetClass() != BP->GeneratedClass);
			// default value propagation occurrs below:
			BP->GeneratedClass->ClassDefaultObject = nullptr;
			BP->bIsRegeneratingOnLoad = true;
		
			FKismetCompilerContext& CompilerContext = *(CompilerData.Compiler);
			CompilerContext.CompileFunctions(EInternalCompilerFlags::PostponeLocalsGenerationUntilPhaseTwo);
		
			BP->bBeingCompiled = false;

			if (CompilerData.ResultsLog->NumErrors == 0)
			{
				// Blueprint is error free.  Go ahead and fix up debug info
				BP->Status = (0 == CompilerData.ResultsLog->NumWarnings) ? BS_UpToDate : BS_UpToDateWithWarnings;
			}
			else
			{
				BP->Status = BS_Error; // do we still have the old version of the class?
			}

			BP->bIsRegeneratingOnLoad = false;
			ensure(BP->GeneratedClass->ClassDefaultObject->GetClass() == *(BP->GeneratedClass));
		
			if( Package )
			{
				Package->SetDirtyFlag(bIsPackageDirty);
			}
		}
	} // end GTimeCompiling scope

	// and now we can finish the first stage of the reinstancing operation, moving old classes to new classes:
	{
		FScopedDurationTimer ReinstTimer(GTimeReinstancing);
		ReinstanceBatch(Reinstancers, ClassesToReinstance, ObjLoaded);
		
		// make sure no junk in bytecode, this can happen only for blueprints that were in QueuedBPsLocal because
		// the reinstancer can detect all other references (see UpdateBytecodeReferences):
		for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
		{
			if(CompilerData.IsSkeletonOnly())
			{
				continue;
			}

			UBlueprint* BP = CompilerData.BP;
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

	for (FCompilerData& CompilerData : CurrentlyCompilingBPs)
	{
		CompilerData.ResultsLog->EndEvent();
	}

	UE_LOG(LogBlueprint, Display, TEXT("Time Compiling: %f, Time Reinstancing: %f"),  GTimeCompiling, GTimeReinstancing);
	//GTimeCompiling = 0.0;
	//GTimeReinstancing = 0.0;
	ensure(QueuedBPs.Num() == 0);
}

void FBlueprintCompilationManagerImpl::FlushReinstancingQueueImpl()
{
	// we can finalize reinstancing now:
	if(ClassesToReinstance.Num() == 0)
	{
		return;
	}

	{
		FScopedDurationTimer ReinstTimer(GTimeReinstancing);

		FBlueprintCompileReinstancer::BatchReplaceInstancesOfClass(ClassesToReinstance);
		ClassesToReinstance.Empty();
	}
	
#if VERIFY_NO_STALE_CLASS_REFERENCES
	FBlueprintSupport::ValidateNoRefsToOutOfDateClasses();
#endif

#if VERIFY_NO_BAD_SKELETON_REFERENCES
	FBlueprintSupport::ValidateNoExternalRefsToSkeletons();
#endif

	UE_LOG(LogBlueprint, Display, TEXT("Time Compiling: %f, Time Reinstancing: %f"),  GTimeCompiling, GTimeReinstancing);
}

bool FBlueprintCompilationManagerImpl::HasBlueprintsToCompile() const
{
	return QueuedBPs.Num() != 0;
}

bool FBlueprintCompilationManagerImpl::IsGeneratedClassLayoutReady() const
{
	return bGeneratedClassLayoutReady;
}

void FBlueprintCompilationManagerImpl::ReinstanceBatch(TArray<TUniquePtr<FBlueprintCompileReinstancer>>& Reinstancers, TMap< UClass*, UClass* >& InOutOldToNewClassMap, TArray<UObject*>* ObjLoaded)
{
	const auto FilterOutOfDateClasses = [](TArray<UClass*>& ClassList)
	{
		ClassList.RemoveAllSwap( [](UClass* Class) { return Class->HasAnyClassFlags(CLASS_NewerVersionExists); } );
	};

	const auto HasChildren = [FilterOutOfDateClasses](UClass* InClass) -> bool
	{
		TArray<UClass*> ChildTypes;
		GetDerivedClasses(InClass, ChildTypes, false);
		FilterOutOfDateClasses(ChildTypes);
		return ChildTypes.Num() > 0;
	};

	TSet<UClass*> ClassesToReparent;
	TSet<UClass*> ClassesToReinstance;

	// Reinstancers may contain *part* of a class hierarchy, so we first need to reparent any child types that 
	// haven't already been reinstanced:
	for (const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers)
	{
		UClass* OldClass = Reinstancer->DuplicatedClass;
		InOutOldToNewClassMap.Add(Reinstancer->DuplicatedClass, Reinstancer->ClassToReinstance);
		if(!OldClass)
		{
			continue;
		}

		if(!HasChildren(OldClass))
		{
			continue;
		}

		bool bParentLayoutChanged = FStructUtils::TheSameLayout(OldClass, Reinstancer->ClassToReinstance);
		if(bParentLayoutChanged)
		{
			// we need *all* derived types:
			TArray<UClass*> ClassesToReinstanceList;
			GetDerivedClasses(OldClass, ClassesToReinstanceList);
			FilterOutOfDateClasses(ClassesToReinstanceList);
			
			for(UClass* ClassToReinstance : ClassesToReinstanceList)
			{
				ClassesToReinstance.Add(ClassToReinstance);
			}
		}
		else
		{
			// parent layout did not change, we can just relink the direct children:
			TArray<UClass*> ClassesToReparentList;
			GetDerivedClasses(OldClass, ClassesToReparentList, false);
			FilterOutOfDateClasses(ClassesToReparentList);
			
			for(UClass* ClassToReparent : ClassesToReparentList)
			{
				ClassesToReparent.Add(ClassToReparent);
			}
		}
	}

	for(UClass* Class : ClassesToReparent)
	{
		UClass** NewParent = InOutOldToNewClassMap.Find(Class->GetSuperClass());
		check(NewParent && *NewParent);
		Class->SetSuperStruct(*NewParent);
		Class->Children = nullptr;
		Class->Script.Empty();
		Class->MinAlignment = 0;
		Class->RefLink = nullptr;
		Class->PropertyLink = nullptr;
		Class->DestructorLink = nullptr;
		Class->ScriptObjectReferences.Empty();
		Class->PropertyLink = nullptr;
	}

	// make new hierarchy
	for(UClass* Class : ClassesToReinstance)
	{
		UObject* OriginalCDO = Class->ClassDefaultObject;
		Reinstancers.Emplace(TUniquePtr<FBlueprintCompileReinstancer>( 
			new FBlueprintCompileReinstancer(
				Class, 
				EBlueprintCompileReinstancerFlags::AutoInferSaveOnCompile|EBlueprintCompileReinstancerFlags::AvoidCDODuplication
			)
		));

		// make sure we have the newest parent now that CDO has been moved to duplicate class:
		TUniquePtr<FBlueprintCompileReinstancer>& NewestReinstancer = Reinstancers.Last();

		UClass* SuperClass = NewestReinstancer->ClassToReinstance->GetSuperClass();
		if(ensure(SuperClass))
		{
			if(SuperClass->HasAnyClassFlags(CLASS_NewerVersionExists))
			{
				NewestReinstancer->ClassToReinstance->SetSuperStruct(SuperClass->GetAuthoritativeClass());
			}
		}
		
		// relink the new class:
		NewestReinstancer->ClassToReinstance->Bind();
		NewestReinstancer->ClassToReinstance->StaticLink(true);
	}

	// run UpdateBytecodeReferences:
	for (const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers)
	{
		InOutOldToNewClassMap.Add( Reinstancer->DuplicatedClass, Reinstancer->ClassToReinstance );
			
		UBlueprint* CompiledBlueprint = UBlueprint::GetBlueprintFromClass(Reinstancer->ClassToReinstance);
		CompiledBlueprint->bIsRegeneratingOnLoad = true;
		Reinstancer->UpdateBytecodeReferences();

		CompiledBlueprint->bIsRegeneratingOnLoad = false;
	}
	
	// Now we can update templates and archetypes - note that we don't look for direct references to archetypes - doing
	// so is very expensive and it will be much faster to directly update anything that cares to cache direct references
	// to an archetype here (e.g. a UClass::ClassDefaultObject member):
	
	// 1. Sort classes so that most derived types are updated last - right now the only caller of this function
	// also sorts, but we don't want to make too many assumptions about caller. We could refine this API so that
	// we're not taking a raw list of reinstancers:
	Reinstancers.Sort(
		[](const TUniquePtr<FBlueprintCompileReinstancer>& CompilerDataA, const TUniquePtr<FBlueprintCompileReinstancer>& CompilerDataB)
		{
			UClass* A = CompilerDataA->ClassToReinstance;
			UClass* B = CompilerDataB->ClassToReinstance;
			int32 DepthA = 0;
			int32 DepthB = 0;
			UStruct* Iter = A ? A->GetSuperStruct() : nullptr;
			while (Iter)
			{
				++DepthA;
				Iter = Iter->GetSuperStruct();
			}

			Iter = B ? B->GetSuperStruct() : nullptr;
			while (Iter)
			{
				++DepthB;
				Iter = Iter->GetSuperStruct();
			}

			if (DepthA == DepthB && A && B)
			{
				return A->GetFName() < B->GetFName(); 
			}
			return DepthA < DepthB;
		}
	);

	// 2. Copy defaults from old CDO - CDO may be missing if this class was reinstanced and relinked here,
	// so use GetDefaultObject(true):
	for( const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers )
	{
		UObject* OldCDO = Reinstancer->DuplicatedClass->ClassDefaultObject;
		if(OldCDO)
		{
			FBlueprintCompileReinstancer::CopyPropertiesForUnrelatedObjects(OldCDO,  Reinstancer->ClassToReinstance->GetDefaultObject(true));
			
			if (UBlueprintGeneratedClass* BPGClass = CastChecked<UBlueprintGeneratedClass>(Reinstancer->ClassToReinstance))
			{
				BPGClass->UpdateCustomPropertyListForPostConstruction();

				// patch new cdo into linker table:
				if(ObjLoaded)
				{
					UBlueprint* CurrentBP = CastChecked<UBlueprint>(BPGClass->ClassGeneratedBy);
					if(FLinkerLoad* CurrentLinker = CurrentBP->GetLinker())
					{
						int32 OldCDOIndex = INDEX_NONE;

						for (int32 i = 0; i < CurrentLinker->ExportMap.Num(); i++)
						{
							FObjectExport& ThisExport = CurrentLinker->ExportMap[i];
							if (ThisExport.ObjectFlags & RF_ClassDefaultObject)
							{
								OldCDOIndex = i;
								break;
							}
						}

						if(OldCDOIndex != INDEX_NONE)
						{
							FBlueprintEditorUtils::PatchNewCDOIntoLinker(CurrentBP->GeneratedClass->ClassDefaultObject, CurrentLinker, OldCDOIndex, *ObjLoaded);
							FBlueprintEditorUtils::PatchCDOSubobjectsIntoExport(OldCDO, CurrentBP->GeneratedClass->ClassDefaultObject);
						}
					}
				}
			}
		}
	}

	TSet<UObject*> ObjectsThatShouldUseOldStuff;
	TMap<UObject*, UObject*> OldArchetypeToNewArchetype;

	// 3. Update any remaining instances that are tagged as RF_ArchetypeObject or RF_InheritableComponentTemplate - 
	// we may need to do further sorting to ensure that interdependent archetypes are initialized correctly:
	for( const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers )
	{
		UClass* OldClass = Reinstancer->DuplicatedClass;
		if(ensure(OldClass))
		{
			TArray<UObject*> ArchetypeObjects;
			GetObjectsOfClass(OldClass, ArchetypeObjects, false);
			
			// filter out non-archetype instances, note that WidgetTrees and some component
			// archetypes do not have RF_ArchetypeObject or RF_InheritableComponentTemplate so
			// we simply detect that they are outered to a UBPGC or UBlueprint and assume that 
			// they are archetype objects in practice:
			ArchetypeObjects.RemoveAllSwap(
				[](UObject* Obj) 
				{ 
					bool bIsArchetype = 
						Obj->HasAnyFlags(RF_ArchetypeObject|RF_InheritableComponentTemplate)
						|| Obj->GetTypedOuter<UBlueprintGeneratedClass>()
						|| Obj->GetTypedOuter<UBlueprint>();
					// remove if this is not an archetype or its an archetype in the transient package:
					return !bIsArchetype || Obj->GetOutermost() == GetTransientPackage(); 
				}
			);

			// for each archetype:
			for(UObject* Archetype : ArchetypeObjects )
			{
				// move aside:
				FName OriginalName = Archetype->GetFName();
				UObject* OriginalOuter = Archetype->GetOuter();
				EObjectFlags OriginalFlags = Archetype->GetFlags();
				Archetype->Rename(
					nullptr,
					// destination - this is the important part of this call. Moving the object 
					// out of the way so we can reuse its name:
					GetTransientPackage(), 
					// Rename options:
					REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders );

				// reconstruct
				FMakeClassSpawnableOnScope TemporarilySpawnable(Reinstancer->ClassToReinstance);
				const EObjectFlags FlagMask = RF_Public | RF_ArchetypeObject | RF_Transactional | RF_Transient | RF_TextExportTransient | RF_InheritableComponentTemplate | RF_Standalone; //TODO: what about RF_RootSet?
				UObject* NewArchetype = NewObject<UObject>(OriginalOuter, Reinstancer->ClassToReinstance, OriginalName, OriginalFlags & FlagMask);

				// copy old data:
				FBlueprintCompileReinstancer::CopyPropertiesForUnrelatedObjects(Archetype, NewArchetype);

				OldArchetypeToNewArchetype.Add(Archetype, NewArchetype);
				ObjectsThatShouldUseOldStuff.Add(Archetype);

				Archetype->MarkPendingKill();
			}
		}
	}

	// 4. update known references to archetypes (e.g. component templates, WidgetTree). We don't want to run the normal 
	// reference finder to update these because searching the entire object graph is time consuming. Instead we just replace
	// all references in our UBlueprint and its generated class:
	TSet<UObject*> ArchetypeReferencers;
	for( const TUniquePtr<FBlueprintCompileReinstancer>& Reinstancer : Reinstancers )
	{
		ArchetypeReferencers.Add(Reinstancer->ClassToReinstance);
		ArchetypeReferencers.Add(Reinstancer->ClassToReinstance->ClassGeneratedBy);
		if(UBlueprint* BP = Cast<UBlueprint>(Reinstancer->ClassToReinstance->ClassGeneratedBy))
		{
			ensure(BP->bCachedDependenciesUpToDate);
			for(TWeakObjectPtr<UBlueprint> Dependendency : BP->CachedDependencies)
			{
				ArchetypeReferencers.Add(Dependendency.Get());
			}
		}
	}

	for(UObject* ArchetypeReferencer : ArchetypeReferencers)
	{
		FArchiveReplaceObjectRef< UObject > ReplaceAr(ArchetypeReferencer, OldArchetypeToNewArchetype, false, false, false);
	}
}


UClass* FBlueprintCompilationManagerImpl::FastGenerateSkeletonClass(UBlueprint* BP, FKismetCompilerContext& CompilerContext)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	FCompilerResultsLog MessageLog;

	UClass* ParentClass = BP->ParentClass;
	if(ParentClass->ClassGeneratedBy)
	{
		if(UBlueprint* ParentBP = Cast<UBlueprint>(ParentClass->ClassGeneratedBy))
		{
			if(ParentBP->SkeletonGeneratedClass)
			{
				ParentClass = ParentBP->SkeletonGeneratedClass;
			}
		}
	}

	ensure(BP->SkeletonGeneratedClass == nullptr);
	UBlueprintGeneratedClass* OriginalNewClass = CompilerContext.NewClass;
	CompilerContext.SpawnNewClass(FString::Printf(TEXT("SKEL_%s_C"), *BP->GetName()));
	UBlueprintGeneratedClass* Ret = CompilerContext.NewClass;
	CompilerContext.NewClass = OriginalNewClass;
	Ret->ClassGeneratedBy = BP;
	
	const auto MakeFunction = [Ret, ParentClass, Schema, BP, &MessageLog](FName FunctionNameFName, UField**& InCurrentFieldStorageLocation, UField**& InCurrentParamStorageLocation, bool bIsStaticFunction, uint32 InFunctionFlags, const TArray<UK2Node_FunctionResult*>& ReturnNodes, const TArray<UEdGraphPin*>& InputPins) -> UFunction*
	{
		if(!ensure(FunctionNameFName != FName())
			|| FindObjectFast<UFunction>(Ret, FunctionNameFName, true ))
		{
			return nullptr;
		}
		
		UFunction* NewFunction = NewObject<UFunction>(Ret, FunctionNameFName, RF_Public);
					
		Ret->AddFunctionToFunctionMap(NewFunction);

		*InCurrentFieldStorageLocation = NewFunction;
		InCurrentFieldStorageLocation = &NewFunction->Next;

		if(bIsStaticFunction)
		{
			NewFunction->FunctionFlags |= FUNC_Static;
		}

		UFunction* ParentFn = ParentClass->FindFunctionByName(NewFunction->GetFName());
		if(ParentFn == nullptr)
		{
			// check for function in implemented interfaces:
			for(const FBPInterfaceDescription& BPID : BP->ImplementedInterfaces)
			{
				// we only want the *skeleton* version of the function:
				UClass* InterfaceClass = BPID.Interface;
				if(UBlueprint* Owner = Cast<UBlueprint>(InterfaceClass->ClassGeneratedBy))
				{
					if( ensure(Owner->SkeletonGeneratedClass) )
					{
						InterfaceClass = Owner->SkeletonGeneratedClass;
					}
				}

				if(UFunction* ParentInterfaceFn = InterfaceClass->FindFunctionByName(NewFunction->GetFName()))
				{
					ParentFn = ParentInterfaceFn;
					break;
				}
			}
		}
		NewFunction->SetSuperStruct( ParentFn );
					
		InCurrentParamStorageLocation = &NewFunction->Children;

		// params:
		if(ParentFn)
		{
			NewFunction->FunctionFlags |= (ParentFn->FunctionFlags & (FUNC_FuncInherit | FUNC_Public | FUNC_Protected | FUNC_Private | FUNC_BlueprintPure));
			for (TFieldIterator<UProperty> PropIt(ParentFn); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
			{
				UProperty* ClonedParam = CastChecked<UProperty>(StaticDuplicateObject(*PropIt, NewFunction, PropIt->GetFName(), RF_AllFlags, nullptr, EDuplicateMode::Normal, EInternalObjectFlags::AllFlags & ~(EInternalObjectFlags::Native) ));
				ClonedParam->Next = nullptr;
				*InCurrentParamStorageLocation = ClonedParam;
				InCurrentParamStorageLocation = &ClonedParam->Next;
			}
		}
		else
		{
			NewFunction->FunctionFlags |= InFunctionFlags;
			for(UEdGraphPin* Pin : InputPins)
			{
				if(Pin->Direction == EEdGraphPinDirection::EGPD_Output && !Schema->IsExecPin(*Pin) && Pin->ParentPin == nullptr && Pin->GetName() != UK2Node_Event::DelegateOutputName)
				{
					// Reimplmentation of FKismetCompilerContext::CreatePropertiesFromList without dependence on 'terms'
					UProperty* Param = FKismetCompilerUtilities::CreatePropertyOnScope(NewFunction, *(Pin->PinName), Pin->PinType, Ret, 0, Schema, MessageLog);
					Param->PropertyFlags |= CPF_Parm;
					if(Pin->PinType.bIsReference)
					{
						Param->PropertyFlags |= CPF_ReferenceParm | CPF_OutParm;
					}
					if(Pin->PinType.bIsConst)
					{
						Param->PropertyFlags |= CPF_ConstParm;
					}

					if (UObjectProperty* ObjProp = Cast<UObjectProperty>(Param))
					{
						UClass* EffectiveClass = nullptr;
						if (ObjProp->PropertyClass != nullptr)
						{
							EffectiveClass = ObjProp->PropertyClass;
						}
						else if (UClassProperty* ClassProp = Cast<UClassProperty>(ObjProp))
						{
							EffectiveClass = ClassProp->MetaClass;
						}

						if ((EffectiveClass != nullptr) && (EffectiveClass->HasAnyClassFlags(CLASS_Const)))
						{
							Param->PropertyFlags |= CPF_ConstParm;
						}
					}
					else if (UArrayProperty* ArrayProp = Cast<UArrayProperty>(Param))
					{
						Param->PropertyFlags |= CPF_ReferenceParm;

						// ALWAYS pass array parameters as out params, so they're set up as passed by ref
						Param->PropertyFlags |= CPF_OutParm;
					}

					*InCurrentParamStorageLocation = Param;
					InCurrentParamStorageLocation = &Param->Next;
				}
			}
			
			if(ReturnNodes.Num() > 0)
			{
				// Gather all input pins on these nodes, these are 
				// the outputs of the function:
				TSet<FString> UsedPinNames;
				for(UK2Node_FunctionResult* Node : ReturnNodes)
				{
					for(UEdGraphPin* Pin : Node->Pins)
					{
						if(!Schema->IsExecPin(*Pin) && Pin->ParentPin == nullptr)
						{								
							if(!UsedPinNames.Contains(Pin->PinName))
							{
								UsedPinNames.Add(Pin->PinName);
							
								UProperty* Param = FKismetCompilerUtilities::CreatePropertyOnScope(NewFunction, *(Pin->PinName), Pin->PinType, Ret, 0, Schema, MessageLog);
								Param->PropertyFlags |= CPF_Parm|CPF_ReturnParm;
								*InCurrentParamStorageLocation = Param;
								InCurrentParamStorageLocation = &Param->Next;
							}
						}
					}
				}
			}
		}
		return NewFunction;
	};

	// helpers:
	const auto AddFunctionForGraphs = [Schema, &MessageLog, ParentClass, Ret, BP, MakeFunction](const TCHAR* FunctionNamePostfix, const TArray<UEdGraph*>& Graphs, UField**& InCurrentFieldStorageLocation, bool bIsStaticFunction)
	{
		for( const UEdGraph* Graph : Graphs )
		{
			TArray<UK2Node_FunctionEntry*> EntryNodes;
			Graph->GetNodesOfClass(EntryNodes);
			if(EntryNodes.Num() > 0)
			{
				TArray<UK2Node_FunctionResult*> ReturnNodes;
				Graph->GetNodesOfClass(ReturnNodes);
				UK2Node_FunctionEntry* EntryNode = EntryNodes[0];
				
				UField** CurrentParamStorageLocation = nullptr;
				UFunction* NewFunction = MakeFunction(
					FName(*(Graph->GetName() + FunctionNamePostfix)), 
					InCurrentFieldStorageLocation, 
					CurrentParamStorageLocation, 
					bIsStaticFunction, 
					EntryNode->GetFunctionFlags(), 
					ReturnNodes, 
					EntryNode->Pins
				);

				if(NewFunction)
				{
					// locals:
					for( const FBPVariableDescription& BPVD : EntryNode->LocalVariables )
					{
						UProperty* NewProperty = FKismetCompilerUtilities::CreatePropertyOnScope(NewFunction, BPVD.VarName, BPVD.VarType, Ret, 0, Schema, MessageLog);
						*CurrentParamStorageLocation = NewProperty;
						CurrentParamStorageLocation = &NewProperty->Next;
					}

					// __WorldContext:
					if(bIsStaticFunction && FindField<UObjectProperty>(NewFunction, TEXT("__WorldContext")) == nullptr)
					{
						FEdGraphPinType WorldContextPinType(Schema->PC_Object, FString(), UObject::StaticClass(), false, false, false, false, FEdGraphTerminalType());
						UProperty* Param = FKismetCompilerUtilities::CreatePropertyOnScope(NewFunction, TEXT("__WorldContext"), WorldContextPinType, Ret, 0, Schema, MessageLog);
						Param->PropertyFlags |= CPF_Parm;
						*CurrentParamStorageLocation = Param;
						CurrentParamStorageLocation = &Param->Next;
					}

					NewFunction->Bind();
					NewFunction->StaticLink(true);
				}
			}
		}
	};

	UField** CurrentFieldStorageLocation = &Ret->Children;

	Ret->SetSuperStruct(ParentClass);
	
	/*Ret->ClassFlags |= (ParentClass->ClassFlags & CLASS_Inherit);
	Ret->ClassCastFlags |= ParentClass->ClassCastFlags;*/
	
	if (FBlueprintEditorUtils::IsInterfaceBlueprint(BP))
	{
		Ret->ClassFlags |= CLASS_Interface;
	}

	// link in delegate signatures, variables will reference these 
	AddFunctionForGraphs(HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX, BP->DelegateSignatureGraphs, CurrentFieldStorageLocation, false);

	// handle event entry ponts (mostly custom events) - this replaces
	// the skeleton compile pass CreateFunctionStubForEvent call:
	TArray<UEdGraph*> AllEventGraphs;
	
	for (UEdGraph* UberGraph : BP->UbergraphPages)
	{
		AllEventGraphs.Add(UberGraph);
		UberGraph->GetAllChildrenGraphs(AllEventGraphs);
	}

	for( const UEdGraph* Graph : AllEventGraphs )
	{
		TArray<UK2Node_Event*> EventNodes;
		Graph->GetNodesOfClass(EventNodes);
		for( UK2Node_Event* Event : EventNodes )
		{
			FName EventNodeName = CompilerContext.GetEventStubFunctionName(Event);

			UField** CurrentParamStorageLocation = nullptr;

			UFunction* NewFunction = MakeFunction(
				EventNodeName, 
				CurrentFieldStorageLocation, 
				CurrentParamStorageLocation, 
				false, 
				Event->FunctionFlags|FUNC_BlueprintCallable, 
				TArray<UK2Node_FunctionResult*>(), 
				Event->Pins
			);

			if(NewFunction)
			{
				NewFunction->Bind();
				NewFunction->StaticLink(true);
			}
		}
	}

	CompilerContext.NewClass = Ret;
	CompilerContext.bAssignDelegateSignatureFunction = true;
	CompilerContext.bGenerateSubInstanceVariables = true;
	CompilerContext.CreateClassVariablesFromBlueprint();
	CompilerContext.bAssignDelegateSignatureFunction = false;
	CompilerContext.bGenerateSubInstanceVariables = false;
	CompilerContext.NewClass = OriginalNewClass;
	UField* Iter = Ret->Children;
	while(Iter)
	{
		CurrentFieldStorageLocation = &Iter->Next;
		Iter = Iter->Next;
	}
	
	AddFunctionForGraphs(TEXT(""), BP->FunctionGraphs, CurrentFieldStorageLocation, BPTYPE_FunctionLibrary == BP->BlueprintType);

	CompilerContext.NewClass = Ret;
	CompilerContext.bAssignDelegateSignatureFunction = true;
	CompilerContext.FinishCompilingClass(Ret);
	CompilerContext.bAssignDelegateSignatureFunction = false;
	CompilerContext.NewClass = OriginalNewClass;

	Ret->Bind();
	Ret->StaticLink(/*bRelinkExistingProperties =*/true);

	return Ret;
}

// Singleton boilperplate:
FBlueprintCompilationManagerImpl* BPCMImpl = nullptr;

void FlushReinstancingQueueImplWrapper()
{
	BPCMImpl->FlushReinstancingQueueImpl();
}

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
void FBlueprintCompilationManager::FlushCompilationQueue(TArray<UObject*>* ObjLoaded)
{
	if(BPCMImpl)
	{
		BPCMImpl->FlushCompilationQueueImpl(ObjLoaded);
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

bool FBlueprintCompilationManager::IsGeneratedClassLayoutReady()
{
	if(!BPCMImpl)
	{
		// legacy behavior: always assume generated class layout is good:
		return true;
	}
	return BPCMImpl->IsGeneratedClassLayoutReady();
}


