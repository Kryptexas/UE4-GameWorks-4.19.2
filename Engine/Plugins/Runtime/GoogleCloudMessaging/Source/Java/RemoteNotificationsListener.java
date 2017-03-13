// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
package com.epicgames.ue4;

import android.os.Bundle;
import com.google.android.gms.gcm.GcmListenerService;

public class RemoteNotificationsListener extends GcmListenerService
{
  @Override
  public void onMessageReceived( String from, Bundle data ) 
  {
    String message = data.getString( "message" );
    GameActivity._activity.nativeGCMReceivedRemoteNotification( message );
  }
}
