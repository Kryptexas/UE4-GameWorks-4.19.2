// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "WmfMediaSession.h"

#if WMFMEDIA_SUPPORTED_PLATFORM

#include "IMediaEventSink.h"
#include "Misc/ScopeLock.h"

#include "WmfMediaUtils.h"

#include "AllowWindowsPlatformTypes.h"


/* Local helpers
 *****************************************************************************/

namespace WmfMediaSession
{
	/** Get the human readable string representation of a media player state. */
	const TCHAR* StateToString(EMediaState State)
	{
		switch (State)
		{
		case EMediaState::Closed: return TEXT("Closed");
		case EMediaState::Error: return TEXT("Error");
		case EMediaState::Paused: return TEXT("Paused");
		case EMediaState::Preparing: return TEXT("Preparing");
		case EMediaState::Stopped: return TEXT("Stopped");
		default: return TEXT("Unknown");
		}
	}
}


/* FWmfMediaSession structors
 *****************************************************************************/

FWmfMediaSession::FWmfMediaSession()
	: CanScrub(false)
	, Capabilities(0)
	, CurrentDuration(FTimespan::Zero())
	, CurrentRate(0.0f)
	, CurrentState(EMediaState::Closed)
	, LastTime(FTimespan::Zero())
	, PendingChange(false)
	, RefCount(0)
	, RequestedRate(0.0f)
	, RequestedTime(FTimespan::MinValue())
	, ShouldLoop(false)
	, Status(EMediaStatus::None)
{ }


FWmfMediaSession::~FWmfMediaSession()
{
	check(RefCount == 0);
	Shutdown();

	UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Destroyed"), this);
}


/* FWmfMediaSession interface
 *****************************************************************************/

void FWmfMediaSession::GetEvents(TArray<EMediaEvent>& OutEvents)
{
	EMediaEvent Event;

	while (DeferredEvents.Dequeue(Event))
	{
		OutEvents.Add(Event);
	}
}


bool FWmfMediaSession::Initialize(bool LowLatency)
{
	Shutdown();

	// create session attributes
	TComPtr<IMFAttributes> Attributes;
	{
		HRESULT Result = ::MFCreateAttributes(&Attributes, 2);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Error, TEXT("Failed to create media session attributes (%s)"), *WmfMedia::ResultToString(Result));
			return false;
		}
	}

	if (LowLatency)
	{
		if (FWindowsPlatformMisc::VerifyWindowsVersion(6, 2))
		{
			HRESULT Result = Attributes->SetUINT32(MF_LOW_LATENCY, TRUE);

			if (FAILED(Result))
			{
				UE_LOG(LogWmfMedia, Warning, TEXT("Failed to set low latency session attribute (%s)"), *WmfMedia::ResultToString(Result));
			}
		}
		else
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Low latency media processing requires Windows 8 or newer"));
		}
	}

	// create media session
	HRESULT Result = ::MFCreateMediaSession(Attributes, &MediaSession);

	if (FAILED(Result))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to create media session (%s)"), *WmfMedia::ResultToString(Result));
		return false;
	}

	// start media event processing
	Result = MediaSession->BeginGetEvent(this, NULL);

	if (FAILED(Result))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to start media session event processing (%s)"), *WmfMedia::ResultToString(Result));
		return false;
	}

	CurrentState = EMediaState::Preparing;

	return true;
}


bool FWmfMediaSession::SetTopology(const TComPtr<IMFTopology>& InTopology, FTimespan InDuration)
{
	if (MediaSession == NULL)
	{
		return false;
	}

	UE_LOG(LogWmfMedia, Verbose, TEXT("Session: %llx: Setting topology %llx (duration = %s)"), this, InTopology.Get(), *InDuration.ToString());

	FScopeLock Lock(&CriticalSection);

	if (CurrentState == EMediaState::Preparing)
	{
		// media source resolved
		if (InTopology != NULL)
		{
			// at least one track selected
			HRESULT Result = MediaSession->SetTopology(MFSESSION_SETTOPOLOGY_IMMEDIATE, InTopology);

			if (FAILED(Result))
			{
				UE_LOG(LogWmfMedia, Error, TEXT("Failed to set topology: %s"), *WmfMedia::ResultToString(Result));
				
				CurrentState = EMediaState::Error;
				DeferredEvents.Enqueue(EMediaEvent::MediaOpenFailed);
			}
			else
			{
				// do nothing (Preparing state will exit in MESessionTopologyStatus event)
			}
		}
		else
		{
			// no tracks selected
			CurrentState = EMediaState::Stopped;
			DeferredEvents.Enqueue(EMediaEvent::MediaOpened);
		}
	}
	else
	{
		// topology changed during playback, i.e. track switching
		if (PendingChange)
		{
			RequestedTopology = InTopology;
		}
		else
		{
			CommitTopology(InTopology);
		}
	}

	CurrentDuration = InDuration;

	return true;
}


