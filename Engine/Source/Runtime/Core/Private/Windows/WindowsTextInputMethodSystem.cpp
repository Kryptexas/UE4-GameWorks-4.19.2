// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "WindowsTextInputMethodSystem.h"
#include "TextStoreACP.h"

DEFINE_LOG_CATEGORY_STATIC(LogWindowsTextInputMethodSystem, Log, All);

#include "AllowWindowsPlatformTypes.h"

namespace
{
	FString GetIMMStringAsFString(HIMC IMMContext, const DWORD StringType)
	{
		const LONG StringNeededSize = ::ImmGetCompositionString(IMMContext, StringType, nullptr, 0);
		TCHAR* RawString = reinterpret_cast<TCHAR*>(malloc(StringNeededSize));
		::ImmGetCompositionString(IMMContext, StringType, RawString, StringNeededSize);
		const FString Str(StringNeededSize / sizeof(TCHAR), RawString);
		free(RawString);
		return Str;
	}

	class FTextInputMethodChangeNotifier : public ITextInputMethodChangeNotifier
	{
	public:
		FTextInputMethodChangeNotifier(const FCOMPtr<FTextStoreACP>& InTextStoreACP)
			:	TextStoreACP(InTextStoreACP)
		{}

		virtual ~FTextInputMethodChangeNotifier() {}

		virtual void NotifyLayoutChanged(const ELayoutChangeType ChangeType) OVERRIDE;
		virtual void NotifySelectionChanged() OVERRIDE;
		virtual void NotifyTextChanged(const uint32 BeginIndex, const uint32 OldLength, const uint32 NewLength) OVERRIDE;
		virtual void CancelComposition() OVERRIDE;

	private:
		const FCOMPtr<FTextStoreACP> TextStoreACP;
	};

	void FTextInputMethodChangeNotifier::NotifyLayoutChanged(const ELayoutChangeType ChangeType)
	{
		if(TextStoreACP->AdviseSinkObject.TextStoreACPSink != nullptr)
		{
			TsLayoutCode LayoutCode = TS_LC_CHANGE;
			switch(ChangeType)
			{
			case ELayoutChangeType::Created:	{ LayoutCode = TS_LC_CREATE; } break;
			case ELayoutChangeType::Changed:	{ LayoutCode = TS_LC_CHANGE; } break;
			case ELayoutChangeType::Destroyed:	{ LayoutCode = TS_LC_DESTROY; } break;
			}
			TextStoreACP->AdviseSinkObject.TextStoreACPSink->OnLayoutChange(LayoutCode, 0);
		}
	}

	void FTextInputMethodChangeNotifier::NotifySelectionChanged()
	{
		if(TextStoreACP->AdviseSinkObject.TextStoreACPSink != nullptr)
		{
			TextStoreACP->AdviseSinkObject.TextStoreACPSink->OnSelectionChange();
		}
	}

	void FTextInputMethodChangeNotifier::NotifyTextChanged(const uint32 BeginIndex, const uint32 OldLength, const uint32 NewLength)
	{
		if(TextStoreACP->AdviseSinkObject.TextStoreACPSink != nullptr)
		{
			TS_TEXTCHANGE TextChange;
			TextChange.acpStart = BeginIndex;
			TextChange.acpOldEnd = BeginIndex + OldLength;
			TextChange.acpNewEnd = BeginIndex + NewLength;
			TextStoreACP->AdviseSinkObject.TextStoreACPSink->OnTextChange(0, &(TextChange));
		}
	}

	void FTextInputMethodChangeNotifier::CancelComposition()
	{
		if(TextStoreACP->TSFContextOwnerCompositionServices != nullptr && TextStoreACP->Composition.TSFCompositionView != nullptr)
		{
			TextStoreACP->TSFContextOwnerCompositionServices->TerminateComposition(TextStoreACP->Composition.TSFCompositionView);
		}
	}
}

