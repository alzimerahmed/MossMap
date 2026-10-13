package app.organicmaps.util;

import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.speech.RecognitionListener;
import android.speech.RecognizerIntent;
import android.speech.SpeechRecognizer;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.ArrayList;

public class InputUtils
{
  private static Boolean mVoiceInputSupported = null;

  private InputUtils()
  { /* static class */
  }

  public static boolean isVoiceInputSupported(@NonNull Context context)
  {
    if (mVoiceInputSupported == null)
      mVoiceInputSupported = Utils.isIntentSupported(context, new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH));

    return mVoiceInputSupported;
  }

  public static Intent createIntentForVoiceRecognition(String promptText)
  {
    final Intent vrIntent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
    vrIntent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_WEB_SEARCH)
        .putExtra(RecognizerIntent.EXTRA_PROMPT, promptText)
        // Prefer an on-device recognition service when one is installed: works offline and
        // keeps the audio from leaving the phone (privacy-first, see phase 11 gap #25).
        .putExtra(RecognizerIntent.EXTRA_PREFER_OFFLINE, true);

    return vrIntent;
  }

  /**
   * On-device speech input via the platform SpeechRecognizer API (AOSP, no GMS dependency).
   * Works offline when the user has installed offline recognition data, and does not route
   * audio through a cloud service. Returns false when no recognition service is available.
   */
  public static boolean isOnDeviceRecognitionAvailable(@NonNull Context context)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M)
      return false;
    return SpeechRecognizer.isRecognitionAvailable(context);
  }

  /**
   * Callback for {@link #startOnDeviceRecognition}.
   */
  public interface VoiceRecognitionCallback
  {
    void onResult(@Nullable String text);
  }

  /**
   * Run speech recognition fully on-device via SpeechRecognizer and deliver the best hypothesis
   * (or null on error / no speech) on the UI thread. The recognizer is released after one shot.
   */
  public static void startOnDeviceRecognition(@NonNull Context context, @NonNull String prompt,
                                              @NonNull VoiceRecognitionCallback callback)
  {
    final SpeechRecognizer recognizer = SpeechRecognizer.createSpeechRecognizer(context);
    final Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
    intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM)
        .putExtra(RecognizerIntent.EXTRA_PROMPT, prompt)
        .putExtra(RecognizerIntent.EXTRA_PREFER_OFFLINE, true);

    recognizer.setRecognitionListener(new RecognitionListener()
    {
      private void finish(@Nullable String text)
      {
        recognizer.destroy();
        final String result = text;
        UiThread.run(() -> callback.onResult(result));
      }

      @Override
      public void onResults(@Nullable android.os.Bundle results)
      {
        final ArrayList<String> matches =
            results == null ? null : results.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION);
        finish(matches == null || matches.isEmpty() ? null : matches.get(0));
      }

      @Override
      public void onError(int error)
      {
        finish(null);
      }

      @Override
      public void onReadyForSpeech(@Nullable android.os.Bundle params) {}

      @Override
      public void onBeginningOfSpeech() {}

      @Override
      public void onRmsChanged(float rmsdB) {}

      @Override
      public void onBufferReceived(@Nullable byte[] buffer) {}

      @Override
      public void onEndOfSpeech() {}

      @Override
      public void onPartialResults(@Nullable android.os.Bundle partialResults) {}

      @Override
      public void onEvent(int eventType, @Nullable android.os.Bundle params) {}
    });
    recognizer.startListening(intent);
  }

  /**
   * Get most confident recognition result or null if nothing is available
   */
  public static String getBestRecognitionResult(Intent vrIntentResult)
  {
    final ArrayList<String> recognizedStrings = vrIntentResult.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS);

    if (recognizedStrings == null)
      return null;

    return recognizedStrings.isEmpty() ? null : recognizedStrings.get(0);
  }

  private static void showKeyboardSync(final View input)
  {
    if (input != null)
      ((InputMethodManager) input.getContext().getSystemService(Context.INPUT_METHOD_SERVICE))
          .showSoftInput(input, InputMethodManager.SHOW_IMPLICIT);
  }

  private static void showKeyboardDelayed(final View input, int delay)
  {
    if (input == null)
      return;

    UiThread.runLater(() -> showKeyboardSync(input), delay);
  }

  public static void showKeyboard(View input)
  {
    if (input == null)
      return;
    if (input.hasWindowFocus())
      showKeyboardSync(input);
    else
      showKeyboardDelayed(input, 100);
  }

  public static void hideKeyboard(View view)
  {
    InputMethodManager imm = (InputMethodManager) view.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
    imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
  }

  /*
   Hacky method to remove focus from the only EditText at activity
   */
  public static void removeFocusEditTextHack(EditText editText)
  {
    editText.setFocusableInTouchMode(false);
    editText.setFocusable(false);
    editText.setFocusableInTouchMode(true);
    editText.setFocusable(true);
  }
}
