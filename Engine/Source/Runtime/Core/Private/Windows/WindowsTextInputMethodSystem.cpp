// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "WindowsTextInputMethodSystem.h"
#include "TextStoreACP.h"

DEFINE_LOG_CATEGORY_STATIC(LogWindowsTextInputMethodSystem, Log, All);

#include "AllowWindowsPlatformTypes.h"

namespace
{
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
		if(TextStoreACP->TextFrameworkContextOwnerCompositionServices != nullptr && TextStoreACP->Composition.TextFrameworkCompositionView != nullptr)
		{
			TextStoreACP->TextFrameworkContextOwnerCompositionServices->TerminateComposition(TextStoreACP->Composition.TextFrameworkCompositionView);
		}
	}
}

bool FWindowsTextInputMethodSystem::Initialize()
{
	HRESULT Result = S_OK;
	
	ITfThreadMgr* RawPointerTextFrameworkThreadManager;
	Result = ::CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, reinterpret_cast<void**>(&(RawPointerTextFrameworkThreadManager)));
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while creating the TSF thread manager."));
		return false;
	}
	TextFrameworkThreadManager.Attach(RawPointerTextFrameworkThreadManager);

	Result = TextFrameworkThreadManager->Activate(&(TextFrameworkClientId));
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Initialzation failed while activating the TSF thread manager."));
		TextFrameworkThreadManager.Reset();
		return false;
	}

	return true;
}

void FWindowsTextInputMethodSystem::Terminate()
{
	HRESULT Result;

	Result = TextFrameworkThreadManager->Deactivate();
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Terminating failed while deactivating the TSF thread manager."));
	}

	TextFrameworkThreadManager.Reset();
}

TSharedPtr<ITextInputMethodChangeNotifier> FWindowsTextInputMethodSystem::RegisterContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	HRESULT Result;

	FCOMPtr<FTextStoreACP>& TextStore = ContextToStoreMap.Add(Context);
	TextStore.Attach(new FTextStoreACP(Context));

	Result = TextFrameworkThreadManager->CreateDocumentMgr(&(TextStore->TextFrameworkDocumentManager));
	if(FAILED(Result) || !TextStore->TextFrameworkDocumentManager)
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Registering a context failed while creating the TSF document manager."));
		TextStore.Reset();
		return nullptr;
	}

	Result = TextStore->TextFrameworkDocumentManager->CreateContext(TextFrameworkClientId, 0, static_cast<ITextStoreACP*>(TextStore), &(TextStore->TextFrameworkContext), &(TextStore->TextFrameworkEditCookie));	
	if(FAILED(Result) || !TextStore->TextFrameworkContext)
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Registering a context failed while creating the TSF context."));
		TextStore.Reset();
		return nullptr;
	}

	Result = TextStore->TextFrameworkDocumentManager->Push(TextStore->TextFrameworkContext);
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Registering a context failed while pushing a TSF context onto the TSF document manager."));
		TextStore.Reset();
		return nullptr;
	}

	Result = TextStore->TextFrameworkContextOwnerCompositionServices.FromQueryInterface(IID_ITfContextOwnerCompositionServices, TextStore->TextFrameworkContext);
	if(FAILED(Result) || !TextStore->TextFrameworkContextOwnerCompositionServices)
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Registering a context failed while getting the TSF context owner composition services."));
		Result = TextStore->TextFrameworkDocumentManager->Pop(TF_POPF_ALL);
		if(FAILED(Result))
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Failed to pop a TSF context off from TSF document manager while recovering from failing."));
		}
		TextStore.Reset();
		return nullptr;
	}

	return MakeShareable( new FTextInputMethodChangeNotifier(TextStore) );
}

void FWindowsTextInputMethodSystem::UnregisterContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	HRESULT Result;

	FCOMPtr<FTextStoreACP>& TextStore = ContextToStoreMap[Context];

	Result = TextStore->TextFrameworkDocumentManager->Pop(TF_POPF_ALL);
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Unregistering a context failed while popping a TSF context off from the TSF document manager."));
		return;
	}

	ContextToStoreMap.Remove(Context);
}

void FWindowsTextInputMethodSystem::ActivateContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	HRESULT Result;

	FCOMPtr<FTextStoreACP>& TextStore = ContextToStoreMap[Context];

	Result = TextFrameworkThreadManager->SetFocus(TextStore->TextFrameworkDocumentManager);
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Activating a context failed while setting focus on a TSF document manager."));
		return;
	}

	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("ActivateContext."));
}

void FWindowsTextInputMethodSystem::DeactivateContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	HRESULT Result;

	FTextStoreACP* TextStore = ContextToStoreMap[Context];

	ITfDocumentMgr* FocusedDocumentManger;
	Result = TextFrameworkThreadManager->GetFocus(&(FocusedDocumentManger));
	if(FAILED(Result))
	{
		UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Deactivating a context failed while getting the currently focused document manager."));
		return;
	}
	if(FocusedDocumentManger == TextStore->TextFrameworkDocumentManager)
	{
		Result = TextFrameworkThreadManager->SetFocus(nullptr);
		if(FAILED(Result))
		{
			UE_LOG(LogWindowsTextInputMethodSystem, Error, TEXT("Deactivating a context failed while setting focus to the disabled TSF document manager."));
			return;
		}
	}

	UE_LOG(LogWindowsTextInputMethodSystem, Verbose, TEXT("DeactivateContext."));
}

#include "HideWindowsPlatformTypes.h"