void FWmfMediaSession::Shutdown()
{
	if (MediaSession == NULL)
	{
		return;
	}

	FScopeLock Lock(&CriticalSection);

	DiscardPendingState();

	MediaSession->Close();
	MediaSession->Shutdown();
	MediaSession.Reset();

	CurrentTopology.Reset();
	PresentationClock.Reset();
	RateControl.Reset();
	RateSupport.Reset();

	CanScrub = false;
	Capabilities = 0;
	CurrentDuration = FTimespan::Zero();
	CurrentRate = 0.0f;
	CurrentState = EMediaState::Closed;
	LastTime = FTimespan::Zero();
	RequestedRate = 0.0f;
	Status = EMediaStatus::None;
	ThinnedRates.Empty();
	UnthinnedRates.Empty();
}


/* IMediaControls interface
 *****************************************************************************/

bool FWmfMediaSession::CanControl(EMediaControl Control) const
{
	if (MediaSession == NULL)
	{
		return false;
	}

	if (Control == EMediaControl::Pause)
	{
		return ((CurrentState == EMediaState::Playing) && (((Capabilities & MFSESSIONCAP_PAUSE) != 0) || UnthinnedRates.Contains(0.0f)));
	}

	if (Control == EMediaControl::Resume)
	{
		return ((CurrentState != EMediaState::Playing) && UnthinnedRates.Contains(1.0f));
	}

	if (Control == EMediaControl::Scrub)
	{
		return CanScrub;
	}

	if (Control == EMediaControl::Seek)
	{
		return (((Capabilities & MFSESSIONCAP_SEEK) != 0) && (CurrentDuration > FTimespan::Zero()));
	}

	return false;
}


FTimespan FWmfMediaSession::GetDuration() const
{
	return CurrentDuration;
}


float FWmfMediaSession::GetRate() const
{
	return CurrentRate;
}


EMediaState FWmfMediaSession::GetState() const
{
	return CurrentState;
}


EMediaStatus FWmfMediaSession::GetStatus() const
{
	return Status;
}


TRangeSet<float> FWmfMediaSession::GetSupportedRates(EMediaRateThinning Thinning) const
{
	return (Thinning == EMediaRateThinning::Thinned) ? ThinnedRates : UnthinnedRates;
}


FTimespan FWmfMediaSession::GetTime() const
{
	MFCLOCK_STATE ClockState;

	if ((PresentationClock == NULL) || FAILED(PresentationClock->GetState(0, &ClockState)) || (ClockState == MFCLOCK_STATE_INVALID))
	{
		return FTimespan::Zero(); // topology not initialized, or clock not started yet
	}

	if (ClockState == MFCLOCK_STATE_STOPPED)
	{
		return LastTime; // WMF always reports zero when stopped
	}

	MFTIME ClockTime;

	if (FAILED(PresentationClock->GetTime(&ClockTime)))
	{
		return FTimespan::Zero();
	}

	return FTimespan(ClockTime);
}


bool FWmfMediaSession::IsLooping() const
{
	return ShouldLoop;
}


bool FWmfMediaSession::Seek(const FTimespan& Time)
{
	if (MediaSession == NULL)
	{
		return false;
	}

	UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Seeking to %s"), this, *Time.ToString());

	FScopeLock Lock(&CriticalSection);

	// validate seek
	if ((CurrentState == EMediaState::Closed) || (CurrentState == EMediaState::Error))
	{
		UE_LOG(LogWmfMedia, Warning, TEXT("Cannot seek while closed or in error state"));
		return false;
	}

	if ((Time < FTimespan::Zero()) || (Time > CurrentDuration))
	{
		UE_LOG(LogWmfMedia, Warning, TEXT("Invalid seek time %s (media duration is %s)"), *Time.ToString(), *CurrentDuration.ToString());
		return false;
	}

	// wait for pending changes to complete
	if (PendingChange)
	{
		UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Requesting seek after pending command"), this);

		RequestedTime = Time;

		return true;
	}

	return CommitTime(Time);
}


