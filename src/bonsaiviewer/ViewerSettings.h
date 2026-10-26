// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCINTERFACE_INTERFACESETTINGS_H
#define IFCINTERFACE_INTERFACESETTINGS_H

#include <QObject>
#include <QString>
#include <vector>

namespace bonsaiviewer {

class ViewerSettings : public QObject {
    Q_OBJECT
public:
    enum class ThemeMode {
        Dark = 0,
        Light = 1,
        Custom = 2,
    };
    Q_ENUM(ThemeMode)

    // Navigation presets (spec BonsaiViewer08). Values are persisted: keep order.
    enum class ZoomPreset {
        CenterView = 0,  // zoom toward the camera target
        Cursor = 1,      // zoom toward the first visible surface under the mouse
        Selection = 2,   // zoom toward the bounding-box centre of the selection
    };
    enum class OrbitPreset {
        CenterView = 0,  // orbit around the current centre of view
        Cursor = 1,      // orbit around the first visible surface under the mouse, fixed per drag
    };

    struct ThemeColorSpec {
        const char* key;
        const char* label;
        const char* dark_default;
        const char* light_default;
    };

    static ViewerSettings& instance();
    static const std::vector<ThemeColorSpec>& themeColorSpecs();

    ThemeMode themeMode() const;
    void setThemeMode(ThemeMode mode);

    QString color(const QString& key) const;
    QString customColor(const QString& key) const;
    void setCustomColor(const QString& key, const QString& value);

    ZoomPreset zoomPreset() const;
    void setZoomPreset(ZoomPreset preset);
    OrbitPreset orbitPreset() const;
    void setOrbitPreset(OrbitPreset preset);

signals:
    void themeModeChanged(ThemeMode mode);
    void themeChanged();
    void navigationPresetsChanged();

private:
    ViewerSettings();
    void load();
    void persist() const;

    ThemeMode theme_mode_ = ThemeMode::Dark;
    std::vector<QString> custom_colors_;
    ZoomPreset zoom_preset_ = ZoomPreset::CenterView;
    OrbitPreset orbit_preset_ = OrbitPreset::CenterView;
};

} // namespace bonsaiviewer

#endif
