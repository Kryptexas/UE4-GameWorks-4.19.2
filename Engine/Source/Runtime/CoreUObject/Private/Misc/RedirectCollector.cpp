// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Misc/RedirectCollector.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Object.h"
#include "UObject/Package.h"
#include "Templates/Casts.h"
#include "UObject/UnrealType.h"
#include "UObject/ObjectRedirector.h"
#include "Misc/PackageName.h"
#include "UObject/LinkerLoad.h"
#include "UObject/UObjectThreadContext.h"

#if WITH_EDITOR

DEFINE_LOG_CATEGORY_STATIC(LogRedirectors, Log, All);

void FRedirectCollector::OnStringAssetReferenceLoaded(const FString& InString)
{
	FPackagePropertyPair ContainingPackageAndProperty;

	if (InString.IsEmpty())
	{
		// No need to track empty strings
		return;
	}

	FStringAssetReferenceThreadContext& ThreadContext = FStringAssetReferenceThreadContext::Get();

	FName PackageName, PropertyName;
	EStringAssetReferenceCollectType CollectType = EStringAssetReferenceCollectType::AlwaysCollect;

	ThreadContext.GetSerializationOptions(PackageName, PropertyName, CollectType);

	if (CollectType == EStringAssetReferenceCollectType::NeverCollect)
	{
		// Do not track
		return;
	}

	if (PackageName != NAME_None)
	{
		ContainingPackageAndProperty.SetPackage(PackageName);
		if (PropertyName != NAME_None)
		{
			ContainingPackageAndProperty.SetProperty(PropertyName);
		}
		ContainingPackageAndProperty.SetReferencedByEditorOnlyProperty(CollectType == EStringAssetReferenceCollectType::EditorOnlyCollect);
	}

	FScopeLock ScopeLock(&CriticalSection);

	StringAssetReferences.AddUnique(FName(*InString), ContainingPackageAndProperty);
}

FString FRedirectCollector::OnStringAssetReferenceSaved(const FString& InString)
{
	FScopeLock ScopeLock(&CriticalSection);

	FString* Found = AssetPathRedirectionMap.Find(InString);
	if (Found)
	{
		return *Found;
	}
	return InString;
}

#define REDIRECT_TIMERS 1
#if REDIRECT_TIMERS
struct FRedirectScopeTimer
{
	double& Time;
	double StartTime;
	FRedirectScopeTimer(double& InTime) : Time(InTime)
	{
		StartTime = FPlatformTime::Seconds();
	}
	~FRedirectScopeTimer()
	{
		Time += FPlatformTime::Seconds() - StartTime;
	}
};

double ResolveTimeTotal;
double ResolveTimeLoad;
double ResolveTimeDelegate;

#define SCOPE_REDIRECT_TIMER(TimerName) FRedirectScopeTimer Timer##TimerName(TimerName);

#define LOG_REDIRECT_TIMER( TimerName) UE_LOG(LogRedirectors, Display, TEXT("Timer %ls %f"), TEXT(#TimerName), TimerName);

#define LOG_REDIRECT_TIMERS() \
	LOG_REDIRECT_TIMER(ResolveTimeLoad); \
	LOG_REDIRECT_TIMER(ResolveTimeDelegate);\
	LOG_REDIRECT_TIMER(ResolveTimeTotal);\

#else
#define SCOPE_REDIRECT_TIMER(TimerName)
#define LOG_REDIRECT_TIMERS()
#endif

void FRedirectCollector::LogTimers() const
{
	LOG_REDIRECT_TIMERS();
}

