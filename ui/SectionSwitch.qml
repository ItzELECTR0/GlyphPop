import QtQuick
import GlyphPop.Core

Segmented {
    labels: [qsTr("Emoji"), qsTr("Kaomoji"), qsTr("Symbols")]
    current: Picker.section
    onPicked: index => Picker.section = index
}
