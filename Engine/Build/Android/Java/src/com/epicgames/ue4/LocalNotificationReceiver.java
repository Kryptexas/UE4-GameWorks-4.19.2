// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

 package com.epicgames.ue4;
 
 import android.app.Notification;
 import android.app.NotificationManager;
 import android.app.PendingIntent;
 import android.content.BroadcastReceiver;
 import android.content.Context;
 import android.content.Intent;
 
 public class LocalNotificationReceiver extends BroadcastReceiver
 {
 
	 public void onReceive(Context context, Intent intent)
	 {
		int notificationID = intent.getIntExtra("local-notification-ID" , 0);
		String title  = intent.getStringExtra("local-notification-title");
		String details  = intent.getStringExtra("local-notification-body");
		String action = intent.getStringExtra("local-notification-action");
		String activationEvent = intent.getStringExtra("local-notification-activationEvent");

		if(title == null || details == null || action == null || activationEvent == null)
		{
			// Do not schedule any local notification if any allocation failed
			return;
		}

		int notificationIconID = context.getResources().getIdentifier("icon", "drawable", context.getPackageName());

		Notification notification = new Notification(notificationIconID , details ,System.currentTimeMillis());

		// Stick with the defaults
		notification.flags |= Notification.FLAG_AUTO_CANCEL;
		notification.defaults |= Notification.DEFAULT_SOUND | Notification.DEFAULT_VIBRATE;

		// Open UE4 app if clicked
		Intent notificationIntent = new Intent(context, GameActivity.class);

		// launch if closed but resume if running
		notificationIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
		notificationIntent.putExtra("localNotificationID" , notificationID);
		notificationIntent.putExtra("localNotificationAppLaunched" , true);
		notificationIntent.putExtra("localNotificationLaunchActivationEvent", activationEvent);

		PendingIntent pendingNotificationIntent = PendingIntent.getActivity(context, notificationID, notificationIntent, 0);

		notification.setLatestEventInfo(context, title, details, pendingNotificationIntent);
		NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);

		// show the notification
		notificationManager.notify(notificationID, notification); 
	 }
 }
 