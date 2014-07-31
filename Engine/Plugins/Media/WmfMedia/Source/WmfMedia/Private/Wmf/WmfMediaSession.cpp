// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WmfMediaPrivatePCH.h"
#include "AllowWindowsPlatformTypes.h"


/* FWmfAsyncCallback structors
 *****************************************************************************/

FWmfMediaSession::FWmfMediaSession( const CComPtr<IMFTopology>& InTopology )
	: Capabilities(0)
	, ChangeRequested(false)
	, CurrentRate(0.0f)
	, LastError(S_OK)
	, RefCount(1)
	, RequestedPosition(FTimespan::MinValue())
	, RequestedRate(0.0f)
	, StateChangePending(false)
{
	CurrentState = EMediaStates::Error;

	if (FAILED(::MFCreateMediaSession(NULL, &MediaSession)))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to create media session"));
	}
	else if (FAILED(MediaSession->SetTopology(0, InTopology)))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to set topology in media session"));
	}
	else if (FAILED(MediaSession->BeginGetEvent(this, NULL)))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to start media session event processing"));
	}
	else
	{
		// get presentation clock, if available
		CComPtr<IMFClock> Clock;

		if (SUCCEEDED(MediaSession->GetClock(&Clock)))
		{
			Clock->QueryInterface(IID_PPV_ARGS(&PresentationClock));
		}

		// get rate control and support, if available
		if (SUCCEEDED(::MFGetService(MediaSession, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&RateControl))) &&
			SUCCEEDED(::MFGetService(MediaSession, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&RateSupport))))
		{
			CanScrub = SUCCEEDED(RateSupport->IsRateSupported(TRUE, 0, NULL));
		}

		CurrentState = EMediaStates::Stopped;
	}

	RequestedState = CurrentState;
}


/* FWmfAsyncCallback interface
 *****************************************************************************/

FTimespan FWmfMediaSession::GetPosition( ) const
{
	if (PresentationClock == NULL)
	{
		return FTimespan::MinValue();
	}

	if (RequestedPosition >= FTimespan::Zero())
	{
		return RequestedPosition;
	}
	
	return GetInternalPosition();
}


bool FWmfMediaSession::IsRateSupported( float Rate, bool Unthinned ) const
{
	if (Rate == 1.0f)
	{
		return true;
	}

	if ((RateControl == NULL) || (RateSupport == NULL))
	{
		return false;
	}

	if (Unthinned)
	{
		return SUCCEEDED(RateSupport->IsRateSupported(FALSE, Rate, NULL));
	}

	return SUCCEEDED(RateSupport->IsRateSupported(TRUE, Rate, NULL));
}


bool FWmfMediaSession::SetPosition( const FTimespan& Position )
{
	RequestedPosition = Position;

	return ChangeState();
}


bool FWmfMediaSession::SetRate( float Rate )
{
	if (!IsRateSupported(Rate, false))
	{
		return false;
	}

	RequestedRate = Rate;

	if (!FMath::IsNearlyZero(Rate))
	{
		RequestedState = EMediaStates::Playing;
	}

	return ChangeState();
}


bool FWmfMediaSession::SetState( EMediaStates NewState )
{
	RequestedState = NewState;

	if ((NewState == EMediaStates::Closed) || (NewState == EMediaStates::Stopped))
	{
		RequestedPosition = FTimespan::Zero();
		RequestedRate = 0.0f;
	}

	return ChangeState();
}


/* IMFAsyncCallback interface
 *****************************************************************************/

STDMETHODIMP_(ULONG) FWmfMediaSession::AddRef( )
{
	return FPlatformAtomics::InterlockedIncrement(&RefCount);
}


STDMETHODIMP FWmfMediaSession::GetParameters( unsigned long*, unsigned long*)
{
	return E_NOTIMPL;
}


STDMETHODIMP FWmfMediaSession::Invoke( IMFAsyncResult* AsyncResult )
{
	CComPtr<IMFMediaEvent> Event;
	
	if (FAILED(MediaSession->EndGetEvent(AsyncResult, &Event)))
	{
		return S_OK;
	}

	MediaEventType EventType = MEUnknown;
	
	if (SUCCEEDED(Event->GetType(&EventType)))
	{
		HRESULT EventResult;
		Event->GetStatus(&EventResult);

		FScopeLock ScopeLock(&CriticalSection);
		{
			if (EventType == MESessionClosed)
			{
				UpdateState(EMediaStates::Closed);
				Capabilities = 0;
			}
			else if (EventType == MEError)
			{
				LastError = EventResult;
			}
			else
			{
				if (EventType == MESessionCapabilitiesChanged)
				{
					Capabilities = ::MFGetAttributeUINT32(Event, MF_EVENT_SESSIONCAPS, Capabilities);
				}
				else if (EventType == MESessionEnded)
				{
					UpdateState(EMediaStates::Stopped);
				}
				else if (EventType == MESessionPaused)
				{
					UpdateState(EMediaStates::Paused);
				}
				else if (EventType == MESessionRateChanged)
				{
					// recover active playback rate if rate change failed
					if (FAILED(EventResult) && (CurrentRate == RequestedRate))
					{
						PROPVARIANT Value;
						PropVariantInit(&Value);

						if (SUCCEEDED(Event->GetValue(&Value)) && (Value.vt == VT_R4))
						{
							CurrentRate = Value.fltVal;
						}

						RequestedRate = CurrentRate;
					}
				}
				else if (EventType == MESessionScrubSampleComplete)
				{
					if (GetInternalPosition() == RequestedPosition)
					{
						RequestedPosition = FTimespan::MinValue();
					}
				}
				else if (EventType == MESessionStarted)
				{
					if (GetInternalPosition() == RequestedPosition)
					{
						RequestedPosition = FTimespan::MinValue();
					}

					UpdateState(EMediaStates::Playing);
				}
				else if (EventType == MESessionStopped)
				{
					UpdateState(EMediaStates::Stopped);
				}

				if (FAILED(MediaSession->BeginGetEvent(this, NULL)))
				{
					Capabilities = 0;
					CurrentState = EMediaStates::Error;
				}
			}
		}
	}
	
	return S_OK;
}