bool FWmfMediaSession::SetLooping(bool Looping)
{
	ShouldLoop = Looping;

	return true;
}


bool FWmfMediaSession::SetRate(float Rate)
{
	if (MediaSession == NULL)
	{
		return false;
	}

	FScopeLock Lock(&CriticalSection);

	if (Rate == CurrentRate)
	{
		return true; // rate already set
	}

	EMediaRateThinning Thinning;

	// validate rate
	if (UnthinnedRates.Contains(Rate))
	{
		UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Setting rate from to %f to %f (unthinned)"), this, CurrentRate, Rate);
		Thinning = EMediaRateThinning::Unthinned;
	}
	else if (ThinnedRates.Contains(Rate))
	{
		UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Setting rate from to %f to %f (thinned)"), this, CurrentRate, Rate);
		Thinning = EMediaRateThinning::Thinned;
	}
	else
	{
		UE_LOG(LogWmfMedia, Warning, TEXT("The rate %f is not supported"), Rate);
		return false;
	}

	// wait for pending changes to complete
	if (PendingChange)
	{
		UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Requesting rate change after pending command"), this);

		RequestedRate = Rate;
		RequestedThinning = Thinning;

		return true;
	}

	return CommitRate(Rate, Thinning);
}


/* IMFAsyncCallback interface
 *****************************************************************************/

STDMETHODIMP_(ULONG) FWmfMediaSession::AddRef()
{
	return FPlatformAtomics::InterlockedIncrement(&RefCount);
}


STDMETHODIMP FWmfMediaSession::GetParameters(unsigned long*, unsigned long*)
{
	return E_NOTIMPL; // default behavior
}


