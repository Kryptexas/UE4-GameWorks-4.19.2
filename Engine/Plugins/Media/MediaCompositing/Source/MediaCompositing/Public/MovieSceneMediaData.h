#pragma once

#include "Misc/Timespan.h"
#include "Evaluation/MovieScenePropertyTemplate.h"

class UMediaPlayer;
enum class EMediaEvent;


/**
 * Persistent data that's stored for each currently evaluating section.
 */
struct MEDIACOMPOSITING_API FMovieSceneMediaData
	: PropertyTemplate::FSectionData
{
	/** Default constructor. */
	FMovieSceneMediaData();

	/** Virtual destructor. */
	virtual ~FMovieSceneMediaData() override;

public:

	/**
	 * Get the media player used by this persistent data.
	 *
	 * @return The currently used media player, if any.
	 */
	UMediaPlayer* GetMediaPlayer();

	/**
	 * Set the time to seek to after opening a media source has finished.
	 *
	 * @param Time The time to seek to.
	 */
	void SeekOnOpen(FTimespan Time);

	/** Set up this persistent data object. */
	void Setup();

private:

	/** Callback for media player events. */
	void HandleMediaPlayerEvent(EMediaEvent Event);

private:

	/** The media player used by this object. */
	UMediaPlayer* MediaPlayer;

	/** The time to seek to after the media source is opened. */
	FTimespan SeekOnOpenTime;
};
