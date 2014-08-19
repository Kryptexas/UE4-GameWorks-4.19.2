// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"

#include "MacWindow.h"
#include "MacTextInputMethodSystem.h"
#include "CocoaTextView.h"

DEFINE_LOG_CATEGORY_STATIC(LogMacTextInputMethodSystem, Log, All);

namespace
{
	class FTextInputMethodChangeNotifier : public ITextInputMethodChangeNotifier
	{
	public:
		FTextInputMethodChangeNotifier(const TSharedRef<ITextInputMethodContext>& InContext)
		:	Context(InContext)
		{
			ContextWindow = Context.Pin()->GetWindow();
		}
		
		virtual ~FTextInputMethodChangeNotifier() {}
		
		void SetContextWindow(TSharedPtr<FGenericWindow> Window);
		TSharedPtr<FGenericWindow> GetContextWindow(void) const;
		
		virtual void NotifyLayoutChanged(const ELayoutChangeType ChangeType) override;
		virtual void NotifySelectionChanged() override;
		virtual void NotifyTextChanged(const uint32 BeginIndex, const uint32 OldLength, const uint32 NewLength) override;
		virtual void CancelComposition() override;
		
	private:
		TWeakPtr<ITextInputMethodContext> Context;
		TSharedPtr<FGenericWindow> ContextWindow;
	};
	
	void FTextInputMethodChangeNotifier::SetContextWindow(TSharedPtr<FGenericWindow> Window)
	{
		ContextWindow = Window;
	}
	
	TSharedPtr<FGenericWindow> FTextInputMethodChangeNotifier::GetContextWindow(void) const
	{
		return ContextWindow;
	}
	
	void FTextInputMethodChangeNotifier::NotifyLayoutChanged(const ELayoutChangeType ChangeType)
	{
		if(ContextWindow.IsValid())
		{
			FCocoaWindow* CocoaWindow = (FCocoaWindow*)ContextWindow->GetOSWindowHandle();
			if(CocoaWindow && [CocoaWindow openGLView])
			{
				FCocoaTextView* TextView = (FCocoaTextView*)[CocoaWindow openGLView];
				[[TextView inputContext] invalidateCharacterCoordinates];
			}
		}
	}
	
	void FTextInputMethodChangeNotifier::NotifySelectionChanged()
	{
		if(ContextWindow.IsValid())
		{
			FCocoaWindow* CocoaWindow = (FCocoaWindow*)ContextWindow->GetOSWindowHandle();
			if(CocoaWindow && [CocoaWindow openGLView])
			{
				FCocoaTextView* TextView = (FCocoaTextView*)[CocoaWindow openGLView];
				[[TextView inputContext] invalidateCharacterCoordinates];
			}
		}
	}
	
	void FTextInputMethodChangeNotifier::NotifyTextChanged(const uint32 BeginIndex, const uint32 OldLength, const uint32 NewLength)
	{
		if(ContextWindow.IsValid())
		{
			FCocoaWindow* CocoaWindow = (FCocoaWindow*)ContextWindow->GetOSWindowHandle();
			if(CocoaWindow && [CocoaWindow openGLView])
			{
				FCocoaTextView* TextView = (FCocoaTextView*)[CocoaWindow openGLView];
				[[TextView inputContext] invalidateCharacterCoordinates];
			}
		}
	}
	
	void FTextInputMethodChangeNotifier::CancelComposition()
	{
		if(ContextWindow.IsValid())
		{
			FCocoaWindow* CocoaWindow = (FCocoaWindow*)ContextWindow->GetOSWindowHandle();
			if(CocoaWindow && [CocoaWindow openGLView])
			{
				FCocoaTextView* TextView = (FCocoaTextView*)[CocoaWindow openGLView];
				[[TextView inputContext] discardMarkedText];
				[TextView unmarkText];
			}
		}
	}
}

bool FMacTextInputMethodSystem::Initialize()
{
	// Nothing to do here for Cocoa
	return true;
}

