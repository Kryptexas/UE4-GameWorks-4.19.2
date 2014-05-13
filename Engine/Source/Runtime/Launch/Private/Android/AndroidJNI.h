// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <jni.h>
#include <android/log.h>

extern JavaVM* GJavaVM;
extern jobject GJavaGlobalThis;

/*
 * NOTE -- At the moment, this is specific to GameActivity
*/

// Define all the Java classes/methods that the game will need to access to
class JDef_GameActivity
{
public:
	static jclass ClassID;

	// Nonstatic methods
	static jmethodID AndroidThunkJava_ShowConsoleWindow;
	static jmethodID AndroidThunkJava_LaunchURL;
	static jmethodID AndroidThunkJava_ShowLeaderboard;
	static jmethodID AndroidThunkJava_ShowAchievements;
	static jmethodID AndroidThunkJava_WriteLeaderboardValue;
	static jmethodID AndroidThunkJava_GooglePlayConnect;
	static jmethodID AndroidThunkJava_WriteAchievement;
	static jmethodID AndroidThunkJava_ShowAdBanner;
	static jmethodID AndroidThunkJava_HideAdBanner;
	static jmethodID AndroidThunkJava_CloseAdBanner;

	static jmethodID AndroidThunkJava_GetAssetManager;
};

// Returns the java environment
JNIEnv* GetJavaEnv(bool bRequireGlobalThis = true);
