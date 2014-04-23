//This file needs to be here so the "ant" build step doesnt fail when looking for a /src folder.

package com.epicgames.ue4;

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
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.games.Games;
import com.google.android.gms.common.GooglePlayServicesUtil;

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

	@Override
	public void onStart()
	{
		super.onStart();
		
		Log.debug("==================================> Inside onStart function in GameActivity");
		
		AndroidThunkJava_GooglePlayConnect();
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

	// Callbacks to handle connections with Google Play
	 @Override
    public void onConnected(Bundle connectionHint)
	{
        Log.debug("Connected to Google Play Services.");
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
		if(nativeIsGooglePlayEnabled())
		{
			googleClient.connect();
		}
	}

	public void AndroidThunkJava_ShowLeaderboard(String LeaderboardID)
	{
		Log.debug("In AndroidThunkJava_ShowLeaderboard, ID is " + LeaderboardID);
		if(googleClient.isConnected())
		{
			startActivityForResult(Games.Leaderboards.getLeaderboardIntent(googleClient, LeaderboardID), 0);
		}
	}

	public void AndroidThunkJava_ShowAchievements()
	{
		Log.debug("In AndroidThunkJava_ShowAchievements");
		if(googleClient.isConnected())
		{
			startActivityForResult(Games.Achievements.getAchievementsIntent(googleClient), 0);
		}
	}

	public void AndroidThunkJava_WriteLeaderboardValue(String LeaderboardID, long Value)
	{
		Log.debug("In AndroidThunkJava_WriteLeaderboardValue, ID is " + LeaderboardID + ", value is " + Value);
		if(googleClient.isConnected())
		{
			Games.Leaderboards.submitScore(googleClient, LeaderboardID, Value);
		}
	}

	public native boolean nativeIsShippingBuild();
	public native void nativeSetGlobalActivity();
	public native void nativeSetWindowInfo(boolean bIsPortrait);
	public native void nativeSetObbInfo(String PackageName, int Version, int PatchVersion);

	public native void nativeConsoleCommand(String commandString);
	
	public native boolean nativeIsGooglePlayEnabled();

	static
	{
		System.loadLibrary("UE4");
	}
}