STDMETHODIMP FTSFActivationProxy::QueryInterface(REFIID riid, void **ppvObj)
{
	*ppvObj = nullptr;

	if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfInputProcessorProfileActivationSink) )
	{
		*ppvObj = static_cast<ITfInputProcessorProfileActivationSink*>(this);
	}
	else if( IsEqualIID(riid, IID_ITfActiveLanguageProfileNotifySink) )
	{
		*ppvObj = static_cast<ITfActiveLanguageProfileNotifySink*>(this);
	}

	// Add a reference if we're (conceptually) returning a reference to our self.
	if (*ppvObj)
	{
		AddRef();
	}

	return *ppvObj ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) FTSFActivationProxy::AddRef()
{
	return ++ReferenceCount;
}

STDMETHODIMP_(ULONG) FTSFActivationProxy::Release()
{
	const ULONG LocalReferenceCount = --ReferenceCount;
	if(!ReferenceCount)
	{
		delete this;
	}
	return LocalReferenceCount;
}

STDAPI FTSFActivationProxy::OnActivated(DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid, REFGUID guidProfile, HKL hkl, DWORD dwFlags)
{
	FString APIString;
	if(::ImmGetIMEFileName(::GetKeyboardLayout(0), nullptr, 0) > 0)
	{
		Owner->CurrentAPI = FWindowsTextInputMethodSystem::EAPI::IMM;
		APIString = TEXT("IMM");
	}
	else
	{
		Owner->CurrentAPI = FWindowsTextInputMethodSystem::EAPI::TSF;
		APIString = TEXT("TSF");
	}
	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("IME system now using %s."), *APIString);
	return S_OK;
}

STDAPI FTSFActivationProxy::OnActivated(REFCLSID clsid, REFGUID guidProfile, BOOL fActivated)
{
	FString APIString;
	if(::ImmGetIMEFileName(::GetKeyboardLayout(0), nullptr, 0) > 0)
	{
		Owner->CurrentAPI = FWindowsTextInputMethodSystem::EAPI::IMM;
		APIString = TEXT("IMM");
	}
	else
	{
		Owner->CurrentAPI = FWindowsTextInputMethodSystem::EAPI::TSF;
		APIString = TEXT("TSF");
	}
	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("IME system now using %s."), *APIString);
	return S_OK;
}

bool FWindowsTextInputMethodSystem::Initialize()
{
	CurrentAPI = EAPI::Unknown;
	bool Result = InitializeIMM() && InitializeTSF();
	return Result;
}

bool FWindowsTextInputMethodSystem::InitializeIMM()
{
	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Initializing IMM..."));

	UpdateIMMProperty(::GetKeyboardLayout(0));

	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Initialized IMM!"));

	return true;
}

void FWindowsTextInputMethodSystem::UpdateIMMProperty(HKL KeyboardLayoutHandle)
{
	WORD langID = LOWORD(KeyboardLayoutHandle);

	IMEProperties = ::ImmGetProperty(KeyboardLayoutHandle, IGP_PROPERTY);
}

bool FWindowsTextInputMethodSystem::ShouldDrawIMMCompositionString() const
{
	// If the IME doesn't have any kind of special UI and it draws the composition window at the caret, we can draw it ourselves.
	return !(IMEProperties & IME_PROP_SPECIAL_UI) && (IMEProperties & IME_PROP_AT_CARET);
}

