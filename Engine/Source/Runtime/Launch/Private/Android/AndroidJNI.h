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
};
