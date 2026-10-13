package app.organicmaps.maplayer;

import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.maplayer.Mode;
import java.util.ArrayList;
import java.util.List;

public class LayersUtils
{
  public static List<Mode> getAvailableLayers()
  {
    List<Mode> availableLayers = new ArrayList<>();
    availableLayers.add(Mode.OUTDOORS);
    availableLayers.add(Mode.ISOLINES);
    availableLayers.add(Mode.HIKING);
    availableLayers.add(Mode.CYCLING);
    availableLayers.add(Mode.SUBWAY);
    // The Satellite toggle is a quick on/off for the active tile source; source management lives in
    // Settings, so only offer the button once a source is configured and picked as active.
    if (Framework.nativeGetActiveBackgroundTilesSourceIndex() >= 0)
      availableLayers.add(Mode.SATELLITE);
    return availableLayers;
  }
}
