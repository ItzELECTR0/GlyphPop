import QtQuick
import GlyphPop.Core

Rectangle {
    id: root

    signal clicked()

    width: 34
    height: 30
    radius: Theme.radiusSmall + 2
    color: mouse.pressed ? Theme.pressed : mouse.containsMouse ? Theme.hover : "transparent"

    Text {
        anchors.centerIn: parent
        text: Picker.toneSamples[Picker.tone]
        font.family: Theme.emojiFamily
        font.pixelSize: 17
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
