// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

typedef TSharedPtr< struct FPreviewInfoContainer > FSoundWaveCompressionListItemPtr;

typedef SListView< FSoundWaveCompressionListItemPtr > SSoundWaveCompressionListView;

/** 
 * Info used to setup the rows of the sound quality previewer
 */
class FPreviewInfo
{
public:
	FPreviewInfo(int32 Quality);

	~FPreviewInfo() 
	{ 
		Cleanup(); 
	}

	void Cleanup();

	int32 QualitySetting;

	int32 OriginalSize;

	int32 OggVorbisSize;
	int32 XMASize;
	int32 PS3Size;

	uint8* DecompressedOggVorbis;
	uint8* DecompressedXMA;
	uint8* DecompressedPS3;
};

class UNREALED_API SSoundWaveCompressionOptions : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SSoundWaveCompressionOptions )
		: _SoundWave(NULL)
	{}
		SLATE_ARGUMENT(USoundWave*, SoundWave)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	~SSoundWaveCompressionOptions();

	/** Sets the window the widget is contained in. */
	void SetWindow( TSharedPtr<SWindow> InWindow )
	{
		MyWindow = InWindow;
	}

	/** Static guard for previewer */
	static bool bQualityPreviewerActive;
	static bool IsQualityPreviewerActive() { return bQualityPreviewerActive; }

private:
	/** Generates a row for the compression list. */
	TSharedRef< ITableRow > OnGenerateRowForSoundWavecompressionList( FSoundWaveCompressionListItemPtr Item, const TSharedRef< STableViewBase >& OwnerTable );

	/** 
		* Converts the raw wave data using all the quality settings in the PreviewInfo table
		*/
	void CreateCompressedWaves( void );

	/** Progress Bar Callbacks */
	TOptional<float> GetProgress() const;
	FText GetProgressString() const;
	EVisibility IsProgressBarVisible() const;

	/** Button click callbacks. */
	FReply OnOK_Clicked();
	FReply OnCancel_Clicked();

private:
	/** Source list data for the compression info. */
	TArray< FSoundWaveCompressionListItemPtr > ListData;

	/** Slate widget for displaying compression info. */
	TSharedPtr< SSoundWaveCompressionListView > SoundWaveCompressionList;

	/** Node to perform all of our modifications on. */
	USoundWave* SoundWave;

	/** Original quality setting of SoundWave */
	int32 OriginalQuality;

	/** Sound quality preview thread */
	class FSoundPreviewThread* SoundPreviewThreadRunnable;
	class FRunnableThread* SoundPreviewThread;

	/** Pointer to the containing window. */
	TWeakPtr< SWindow > MyWindow;

	static FPreviewInfo PreviewInfo[];
};

