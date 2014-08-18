// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	WindowsPlatformIncludes.h: Includes the platform specific headers for windows
==================================================================================*/
#pragma once

// Set up compiler pragmas, etc
#include "Windows/WindowsPlatformCompilerSetup.h"

// include windows.h
#include "Windows/PreWindowsApi.h"
#ifndef STRICT
#define STRICT
#endif
#include "Windows/MinWindows.h"
#include "Windows/PostWindowsApi.h"

// Macro for releasing COM objects
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// Current instance
extern "C" CORE_API HINSTANCE hInstance; 

// SIMD intrinsics
#include <intrin.h>

#include <stdint.h>
#include <tchar.h>

// @todo clang: When compiling under Clang for Windows, these headers cause us particular problems.  We need to make sure they're included
// before we pull in our 'DoNotUseOldUE4Type' namespace.  This is because these headers will redeclare various numeric typedefs, but under
// the Clang compiler it is not allowed to define a typedef with a global scope operator in it (such as ::INT).  So we'll get these headers
// included early on to avoid compiler errors with that.
#if defined(__clang__)
	#include <intsafe.h>
	#include <strsafe.h>
#endif

#if USING_CODE_ANALYSIS
// Source annotation support
#include <CodeAnalysis/SourceAnnotations.h>

// Allows for disabling code analysis warnings by defining ALL_CODE_ANALYSIS_WARNINGS
#include <CodeAnalysis/Warnings.h>

// We import the vc_attributes namespace so we can use annotations more easily.  It only defines
// MSVC-specific attributes so there should never be collisions.
using namespace vc_attributes;
#endif

// include platform implementations
#include "Windows/WindowsPlatformMemory.h"
#include "Windows/WindowsPlatformString.h"
#include "Windows/WindowsPlatformMisc.h"
#include "Windows/WindowsPlatformStackWalk.h"
#include "Windows/WindowsPlatformMath.h"
#include "Windows/WindowsPlatformNamedPipe.h"
#include "Windows/WindowsPlatformTime.h"
#include "Windows/WindowsPlatformProcess.h"
#include "Windows/WindowsPlatformOutputDevices.h"
#include "Windows/WindowsPlatformAtomics.h"
#include "Windows/WindowsPlatformTLS.h"
#include "Windows/WindowsPlatformSplash.h"
#include "Windows/WindowsPlatformFile.h"
#include "Windows/WindowsPlatformSurvey.h"
#include "Windows/WindowsPlatformHttp.h"

typedef FGenericPlatformAffinity FPlatformAffinity;

// include platform properties and typedef it for the runtime

#include "Windows/WindowsPlatformProperties.h"

#ifndef WITH_EDITORONLY_DATA
#error "WITH_EDITORONLY_DATA must be defined"
#endif

#ifndef UE_SERVER
#error "WITH_EDITORONLY_DATA must be defined"
#endif

typedef FWindowsPlatformProperties<WITH_EDITORONLY_DATA, UE_SERVER, !WITH_SERVER_CODE> FPlatformProperties;