STDMETHODIMP FWmfMediaSession::Invoke(IMFAsyncResult* AsyncResult)
{
	FScopeLock Lock(&CriticalSection);

	if (MediaSession == NULL)
	{
		return S_OK;
	}

	// get event
	TComPtr<IMFMediaEvent> Event;
	{
		const HRESULT Result = MediaSession->EndGetEvent(AsyncResult, &Event);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Failed to get event: %s"), this, *WmfMedia::ResultToString(Result));
			return S_OK;
		}
	}

	MediaEventType EventType = MEUnknown;
	{
		const HRESULT Result = Event->GetType(&EventType);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Failed to get session event type: %s"), this, *WmfMedia::ResultToString(Result));
			return S_OK;
		}
	}

	HRESULT EventStatus = S_FALSE;
	{
		const HRESULT Result = Event->GetStatus(&EventStatus);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Failed to get event status: %s"), this, *WmfMedia::ResultToString(Result));
		}
	}

	UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Event [%s] (%s)"), this, *WmfMedia::MediaEventToString(EventType), *WmfMedia::ResultToString(EventStatus));

	// process event
	switch (EventType)
	{
	case MEBufferingStarted:
		Status |= EMediaStatus::Buffering;
		DeferredEvents.Enqueue(EMediaEvent::MediaBuffering);
		break;

	case MEBufferingStopped:
		Status &= ~EMediaStatus::Buffering;
		break;

	case MEError:
		UE_LOG(LogWmfMedia, Error, TEXT("An error occurred in the media session: %s"), *WmfMedia::ResultToString(EventStatus));
		CurrentState = EMediaState::Error;
		DiscardPendingState();
		MediaSession->Close();
		break;

	case MEReconnectEnd:
		Status &= ~EMediaStatus::Connecting;
		break;

	case MEReconnectStart:
		Status |= EMediaStatus::Connecting;
		DeferredEvents.Enqueue(EMediaEvent::MediaConnecting);
		break;

	case MESessionCapabilitiesChanged:
		Capabilities = ::MFGetAttributeUINT32(Event, MF_EVENT_SESSIONCAPS, Capabilities);
		break;

	case MESessionClosed:
		Capabilities = 0;
		LastTime = FTimespan::Zero();
		break;

	case MESessionEnded:
		DeferredEvents.Enqueue(EMediaEvent::PlaybackEndReached);

		if (ShouldLoop)
		{
			// loop back to beginning/end
			RequestedTime = (CurrentRate < 0.0f) ? CurrentDuration : FTimespan::Zero();
			UpdatePendingState();
		}
		else
		{
			CurrentRate = RequestedRate = 0.0f;
			CurrentState = EMediaState::Stopped;
			LastTime = FTimespan::Zero();

			if (RateControl != NULL)
			{
				RateControl->SetRate(TRUE, 0.0f);
			}
		}
		break;

	case MESessionPaused:
		if (SUCCEEDED(EventStatus))
		{
			CurrentState = EMediaState::Paused;
			DeferredEvents.Enqueue(EMediaEvent::PlaybackSuspended);
			UpdatePendingState();
		}
		else
		{
			DiscardPendingState();
		}
		break;

	case MESessionRateChanged:
		if (SUCCEEDED(EventStatus))
		{
			// cache current rate
			PROPVARIANT Value;
			::PropVariantInit(&Value);

			const HRESULT Result = Event->GetValue(&Value);

			if (SUCCEEDED(Result) && (Value.vt == VT_R4))
			{
				CurrentRate = Value.fltVal;
			}
		}
		else if (RateControl != NULL)
		{
			BOOL Thin = FALSE;
			RateControl->GetRate(&Thin, &CurrentRate);
		}

		RequestedRate = CurrentRate;
		UpdatePendingState();
		break;

	case MESessionScrubSampleComplete:
		DeferredEvents.Enqueue(EMediaEvent::SeekCompleted);
		UpdatePendingState();
		break;

	case MESessionStarted:
		if (SUCCEEDED(EventStatus))
		{
			if (CurrentState == EMediaState::Playing)
			{
				DeferredEvents.Enqueue(EMediaEvent::SeekCompleted);
			}

			CurrentState = (CurrentRate == 0.0f) ? EMediaState::Paused : EMediaState::Playing;
			DeferredEvents.Enqueue(EMediaEvent::PlaybackResumed);

			if (CurrentState == EMediaState::Paused)
			{
				// wait for MESessionScrubSampleComplete
			}
			else
			{
				UpdatePendingState();
			}
		}
		else
		{
			DiscardPendingState();
		}
		break;

	case MESessionStopped:
		if (SUCCEEDED(EventStatus))
		{
			CurrentState = EMediaState::Stopped;
			DeferredEvents.Enqueue(EMediaEvent::PlaybackSuspended);
			UpdatePendingState();
		}
		else
		{
			DiscardPendingState();
		}
		break;

	case MESessionTopologySet:
		if (SUCCEEDED(EventStatus))
		{
			if (CurrentState != EMediaState::Preparing)
			{
				// track switching successful
				RequestedTopology.Reset();
				UpdatePendingState();
			}
		}
		else
		{
			if (CurrentState == EMediaState::Preparing)
			{
				// new media has incorrect topology
				CurrentState = EMediaState::Error;
				DeferredEvents.Enqueue(EMediaEvent::MediaOpenFailed);
			}

			DiscardPendingState();
		}
		break;

	case MESessionTopologyStatus:
		if (SUCCEEDED(EventStatus))
		{
			// check if new topology is ready
			UINT32 TopologyStatus = MF_TOPOSTATUS_INVALID;
			{
				const HRESULT Result = Event->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, (UINT32*)&TopologyStatus);

				if (FAILED(Result))
				{
					UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Failed to get topology status: %s"), this, *WmfMedia::ResultToString(Result));
				}
			}

			// initialize new topology
			if (TopologyStatus == MF_TOPOSTATUS_READY)
			{
				UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Topology ready"), this);

				HRESULT Result = MediaSession->GetFullTopology(MFSESSION_GETFULLTOPOLOGY_CURRENT, 0, &CurrentTopology);

				if (SUCCEEDED(Result))
				{
					// get presentation clock, if available
					TComPtr<IMFClock> Clock;

					Result = MediaSession->GetClock(&Clock);

					if (FAILED(Result))
					{
						UE_LOG(LogWmfMedia, Log, TEXT("Session clock unavailable (%s)"), *WmfMedia::ResultToString(Result));
					}
					else
					{
						Result = Clock->QueryInterface(IID_PPV_ARGS(&PresentationClock));

						if (FAILED(Result))
						{
							UE_LOG(LogWmfMedia, Log, TEXT("Presentation clock unavailable (%s)"), *WmfMedia::ResultToString(Result));
						}
						else
						{
							UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Presentation clock ready"), this);
						}
					}

					// get rate control & rate support, if available
					Result = ::MFGetService(MediaSession, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&RateControl));

					if (FAILED(Result))
					{
						UE_LOG(LogWmfMedia, Log, TEXT("Rate control service unavailable (%s)"), *WmfMedia::ResultToString(Result));
					}
					else
					{
						UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Rate control ready"), this);
					}

					Result = ::MFGetService(MediaSession, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&RateSupport));

					if (FAILED(Result))
					{
						UE_LOG(LogWmfMedia, Log, TEXT("Rate support service unavailable (%s)"), *WmfMedia::ResultToString(Result));
					}
					else
					{
						UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Rate support ready"), this);
					}

					// cache rate control properties
					if (RateSupport.IsValid())
					{
						CanScrub = SUCCEEDED(RateSupport->IsRateSupported(TRUE, 0.0f, NULL));

						float MaxRate = 0.0f;
						float MinRate = 0.0f;

						if (SUCCEEDED(RateSupport->GetSlowestRate(MFRATE_FORWARD, TRUE, &MinRate)) &&
							SUCCEEDED(RateSupport->GetFastestRate(MFRATE_FORWARD, TRUE, &MaxRate)))
						{
							ThinnedRates.Add(TRange<float>::Inclusive(MinRate, MaxRate));
						}

						if (SUCCEEDED(RateSupport->GetSlowestRate(MFRATE_REVERSE, TRUE, &MinRate)) &&
							SUCCEEDED(RateSupport->GetFastestRate(MFRATE_REVERSE, TRUE, &MaxRate)))
						{
							ThinnedRates.Add(TRange<float>::Inclusive(MaxRate, MinRate));
						}

						if (SUCCEEDED(RateSupport->GetSlowestRate(MFRATE_FORWARD, FALSE, &MinRate)) &&
							SUCCEEDED(RateSupport->GetFastestRate(MFRATE_FORWARD, FALSE, &MaxRate)))
						{
							UnthinnedRates.Add(TRange<float>::Inclusive(MinRate, MaxRate));
						}

						if (SUCCEEDED(RateSupport->GetSlowestRate(MFRATE_REVERSE, FALSE, &MinRate)) &&
							SUCCEEDED(RateSupport->GetFastestRate(MFRATE_REVERSE, FALSE, &MaxRate)))
						{
							UnthinnedRates.Add(TRange<float>::Inclusive(MaxRate, MinRate));
						}
					}
					else
					{
						CanScrub = false;
						ThinnedRates.Empty();
						UnthinnedRates.Empty();
					}

					// new media opened successfully
					if (CurrentState == EMediaState::Preparing)
					{
						CurrentState = EMediaState::Stopped;
						DeferredEvents.Enqueue(EMediaEvent::MediaOpened);

						UpdatePendingState();
					}
				}
				else
				{
					UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Failed to get full topology: %s"), this, *WmfMedia::ResultToString(Result));

					if (CurrentState == EMediaState::Preparing)
					{
						// incorrect topology for new media
						CurrentState = EMediaState::Error;
						DeferredEvents.Enqueue(EMediaEvent::MediaOpenFailed);
					}

					DiscardPendingState();
				}

				LastTime = FTimespan::Zero();
			}
		}
		else
		{
			if (CurrentState == EMediaState::Preparing)
			{
				UE_LOG(LogWmfMedia, Error, TEXT("An error occured when preparing the topology"));

				// an error occurred in the topology
				CurrentState = EMediaState::Error;
				DeferredEvents.Enqueue(EMediaEvent::MediaOpenFailed);
			}

			DiscardPendingState();
		}
		break;

	default:
		break; // unsupported event
	}

	// request next event
	if ((EventType != MESessionClosed) && (CurrentState != EMediaState::Error) && (MediaSession != NULL))
	{
		const HRESULT Result = MediaSession->BeginGetEvent(this, NULL);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to request next session event; aborting playback: %s"), *WmfMedia::ResultToString(Result));

			Capabilities = 0;
			CurrentState = EMediaState::Error;
		}
	}

	// log current state
	UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Session %llx: CurrentState: %s, CurrentRate: %f, RequestedRate: %f, CurrentTime: %s, RequestedTime: %s, Pending Change: %i"),
		this,
		WmfMediaSession::StateToString(CurrentState),
		CurrentRate,
		RequestedRate,
		*GetTime().ToString(),
		*RequestedTime.ToString(),
		PendingChange
	);
	
	return S_OK;
}


