import QtQuick

// Three sliders with their knobs at different positions.
Item {
    id: root

    property color color: Theme.textMuted

    width: 16
    height: 16

    Repeater {
        model: [0.7, 0.3, 0.55]

        delegate: Item {
            required property real modelData
            required property int index
            y: 2 + index * 5
            width: root.width
            height: 2

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 1.6
                radius: 0.8
                color: root.color
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                x: parent.width * parent.modelData - width / 2
                width: 5
                height: 5
                radius: 2.5
                color: Theme.surface
                border.width: 1.6
                border.color: root.color
            }
        }
    }
}
