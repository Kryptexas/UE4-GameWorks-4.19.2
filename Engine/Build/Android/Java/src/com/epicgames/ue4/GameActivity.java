//This file needs to be here so the "ant" build step doesnt fail when looking for a /src folder.

package com.epicgames.ue4;

import java.util.Map;
import java.util.HashMap;

import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;

import android.app.AlertDialog;
import android.app.Dialog;
import android.widget.EditText;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.IntentSender.SendIntentException;

import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.PendingResult;
import com.google.android.gms.common.api.ResultCallback;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GooglePlayServicesUtil;
import com.google.android.gms.games.achievement.*;
import com.google.android.gms.games.Games;



// TODO: use the resources from the UE4 lib project once we've got the packager up and running
//import com.epicgames.ue4.R;


//Extending NativeActivity so that this Java class is instantiated
//from the beginning of the program.  This will allow the user
//to instantiate other Java libraries from here, that the user
//can then use the functions from C++
//NOTE -- This class is not necessary for the UnrealEngine C++ code
//  to startup, as this is handled through the base NativeActivity class.
//  This class's functionality is to provide a way to instantiate other
//  Java libraries at the startup of the program and store references 
//  to them in this class.

public class GameActivity extends NativeActivity implements GoogleApiClient.ConnectionCallbacks, GoogleApiClient.OnConnectionFailedListener
{
	public static Logger Log = new Logger("UE4");
	
	GameActivity _activity;	
	AlertDialog alert;
	
	private GoogleApiClient googleClient;
	private boolean bResolvingGoogleServicesError = false;
	private int dialogError = 0;

	/** Request code to use when launching the Google Services resolution activity */
    private static final int GOOGLE_SERVICES_REQUEST_RESOLVE_ERROR = 1001;

	/** Unique tag for the error dialog fragment */
    private static final String DIALOG_ERROR = "dialog_error";

	/** Unique ID to identify Google Play Services error dialog */
	private static final int PLAY_SERVICES_DIALOG_ID = 1;

	/** Arbitrary ID for leaderboard display */
	private static final int REQUEST_LEADERBOARDS = 0;
	
	/** Arbitrary ID for achievement display */
	private static final int REQUEST_ACHIEVEMENTS = 1;

	/** Stores the minimum amount of data we need to set achievement progress */
	private class BasicAchievementData
	{
		public BasicAchievementData()
		{
			Type = Achievement.TYPE_STANDARD;
			MaxSteps = 1;
		}

		public BasicAchievementData(int InMaxSteps)
		{
			Type = Achievement.TYPE_INCREMENTAL;
			MaxSteps = InMaxSteps;
		}

		public int Type;
		public int MaxSteps;
	}

	/**
	 * Store achievement data upon login so that we can convert the percentage values from the game to
	 * integer steps for Google Play
	 */
	private Map<String, BasicAchievementData> CachedAchievements = new HashMap<String, BasicAchievementData>();

	@Override
	public void onStart()
	{
		super.onStart();
		
		Log.debug("==================================> Inside onStart function in GameActivity");
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		
		// Suppress java logs in Shipping builds
		if (nativeIsShippingBuild())
		{
			Logger.SuppressLogs();
		}

		_activity = this;

		// tell the engine if this is a portrait app
		nativeSetGlobalActivity();
		nativeSetWindowInfo(getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT);

		try
		{
			int Version = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
			int PatchVersion = 0;
			nativeSetObbInfo(getApplicationContext().getPackageName(), Version, PatchVersion);
		}
		catch (Exception e)
		{
			// if the above failed, then, we can't use obbs
			Log.debug("==================================> PackageInfo failure getting .obb info: " + e.getMessage());
		}
		
		final AlertDialog.Builder alertBuilder = new AlertDialog.Builder(this);
		final EditText consoleInputBox = new EditText(this);
		
		alertBuilder.setView(consoleInputBox);
		alertBuilder.setPositiveButton("Ok", new DialogInterface.OnClickListener()
		{
			public void onClick(DialogInterface dialog, int whichButton) {
				String message = consoleInputBox.getText().toString().trim();
				nativeConsoleCommand(message);
				consoleInputBox.setText(" ");
				dialog.dismiss();
			}
		});

		alertBuilder.setNegativeButton("Cancel", new DialogInterface.OnClickListener()
		{
			public void onClick(DialogInterface dialog, int whichButton)
			{
				consoleInputBox.setText(" ");
				dialog.dismiss();
			}
		});
		
		alertBuilder.setTitle("Console Window - Enter Command");
		
		alert = alertBuilder.create();

		// Connect to Google Play Services
		googleClient = new GoogleApiClient.Builder(this)
         .addApi(Games.API)
         .addScope(Games.SCOPE_GAMES)
         .addConnectionCallbacks(this)
         .addOnConnectionFailedListener(this)
         .build();
	}

