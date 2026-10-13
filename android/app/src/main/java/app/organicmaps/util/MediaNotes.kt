package app.organicmaps.util

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.provider.OpenableColumns
import android.webkit.MimeTypeMap
import androidx.core.content.FileProvider
import app.organicmaps.sdk.util.log.Logger
import java.io.File
import java.io.IOException
import java.util.Locale

/**
 * Storage for media notes (audio/video/photo files attached to bookmarks, Feature Gap #19).
 *
 * Only the file *names* are stored in the bookmark's KML extended data
 * (kml::kMediaNotesPropertyKey, newline-separated); the files themselves are copied into the
 * app-private external files dir, so exports stay small and no absolute paths leak into
 * shareable KML files. Attachments are deliberately local-only: nothing leaves the device
 * unless the user shares the media file itself.
 */
object MediaNotes
{
  private const val DIR_NAME = "media_notes"
  private val TAG = MediaNotes::class.java.simpleName

  private fun dir(context: Context): File
  {
    val base = context.getExternalFilesDir(null) ?: context.filesDir
    return File(base, DIR_NAME).apply { mkdirs() }
  }

  /**
   * Copies the picked [uri] into the media-notes dir and returns the stored file name,
   * or null when the copy fails.
   */
  fun attach(context: Context, uri: Uri): String?
  {
    val displayName = queryDisplayName(context, uri) ?: "note"
    val target = uniqueTarget(dir(context), sanitize(displayName)) ?: return null
    return try {
      context.contentResolver.openInputStream(uri)?.use { input ->
        target.outputStream().use { output -> input.copyTo(output) }
      } ?: return null
      target.name
    } catch (_: IOException) {
      target.delete()
      null
    }
  }

  fun resolve(context: Context, name: String): File? =
    if (isSafeName(name)) File(dir(context), name) else null

  fun delete(context: Context, name: String)
  {
    resolve(context, name)?.delete()
  }

  /** Opens a stored media note with an external app via FileProvider. */
  fun open(context: Context, name: String)
  {
    val file = resolve(context, name) ?: return
    val uri = FileProvider.getUriForFile(context, "${context.packageName}.provider", file)
    val intent = Intent(Intent.ACTION_VIEW).apply {
      setDataAndType(uri, context.contentResolver.getType(uri) ?: guessMimeType(name))
      addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
    }
    try
    {
      context.startActivity(Intent.createChooser(intent, name))
    } catch (_: android.content.ActivityNotFoundException) {
      Logger.w(TAG, "No activity to open media note: $name")
    }
  }

  // Names come from bookmark KML extended data, which may be crafted by an imported file —
  // reject anything that could escape the media-notes dir (path traversal, absolute paths).
  private fun isSafeName(name: String): Boolean =
    name.isNotEmpty() && name != "." && name != ".." &&
      !name.contains('/') && !name.contains('\\') && !name.contains("..")

  private fun guessMimeType(name: String): String
  {
    val ext = name.substringAfterLast('.', "").lowercase(Locale.US)
    return MimeTypeMap.getSingleton().getMimeTypeFromExtension(ext) ?: "*/*"
  }

  private fun queryDisplayName(context: Context, uri: Uri): String?
  {
    return try {
      context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
        val idx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
        if (idx >= 0 && cursor.moveToFirst()) cursor.getString(idx) else null
      }
    } catch (_: SecurityException) {
      null
    }
  }

  private fun sanitize(name: String): String
  {
    val cleaned = name.replace(Regex("[/\\\\?%*:|\"<>()\\[\\]]"), "_").trim()
    return cleaned.ifEmpty { "note" }
  }

  private fun uniqueTarget(dir: File, baseName: String): File?
  {
    val dot = baseName.lastIndexOf('.')
    val stem = if (dot > 0) baseName.substring(0, dot) else baseName
    val ext = if (dot > 0) baseName.substring(dot) else ""
    var candidate = File(dir, baseName)
    var i = 1
    while (candidate.exists())
    {
      if (i > 999) return null
      candidate = File(dir, String.format(Locale.US, "%s_%d%s", stem, i++, ext))
    }
    return candidate
  }
}