STDMETHODIMP FWmfMediaSession::QueryInterface( REFIID RefID, void** Object )
{
	static const QITAB QITab[] =
	{
		QITABENT(FWmfMediaSession, IMFAsyncCallback),
		{ 0 }
	};

	return QISearch(this, QITab, RefID, Object);
}


STDMETHODIMP_(ULONG) FWmfMediaSession::Release( )
{
	int32 CurrentRefCount = FPlatformAtomics::InterlockedDecrement(&RefCount);
	
	if (CurrentRefCount == 0)
	{
		delete this;
	}

	return CurrentRefCount;
}


/* FWmfVideoSession implementation
 *****************************************************************************/

bool FWmfMediaSession::ChangeState( )
{
	// disallow state changes if session was closed or had an error
	if ((CurrentState == EMediaStates::Closed) || (CurrentState == EMediaStates::Error))
	{
		return false;
	}

	FScopeLock ScopeLock(&CriticalSection);
	{
		// defer state change
		if (StateChangePending)
		{
			ChangeRequested = true;

			return true;
		}


		// close session
		if ((RequestedState == EMediaStates::Closed) || (RequestedState == EMediaStates::Error))
		{
			MediaSession->Close();

			return true;
		}

		// handle rate changes
		if (RequestedRate != CurrentRate)
		{
			if (CurrentState != EMediaStates::Stopped)
			{
				if (RequestedPosition == FTimespan::MinValue())
				{
					RequestedPosition = GetPosition();
				}

				MediaSession->Stop();

				return true;
			}

			bool RateChangeFailed;

			if (IsRateSupported(RequestedRate, true))
			{
				RateChangeFailed = FAILED(RateControl->SetRate(FALSE, RequestedRate));
			}
			else
			{
				RateChangeFailed = FAILED(RateControl->SetRate(TRUE, RequestedRate));
			}

			if (RateChangeFailed)
			{
				RequestedRate = CurrentRate;
			}
			else
			{
				CurrentRate = RequestedRate;
			}
		}

		// handle state changes
		bool StateChangeFailed = false;

		if (RequestedState == EMediaStates::Paused)
		{
			if (CurrentState != EMediaStates::Paused)
			{
				StateChangePending = true;
				StateChangeFailed = FAILED(MediaSession->Pause());
			}
		}
		else if (RequestedState == EMediaStates::Playing)
		{
			if ((CurrentState != EMediaStates::Playing) || (RequestedPosition >= FTimespan::Zero()))
			{
				PROPVARIANT StartPosition;

				if (RequestedPosition >= FTimespan::Zero())
				{
					StartPosition.vt = VT_I8;
					StartPosition.hVal.QuadPart = RequestedPosition.GetTicks();
					RequestedPosition = FTimespan::MinValue();
				}
				else
				{
					PropVariantInit(&StartPosition);
				}

				StateChangePending = true;
				StateChangeFailed = FAILED(MediaSession->Start(NULL, &StartPosition));
			}
		}
		else if (RequestedState == EMediaStates::Stopped)
		{
			if (CurrentState != EMediaStates::Stopped)
			{
				StateChangePending = true;
				StateChangeFailed = FAILED(MediaSession->Stop());
			}
		}

		if (StateChangeFailed)
		{
			RequestedState = CurrentState;

			return false;
		}
	}

	return true;
}


FTimespan FWmfMediaSession::GetInternalPosition( ) const
{
	MFTIME ClockTime;

	if (FAILED(PresentationClock->GetTime(&ClockTime)))
	{
		// topology not initialized yet
		return FTimespan::Zero();
	}

	return FTimespan(ClockTime);
}


void FWmfMediaSession::UpdateState( EMediaStates CompletedState )
{
	CurrentState = CompletedState;

	if (CompletedState == RequestedState)
	{
		StateChangePending = false;
	}
	else
	{
		ChangeState();
	}
}


#include "HideWindowsPlatformTypes.h"
