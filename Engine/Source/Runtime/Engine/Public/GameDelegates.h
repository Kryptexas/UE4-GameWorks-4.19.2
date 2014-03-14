// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

/**
 * Collection of delegates for various components to call into game code
 */

#include "Delegate.h"

/** Delegate to modify cooking behavior - return extra packages to cook, load up the asset registry, etc */
// FCookModificationDelegate(TArray<FString>& ExtraPackagesToCook);
DECLARE_DELEGATE_OneParam(FCookModificationDelegate, TArray<FString>&);
// FAssignStreamingChunkDelegate(const FString& PackageToAdd, const FString& LastLoadedMapName, const TArray<int32>& AssetRegistryChunkIDs, const TArray<int32>& ExistingChunkIds, int32& OutChunkIndex);
DECLARE_DELEGATE_FiveParams(FAssignStreamingChunkDelegate, const FString&, const FString&, const TArray<int32>&, const TArray<int32>&, int32&);

/** A delegate for platforms that need extra information to flesh out save data information (name of an icon, for instance) */
// FExtendedSaveGameInfoDelegate(const TCHAR* SaveName, const TCHAR* Key, FString& Value); 
DECLARE_DELEGATE_ThreeParams(FExtendedSaveGameInfoDelegate, const TCHAR*, const TCHAR*, FString&);

/** A delegate for a web server running in engine to tell the game about events received from a client, and for game to respond to the client */
// using a TMap in the DECLARE_DELEGATE macro caused compiler problems (in clang anyway), a typedef solves it
typedef TMap<FString, FString> StringStringMap;
// FWebServerActionDelegate(int32 UserIndex, const FString& Action, const FString& URL, const& TMap<FString, FString> Params, TMap<FString, FString>& Response);
DECLARE_DELEGATE_FiveParams(FWebServerActionDelegate, int32, const FString&, const FString&, const StringStringMap&, StringStringMap&);




// a helper define to make defining the delegate members easy
#define DEFINE_GAME_DELEGATE(DelegateType) \
	public: F##DelegateType& Get##DelegateType() { return DelegateType; } \
	private: F##DelegateType DelegateType;

/** Class to set and get game callbacks */
class ENGINE_API FGameDelegates
{
public:

	/** Return a single FGameDelegates object */
	static FGameDelegates& Get();

	// Implement all delegates declared above
	DEFINE_GAME_DELEGATE(CookModificationDelegate);
	DEFINE_GAME_DELEGATE(AssignStreamingChunkDelegate);
	DEFINE_GAME_DELEGATE(ExtendedSaveGameInfoDelegate);
	DEFINE_GAME_DELEGATE(WebServerActionDelegate);	
};
