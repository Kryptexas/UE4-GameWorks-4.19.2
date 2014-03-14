// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LaunchPrivatePCH.h"
#include "ExceptionHandling.h"

#include "AndroidJNI.h"

#define JNI_CURRENT_VERSION JNI_VERSION_1_6

JavaVM* GJavaVM;
jobject GJavaGlobalThis = NULL;

//////////////////////////////////////////////////////////////////////////
// FJNIHelper

// Caches access to the environment, attached to the current thread
class FJNIHelper : public FThreadSingleton<FJNIHelper>
{
public:
	static JNIEnv* GetEnvironment()
	{
		return Get().CachedEnv;
	}

private:
	JNIEnv* CachedEnv = NULL;

private:
	friend class FThreadSingleton<FJNIHelper>;

	FJNIHelper()
		: CachedEnv(nullptr)
	{
		check(GJavaVM);
		GJavaVM->GetEnv((void **)&CachedEnv, JNI_CURRENT_VERSION);

		const jint AttachResult = GJavaVM->AttachCurrentThread(&CachedEnv, nullptr);
		if (AttachResult == JNI_ERR)
		{
			FPlatformMisc::LowLevelOutputDebugString(TEXT("FJNIHelper failed to attach thread to Java VM!"));
			check(false);
		}
	}

	~FJNIHelper()
	{
		check(GJavaVM);
		const jint DetachResult = GJavaVM->DetachCurrentThread();
		if (DetachResult == JNI_ERR)
		{
			FPlatformMisc::LowLevelOutputDebugString(TEXT("FJNIHelper failed to detach thread from Java VM!"));
			check(false);
		}

		CachedEnv = nullptr;
	}
};

template<> uint32 FThreadSingleton<FJNIHelper>::TlsSlot = 0;

JNIEnv* GetJavaEnv(bool bRequireGlobalThis = true)
{
	//@TODO: ANDROID: Remove the other version if the helper works well
#if 0
	if (!bRequireGlobalThis || (GJavaGlobalThis != nullptr))
	{
		return FJNIHelper::GetEnvironment();
	}
	else
	{
		return nullptr;
	}
#else
	JNIEnv* Env = NULL;
	GJavaVM->GetEnv((void **)&Env, JNI_CURRENT_VERSION);

	jint AttachResult = GJavaVM->AttachCurrentThread(&Env, NULL);
	if (AttachResult == JNI_ERR)
	{
		FPlatformMisc::LowLevelOutputDebugString(L"UNIT TEST -- Failed to get the JNI environment!");
		check(false);
		return nullptr;
	}

	return (!bRequireGlobalThis || (GJavaGlobalThis != nullptr)) ? Env : nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////

//Declare all the static members of the class defs 
jclass JDef_GameActivity::ClassID;
jmethodID JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow;
jmethodID JDef_GameActivity::AndroidThunkJava_LaunchURL;

DEFINE_LOG_CATEGORY_STATIC(LogEngine, Log, All);

//Game-specific crash reporter
void EngineCrashHandler(const FGenericCrashContext & GenericContext)
{
	const FAndroidCrashContext& Context = static_cast< const FAndroidCrashContext& >( GenericContext );

	static int32 bHasEntered = 0;
	if (FPlatformAtomics::InterlockedCompareExchange(&bHasEntered, 1, 0) == 0)
	{
		const SIZE_T StackTraceSize = 65535;
		ANSICHAR* StackTrace = (ANSICHAR*)FMemory::Malloc(StackTraceSize);
		StackTrace[0] = 0;

		// Walk the stack and dump it to the allocated memory.
		FPlatformStackWalk::StackWalkAndDump(StackTrace, StackTraceSize, 0, Context.Context);
		UE_LOG(LogEngine, Error, TEXT("%s"), ANSI_TO_TCHAR(StackTrace));
		FMemory::Free(StackTrace);

		GError->HandleError();
		FPlatformMisc::RequestExit(true);
	}
}

void AndroidThunkCpp_ShowConsoleWindow()
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		// figure out all the possible texture formats that are allowed
		TArray<FString> PossibleTargetPlatforms;
		FPlatformMisc::GetValidTargetPlatforms(PossibleTargetPlatforms);

		// separate the format suffixes with commas
		FString ConsoleText;
		for (int32 FormatIndex = 0; FormatIndex < PossibleTargetPlatforms.Num(); FormatIndex++)
		{
			const FString& Format = PossibleTargetPlatforms[FormatIndex];
			int32 UnderscoreIndex;
			if (Format.FindLastChar('_', UnderscoreIndex))
			{
				if (ConsoleText != TEXT(""))
				{
					ConsoleText += ", ";
				}
	
				ConsoleText += Format.Mid(UnderscoreIndex+1);
			}
		}

		// call the java side
		jstring ConsoleTextJava	= Env->NewStringUTF(TCHAR_TO_UTF8(*ConsoleText));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow, ConsoleTextJava);
		Env->DeleteLocalRef(ConsoleTextJava);
	}
}