void FRedirectCollector::ResolveStringAssetReference(FName FilterPackageFName, bool bProcessAlreadyResolvedPackages)
{
	SCOPE_REDIRECT_TIMER(ResolveTimeTotal);
	
	FScopeLock ScopeLock(&CriticalSection);

	TMultiMap<FName, FPackagePropertyPair> SkippedReferences;
	SkippedReferences.Empty(StringAssetReferences.Num());
	while ( StringAssetReferences.Num())
	{

		TMultiMap<FName, FPackagePropertyPair> CurrentReferences;
		Swap(StringAssetReferences, CurrentReferences);

		for (const auto& CurrentReference : CurrentReferences)
		{
			const FName& ToLoadFName = CurrentReference.Key;
			const FPackagePropertyPair& RefFilenameAndProperty = CurrentReference.Value;

			if ((FilterPackageFName != NAME_None) && // not using a filter
				(FilterPackageFName != RefFilenameAndProperty.GetCachedPackageName()) && // this is the package we are looking for
				(RefFilenameAndProperty.GetCachedPackageName() != NAME_None) // if we have an empty package name then process it straight away
				)
			{
				// If we have a valid filter and it doesn't match, skip this reference
				SkippedReferences.Add(ToLoadFName, RefFilenameAndProperty);
				continue;
			}

			const FString ToLoad = ToLoadFName.ToString();

			if (FCoreDelegates::LoadStringAssetReferenceInCook.IsBound())
			{
				SCOPE_REDIRECT_TIMER(ResolveTimeDelegate);
				if (FCoreDelegates::LoadStringAssetReferenceInCook.Execute(ToLoad) == false)
				{
					// Skip this reference
					continue;
				}
			}
			if ( (bProcessAlreadyResolvedPackages == false) && 
				AlreadyRemapped.Contains(ToLoadFName) )
			{
				// don't load this package again because we already did it once before
				// once is enough for somethings.
				continue;
			}
			if (ToLoad.Len() > 0 )
			{
				SCOPE_REDIRECT_TIMER(ResolveTimeLoad);

				UE_LOG(LogRedirectors, Verbose, TEXT("String Asset Reference '%s'"), *ToLoad);
				UE_CLOG(RefFilenameAndProperty.GetProperty().ToString().Len(), LogRedirectors, Verbose, TEXT("    Referenced by '%s'"), *RefFilenameAndProperty.GetProperty().ToString());

				// LoadObject will mark any failed loads as "KnownMissing", so since we want to print out the referencer if it was missing and not-known-missing
				// then check here before attempting to load (TODO: Can we just not even do the load? Seems like we should be able to...)

				int32 DotIndex = ToLoad.Find(TEXT("."));
				FString PackageName = DotIndex != INDEX_NONE ? ToLoad.Left(DotIndex) : ToLoad;

				// If is known missing don't try
				if (FLinkerLoad::IsKnownMissingPackage(FName(*PackageName)))
				{
					continue;
				}

				UObject *Loaded = LoadObject<UObject>(NULL, *ToLoad, NULL, RefFilenameAndProperty.GetReferencedByEditorOnlyProperty() ? LOAD_EditorOnly | LOAD_NoWarn : LOAD_NoWarn, NULL);

				if (Loaded)
				{
					if (FCoreUObjectDelegates::PackageLoadedFromStringAssetReference.IsBound())
					{
						FCoreUObjectDelegates::PackageLoadedFromStringAssetReference.Broadcast(Loaded->GetOutermost()->GetFName());
					}
					FString Dest = Loaded->GetPathName();
					UE_LOG(LogRedirectors, Verbose, TEXT("    Resolved to '%s'"), *Dest);
					if (Dest != ToLoad)
					{
						AssetPathRedirectionMap.Add(ToLoad, Dest);
					}
					AlreadyRemapped.Add(ToLoadFName);
				}
				else
				{
					const FString Referencer = RefFilenameAndProperty.GetProperty().ToString().Len() ? RefFilenameAndProperty.GetProperty().ToString() : TEXT("Unknown");
					UE_LOG(LogRedirectors, Warning, TEXT("String Asset Reference '%s' was not found! (Referencer '%s')"), *ToLoad, *Referencer);
				}
			}

		}
	}

	check(StringAssetReferences.Num() == 0);
	// Add any skipped references back into the map for the next time this is called
	Swap(StringAssetReferences, SkippedReferences);
	// we shouldn't have any references left if we decided to resolve them all
	check((StringAssetReferences.Num() == 0) || (FilterPackageFName != NAME_None));
}

void FRedirectCollector::ProcessStringAssetReferencePackageList(FName FilterPackage, bool bGetEditorOnly, TSet<FName>& OutReferencedPackages)
{
	FScopeLock ScopeLock(&CriticalSection);

	// Iterate map, remove all matching and potentially add to OutReferencedPackages
	for (auto It = StringAssetReferences.CreateIterator(); It; ++It)
	{
		const FName& ToLoadFName = It->Key;
		const FPackagePropertyPair& RefFilenameAndProperty = It->Value;

		// Package name may be null, if so return the set of things not associated with a package
		if (FilterPackage == RefFilenameAndProperty.GetCachedPackageName())
		{
			FString PackageNameString = FPackageName::ObjectPathToPackageName(ToLoadFName.ToString());

			if (!RefFilenameAndProperty.GetReferencedByEditorOnlyProperty() || bGetEditorOnly)
			{
				OutReferencedPackages.Add(FName(*PackageNameString));
			}

			It.RemoveCurrent();
		}
	}

	StringAssetReferences.Compact();
}

void FRedirectCollector::AddAssetPathRedirection(const FString& OriginalPath, const FString& RedirectedPath)
{
	FScopeLock ScopeLock(&CriticalSection);

	// This replaces an existing mapping, can happen in the editor if things are renamed twice
	AssetPathRedirectionMap.Add(OriginalPath, RedirectedPath);
}

void FRedirectCollector::RemoveAssetPathRedirection(const FString& OriginalPath)
{
	FScopeLock ScopeLock(&CriticalSection);

	FString* Found = AssetPathRedirectionMap.Find(OriginalPath);

	if (ensureMsgf(Found, TEXT("Cannot remove redirection from %s, it was not registered"), *OriginalPath))
	{
		AssetPathRedirectionMap.Remove(OriginalPath);
	}
}

FString* FRedirectCollector::GetAssetPathRedirection(const FString& OriginalPath)
{
	FScopeLock ScopeLock(&CriticalSection);

	return AssetPathRedirectionMap.Find(OriginalPath);
}

FRedirectCollector GRedirectCollector;

#endif // WITH_EDITOR