void FWindowsTextInputMethodSystem::UpdateIMMWindowPositions(HIMC IMMContext)
{
	if(ActiveContext.IsValid())
	{
		FInternalContext& InternalContext = ContextToInternalContextMap[ActiveContext];

		// Get start of composition area.
		const uint32 BeginIndex = InternalContext.IMMContext.CompositionBeginIndex;
		const uint32 Length = InternalContext.IMMContext.CompositionLength;
		FVector2D Position;
		FVector2D Size;
		ActiveContext->GetTextBounds(BeginIndex, Length, Position, Size);

		// Positions provided to IMM are relative to the window, but we retrieved screen-space coordinates.
		TSharedPtr<FGenericWindow> GenericWindow = ActiveContext->GetWindow();
		HWND WindowHandle = reinterpret_cast<HWND>(GenericWindow->GetOSWindowHandle());
		RECT WindowRect;
		GetWindowRect(WindowHandle, &(WindowRect));
		Position.X -= WindowRect.left;
		Position.Y -= WindowRect.top;

		// Update candidate window position.
		CANDIDATEFORM CandidateForm;
		CandidateForm.dwIndex = 0;
		CandidateForm.dwStyle = CFS_EXCLUDE;
		CandidateForm.ptCurrentPos.x = Position.X;
		CandidateForm.ptCurrentPos.y = Position.Y;
		CandidateForm.rcArea.left = CandidateForm.ptCurrentPos.x;
		CandidateForm.rcArea.right = CandidateForm.ptCurrentPos.x;
		CandidateForm.rcArea.top = CandidateForm.ptCurrentPos.y;
		CandidateForm.rcArea.bottom = CandidateForm.ptCurrentPos.y + Size.Y;
		::ImmSetCandidateWindow(IMMContext, &CandidateForm);

		// Update composition window position.
		COMPOSITIONFORM CompositionForm;
		CompositionForm.dwStyle = CFS_POINT;
		CompositionForm.ptCurrentPos.x = Position.X;
		CompositionForm.ptCurrentPos.y = Position.Y + Size.Y;
		::ImmSetCompositionWindow(IMMContext, &CompositionForm);
	}
}

void FWindowsTextInputMethodSystem::BeginIMMComposition()
{
	check(ActiveContext.IsValid());

	FInternalContext& InternalContext = ContextToInternalContextMap[ActiveContext];
				
	InternalContext.IMMContext.IsComposing = true;
	ActiveContext->BeginComposition();

	uint32 SelectionBeginIndex = 0;
	uint32 SelectionLength = 0;
	ITextInputMethodContext::ECaretPosition SelectionCaretPosition = ITextInputMethodContext::ECaretPosition::Ending;
	ActiveContext->GetSelectionRange(SelectionBeginIndex, SelectionLength, SelectionCaretPosition);
				
	// Set the initial composition range based on the start of the current selection
	// We ignore the relative caret position as once you start typing any selected text is 
	// removed before new text is added, so the caret will effectively always be at the start
	InternalContext.IMMContext.CompositionBeginIndex = SelectionBeginIndex;
	InternalContext.IMMContext.CompositionLength = 0;
	ActiveContext->UpdateCompositionRange(InternalContext.IMMContext.CompositionBeginIndex, InternalContext.IMMContext.CompositionLength);
}

void FWindowsTextInputMethodSystem::EndIMMComposition()
{
	check(ActiveContext.IsValid());

	FInternalContext& InternalContext = ContextToInternalContextMap[ActiveContext];
	InternalContext.IMMContext.IsComposing = false;
	ActiveContext->EndComposition();
}