void AndroidThunkCpp_LaunchURL(const FString& URL)
{
	if (JNIEnv* Env = GetJavaEnv())
	{
		jstring Argument = Env->NewStringUTF(TCHAR_TO_UTF8(*URL));
		Env->CallVoidMethod(GJavaGlobalThis, JDef_GameActivity::AndroidThunkJava_LaunchURL, Argument);
		Env->DeleteLocalRef(Argument);
	}
}

//The JNI_OnLoad function is triggered by loading the game library from 
//the Java source file.
//	static
//	{
//		System.loadLibrary("MyGame");
//	}
//
// Use the JNI_OnLoad function to map all the class IDs and method IDs to their respective
// variables. That way, later when the Java functions need to be called, the IDs will be ready.
// It is much slower to keep looking up the class and method IDs.
JNIEXPORT jint JNI_OnLoad(JavaVM* InJavaVM, void* InReserved)
{
	FPlatformMisc::LowLevelOutputDebugString(L"In the JNI_OnLoad function");

	JNIEnv* env = NULL;
	InJavaVM->GetEnv((void **)&env, JNI_CURRENT_VERSION);

	GJavaVM = InJavaVM;

	JDef_GameActivity::ClassID = env->FindClass("com/epicgames/ue4/GameActivity");

	JDef_GameActivity::AndroidThunkJava_ShowConsoleWindow = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_ShowConsoleWindow", "(Ljava/lang/String;)V");
	JDef_GameActivity::AndroidThunkJava_LaunchURL = env->GetMethodID(JDef_GameActivity::ClassID, "AndroidThunkJava_LaunchURL", "(Ljava/lang/String;)V");

	// hook signals
#if UE_BUILD_DEBUG
	if( GAlwaysReportCrash )
#else
	if( !FPlatformMisc::IsDebuggerPresent() || GAlwaysReportCrash )
#endif
	{
		FPlatformMisc::SetCrashHandler(EngineCrashHandler);
	}

	// Wire up to core delegates, so core code can call out to Java
	DECLARE_DELEGATE_OneParam(FAndroidLaunchURLDelegate, const FString&);
	extern CORE_API FAndroidLaunchURLDelegate OnAndroidLaunchURL;
	OnAndroidLaunchURL = FAndroidLaunchURLDelegate::CreateStatic(&AndroidThunkCpp_LaunchURL);

	return JNI_CURRENT_VERSION;
}

//Native-defined functions

//This function is declared in the Java-defined class, GameActivity.java: "public native void nativeSetGlobalActivity();"
extern "C" void Java_com_epicgames_ue4_GameActivity_nativeSetGlobalActivity(JNIEnv* jenv, jobject thiz)
{
	if (!GJavaGlobalThis)
	{
		GJavaGlobalThis = jenv->NewGlobalRef(thiz);
		if(!GJavaGlobalThis)
		{
			FPlatformMisc::LowLevelOutputDebugString(L"Error setting the global GameActivity activity");
			check(false);
		}
	}
}


extern "C" bool Java_com_epicgames_ue4_GameActivity_nativeIsShippingBuild(JNIEnv* LocalJNIEnv, jobject LocalThiz)
{
#if UE_BUILD_SHIPPING
	return JNI_TRUE;
#else
	return JNI_FALSE;
#endif
}

