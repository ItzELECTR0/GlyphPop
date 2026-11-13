pragma Singleton
import QtQuick
import GlyphPop.Core

QtObject {
    readonly property var colors: Picker.theme.colors

    readonly property color surface: colors.surface
    readonly property color containerLow: colors.surface_container_low
    readonly property color container: colors.surface_container
    readonly property color containerHigh: colors.surface_container_high
    readonly property color containerHighest: colors.surface_container_highest
    readonly property color text: colors.on_surface
    readonly property color textMuted: colors.on_surface_variant
    readonly property color outline: colors.outline
    readonly property color outlineVariant: colors.outline_variant
    readonly property color accent: colors.primary
    readonly property color onAccent: colors.on_primary
    readonly property color shadow: colors.shadow

    readonly property color hover: Qt.alpha(text, 0.07)
    readonly property color pressed: Qt.alpha(text, 0.12)
    readonly property color selection: Qt.alpha(accent, Picker.theme.dark ? 0.22 : 0.18)

    readonly property int radius: Picker.theme.radius
    readonly property int radiusSmall: Math.max(2, Math.round(radius * 0.6))

    readonly property string fontFamily: Qt.application.font.family
    readonly property string emojiFamily: Picker.emojiFamily.length ? Picker.emojiFamily : fontFamily
    readonly property int textSize: 13
    readonly property int smallSize: 11
    readonly property int glyphSize: 24

    readonly property int cell: 40
    readonly property int padding: 10
    // Room around the card for its shadow; the popup's caret anchor sits at the outer corner.
    readonly property int shadowMargin: 14
    readonly property int cardWidth: cell * 8 + padding * 2 + 8
    readonly property int cardHeight: 452
}
