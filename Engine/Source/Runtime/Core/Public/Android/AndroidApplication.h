// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericApplication.h"
#include "AndroidWindow.h"

namespace FAndroidAppEntry
{
	void PlatformInit();
	void ReInitWindow();
	void DestroyWindow();
}

struct FPlatformOpenGLContext;
namespace FAndroidEGL
{
	// Back door into more intimate Android OpenGL variables (a.k.a. a hack)
	FPlatformOpenGLContext*	GetRenderingContext();
	FPlatformOpenGLContext*	CreateContext();
	void					MakeCurrent(FPlatformOpenGLContext*);
	void					ReleaseContext(FPlatformOpenGLContext*);
	void					SwapBuffers(FPlatformOpenGLContext*);
	void					SetFlipsEnabled(bool Enabled);
	void					BindDisplayToContext(FPlatformOpenGLContext*);
}


class FAndroidApplication : public GenericApplication
{
public:

	static FAndroidApplication* CreateAndroidApplication();

	// Returns the java environment
	static void InitializeJavaEnv(JavaVM* VM, jint Version, jobject GlobalThis);
	static JNIEnv* GetJavaEnv(bool bRequireGlobalThis = true);
	static jclass FindJavaClass(const char* name);
	static void DetachJavaEnv();


public:	
	
	virtual ~FAndroidApplication() {}

	void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	virtual void PollGameDeviceState( const float TimeDelta ) override;

	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const override;

	virtual IForceFeedbackSystem *GetForceFeedbackSystem() override;

	virtual TSharedRef< FGenericWindow > MakeWindow() override;
	
	void InitializeWindow( const TSharedRef< FGenericWindow >& InWindow, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FGenericWindow >& InParent, const bool bShowImmediately );

private:

	FAndroidApplication();


private:

	TSharedPtr< class FAndroidInputInterface > InputInterface;

	TArray< TSharedRef< FAndroidWindow > > Windows;
};
