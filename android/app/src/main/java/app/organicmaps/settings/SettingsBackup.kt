package app.organicmaps.settings

import android.content.Context
import android.net.Uri
import app.organicmaps.sdk.util.log.Logger
import org.json.JSONObject
import java.io.IOException

/**
 * File-based backup/restore of app settings (Feature Gap #24) — cloud-neutral and privacy-first:
 * the user picks the destination file via the system file picker (SAF), nothing is uploaded
 * anywhere and no tracking backend is involved. Bookmarks/tracks already have their own
 * KML/GPX/GeoJSON export (see BookmarksSharingHelper); this covers the settings half.
 *
 * The backup is a plain JSON dump of the app's SharedPreferences store, minus keys that must
 * never leave the device: OAuth credentials, session/first-run state, device-specific paths
 * and timestamps. Values are stored with a type tag so booleans/ints/longs survive a round-trip.
 */
object SettingsBackup
{
  private val TAG = SettingsBackup::class.java.simpleName

  private const val FORMAT_KEY = "format"
  private const val FORMAT_VERSION = 1
  private const val VALUES_KEY = "values"

  // Credentials and device/session state are excluded on purpose. Prefix filters keep
  // future secret-like keys from leaking even if they aren't added to the denylist.
  private val EXCLUDED_KEYS = setOf(
    "OsmToken",
    "OsmSecret",
    "OsmOAuth2Token",
    "OsmUsername",
    "OsmChangesetsCount",
    "StoragePath",
    "IsDisclaimerApproved",
    "LocationRequested",
    "UseMobileDataTimestamp",
    "AGPSTimestamp",
    "LaunchNumber",
    "LastSessionTimestamp",
    "FirstInstallVersion",
    "LastInstallVersion",
    "FirstStartDialogSeen",
  )

  private val EXCLUDED_PREFIXES = listOf("Osm", "Token", "Secret", "Password")

  private fun isExcluded(key: String): Boolean =
    key in EXCLUDED_KEYS || EXCLUDED_PREFIXES.any { key.startsWith(it, ignoreCase = true) }

  fun export(context: Context, prefsFile: String, target: Uri): Boolean
  {
    val prefs = context.getSharedPreferences(prefsFile, Context.MODE_PRIVATE)
    val values = JSONObject()
    for ((key, value) in prefs.all)
    {
      if (isExcluded(key) || value == null) continue
      // String sets can't round-trip through toString() — restoring them as plain strings
      // would make getStringSet() throw ClassCastException, so they are skipped.
      if (value is Set<*>) continue
      values.put(
        key,
        JSONObject(
          mapOf(
            "type" to typeName(value),
            "value" to value.toString(),
          )
        ),
      )
    }
    val root = JSONObject(mapOf(FORMAT_KEY to FORMAT_VERSION, VALUES_KEY to values))
    return try {
      context.contentResolver.openOutputStream(target)?.use { out ->
        out.write(root.toString(2).toByteArray(Charsets.UTF_8))
      } != null
    } catch (e: IOException) {
      Logger.e(TAG, "Failed to write settings backup", e)
      false
    }
  }

  fun import(context: Context, prefsFile: String, source: Uri): Boolean
  {
    val root = try {
      context.contentResolver.openInputStream(source)?.use { input ->
        JSONObject(input.readBytes().toString(Charsets.UTF_8))
      }
    } catch (e: IOException) {
      Logger.e(TAG, "Failed to read settings backup", e)
      null
    } ?: return false

    if (root.optInt(FORMAT_KEY) != FORMAT_VERSION)
      return false
    val values = root.optJSONObject(VALUES_KEY) ?: return false

    val prefs = context.getSharedPreferences(prefsFile, Context.MODE_PRIVATE)
    val editor = prefs.edit()
    for (key in values.keys())
    {
      if (isExcluded(key)) continue
      val entry = values.optJSONObject(key) ?: continue
      when (entry.optString("type"))
      {
        "boolean" -> editor.putBoolean(key, entry.optString("value") == "true")
        "int" -> entry.optString("value").toIntOrNull()?.let { editor.putInt(key, it) }
        "long" -> entry.optString("value").toLongOrNull()?.let { editor.putLong(key, it) }
        "float" -> entry.optString("value").toFloatOrNull()?.let { editor.putFloat(key, it) }
        "string" -> editor.putString(key, entry.optString("value"))
      }
    }
    editor.apply()
    return true
  }

  private fun typeName(value: Any): String =
    when (value)
    {
      is Boolean -> "boolean"
      is Int -> "int"
      is Long -> "long"
      is Float -> "float"
      is String -> "string"
      else -> "string"
    }
}
