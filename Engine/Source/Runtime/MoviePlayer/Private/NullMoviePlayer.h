// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/** An placeholder implementation of the movie player for the editor */
class FNullGameMoviePlayer : public IGameMoviePlayer,
	public TSharedFromThis<FNullGameMoviePlayer, ESPMode::ThreadSafe>
{
public:
	static TSharedPtr<FNullGameMoviePlayer> Get()
	{
		if (!MoviePlayer.IsValid())
		{
			MoviePlayer = MakeShareable(new FNullGameMoviePlayer);
		}
		return MoviePlayer;
	}

	/** IGameMoviePlayer Interface */
	virtual void RegisterMovieStreamer(TSharedPtr<IMovieStreamer> InMovieStreamer) OVERRIDE {}
	virtual void SetSlateRenderer(TSharedPtr<FSlateRenderer> InSlateRenderer) OVERRIDE {}
	virtual void Initialize() OVERRIDE {}
	virtual void PassLoadingScreenWindowBackToGame() const OVERRIDE {}
	virtual void SetupLoadingScreen(const FLoadingScreenAttributes& InLoadingScreenAttributes) OVERRIDE {}
	virtual bool PlayMovie() OVERRIDE { return false;}
	virtual void WaitForMovieToFinish() OVERRIDE {}
	virtual bool IsLoadingFinished() const OVERRIDE {return true;}
	virtual bool IsMovieCurrentlyPlaying() const OVERRIDE  {return false;}
	virtual bool LoadingScreenIsPrepared() const OVERRIDE {return false;}
	virtual void SetupLoadingScreenFromIni() OVERRIDE {}

private:
	FNullGameMoviePlayer() {}
	
private:
	/** Singleton handle */
	static TSharedPtr<FNullGameMoviePlayer> MoviePlayer;
};
