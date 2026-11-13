import QtQuick

Item {
    property color color: Theme.text

    width: 16
    height: 16

    Rectangle {
        x: 5
        y: 7.4
        width: 7
        height: 1.8
        radius: 0.9
        rotation: -45
        transformOrigin: Item.Left
        color: parent.color
    }
    Rectangle {
        x: 5
        y: 6.8
        width: 7
        height: 1.8
        radius: 0.9
        rotation: 45
        transformOrigin: Item.Left
        color: parent.color
    }
}
