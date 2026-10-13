package app.organicmaps.weather

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.ViewFlipper
import androidx.fragment.app.FragmentManager
import app.organicmaps.R
import app.organicmaps.base.BaseMwmDialogFragment
import app.organicmaps.sdk.Framework
import app.organicmaps.sdk.weather.WeatherForecast
import app.organicmaps.sdk.weather.WeatherForecastListener
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.TimeZone
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Weather forecast bottom sheet (Feature Gap #17).
 *
 * Shows current conditions + a short hourly forecast (temperature, precipitation, wind) for the
 * map viewport center, fetched from the keyless Open-Meteo API. Offline / error states show a
 * clear message with a retry button — never a blank screen.
 *
 * Integration point for the map-layers owner: show this fragment from the layers UI, and toggle
 * the radar raster overlay via [Framework.nativeSetWeatherLayerEnabled].
 */
class WeatherFragment : BaseMwmDialogFragment(), WeatherForecastListener
{
  private lateinit var flipper: ViewFlipper
  private lateinit var pointView: TextView
  private lateinit var currentView: TextView
  private lateinit var hourlyContainer: LinearLayout
  private lateinit var errorView: TextView
  private lateinit var retryButton: Button

  private val requestInFlight = AtomicBoolean(false)

  override fun getCustomTheme(): Int = R.style.MwmTheme_BottomSheetDialog

  override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
    inflater.inflate(R.layout.fragment_weather, container, false)

  override fun onViewCreated(view: View, savedInstanceState: Bundle?)
  {
    super.onViewCreated(view, savedInstanceState)
    flipper = view.findViewById(R.id.weather_flipper)
    pointView = view.findViewById(R.id.weather_point)
    currentView = view.findViewById(R.id.weather_current)
    hourlyContainer = view.findViewById(R.id.weather_hourly_container)
    errorView = view.findViewById(R.id.weather_error)
    retryButton = view.findViewById(R.id.weather_retry)
    retryButton.setOnClickListener { fetchForecast() }
    fetchForecast()
  }

  private fun fetchForecast()
  {
    if (!requestInFlight.compareAndSet(false, true))
      return
    showView(VIEW_LOADING)
    val center = Framework.nativeGetViewportCenterLatLon()
    pointView.text = String.format(Locale.US, "%.4f, %.4f", center[0], center[1])
    Framework.nativeGetWeatherForecast(center[0], center[1], this)
  }

  override fun onForecast(forecast: WeatherForecast?, error: String?)
  {
    if (!isAdded)
      return
    requestInFlight.set(false)
    if (forecast == null || error != null)
    {
      errorView.text = getString(R.string.weather_error_detail, error ?: getString(R.string.weather_error))
      showView(VIEW_ERROR)
      return
    }

    currentView.text = formatCurrent(forecast)
    hourlyContainer.removeAllViews()
    for (i in forecast.mHourlyTime.indices)
      hourlyContainer.addView(makeHourlyRow(forecast, i))
    showView(VIEW_CONTENT)
  }

  private fun showView(index: Int)
  {
    flipper.displayedChild = index
  }

  private fun formatCurrent(forecast: WeatherForecast): String
  {
    val c = forecast.mCurrent
    return getString(
      R.string.weather_current_format,
      formatTemperature(c[1]),
      getString(R.string.weather_feels_like, formatTemperature(c[2])),
      formatPrecipitation(c[3]),
      formatWind(c[5]))
  }

  private fun makeHourlyRow(forecast: WeatherForecast, index: Int): View
  {
    val context = requireContext()
    val row = TextView(context)
    row.layoutParams = LinearLayout.LayoutParams(
      ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
    row.minimumHeight = (48 * resources.displayMetrics.density).toInt() // 48dp touch target
    row.gravity = android.view.Gravity.CENTER_VERTICAL
    val time = TIME_FORMAT.format(Date((forecast.mHourlyTime[index] * 1000).toLong()))
    row.text = getString(
      R.string.weather_hourly_row_format,
      time,
      formatTemperature(forecast.mHourlyTemperature[index]),
      formatPrecipitation(forecast.mHourlyPrecipitation[index]),
      formatWind(forecast.mHourlyWindSpeed[index]))
    return row
  }

  private fun formatTemperature(celsius: Double): String =
    getString(R.string.weather_temperature_c, String.format(Locale.US, "%.1f", celsius))

  private fun formatPrecipitation(mm: Double): String =
    getString(R.string.weather_precipitation_mm, String.format(Locale.US, "%.1f", mm))

  private fun formatWind(kmh: Double): String =
    getString(R.string.weather_wind_kmh, String.format(Locale.US, "%.0f", kmh))

  companion object
  {
    private const val VIEW_LOADING = 0
    private const val VIEW_CONTENT = 1
    private const val VIEW_ERROR = 2

    // Local time: a forecast labeled in UTC misleads the user about "when" it applies.
    private val TIME_FORMAT = SimpleDateFormat("HH:mm", Locale.getDefault())

    @JvmStatic
    fun show(fragmentManager: FragmentManager)
    {
      if (fragmentManager.isStateSaved)
        return
      WeatherFragment().show(fragmentManager, TAG)
    }

    const val TAG = "WeatherFragment"
  }
}
