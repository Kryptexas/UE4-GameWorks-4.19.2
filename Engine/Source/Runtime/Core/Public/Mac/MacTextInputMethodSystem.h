// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core.h"
#include "ITextInputMethodSystem.h"

class FMacTextInputMethodSystem : public ITextInputMethodSystem
{
public:
	virtual ~FMacTextInputMethodSystem() {}
	bool Initialize();
	void Terminate();
	
	// ITextInputMethodSystem Interface Begin
	virtual TSharedPtr<ITextInputMethodChangeNotifier> RegisterContext(const TSharedRef<ITextInputMethodContext>& Context) override;
	virtual void UnregisterContext(const TSharedRef<ITextInputMethodContext>& Context) override;
	virtual void ActivateContext(const TSharedRef<ITextInputMethodContext>& Context) override;
	virtual void DeactivateContext(const TSharedRef<ITextInputMethodContext>& Context) override;
	// ITextInputMethodSystem Interface End
	
private:
	TMap< TWeakPtr<ITextInputMethodContext>, TWeakPtr<ITextInputMethodChangeNotifier> > ContextMap;
};