#if _MSC_VER == 1900
	#pragma warning(push)
	#pragma warning(disable:4838)
#endif // _MSC_VER == 1900

STDMETHODIMP FWmfMediaSession::QueryInterface(REFIID RefID, void** Object)
{
	static const QITAB QITab[] =
	{
		QITABENT(FWmfMediaSession, IMFAsyncCallback),
		{ 0 }
	};

	return QISearch(this, QITab, RefID, Object);
}

#if _MSC_VER == 1900
	#pragma warning(pop)
#endif // _MSC_VER == 1900


STDMETHODIMP_(ULONG) FWmfMediaSession::Release()
{
	int32 CurrentRefCount = FPlatformAtomics::InterlockedDecrement(&RefCount);
	
	if (CurrentRefCount == 0)
	{
		delete this;
	}

	return CurrentRefCount;
}


/* FWmfMediaSession implementation
 *****************************************************************************/

bool FWmfMediaSession::CommitRate(float Rate, EMediaRateThinning Thinning)
{
	check(MediaSession != NULL);
	check(PendingChange == false);

	// pause player for basic pausing or if scrubbing isn't supported
	const bool ShouldPause = (Rate == 0.0f) && ((CurrentRate == 1.0f) || !CanControl(EMediaControl::Scrub));

	if (ShouldPause)
	{
		if (CurrentState == EMediaState::Playing)
		{
			const HRESULT Result = MediaSession->Pause();

			if (FAILED(Result))
			{
				UE_LOG(LogWmfMedia, Warning, TEXT("Failed to pause for rate change"));
				return false;
			}

			UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Paused for rate change from %f to %f"), this, CurrentRate, Rate);

			// request deferred rate change
			if (RateControl != NULL)
			{
				RequestedRate = Rate;
				RequestedThinning = Thinning;
			}

			PendingChange = true;

			return true;
		}
	}

	// most rate changes require transition to Stopped first
	if (((Rate >= 0.0f) || (CurrentRate >= 0.0f)) && ((Rate <= 0.0f) || (CurrentRate <= 0.0f)))
	{
		if (!ShouldPause && (CurrentState != EMediaState::Stopped))
		{
			LastTime = GetTime();
			const HRESULT Result = MediaSession->Stop();

			if (FAILED(Result))
			{
				UE_LOG(LogWmfMedia, Warning, TEXT("Failed to stop for rate change"));
				return false;
			}

			UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Stopped for rate change from %f to %f"), this, CurrentRate, Rate);

			// request deferred rate change
			if (RequestedTime == FTimespan::MinValue())
			{
				RequestedTime = LastTime;
			}

			RequestedRate = Rate;
			RequestedThinning = Thinning;
			PendingChange = true;

			return true;
		}
	}

	// determine restart time
	FTimespan RestartTime;

	if (RequestedTime >= FTimespan::Zero())
	{
		RestartTime = RequestedTime;
	}
	else
	{
		RestartTime = GetTime();
	}

	if ((RestartTime == FTimespan::Zero()) && (Rate < 0.0f))
	{
		RestartTime = CurrentDuration; // loop to end
	}
	else if ((RestartTime == CurrentDuration) && (Rate > 0.0f))
	{
		RestartTime = FTimespan::Zero(); // loop to beginning
	}

	// set desired rate
	if (RateControl != NULL)
	{
		const TCHAR* ThinnedString = (Thinning == EMediaRateThinning::Thinned) ? TEXT("thinned") : TEXT("unthinned");
		const HRESULT Result = RateControl->SetRate((Thinning == EMediaRateThinning::Thinned) ? TRUE : FALSE, Rate);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to commit rate change from %f to %f (%s): %s"), CurrentRate, Rate, ThinnedString, *WmfMedia::ResultToString(Result));
			return false;
		}

		UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Committed rate change from %f to %f (%s)"), this, CurrentRate, Rate, ThinnedString);

		// request deferred restart
		if (Rate != 0.0f)
		{
			RequestedTime = RestartTime;
			PendingChange = true;
		}

		return true;
	}

	// restart playback
	if (Rate != 0.0f)
	{
		CommitTime(RestartTime);
	}

	return true;
}


