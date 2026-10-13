package app.organicmaps.settings

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageButton
import android.widget.LinearLayout
import android.widget.RadioButton
import android.widget.TextView
import androidx.appcompat.app.AlertDialog
import androidx.core.view.ViewCompat
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragment
import app.organicmaps.sdk.Framework
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener
import com.google.android.material.slider.Slider
import com.google.android.material.textfield.TextInputEditText
import com.google.android.material.textfield.TextInputLayout
import org.json.JSONArray
import org.json.JSONObject

// Settings sub-screen managing the list of user tile sources (XYZ {z}/{x}/{y} or simple WMS
// {bbox} URLs). The list is persisted in the C++ core as JSON; exactly one source can be active
// at a time (single raster overlay constraint of the tile-background renderer), and each source
// has its own imagery opacity. Tapping a row makes it active; edit/delete via the row buttons.
class TileSourcesSettingsFragment : BaseMwmFragment() {

    // Local mirror of the persisted source list: [name, url, type, opacityPct].
    private data class Source(val name: String, val url: String, val type: String, val opacityPct: Int)

    private lateinit var container: LinearLayout
    private lateinit var emptyView: TextView
    private val sources = mutableListOf<Source>()
    private var activeIndex = -1

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
        inflater.inflate(R.layout.fragment_tile_sources_settings, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        ViewCompat.setOnApplyWindowInsetsListener(view, ScrollableContentInsetsListener(view))

        this.container = view.findViewById(R.id.tile_sources_container)
        emptyView = view.findViewById(R.id.tile_sources_empty)
        view.findViewById<View>(R.id.tile_sources_add).setOnClickListener { showEditDialog(-1) }

        reloadFromCore()
    }

    private fun reloadFromCore() {
        sources.clear()
        runCatching {
            val array = JSONArray(Framework.nativeGetBackgroundTilesSourcesJson())
            for (i in 0 until array.length()) {
                val obj = array.getJSONObject(i)
                sources.add(
                    Source(
                        obj.optString("name"),
                        obj.optString("url"),
                        obj.optString("type", "xyz"),
                        obj.optInt("opacity", 100),
                    )
                )
            }
        }
        activeIndex = Framework.nativeGetActiveBackgroundTilesSourceIndex()
        renderRows()
    }

    private fun renderRows() {
        container.removeAllViews()
        emptyView.visibility = if (sources.isEmpty()) View.VISIBLE else View.GONE
        val inflater = layoutInflater
        for (i in sources.indices) {
            val source = sources[i]
            val row = inflater.inflate(R.layout.item_tile_source, container, false)
            row.findViewById<RadioButton>(R.id.tile_source_active).isChecked = i == activeIndex
            row.findViewById<TextView>(R.id.tile_source_name).text = source.name.ifEmpty { source.url }
            row.findViewById<TextView>(R.id.tile_source_url).text = source.url
            row.findViewById<TextView>(R.id.tile_source_opacity).text =
                getString(R.string.tile_sources_opacity_value, Integer.toString(source.opacityPct))
            row.setOnClickListener { setActive(i) }
            row.findViewById<ImageButton>(R.id.tile_source_edit).setOnClickListener { showEditDialog(i) }
            row.findViewById<ImageButton>(R.id.tile_source_delete).setOnClickListener { delete(i) }
            container.addView(row)
        }
    }

    private fun setActive(index: Int) {
        Framework.nativeSetActiveBackgroundTilesSource(index)
        activeIndex = index
        renderRows()
    }

    private fun delete(index: Int) {
        val updated = sources.toMutableList().apply { removeAt(index) }
        var newActive = activeIndex
        if (activeIndex == index)
            newActive = -1
        else if (activeIndex > index)
            newActive = activeIndex - 1
        persist(updated, newActive)
    }

    private fun persist(updated: List<Source>, newActive: Int) {
        val array = JSONArray()
        for (s in updated) {
            array.put(
                JSONObject()
                    .put("name", s.name)
                    .put("url", s.url)
                    .put("type", s.type)
                    .put("opacity", s.opacityPct)
            )
        }
        Framework.nativeSetBackgroundTilesSources(array.toString(), newActive)
        reloadFromCore()
    }

    // index == -1 means "add a new source".
    private fun showEditDialog(index: Int) {
        val dialogView = layoutInflater.inflate(R.layout.dialog_tile_source_edit, null)
        val nameInput = dialogView.findViewById<TextInputLayout>(R.id.tile_source_name_input)
        val nameField = dialogView.findViewById<TextInputEditText>(R.id.tile_source_name)
        val urlInput = dialogView.findViewById<TextInputLayout>(R.id.tile_source_url_input)
        val urlField = dialogView.findViewById<TextInputEditText>(R.id.tile_source_url)
        val typeXyz = dialogView.findViewById<RadioButton>(R.id.tile_source_type_xyz)
        val typeWms = dialogView.findViewById<RadioButton>(R.id.tile_source_type_wms)
        val opacitySlider = dialogView.findViewById<Slider>(R.id.tile_source_opacity)
        val opacityLabel = dialogView.findViewById<TextView>(R.id.tile_source_opacity_value)

        if (index >= 0) {
            val source = sources[index]
            nameField.setText(source.name)
            urlField.setText(source.url)
            (if (source.type == "wms") typeWms else typeXyz).isChecked = true
            opacitySlider.value = source.opacityPct.toFloat().coerceIn(opacitySlider.valueFrom, opacitySlider.valueTo)
        }
        opacityLabel.text = getString(R.string.tile_sources_opacity_value, Integer.toString(opacitySlider.value.toInt()))
        opacitySlider.addOnChangeListener { _, value, _ -> opacityLabel.text = getString(R.string.tile_sources_opacity_value, Integer.toString(value.toInt())) }

        val dialog = AlertDialog.Builder(requireActivity())
            .setTitle(if (index >= 0) R.string.tile_sources_edit else R.string.tile_sources_add)
            .setView(dialogView)
            .setPositiveButton(R.string.ok, null)
            .setNegativeButton(R.string.cancel, null)
            .create()
        // Validate before dismissing: an invalid URL keeps the dialog open with the field in red.
        dialog.setOnShowListener {
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener {
                val url = urlField.text.toString().trim()
                val name = nameField.text.toString().trim()
                val type = if (typeWms.isChecked) "wms" else "xyz"
                if (!Framework.nativeIsWellFormedBackgroundTilesUrl(url)) {
                    urlInput.error = getString(R.string.tile_sources_url_error)
                    return@setOnClickListener
                }
                // Enforce type ↔ placeholder match so the stored type is meaningful.
                val hasBbox = url.contains("{bbox}")
                if (type == "wms" != hasBbox) {
                    urlInput.error = getString(
                        if (type == "wms") R.string.tile_sources_url_error_wms else R.string.tile_sources_url_error_xyz,
                    )
                    return@setOnClickListener
                }
                val updated = sources.toMutableList()
                val source = Source(name, url, type, opacitySlider.value.toInt())
                if (index >= 0) updated[index] = source else updated.add(source)
                var newActive = activeIndex
                if (index < 0 && activeIndex < 0) newActive = updated.size - 1
                persist(updated, newActive)
                dialog.dismiss()
            }
        }
        dialog.show()
    }
}
