// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

/**
 * Interface for platform feature modules
 */

/** Defines the interface to platform's save game system (or a generic file based one) */
class ISaveGameSystem
{
public:

	/** Returns true if the platform has a native UI (like many consoles) */
	virtual bool PlatformHasNativeUI() = 0;

	/** Return true if the named savegame exists (probably not useful with NativeUI */
	virtual bool DoesSaveGameExist(const TCHAR* Name) = 0;

	/** Saves the game, blocking until complete. Platform may use FGameDelegates to get more information from the game */
	virtual bool SaveGame(bool bAttemptToUseUI, const TCHAR* Name, const TArray<uint8>& Data) = 0;

	/** Loads the game, blocking until complete */
	virtual bool LoadGame(bool bAttemptToUseUI, const TCHAR* Name, TArray<uint8>& Data) = 0;
};



/** A generic save game system that just uses IFileManager to save/load with normal files */
class FGenericSaveGameSystem : public ISaveGameSystem
{
public:
	virtual bool PlatformHasNativeUI() OVERRIDE
	{
		return false;
	}

	virtual bool DoesSaveGameExist(const TCHAR* Name) OVERRIDE
	{
		return IFileManager::Get().FileSize(*GetSaveGamePath(Name)) >= 0;
	}

	virtual bool SaveGame(bool bAttemptToUseUI, const TCHAR* Name, const TArray<uint8>& Data) OVERRIDE
	{
		return FFileHelper::SaveArrayToFile(Data, *GetSaveGamePath(Name));
	}

	virtual bool LoadGame(bool bAttemptToUseUI, const TCHAR* Name, TArray<uint8>& Data) OVERRIDE
	{
		return FFileHelper::LoadFileToArray(Data, *GetSaveGamePath(Name));
	}

protected:

	/** Get the path to save game file for the given name, a platform _may_ be able to simply override this and no other functions above */
	virtual FString GetSaveGamePath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/SaveGames/%s.sav"), *FPaths::GameSavedDir(), Name);
	}
};
