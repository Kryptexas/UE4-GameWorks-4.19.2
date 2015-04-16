// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 * Sound quality preview helper thread.
 *
 */

#pragma once

class FPreviewInfo;

class FSoundPreviewThread : public FRunnable
{
public:
	/**
	 * @param PreviewCount	number of sounds to preview
	 * @param Node			sound node to perform compression/decompression
	 * @param Info			preview info class
	 */
	FSoundPreviewThread(int32 PreviewCount, USoundWave* Node, FPreviewInfo* Info);
	
	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	// End of FRunnable interface

	/**
	* @return true if job is finished
	*/
	bool IsTaskFinished() const
	{
		return TaskFinished;
	}

	/**
	 * @return number of sounds to conversion.
	 */
	int32 GetCount() const
	{ 
		return Count; 
	}

	/**
	 * @return index of last converted sound.
	 */
	int32 GetIndex() const 
	{ 
		return Index; 
	}

private:
	/**
	 * Compresses SoundWave for all available platforms, and then decompresses to PCM 
	 *
	 * @param	SoundWave				Wave file to compress
	 * @param	PreviewInfo				Compressed stats and decompressed data
	 */
	static void SoundWaveQualityPreview( USoundWave* SoundWave, FPreviewInfo* PreviewInfo );

private:
	/** Number of sound to preview.	*/
	int32 Count;					
	/** Last converted sound index, also using to conversion loop. */
	int32 Index;					
	/** Pointer to sound wave to perform preview. */
	USoundWave* SoundWave;
	/** Pointer to preview info table. */
	FPreviewInfo* PreviewInfo; 
	/** True if job is finished. */
	bool TaskFinished;
	/** True if cancel calculations and stop thread. */
	bool CancelCalculations;	
};