bool FWindowsTextInputMethodSystem::InitializeTSF()
{
	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Initializing TSF..."));

	HRESULT Result = S_OK;

	// Input Processors
	{
		// Get input processor profiles.
		ITfInputProcessorProfiles* RawPointerTSFInputProcessorProfiles;
		Result = ::CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, reinterpret_cast<void**>(&(RawPointerTSFInputProcessorProfiles)));
		if(FAILED(Result))
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while creating the TSF input processor profiles."));
			return false;
		}
		TSFInputProcessorProfiles.Attach(RawPointerTSFInputProcessorProfiles);

		// Get input processor profile manager from profiles.
		Result = TSFInputProcessorProfileManager.FromQueryInterface(IID_ITfInputProcessorProfileMgr, TSFInputProcessorProfiles);
		if(FAILED(Result))
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while acquiring the TSF input processor profile manager."));
			TSFInputProcessorProfiles.Reset();
			return false;
		}
	}

	// Thread Manager
	{
		// Create thread manager.
		ITfThreadMgr* RawPointerTSFThreadManager;
		Result = ::CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, reinterpret_cast<void**>(&(RawPointerTSFThreadManager)));
		if(FAILED(Result))
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while creating the TSF thread manager."));
			TSFInputProcessorProfiles.Reset();
			TSFInputProcessorProfileManager.Reset();
			return false;
		}
		TSFThreadManager.Attach(RawPointerTSFThreadManager);

		// Activate thread manager.
		Result = TSFThreadManager->Activate(&(TSFClientId));
		if(FAILED(Result))
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while activating the TSF thread manager."));
			TSFInputProcessorProfiles.Reset();
			TSFInputProcessorProfileManager.Reset();
			TSFThreadManager.Reset();
			return false;
		}

		// Get source from thread manager, needed to install profile processor related sinks.
		FCOMPtr<ITfSource> TSFSource;
		Result = TSFSource.FromQueryInterface(IID_ITfSource, TSFThreadManager);
		if(FAILED(Result))
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while acquiring the TSF source from TSF thread manager."));
			TSFInputProcessorProfiles.Reset();
			TSFInputProcessorProfileManager.Reset();
			TSFThreadManager.Reset();
			return false;
		}

		TSFActivationProxy = new FTSFActivationProxy(this);

		const DWORD WindowsVersion = ::GetVersion();
		const DWORD WindowsMajorVersion = LOBYTE(LOWORD(WindowsVersion));
		const DWORD WindowsMinorVersion = HIBYTE(LOWORD(WindowsVersion));

		static const DWORD WindowsVistaMajorVersion = 6;
		static const DWORD WindowsVistaMinorVersion = 0;

		// Install profile notification sink for versions of Windows Vista and after.
		if(WindowsMajorVersion > WindowsVistaMajorVersion || (WindowsMajorVersion == WindowsVistaMajorVersion && WindowsMinorVersion >= WindowsMinorVersion))
		{
			Result = TSFSource->AdviseSink(IID_ITfInputProcessorProfileActivationSink, static_cast<ITfInputProcessorProfileActivationSink*>(TSFActivationProxy), &(TSFActivationProxy->TSFProfileCookie));
			if(FAILED(Result))
			{
				UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while advising the profile notification sink to the TSF source."));
				TSFInputProcessorProfiles.Reset();
				TSFInputProcessorProfileManager.Reset();
				TSFThreadManager.Reset();
				TSFActivationProxy.Reset();
				return false;
			}
		}
		// Install language notification sink for versions before Windows Vista.
		else
		{
			Result = TSFSource->AdviseSink(IID_ITfActiveLanguageProfileNotifySink, static_cast<ITfActiveLanguageProfileNotifySink*>(TSFActivationProxy), &(TSFActivationProxy->TSFLanguageCookie));
			if(FAILED(Result))
			{
				UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while advising the language notification sink to the TSF source."));
				TSFInputProcessorProfiles.Reset();
				TSFInputProcessorProfileManager.Reset();
				TSFThreadManager.Reset();
				TSFActivationProxy.Reset();
				return false;
			}
		}
	}

	// Disabled Document Manager
	Result = TSFThreadManager->CreateDocumentMgr(&(TSFDisabledDocumentManager));
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while creating the TSF thread manager."));
		TSFInputProcessorProfiles.Reset();
		TSFInputProcessorProfileManager.Reset();
		TSFThreadManager.Reset();
		TSFActivationProxy.Reset();
		return false;
	}

	// Default the focus to the disabled document manager.
	Result = TSFThreadManager->SetFocus(TSFDisabledDocumentManager);
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while activating the TSF thread manager."));
		TSFInputProcessorProfiles.Reset();
		TSFInputProcessorProfileManager.Reset();
		TSFThreadManager.Reset();
		TSFActivationProxy.Reset();
		TSFThreadManager.Reset();
		return false;
	}

	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Initialized TSF!"));

	return true;
}