	@Override
	public void onStop()
	{
		super.onStop();

		googleClient.disconnect();
	}

	/** Callback that fills in CachedAchievements when the load operation completes. */
	private class AchievementsResultStartupCallback implements ResultCallback<Achievements.LoadAchievementsResult>
	{
		@Override
		public void onResult(Achievements.LoadAchievementsResult result)
		{
			Log.debug("Google Play Services: Loaded achievements with status " + result.getStatus().toString());

			AchievementBuffer Achievements = result.getAchievements();

			CachedAchievements.clear();
			for(int i = 0; i < Achievements.getCount(); ++i)
			{
				Achievement CurrentAchievement = Achievements.get(i);

				if(CurrentAchievement.getType() == Achievement.TYPE_STANDARD)
				{
					CachedAchievements.put(new String(CurrentAchievement.getAchievementId()), new BasicAchievementData());
				}
				else if(CurrentAchievement.getType() == Achievement.TYPE_INCREMENTAL)
				{
					CachedAchievements.put(new String(CurrentAchievement.getAchievementId()),
						new BasicAchievementData(CurrentAchievement.getTotalSteps()));
				}
			}

			Achievements.close();
			result.release();
		}
	}

	// Callbacks to handle connections with Google Play
	@Override
    public void onConnected(Bundle connectionHint)
	{
        Log.debug("Connected to Google Play Services.");

		// Load achievements. Since games are expected to pass in achievement progress as a percentage,
		// we need to know what the maximum steps are in order to convert the percentage to an integer
		// number of steps.
		PendingResult<Achievements.LoadAchievementsResult> loadAchievementsResult = Games.Achievements.load(googleClient, false);
		loadAchievementsResult.setResultCallback(new AchievementsResultStartupCallback());
    }

    @Override
    public void onConnectionSuspended(int cause)
	{
        // The connection has been interrupted.
        // TODO: Disable any UI components that depend on Google APIs
        // until onConnected() is called.
		Log.debug("Google Play Services connection suspended.");
    }
	
    @Override
    public void onConnectionFailed(ConnectionResult result)
	{
		Log.debug("Google Play Services connection failed: " + result.toString());

		if (bResolvingGoogleServicesError)
		{
			// Already attempting to resolve an error.
			return;
		}
		else if (result.hasResolution())
		{
            try
			{
				Log.debug("Starting Google Play Services connection resolution");
                bResolvingGoogleServicesError = true;
                result.startResolutionForResult(this, GOOGLE_SERVICES_REQUEST_RESOLVE_ERROR);
            }
			catch (SendIntentException e)
			{
                // There was an error with the resolution intent. Try again.
                googleClient.connect();
            }
        }
		else
		{
            // Show dialog using GooglePlayServicesUtil.getErrorDialog()
			dialogError = result.getErrorCode();
			showDialog(PLAY_SERVICES_DIALOG_ID);

            bResolvingGoogleServicesError = true;
        }
    }

	@Override
	protected Dialog onCreateDialog(int id)
	{
		if(id == PLAY_SERVICES_DIALOG_ID)
		{
			Dialog dialog = GooglePlayServicesUtil.getErrorDialog(dialogError, this, GOOGLE_SERVICES_REQUEST_RESOLVE_ERROR);
			dialog.show();
		}

		return super.onCreateDialog(id);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if (requestCode == GOOGLE_SERVICES_REQUEST_RESOLVE_ERROR)
		{
			Log.debug("Google Play Services connection resolution finished with resultCode " + resultCode);
			bResolvingGoogleServicesError = false;
			if (resultCode == RESULT_OK)
			{
				// Make sure the app is not already connected or attempting to connect
				if (!googleClient.isConnecting() &&	!googleClient.isConnected())
				{
					googleClient.connect();
				}
			}
		}
	}

