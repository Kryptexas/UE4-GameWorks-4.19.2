// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsNativeFeedbackContext.h: Unreal Windows user interface interaction.
=============================================================================*/

#pragma once

#include "AllowWindowsPlatformTypes.h"
#include <CommCtrl.h>

/**
 * Feedback context implementation for windows.
 */
class FWindowsNativeFeedbackContext : public FFeedbackContext
{
public:
	// Constructor.
	FWindowsNativeFeedbackContext();
	virtual ~FWindowsNativeFeedbackContext();

	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE;

	VARARG_BODY( bool, YesNof, const TCHAR*, VARARG_NONE );

	virtual bool ReceivedUserCancel() OVERRIDE;
	virtual void BeginSlowTask( const FText& Task, bool bShowProgressDialog, bool bInShowCancelButton=false ) OVERRIDE;
	virtual void EndSlowTask() OVERRIDE;

	virtual bool StatusUpdate( int32 Numerator, int32 Denominator, const FText& NewStatus ) OVERRIDE;
	virtual bool StatusForceUpdate( int32 Numerator, int32 Denominator, const FText& StatusText ) OVERRIDE;
	virtual void UpdateProgress(int32 Numerator, int32 Denominator) OVERRIDE;

	FContextSupplier* GetContext() const;
	void SetContext( FContextSupplier* InSupplier );

private:
	struct FBufferedLine
	{
		FString Text;
		ELogVerbosity::Type Verbosity;
		FName Category;
	};

	struct FWindowParams
	{
		FWindowsNativeFeedbackContext* Context;
		int32 ScaleX;
		int32 ScaleY;
		int32 StandardW;
		int32 StandardH;
		bool bLogVisible;
	};

	static const uint16 StatusCtlId = 200;
	static const uint16 ProgressCtlId = 201;
	static const uint16 ShowLogCtlId = 202;
	static const uint16 LogOutputCtlId = 203;

	FContextSupplier* Context;
	int32 SlowTaskCount;
	HANDLE hThread;
	HANDLE hCloseEvent;
	HANDLE hUpdateEvent;
	FCriticalSection CriticalSection;
	FString Status;
	float Progress;
	FString LogOutput;
	bool bReceivedUserCancel;
	bool bShowCancelButton;

	void CreateSlowTaskWindow(const FText &InStatus, bool bInShowCancelButton);
	void DestroySlowTaskWindow();

	static DWORD WINAPI SlowTaskThreadProc(void *Params);
	static LRESULT CALLBACK SlowTaskWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	static void LayoutControls(HWND hWnd, const FWindowParams* Params);
};

#include "HideWindowsPlatformTypes.h"
