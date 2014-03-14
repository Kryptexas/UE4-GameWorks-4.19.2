// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITextInputMethodSystem.h"
#include "TextStoreACP.h"

#include "AllowWindowsPlatformTypes.h"
#include <msctf.h>
#include "COMPointer.h"

class FTextStoreACP;

class FWindowsTextInputMethodSystem : public ITextInputMethodSystem
{
public:
	bool Initialize();
	void Terminate();

	// ITextInputMethodSystem Interface Begin
	virtual TSharedPtr<ITextInputMethodChangeNotifier> RegisterContext(const TSharedRef<ITextInputMethodContext>& Context) OVERRIDE;
	virtual void UnregisterContext(const TSharedRef<ITextInputMethodContext>& Context) OVERRIDE;
	virtual void ActivateContext(const TSharedRef<ITextInputMethodContext>& Context) OVERRIDE;
	virtual void DeactivateContext(const TSharedRef<ITextInputMethodContext>& Context) OVERRIDE;
	// ITextInputMethodSystem Interface End

private:
	FCOMPtr<ITfThreadMgr> TextFrameworkThreadManager;
	TfClientId TextFrameworkClientId;

	TMap< TWeakPtr<ITextInputMethodContext>, FCOMPtr<FTextStoreACP> > ContextToStoreMap;
};

#include "HideWindowsPlatformTypes.h"