	// Called from event thread in NativeActivity	
	public void AndroidThunkJava_ShowConsoleWindow(String Formats)
	{
		if(alert.isShowing() == true)
		{
			Log.debug("Console already showing.");
			return;
		}

		alert.setMessage("[Availble texture formats: " + Formats + "]");
		_activity.runOnUiThread(new Runnable()
		{
			public void run()
			{
				if(alert.isShowing() == false)
				{
					Log.debug("Console not showing yet");
					alert.show(); 
				}
			}
		});
	}
	
	public void AndroidThunkJava_LaunchURL(String URL)
	{
		try
		{
			Intent BrowserIntent = new Intent(Intent.ACTION_VIEW, android.net.Uri.parse(URL));
			startActivity(BrowserIntent);
		}
		catch (Exception e)
		{
			Log.debug("LaunchURL failed with exception " + e.getMessage());
		}
	}

	public void AndroidThunkJava_GooglePlayConnect()
	{
		googleClient.connect();
	}

	public void AndroidThunkJava_ShowLeaderboard(String LeaderboardID)
	{
		Log.debug("In AndroidThunkJava_ShowLeaderboard, ID is " + LeaderboardID);
		if(!googleClient.isConnected())
		{
			Log.debug("Not connected to Google Play, can't show leaderboards UI.");
			return;
		}

		startActivityForResult(Games.Leaderboards.getLeaderboardIntent(googleClient, LeaderboardID), REQUEST_LEADERBOARDS);
	}

	public void AndroidThunkJava_ShowAchievements()
	{
		Log.debug("In AndroidThunkJava_ShowAchievements");
		if(!googleClient.isConnected())
		{
			Log.debug("Not connected to Google Play, can't show achievements UI.");
			return;
		}
		
		startActivityForResult(Games.Achievements.getAchievementsIntent(googleClient), REQUEST_ACHIEVEMENTS);
	}

	public void AndroidThunkJava_WriteLeaderboardValue(String LeaderboardID, long Value)
	{
		Log.debug("In AndroidThunkJava_WriteLeaderboardValue, ID is " + LeaderboardID + ", value is " + Value);
		if(googleClient.isConnected())
		{
			Games.Leaderboards.submitScore(googleClient, LeaderboardID, Value);
		}
	}

	public void AndroidThunkJava_WriteAchievement(String AchievementID, float Percentage)
	{
		BasicAchievementData Data = CachedAchievements.get(AchievementID);

		if(Data == null)
		{
			Log.debug("Couldn't find cached achievement for ID " + AchievementID + ", not setting progress.");
			return;
		}

		if(!googleClient.isConnected())
		{
			Log.debug("Not connected to Google Play, can't set achievement progress.");
			return;
		}

		// Found the one to unlock.
		switch(Data.Type)
		{
			case Achievement.TYPE_INCREMENTAL:
			{
				float StepFraction = (Percentage / 100.0f) * Data.MaxSteps;
				int RoundedSteps = Math.round(StepFraction);

				if(RoundedSteps > 0)
				{
					Log.debug("Incremental achievement ID " + AchievementID + ": setting progress to " + RoundedSteps);
					Games.Achievements.setSteps(googleClient, AchievementID, RoundedSteps);
				}
				else
				{
					Log.debug("Incremental achievement ID " + AchievementID + ": not setting progress to " + RoundedSteps);
				}
				break;
			}

			case Achievement.TYPE_STANDARD:
			{
				// Standard achievements only unlock if the progress is at least 100%.
				if(Percentage >= 100.0f)
				{
					Log.debug("Standard achievement ID " + AchievementID + ": unlocking");
					Games.Achievements.unlock(googleClient, AchievementID);
				}
				break;
			}
		}
	}

	public native boolean nativeIsShippingBuild();
	public native void nativeSetGlobalActivity();
	public native void nativeSetWindowInfo(boolean bIsPortrait);
	public native void nativeSetObbInfo(String PackageName, int Version, int PatchVersion);

	public native void nativeConsoleCommand(String commandString);
	
	static
	{
		System.loadLibrary("UE4");
	}
}

