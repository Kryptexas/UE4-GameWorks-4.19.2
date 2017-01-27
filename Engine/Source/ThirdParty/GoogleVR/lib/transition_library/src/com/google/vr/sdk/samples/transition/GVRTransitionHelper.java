package com.google.vr.sdk.samples.transition;
import android.app.Activity;
import android.util.Log;
import com.google.vr.ndk.base.DaydreamApi;

public class GVRTransitionHelper {
	static public final int EXITVR_REQ_CODE = 77779;

	//exitFromVr with the game activity and OnActivityResult will take care of the result
	static public void transitionTo2D(final Activity vrActivity) {
		vrActivity.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				try {
					DaydreamApi daydreamApi = (DaydreamApi)vrActivity.getClass().getMethod("getDaydreamApi").invoke(vrActivity);
					daydreamApi.exitFromVr(vrActivity, GVRTransitionHelper.EXITVR_REQ_CODE, null);
				} catch (Exception e) {
					Log.e("GVRTransitionHelper", "exception: " + e.toString());
				}
			}
		});
	}

	// show the backToVR button and let user colse the 2D activity and resume to the game
	static public void transitionToVR() {
		GVRTransition2DActivity.getActivity().showReturnButton();
	}

	// let native modules know that 2D transition is completed
	public static native void onTransitionTo2D(Activity activity);
}