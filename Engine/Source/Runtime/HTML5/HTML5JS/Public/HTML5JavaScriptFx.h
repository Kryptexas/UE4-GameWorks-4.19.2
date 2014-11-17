// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma  once 

	extern "C" { 
		 bool UE_SendAndRecievePayLoad( char *URL, char* indata, int insize, char** outdata, int* outsize );
		 bool UE_DoesSaveGameExist(const char* Name, const int UserIndex);
		 bool UE_SaveGame(const char* Name, const int UserIndex, const char* indata, int insize);
		 bool UE_LoadGame(const char* Name, const int UserIndex, char **outdata, int* outsize);
	}

