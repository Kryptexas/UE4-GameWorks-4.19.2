// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Misc/MessageDialog.h"
#include "Misc/CString.h"
#include "Logging/LogMacros.h"
#include "CoreGlobals.h"
#include "Internationalization/Text.h"
#include "Internationalization/Internationalization.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/FeedbackContext.h"
#include "Misc/CoreDelegates.h"
#include "Misc/App.h"

namespace
{
	/**
	 * Singleton to only create error title text if needed (and after localization system is in place)
	 */
	const FText& GetDefaultMessageTitle()
	{
		// Will be initialised on first call
		static FText DefaultMessageTitle(NSLOCTEXT("MessageDialog", "DefaultMessageTitle", "Message"));
		return DefaultMessageTitle;
	}
}

void FMessageDialog::Debugf( const FText& Message, const FText* OptTitle )
{
	if( FApp::IsUnattended() == true )
	{
		GLog->Logf( TEXT("%s"), *Message.ToString() );
	}
	else
	{
		FText Title = OptTitle ? *OptTitle : GetDefaultMessageTitle();
		if ( GIsEditor && FCoreDelegates::ModalErrorMessage.IsBound() )
		{
			FCoreDelegates::ModalErrorMessage.Execute(EAppMsgType::Ok, Message, Title);
		}
		else
		{
			FPlatformMisc::MessageBoxExt( EAppMsgType::Ok, *Message.ToString(), *NSLOCTEXT("MessageDialog", "DefaultDebugMessageTitle", "ShowDebugMessagef").ToString() );
		}
	}
}

void FMessageDialog::ShowLastError()
{
	uint32 LastError = FPlatformMisc::GetLastError();

	TCHAR TempStr[MAX_SPRINTF]=TEXT("");
	TCHAR ErrorBuffer[1024];
	FCString::Sprintf( TempStr, TEXT("GetLastError : %d\n\n%s"), LastError, FPlatformMisc::GetSystemErrorMessage(ErrorBuffer, 1024, 0) );
	if( FApp::IsUnattended() == true )
	{
		UE_LOG(LogOutputDevice, Fatal, TempStr);
	}
	else
	{
		FPlatformMisc::MessageBoxExt( EAppMsgType::Ok, TempStr, *NSLOCTEXT("MessageDialog", "DefaultSystemErrorTitle", "System Error").ToString() );
	}
}

EAppReturnType::Type FMessageDialog::Open( EAppMsgType::Type MessageType, const FText& Message, const FText* OptTitle )
{
	EAppReturnType::Type DefaultValue = EAppReturnType::Yes;
	switch(MessageType)
	{
	case EAppMsgType::Ok:
		DefaultValue = EAppReturnType::Ok;
		break;
	case EAppMsgType::YesNo:
		DefaultValue = EAppReturnType::No;
		break;
	case EAppMsgType::OkCancel:
		DefaultValue = EAppReturnType::Cancel;
		break;
	case EAppMsgType::YesNoCancel:
		DefaultValue = EAppReturnType::Cancel;
		break;
	case EAppMsgType::CancelRetryContinue:
		DefaultValue = EAppReturnType::Cancel;
		break;
	case EAppMsgType::YesNoYesAllNoAll:
		DefaultValue = EAppReturnType::No;
		break;
	case EAppMsgType::YesNoYesAllNoAllCancel:
	default:
		DefaultValue = EAppReturnType::Yes;
		break;
	}

	if (GIsRunningUnattendedScript && MessageType != EAppMsgType::Ok)
	{
		if (GWarn)
		{
			GWarn->Logf(TEXT("Message Dialog was triggered in unattended script mode without a default value. %d will be used."), (int32)DefaultValue);
		}

		if (FPlatformMisc::IsDebuggerPresent())
		{
			UE_DEBUG_BREAK();
		}
		else
		{
			FDebug::DumpStackTraceToLog();
		}
	}

	return Open(MessageType, DefaultValue, Message, OptTitle);
}

EAppReturnType::Type FMessageDialog::Open(EAppMsgType::Type MessageType, EAppReturnType::Type DefaultValue, const FText& Message, const FText* OptTitle)
{
	if ( FApp::IsUnattended() == true || GIsRunningUnattendedScript )
	{
		if (GWarn)
		{
			GWarn->Logf( TEXT("%s"), *Message.ToString() );
		}

		return DefaultValue;
	}
	else
	{
		FText Title = OptTitle ? *OptTitle : GetDefaultMessageTitle();
		if ( GIsEditor && !IsRunningCommandlet() && FCoreDelegates::ModalErrorMessage.IsBound() )
		{
			return FCoreDelegates::ModalErrorMessage.Execute( MessageType, Message, Title );
		}
		else
		{
			return FPlatformMisc::MessageBoxExt( MessageType, *Message.ToString(), *Title.ToString() );
		}
	}
}