void FMacTextInputMethodSystem::Terminate()
{
	// Nothing to do here for Cocoa
}

// ITextInputMethodSystem Interface Begin
TSharedPtr<ITextInputMethodChangeNotifier> FMacTextInputMethodSystem::RegisterContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	TSharedRef<ITextInputMethodChangeNotifier> Notifier = MakeShareable( new FTextInputMethodChangeNotifier(Context) );
	TWeakPtr<ITextInputMethodChangeNotifier>& NotifierRef = ContextMap.Add(Context);
	NotifierRef = Notifier;
	return Notifier;
}

void FMacTextInputMethodSystem::UnregisterContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	TWeakPtr<ITextInputMethodChangeNotifier>& NotifierRef = ContextMap[Context];
	if(!NotifierRef.IsValid())
	{
		UE_LOG(LogMacTextInputMethodSystem, Error, TEXT("Unregistering a context failed when its registration couldn't be found."));
		return;
	}
	
	ContextMap.Remove(Context);
}

void FMacTextInputMethodSystem::ActivateContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	TWeakPtr<ITextInputMethodChangeNotifier>& NotifierRef = ContextMap[Context];
	if(!NotifierRef.IsValid())
	{
		UE_LOG(LogMacTextInputMethodSystem, Error, TEXT("Activating a context failed when its registration couldn't be found."));
		return;
	}
	
	TSharedPtr<ITextInputMethodChangeNotifier> Notifier(NotifierRef.Pin());
	FTextInputMethodChangeNotifier* MacNotifier = (FTextInputMethodChangeNotifier*)Notifier.Get();
	TSharedPtr<FGenericWindow> GenericWindow = Context->GetWindow();
	if(GenericWindow.IsValid())
	{
		MacNotifier->SetContextWindow(GenericWindow);
		FCocoaWindow* CocoaWindow = (FCocoaWindow*)GenericWindow->GetOSWindowHandle();
		if(CocoaWindow && [CocoaWindow openGLView])
		{
			NSView* GLView = [CocoaWindow openGLView];
			if([GLView isKindOfClass:[FCocoaTextView class]])
			{
				FCocoaTextView* TextView = (FCocoaTextView*)GLView;
				[TextView activateInputMethod:Context];
				return;
			}
		}
	}
	UE_LOG(LogMacTextInputMethodSystem, Error, TEXT("Activating a context failed when its window couldn't be found."));
}

void FMacTextInputMethodSystem::DeactivateContext(const TSharedRef<ITextInputMethodContext>& Context)
{
	TWeakPtr<ITextInputMethodChangeNotifier>& NotifierRef = ContextMap[Context];
	if(!NotifierRef.IsValid())
	{
		UE_LOG(LogMacTextInputMethodSystem, Error, TEXT("Deactivating a context failed when its registration couldn't be found."));
		return;
	}
	
	TSharedPtr<ITextInputMethodChangeNotifier> Notifier(NotifierRef.Pin());
	FTextInputMethodChangeNotifier* MacNotifier = (FTextInputMethodChangeNotifier*)Notifier.Get();
	TSharedPtr<FGenericWindow> GenericWindow = MacNotifier->GetContextWindow();
	if(GenericWindow.IsValid())
	{
		FCocoaWindow* CocoaWindow = (FCocoaWindow*)GenericWindow->GetOSWindowHandle();
		if(CocoaWindow && [CocoaWindow openGLView])
		{
			NSView* GLView = [CocoaWindow openGLView];
			if([GLView isKindOfClass:[FCocoaTextView class]])
			{
				FCocoaTextView* TextView = (FCocoaTextView*)GLView;
				[TextView deactivateInputMethod];
				return;
			}
		}
	}
	UE_LOG(LogMacTextInputMethodSystem, Error, TEXT("Deactivating a context failed when its window couldn't be found."));
}
// ITextInputMethodSystem Interface End
