import QtQuick

Rectangle {
    id: root

    property var labels: []
    property int current: 0
    signal picked(int index)

    width: row.width + 6
    height: 30
    radius: Theme.radiusSmall + 2
    color: Theme.containerLow
    border.width: 1
    border.color: Theme.outlineVariant
    opacity: enabled ? 1 : 0.45

    Rectangle {
        readonly property Item target: repeater.count > 0 ? repeater.itemAt(root.current) : null
        x: target ? target.x + row.x : 0
        y: 3
        width: target ? target.width : 0
        height: parent.height - 6
        radius: Theme.radiusSmall
        color: Theme.containerHighest
        Behavior on x { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
        Behavior on width { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 2
            anchors.horizontalCenter: parent.horizontalCenter
            width: 14
            height: 2
            radius: 1
            color: Theme.accent
        }
    }

    Row {
        id: row
        x: 3
        anchors.verticalCenter: parent.verticalCenter

        Repeater {
            id: repeater
            model: root.labels

            delegate: Item {
                id: segment
                required property string modelData
                required property int index
                readonly property bool chosen: root.current === index
                width: label.implicitWidth + 22
                height: root.height - 6

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.radiusSmall
                    color: mouse.containsMouse && !segment.chosen ? Theme.hover : "transparent"
                }

                Text {
                    id: label
                    anchors.centerIn: parent
                    text: segment.modelData
                    color: segment.chosen ? Theme.text : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.textSize - 1
                    font.weight: segment.chosen ? Font.DemiBold : Font.Normal
                }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.picked(segment.index)
                }
            }
        }
    }
}
