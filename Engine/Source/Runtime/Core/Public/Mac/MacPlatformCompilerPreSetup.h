// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Clang/ClangPlatformCompilerPreSetup.h"

// Disable common CA warnings around SDK includes
#ifndef THIRD_PARTY_INCLUDES_START
	#define THIRD_PARTY_INCLUDES_START \
		PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS \
		PRAGMA_DISABLE_UNDEFINED_IDENTIFIER_WARNINGS \
		PRAGMA_DISABLE_MISSING_VIRTUAL_DESTRUCTOR_WARNINGS
#endif

#ifndef THIRD_PARTY_INCLUDES_END
	#define THIRD_PARTY_INCLUDES_END \
		PRAGMA_ENABLE_MISSING_VIRTUAL_DESTRUCTOR_WARNINGS \
		PRAGMA_ENABLE_UNDEFINED_IDENTIFIER_WARNINGS \
		PRAGMA_ENABLE_SHADOW_VARIABLE_WARNINGS
#endif

// Make certain warnings always be warnings, even despite -Werror.
// Rationale: we don't want to suppress those as there are plans to address them (e.g. UE-12341), but breaking builds due to these warnings is very expensive
// since they cannot be caught by all compilers that we support. They are deemed to be relatively safe to be ignored, at least until all SDKs/toolchains start supporting them.
#pragma clang diagnostic warning "-Wreorder"
#pragma clang diagnostic warning "-Wparentheses-equality"
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#pragma clang diagnostic ignored "-Wundefined-bool-conversion"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Wconstant-logical-operand"
#pragma clang diagnostic ignored "-Wreserved-user-defined-literal"

// Apple LLVM 9.0 (Xcode 9.0)
#if (__clang_major__ >= 9)
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
#endif

// Apple LLVM 8.1.0 (Xcode 8.3) introduced -Wundefined-var-template
#if (__clang_major__ > 8) || (__clang_major__ == 8 && __clang_minor__ >= 1)
#pragma clang diagnostic ignored "-Wundefined-var-template"
#pragma clang diagnostic ignored "-Wnullability-inferred-on-nested-type"
#pragma clang diagnostic ignored "-Wobjc-protocol-property-synthesis"
#pragma clang diagnostic ignored "-Wnullability-completeness-on-arrays"
#pragma clang diagnostic ignored "-Wnull-dereference"
#pragma clang diagnostic ignored "-Wnonportable-include-path" // Ideally this one would be set in MacToolChain, but we don't have a way to check the compiler version in there yet
#endif

#if (__clang_major__ > 8)
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
#endif

// We can use pragma optimisation's on and off as of Apple LLVM 7.3.0 but not before.
#if (__clang_major__ > 7) || (__clang_major__ == 7 && __clang_minor__ >= 3)
#define PRAGMA_DISABLE_OPTIMIZATION_ACTUAL _Pragma("clang optimize off")
#define PRAGMA_ENABLE_OPTIMIZATION_ACTUAL  _Pragma("clang optimize on")
#endif

// Apple LLVM 9.1.0 (Xcode 9.3)
#if (__clang_major__ > 9) || (__clang_major__ == 9 && __clang_minor__ >= 1)
#pragma clang diagnostic ignored "-Wunused-lambda-capture"
#endif
