import QtQuick
import QtQuick.Effects
import GlyphPop.Core

ListView {
    id: strip

    property int current: -1
    signal picked(int id)

    height: 40
    orientation: ListView.Horizontal
    spacing: 2
    clip: true
    acceptedButtons: Qt.NoButton
    boundsBehavior: Flickable.StopAtBounds
    model: Picker.categories
    // Centre the icon row when it fits, as it does for emoji and symbols.
    leftMargin: Math.max(0, (width - contentWidth) / 2)

    onCurrentChanged: {
        for (let i = 0; i < count; ++i) {
            if (model[i].id === current) {
                positionViewAtIndex(i, ListView.Contain);
                break;
            }
        }
    }

    WheelScroll { view: strip }

    delegate: Item {
        id: button

        required property var modelData
        readonly property bool chosen: strip.current === modelData.id
        readonly property bool iconic: modelData.icon.length > 0

        width: iconic ? Math.max(26, Math.min(34, (strip.width - strip.spacing * (strip.count - 1)) / strip.count)) : chip.implicitWidth + 20
        height: strip.height

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 4
            anchors.bottomMargin: 4
            radius: Theme.radiusSmall
            color: mouse.pressed ? Theme.pressed : mouse.containsMouse ? Theme.hover : "transparent"
        }

        Text {
            id: icon
            visible: false
            anchors.centerIn: parent
            text: button.modelData.icon
            color: Theme.text
            font.family: Picker.section === 0 || button.modelData.id === 999 ? Theme.emojiFamily : Theme.fontFamily
            font.pixelSize: 17
        }

        // Inactive emoji go grey so the strip reads as one quiet row with the current one in colour.
        MultiEffect {
            visible: button.iconic
            source: icon
            anchors.fill: icon
            saturation: button.chosen || mouse.containsMouse ? 0 : -1
            opacity: button.chosen ? 1 : mouse.containsMouse ? 0.9 : 0.6
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        Text {
            id: chip
            visible: !button.iconic
            anchors.centerIn: parent
            text: button.modelData.title
            color: button.chosen ? Theme.text : Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.smallSize + 1
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 3
            anchors.horizontalCenter: parent.horizontalCenter
            width: button.chosen ? (button.iconic ? 14 : chip.implicitWidth) : 0
            height: 2
            radius: 1
            color: Theme.accent
            Behavior on width { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: strip.picked(button.modelData.id)
        }
    }
}
