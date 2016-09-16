// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "ErrorException.h"
#include "ModuleManager.h"
#include "ScriptInterface.h"			// Script interface definitions.
#include "Script.h"						// Script constants and forward declarations.
#include "ScriptMacros.h"				// Script macro definitions
#include "ObjectMacros.h"				// Object macros.
#include "UObjectAllocator.h"
#include "UObjectGlobals.h"
#include "UObjectMarks.h"
#include "UObjectBase.h"
#include "UObjectBaseUtility.h"
#include "UObjectArray.h"
#include "UObjectHash.h"
#include "WeakObjectPtr.h"
#include "Object.h"
#include "UObjectIterator.h"
#include "CoreNet.h"					// Core networking.
#include "ArchiveUObject.h"				// UObject-related Archive classes.
#include "GarbageCollection.h"			// Realtime garbage collection helpers
#include "TextBuffer.h"					// UObjects for text buffers
#include "Class.h"						// Class definition.
#include "Templates/SubclassOf.h"
#include "StructOnScope.h"
#include "Casts.h"                      // Cast templates
#include "LazyObjectPtr.h"				// Object pointer types
#include "AssetPtr.h"					// Object pointer types
#include "Interface.h"					// Interface base classes.
#include "LevelGuids.h"					// Core object class definitions.
#include "Package.h"					// Package class definition
#include "MetaData.h"					// Metadata for packages
#include "UnrealType.h"					// Base property type.
#include "TextProperty.h"				// FText property type
#include "Stack.h"						// Script stack frame definition.
#include "ObjectRedirector.h"			// Cross-package object redirector
#include "UObjectAnnotation.h"
#include "ObjectMemoryAnalyzer.h"		// Object memory usage analyzer
#include "ReferenceChainSearch.h"		// Search for referencers of an objects
#include "ArchiveUObject.h"				// UObject-related Archive classes.
#include "AsyncFileHandle.h"
#include "TextPackageNamespaceUtil.h"
#include "ArchiveCountMem.h"
#include "ObjectAndNameAsStringProxyArchive.h"
#include "ObjectWriter.h"
#include "ObjectReader.h"
#include "ReloadObjectArc.h"
#include "ArchiveReplaceArchetype.h"
#include "ArchiveShowReferences.h"
#include "FindReferencersArchive.h"
#include "FindObjectReferencers.h"
#include "ArchiveFindCulprit.h"
#include "ArchiveObjectGraph.h"
#include "TraceReferences.h"
#include "ArchiveTraceRoute.h"
#include "DuplicatedObject.h"
#include "DuplicatedDataReader.h"
#include "DuplicatedDataWriter.h"
#include "ArchiveReplaceObjectRef.h"
#include "ArchiveReplaceOrClearExternalReferences.h"
#include "ArchiveObjectPropertyMapper.h"
#include "ArchiveReferenceMarker.h"
#include "ArchiveAsync.h"
#include "ArchiveObjectCrc32.h"
#include "PackageName.h"
#include "ConstructorHelpers.h"			// Helper templates for object construction.
#include "BulkData.h"					// Bulk data classes
#include "Linker.h"						// Linker.
#include "LinkerLoad.h"
#include "LinkerSave.h"
#include "GCObject.h"			        // non-UObject object referencer
#include "AsyncLoading.h"				// FAsyncPackage definition
#include "StartupPackages.h"
#include "NotifyHook.h"
#include "RedirectCollector.h"
#include "ScriptStackTracker.h"
#include "WorldCompositionUtility.h"
#include "StringClassReference.h"