void FWindowsTextInputMethodSystem::Terminate()
{
	HRESULT Result;

	// Get source from thread manager, needed to uninstall profile processor related sinks.
	FCOMPtr<ITfSource> TSFSource;
	Result = TSFSource.FromQueryInterface(IID_ITfSource, TSFThreadManager);
	if(FAILED(Result) || !TSFSource)
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Terminating failed while acquiring the TSF source from the TSF thread manager."));
	}

	if(TSFSource && TSFActivationProxy)
	{
		// Uninstall language notification sink.
		if(TSFActivationProxy->TSFLanguageCookie != TF_INVALID_COOKIE)
		{
			Result = TSFSource->UnadviseSink(TSFActivationProxy->TSFLanguageCookie);
			if(FAILED(Result))
			{
				UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Terminating failed while unadvising the language notification sink from the TSF source."));
			}
		}

		// Uninstall profile notification sink.
		if(TSFActivationProxy->TSFProfileCookie != TF_INVALID_COOKIE)
		{
			Result = TSFSource->UnadviseSink(TSFActivationProxy->TSFProfileCookie);
			if(FAILED(Result))
			{
				UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Terminating failed while unadvising the profile notification sink from the TSF source."));
			}
		}
	}
	TSFActivationProxy.Reset();

	Result = TSFThreadManager->Deactivate();
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Terminating failed while deactivating the TSF thread manager."));
	}

	TSFThreadManager.Reset();
}

TSharedPtr<ITextInputMethodChangeNotifier> FWindowsTextInputMethodSystem::RegisterContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Registering context %p..."), &(Context.Get()));

	HRESULT Result;

	FInternalContext& InternalContext = ContextToInternalContextMap.Add(Context);

	// TSF Implementation
	FCOMPtr<FTextStoreACP>& TextStore = InternalContext.TSFContext;
	TextStore.Attach(new FTextStoreACP(Context));

	Result = TSFThreadManager->CreateDocumentMgr(&(TextStore->TSFDocumentManager));
	if(FAILED(Result) || !TextStore->TSFDocumentManager)
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Registering a context failed while creating the TSF document manager."));
		TextStore.Reset();
		return nullptr;
	}

	Result = TextStore->TSFDocumentManager->CreateContext(TSFClientId, 0, static_cast<ITextStoreACP*>(TextStore), &(TextStore->TSFContext), &(TextStore->TSFEditCookie));	
	if(FAILED(Result) || !TextStore->TSFContext)
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Registering a context failed while creating the TSF context."));
		TextStore.Reset();
		return nullptr;
	}

	Result = TextStore->TSFDocumentManager->Push(TextStore->TSFContext);
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Registering a context failed while pushing a TSF context onto the TSF document manager."));
		TextStore.Reset();
		return nullptr;
	}

	Result = TextStore->TSFContextOwnerCompositionServices.FromQueryInterface(IID_ITfContextOwnerCompositionServices, TextStore->TSFContext);
	if(FAILED(Result) || !TextStore->TSFContextOwnerCompositionServices)
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Registering a context failed while getting the TSF context owner composition services."));
		Result = TextStore->TSFDocumentManager->Pop(TF_POPF_ALL);
		if(FAILED(Result))
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Failed to pop a TSF context off from TSF document manager while recovering from failing."));
		}
		TextStore.Reset();
		return nullptr;
	}

	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Registered context %p!"), &(Context.Get()));

	return MakeShareable( new FTextInputMethodChangeNotifier(TextStore) );
}

void FWindowsTextInputMethodSystem::UnregisterContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Unregistering context %p..."), &(Context.Get()));

	HRESULT Result;

	check(ContextToInternalContextMap.Contains(Context));
	FInternalContext& InternalContext = ContextToInternalContextMap[Context];

	// TSF Implementation
	FCOMPtr<FTextStoreACP>& TextStore = InternalContext.TSFContext;

	Result = TextStore->TSFDocumentManager->Pop(TF_POPF_ALL);
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Unregistering a context failed while popping a TSF context off from the TSF document manager."));
	}

	ContextToInternalContextMap.Remove(Context);
	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Unregistered context %p!"), &(Context.Get()));
}