bool FWmfMediaSession::CommitTime(FTimespan Time)
{
	check(MediaSession != NULL);
	check(PendingChange == false);

	// reapply topology if needed
#if 0
	if ((CurrentState != EMediaState::Paused) && (CurrentState != EMediaState::Playing))
	{
		HRESULT Result = MediaSession->SetTopology(0, CurrentTopology);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Error, TEXT("Failed to reapply topology (%s)"), *WmfMedia::ResultToString(Result));
			return false;
		}

		UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Reapplied topology"), this);
	}
#endif

	// seek to requested time
	PROPVARIANT StartPosition;

	if (!CanControl(EMediaControl::Seek) || (Time == FTimespan::MaxValue()))
	{
		StartPosition.vt = VT_EMPTY; // current time
	}
	else
	{
		StartPosition.vt = VT_I8;
		StartPosition.hVal.QuadPart = Time.GetTicks();
	}

	HRESULT Result = MediaSession->Start(NULL, &StartPosition);

	if (FAILED(Result))
	{
		UE_LOG(LogWmfMedia, Warning, TEXT("Failed to commit time %s: %s"), (Time == FTimespan::MaxValue()) ? TEXT("current") : *Time.ToString(), *WmfMedia::ResultToString(Result));
		return false;
	}

	UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Commited time %s"), this, (Time == FTimespan::MaxValue()) ? TEXT("current") : *Time.ToString());

	PendingChange = true;

	return true;
}


