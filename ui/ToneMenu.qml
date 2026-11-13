import QtQuick
import QtQuick.Effects
import GlyphPop.Core

Item {
    id: root

    property bool open: false

    width: row.width + 8
    height: 42
    visible: opacity > 0
    opacity: open ? 1 : 0
    scale: open ? 1 : 0.96
    transformOrigin: Item.TopRight
    Behavior on opacity { NumberAnimation { duration: 110 } }
    Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }

    RectangularShadow {
        anchors.fill: parent
        radius: Theme.radiusSmall + 2
        blur: 14
        offset.y: 3
        color: Qt.alpha(Theme.shadow, Picker.theme.dark ? 0.45 : 0.14)
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusSmall + 2
        color: Theme.containerHigh
        border.width: 1
        border.color: Theme.outlineVariant
    }

    Row {
        id: row
        x: 4
        anchors.verticalCenter: parent.verticalCenter

        Repeater {
            model: Picker.toneSamples

            delegate: Rectangle {
                id: tone
                required property string modelData
                required property int index

                width: 34
                height: 34
                radius: Theme.radiusSmall
                color: Picker.tone === index ? Theme.selection : mouse.containsMouse ? Theme.hover : "transparent"
                border.width: Picker.tone === index ? 1 : 0
                border.color: Qt.alpha(Theme.accent, 0.55)

                Text {
                    anchors.centerIn: parent
                    text: tone.modelData
                    font.family: Theme.emojiFamily
                    font.pixelSize: 20
                }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        Picker.tone = tone.index;
                        root.open = false;
                    }
                }
            }
        }
    }
}