void FWindowsTextInputMethodSystem::ActivateContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Activating context %p..."), &(Context.Get()));
	HRESULT Result;

	// General Implementation
	ActiveContext = Context;

	check(ContextToInternalContextMap.Contains(Context));
	FInternalContext& InternalContext = ContextToInternalContextMap[Context];

	// IMM Implementation
	InternalContext.IMMContext.IsComposing = false;

	// TSF Implementation
	FCOMPtr<FTextStoreACP>& TextStore = InternalContext.TSFContext;
	Result = TSFThreadManager->SetFocus(TextStore->TSFDocumentManager);
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Activating a context failed while setting focus on a TSF document manager."));
	}

	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Activated context %p!"), &(Context.Get()));
}

void FWindowsTextInputMethodSystem::DeactivateContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Deactivating context %p..."), &(Context.Get()));
	HRESULT Result;

	FInternalContext& InternalContext = ContextToInternalContextMap[Context];

	// TSF Implementation
	FCOMPtr<FTextStoreACP> TextStore = InternalContext.TSFContext;

	ITfDocumentMgr* FocusedDocumentManger = nullptr;
	Result = TSFThreadManager->GetFocus(&(FocusedDocumentManger));
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Deactivating a context failed while getting the currently focused document manager."));
	}
	if(FocusedDocumentManger == TextStore->TSFDocumentManager)
	{
		Result = TSFThreadManager->SetFocus(TSFDisabledDocumentManager);
		if(FAILED(Result))
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Deactivating a context failed while setting focus to the disabled TSF document manager."));
		}
	}
	else
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Deactivating a context ignored because it is not the focused TSF document manager."));
	}

	// IMM Implementation
	const TSharedPtr<FGenericWindow> GenericWindow = Context->GetWindow();
	const HWND Hwnd = GenericWindow.IsValid() ? reinterpret_cast<HWND>(GenericWindow->GetOSWindowHandle()) : nullptr;
	HIMC IMMContext = ::ImmGetContext(Hwnd);
	// Request the composition is canceled to ensure that the composition input UI is closed, and that a WM_IME_ENDCOMPOSITION message is sent
	::ImmNotifyIME(IMMContext, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
	::ImmReleaseContext(Hwnd, IMMContext);

	// General Implementation
	ActiveContext = nullptr;

	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Deactivated context %p!"), &(Context.Get()));
}