bool FWmfMediaSession::CommitTopology(IMFTopology* Topology)
{
	check(MediaSession != NULL);
	check(PendingChange == false);

	if (CurrentState != EMediaState::Stopped)
	{
		LastTime = GetTime();

		// topology change requires transition to Stopped; playback is resumed afterwards
		HRESULT Result = MediaSession->Stop();

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to stop for topology change"));
			return false;
		}

		UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Stopped for topology change"), this);

		// request deferred playback restart
		if (RequestedTime == FTimespan::MinValue())
		{
			RequestedTime = LastTime;
		}

		RequestedTopology = Topology;
		PendingChange = true;

		return true;
	}

	// set new topology
	HRESULT Result = MediaSession->SetTopology(MFSESSION_SETTOPOLOGY_IMMEDIATE, Topology);

	if (SUCCEEDED(Result))
	{
		UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Committed topology"), this);
	}
	else
	{
		UE_LOG(LogWmfMedia, Warning, TEXT("Failed to set topology: %s"), *WmfMedia::ResultToString(Result));
	}

	PendingChange = true;

	return true;
}


void FWmfMediaSession::DiscardPendingState()
{
	PendingChange = false;
	RequestedRate = CurrentRate;
	RequestedTime = FTimespan::MinValue();
	RequestedTopology.Reset();
}


void FWmfMediaSession::UpdatePendingState()
{
	UE_LOG(LogWmfMedia, Verbose, TEXT("Session %llx: Updating pending state"), this);

	PendingChange = false;

	// commit pending topology changes
	if (RequestedTopology != NULL)
	{
		if (!CommitTopology(RequestedTopology))
		{
			RequestedTopology.Reset();
		}

		if (PendingChange)
		{
			return;
		}
	}

	// commit pending rate changes
	if (RequestedRate != CurrentRate)
	{
		if (!CommitRate(RequestedRate, RequestedThinning))
		{
			RequestedRate = CurrentRate;
		}

		if (PendingChange)
		{
			return;
		}
	}

	// commit pending seeks/restarts
	if (RequestedTime >= FTimespan::Zero())
	{
		CommitTime(RequestedTime);

		RequestedTime = FTimespan::MinValue();
	}
}


#include "HideWindowsPlatformTypes.h"

#endif //WMFMEDIA_SUPPORTED_PLATFORM
