// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITextInputMethodSystem.h"
#include "TextStoreACP.h"

#include "AllowWindowsPlatformTypes.h"
#include <msctf.h>
#include "COMPointer.h"

class FTextStoreACP;
class FWindowsTextInputMethodSystem;

class FTSFActivationProxy : public ITfInputProcessorProfileActivationSink, public ITfActiveLanguageProfileNotifySink
{
public:
	FTSFActivationProxy(FWindowsTextInputMethodSystem* InOwner) : Owner(InOwner), ReferenceCount(1)
	{}

	// IUnknown Interface Begin
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) OVERRIDE;
	STDMETHODIMP_(ULONG) AddRef() OVERRIDE;
	STDMETHODIMP_(ULONG) Release() OVERRIDE;
	// IUnknown Interface End

	// ITfInputProcessorProfileActivationSink Interface Begin
	STDMETHODIMP OnActivated(DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid, REFGUID guidProfile, HKL hkl, DWORD dwFlags) OVERRIDE;
	// ITfInputProcessorProfileActivationSink Interface End

	// ITfActiveLanguageProfileNotifySink Interface Begin
	STDMETHODIMP OnActivated(REFCLSID clsid, REFGUID guidProfile, BOOL fActivated) OVERRIDE;
	// ITfActiveLanguageProfileNotifySink Interface End

public:
	DWORD TSFProfileCookie;
	DWORD TSFLanguageCookie;

private:
	FWindowsTextInputMethodSystem* Owner;

	// Reference count for IUnknown Implementation
	ULONG ReferenceCount;
};

class FWindowsTextInputMethodSystem : public ITextInputMethodSystem
{
	friend class FTSFActivationProxy;

public:
	bool Initialize();
	void Terminate();

	// ITextInputMethodSystem Interface Begin
	virtual TSharedPtr<ITextInputMethodChangeNotifier> RegisterContext(const TSharedRef<ITextInputMethodContext>& Context) OVERRIDE;
	virtual void UnregisterContext(const TSharedRef<ITextInputMethodContext>& Context) OVERRIDE;
	virtual void ActivateContext(const TSharedRef<ITextInputMethodContext>& Context) OVERRIDE;
	virtual void DeactivateContext(const TSharedRef<ITextInputMethodContext>& Context) OVERRIDE;
	// ITextInputMethodSystem Interface End

	int32 ProcessMessage(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

private:
	// IMM Implementation
	bool InitializeIMM();
	void UpdateIMMProperty(HKL KeyboardLatoutHandle);
	bool ShouldDrawCompositionString() const;
	void UpdateWindowPositions(HIMC IMMContext);

	// TSF Implementation
	bool InitializeTSF();

private:
	TSharedPtr<ITextInputMethodContext> ActiveContext;
	enum class EAPI
	{
		Unknown,
		IMM,
		TSF
	} CurrentAPI;

	// TSF Implementation
	FCOMPtr<ITfInputProcessorProfiles> TSFInputProcessorProfiles;
	FCOMPtr<ITfInputProcessorProfileMgr> TSFInputProcessorProfileManager;
	FCOMPtr<ITfThreadMgr> TSFThreadManager;
	TfClientId TSFClientId;
	FCOMPtr<ITfDocumentMgr> TSFDisabledDocumentManager;
	FCOMPtr<FTSFActivationProxy> TSFActivationProxy;

	struct FInternalContext
	{
		FCOMPtr<FTextStoreACP> TSFContext;
		struct
		{
			HWND AssociatedWindow;
			HIMC IMMContext;
			bool IsComposing;
			int32 CompositionBeginIndex;
			uint32 CompositionLength;
		} IMMContext;
	};
	TMap< TWeakPtr<ITextInputMethodContext>, FInternalContext > ContextToInternalContextMap;

	// IMM Implementation
	DWORD IMEProperties;
};

#include "HideWindowsPlatformTypes.h"