int32 FWindowsTextInputMethodSystem::ProcessMessage(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam)
{
	if(CurrentAPI == EAPI::TSF)
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	switch(msg)
	{
	case WM_INPUTLANGCHANGEREQUEST:
	case WM_INPUTLANGCHANGE:
		{
			HKL KeyboardLayoutHandle = reinterpret_cast<HKL>(lParam);

			UpdateIMMProperty(KeyboardLayoutHandle);

			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_IME_SETCONTEXT:
		{
			if(ActiveContext.IsValid())
			{
				// Disable showing an IME-implemented composition window if we're going to draw the composition string ourselves.
				if(wParam && ShouldDrawIMMCompositionString())
				{
					lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
				}

				UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Setting IMM context."));
			}

			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_IME_NOTIFY:
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_IME_REQUEST:
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_IME_STARTCOMPOSITION:
		{
			if(ActiveContext.IsValid())
			{
				BeginIMMComposition();
				
				UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Beginning IMM composition."));
			}

			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_IME_COMPOSITION:
		{
			if(ActiveContext.IsValid())
			{
				FInternalContext& InternalContext = ContextToInternalContextMap[ActiveContext];
				check(InternalContext.IMMContext.IsComposing);

				HIMC IMMContext = ::ImmGetContext(hwnd);

				UpdateIMMWindowPositions(IMMContext);

				const bool bHasCompositionStringFlag = !!(lParam & GCS_COMPSTR);
				const bool bHasResultStringFlag = !!(lParam & GCS_RESULTSTR);
				const bool bHasNoMoveCaretFlag = !!(lParam & CS_NOMOVECARET);
				const bool bHasCursorPosFlag = !!(lParam & GCS_CURSORPOS);

				// Check Result
				if(bHasResultStringFlag)
				{
					const FString ResultString = GetIMMStringAsFString(IMMContext, GCS_RESULTSTR);
					UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("WM_IME_COMPOSITION Result String: %s"), *ResultString);

					// Update Result
					ActiveContext->SetTextInRange(InternalContext.IMMContext.CompositionBeginIndex, InternalContext.IMMContext.CompositionLength, ResultString);

					// Once we get a result, we're done; set the caret to the end of the result and end the current composition
					ActiveContext->SetSelectionRange(InternalContext.IMMContext.CompositionBeginIndex + ResultString.Len(), 0, ITextInputMethodContext::ECaretPosition::Ending);
					EndIMMComposition();
				}

				// Check Composition
				if(bHasCompositionStringFlag)
				{
					const FString CompositionString = GetIMMStringAsFString(IMMContext, GCS_COMPSTR);
					UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("WM_IME_COMPOSITION Composition String: %s"), *CompositionString);

					// We've typed a character, so we need to clear out any currently selected text to mimic what happens when you normally type into a text input
					uint32 SelectionBeginIndex = 0;
					uint32 SelectionLength = 0;
					ITextInputMethodContext::ECaretPosition SelectionCaretPosition = ITextInputMethodContext::ECaretPosition::Ending;
					ActiveContext->GetSelectionRange(SelectionBeginIndex, SelectionLength, SelectionCaretPosition);
					if(SelectionLength)
					{
						ActiveContext->SetTextInRange(SelectionBeginIndex, SelectionLength, TEXT(""));
					}

					// If we received a result (handled above) then the previous composition will have been ended, so we need to start a new one now
					// This ensures that each composition ends up as its own distinct undo
					if(!InternalContext.IMMContext.IsComposing)
					{
						BeginIMMComposition();
					}

					// Update Composition
					ActiveContext->SetTextInRange(InternalContext.IMMContext.CompositionBeginIndex, InternalContext.IMMContext.CompositionLength, CompositionString);

					// Update Composition Range
					InternalContext.IMMContext.CompositionLength = CompositionString.Len();
					ActiveContext->UpdateCompositionRange(InternalContext.IMMContext.CompositionBeginIndex, InternalContext.IMMContext.CompositionLength);
				}

				// Check Cursor
				if(!bHasNoMoveCaretFlag && bHasCursorPosFlag)
				{
					const LONG CursorPositionResult = ::ImmGetCompositionString(IMMContext, GCS_CURSORPOS, nullptr, 0);
					const int16 CursorPosition = CursorPositionResult & 0xFFFF;

					// Update Cursor
					UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("WM_IME_COMPOSITION Cursor Position: %d"), CursorPosition);
					ActiveContext->SetSelectionRange(InternalContext.IMMContext.CompositionBeginIndex + CursorPosition, 0, ITextInputMethodContext::ECaretPosition::Ending);
				}

				::ImmReleaseContext(hwnd, IMMContext);

				UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Updating IMM composition."));
			}

			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_IME_ENDCOMPOSITION:
		{
			// On composition end, notify context of the end.
			if(ActiveContext.IsValid())
			{
				EndIMMComposition();
				
				UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Ending IMM composition."));
			}

			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_IME_CHAR:
		{
			// Suppress sending a WM_CHAR for this WM_IME_CHAR - the composition windows messages will have handled this.
			UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("Ignoring WM_IME_CHAR message."));
			return 0;
		}
		break;
	default:
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Warning, TEXT("Unexpected windows message received for processing."));

			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	}
}

#include "HideWindowsPlatformTypes.h"
