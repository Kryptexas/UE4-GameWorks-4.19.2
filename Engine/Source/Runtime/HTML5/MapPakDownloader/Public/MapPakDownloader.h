// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"

class FMapPakDownloader
{

public: 

	FMapPakDownloader();
	
	/**
	 *  Setup hostname and pak directories.
	 */ 
	bool Init();

	/**
	 *   Cache Map if needed and then transition			                     
	 */
	void Cache(FString& Url, FString& LastUrl, void* DynData);

private: 

	/**
	 *   http download pak file and then transition to requested map. 															
	 */
	void CachePak();

	// URL being Cached.
	FString MapToCache; 

	// Last URL. 
	FString LastMap;

	// Dynamic Data.
	void* DynData; 
	
	// Server where paks are located. 
	FString HostName;

	// Relative dir of Pak files 
	FString PakLocation;

};

