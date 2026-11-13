import QtQuick

// A flat square button; put the icon inside as a child.
Rectangle {
    id: root

    default property alias content: holder.data
    property bool checked: false
    signal clicked()

    width: 30
    height: 30
    radius: Theme.radiusSmall + 2
    color: mouse.pressed ? Theme.pressed : checked ? Theme.containerHighest : mouse.containsMouse ? Theme.hover : "transparent"

    Item {
        id: holder
        anchors.centerIn: parent
        width: 16
        height: 16
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
