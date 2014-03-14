// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITextInputMethodSystem.h"

#include "AllowWindowsPlatformTypes.h"
#include <TextStor.h>
#include <msctf.h>
#include "COMPointer.h"

class FTextStoreACP : public ITextStoreACP, public ITfContextOwnerCompositionSink
{
public:
	FTextStoreACP(const TSharedRef<ITextInputMethodContext>& Context);
	virtual ~FTextStoreACP() {}

	// IUnknown Interface Begin
	STDMETHODIMP			QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);
	// IUnknown Interface End

	// ITextStoreACP Interface Begin
	STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD dwMask);
	STDMETHODIMP UnadviseSink(IUnknown *punk);
	STDMETHODIMP RequestLock(DWORD dwLockFlags, HRESULT *phrSession);

	STDMETHODIMP GetStatus(TS_STATUS *pdcs);
	STDMETHODIMP GetEndACP(LONG *pacp);

	// Selection Methods
	STDMETHODIMP GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP *pSelection, ULONG *pcFetched);
	STDMETHODIMP SetSelection(ULONG ulCount, const TS_SELECTION_ACP *pSelection);

	// Attributes Methods
	STDMETHODIMP RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs);
	STDMETHODIMP RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
	STDMETHODIMP RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags);
	STDMETHODIMP FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags, LONG *pacpNext, BOOL *pfFound, LONG *plFoundOffset);
	STDMETHODIMP RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched);

	// View Methods
	STDMETHODIMP GetActiveView(TsViewCookie *pvcView);
	STDMETHODIMP GetACPFromPoint(TsViewCookie vcView, const POINT *pt, DWORD dwFlags, LONG *pacp);
	STDMETHODIMP GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped);
	STDMETHODIMP GetScreenExt(TsViewCookie vcView, RECT *prc);
	STDMETHODIMP GetWnd(TsViewCookie vcView, HWND *phwnd);

	// Plain Character Methods
	STDMETHODIMP GetText(LONG acpStart, LONG acpEnd, __out_ecount(cchPlainReq) WCHAR *pchPlain, ULONG cchPlainReq, ULONG *pcchPlainOut, TS_RUNINFO *prgRunInfo, ULONG ulRunInfoReq, ULONG *pulRunInfoOut, LONG *pacpNext);
	STDMETHODIMP QueryInsert(LONG acpInsertStart, LONG acpInsertEnd, ULONG cch, LONG *pacpResultStart, LONG *pacpResultEnd);
	STDMETHODIMP InsertTextAtSelection(DWORD dwFlags, __in_ecount(cch) const WCHAR *pchText, ULONG cch, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange);
	STDMETHODIMP SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, __in_ecount(cch) const WCHAR *pchText, ULONG cch, TS_TEXTCHANGE *pChange);

	// Embedded Character Methods
	STDMETHODIMP GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk);
	STDMETHODIMP GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject **ppDataObject);
	STDMETHODIMP QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable);
	STDMETHODIMP InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject *pDataObject, TS_TEXTCHANGE *pChange);
	STDMETHODIMP InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject *pDataObject, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange);
	// ITextStoreACP Interface End

	// ITfContextOwnerCompositionSink Interface Begin
	STDMETHODIMP OnStartComposition(ITfCompositionView *pComposition, BOOL *pfOk);
	STDMETHODIMP OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew);
	STDMETHODIMP OnEndComposition(ITfCompositionView *pComposition);
	// ITfContextOwnerCompositionSink Interface End

private:
	// Reference count for IUnknown Implementation
	ULONG ReferenceCount;

	// Associated text context that genericizes interfacing with text editing widgets.
	const TSharedRef<ITextInputMethodContext> TextContext;

	struct FSupportedAttribute
	{
		FSupportedAttribute(const TS_ATTRID* const InId) : WantsDefault(false), Id(InId)
		{
			VariantInit(&(DefaultValue));
		}

		bool WantsDefault;
		const TS_ATTRID* const Id;
		VARIANT DefaultValue;
	};

	TArray<FSupportedAttribute> SupportedAttributes;

	struct FLockManager
	{
		FLockManager() : LockType(0), IsPendingLockUpgrade(false)
		{}

		DWORD LockType;
		bool IsPendingLockUpgrade;
	} LockManager;

public:
	struct FAdviseSinkObject
	{
		FAdviseSinkObject() : TextStoreACPSink(nullptr), SinkFlags(0) {}

		// Sink object for ITextStoreACP Implementation
		FCOMPtr<ITextStoreACPSink> TextStoreACPSink;

		FCOMPtr<ITextStoreACPServices> TextStoreACPServices;

		// Flags defining what events the sink object should be notified of.
		DWORD SinkFlags;
	} AdviseSinkObject;

	struct FComposition
	{
		FComposition() : TextFrameworkCompositionView(nullptr)
		{}

		// Composition view object for managing compositions.
		FCOMPtr<ITfCompositionView> TextFrameworkCompositionView;

	} Composition;

public:
	// Document manager object for managing contexts.
	FCOMPtr<ITfDocumentMgr> TextFrameworkDocumentManager;

	// Context object for pushing context on to document manager.
	FCOMPtr<ITfContext> TextFrameworkContext;

	// Context owner composition services object for terminating compositions.
	FCOMPtr<ITfContextOwnerCompositionServices> TextFrameworkContextOwnerCompositionServices;

	TfEditCookie TextFrameworkEditCookie;
};

#include "HideWindowsPlatformTypes.h"