package app.organicmaps.sdk.location;

import androidx.annotation.Keep;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import app.organicmaps.sdk.bookmarks.data.TrackStatistics;

public class TrackRecorder
{
  public static native void nativeStartTrackRecording();

  public static native void nativeStopTrackRecording();

  public static native void nativeSaveTrackRecordingWithName(String name);

  public static native boolean nativeIsTrackRecordingEmpty();

  public static native boolean nativeIsTrackRecordingEnabled();

  /** Minimal interval in seconds between recorded track points. Persisted in the core settings. */
  public static native void nativeSetTrackRecordingInterval(double seconds);

  public static native double nativeGetTrackRecordingInterval();

  /** Minimal horizontal accuracy in meters for a point to be recorded. Persisted in the core settings. */
  public static native void nativeSetTrackRecordingAccuracy(double meters);

  public static native double nativeGetTrackRecordingAccuracy();

  public static native void nativeSetTrackRecordingStatsListener(TrackRecorder.TrackRecordingUpdateHandler listener);

  public static native ElevationInfo nativeGetElevationInfo();

  /** Saves the recorded track, if there is anything to save, stops the recording and reports whether it saved. */
  public static boolean saveAndStop()
  {
    final boolean saved = !nativeIsTrackRecordingEmpty();
    if (saved)
      nativeSaveTrackRecordingWithName("");
    nativeStopTrackRecording();
    return saved;
  }

  public interface TrackRecordingUpdateHandler
  {
    @Keep
    @SuppressWarnings("unused")
    void onTrackRecordingUpdate(TrackStatistics trackStatistics);
  }
}
