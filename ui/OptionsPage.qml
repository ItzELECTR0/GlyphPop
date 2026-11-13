import QtQuick
import GlyphPop.Core

FocusScope {
    id: root

    readonly property var theme: Picker.theme
    readonly property var modes: ["system", "light", "dark"]

    Keys.onEscapePressed: Picker.options = false

    Item {
        id: header
        width: parent.width
        height: 30

        IconButton {
            id: back
            onClicked: Picker.options = false
            BackIcon {}
        }

        Text {
            anchors.left: back.right
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Options")
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.textSize + 1
            font.weight: Font.DemiBold
        }
    }

    Flickable {
        anchors.top: header.bottom
        anchors.topMargin: 6
        anchors.bottom: parent.bottom
        width: parent.width
        contentHeight: column.height + 8
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: column
            width: parent.width
            spacing: 6

            component Heading: Text {
                topPadding: 10
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.smallSize
                font.weight: Font.DemiBold
            }

            Heading { text: qsTr("Theme") }

            Flow {
                width: parent.width
                spacing: 6

                Repeater {
                    model: root.theme.themes

                    delegate: Rectangle {
                        id: chip
                        required property string modelData
                        readonly property bool chosen: root.theme.theme === modelData
                        width: label.implicitWidth + 24
                        height: 30
                        radius: Theme.radiusSmall + 2
                        color: chosen ? Theme.containerHighest : mouse.containsMouse ? Theme.hover : Theme.containerLow
                        border.width: 1
                        border.color: chosen ? Qt.alpha(Theme.accent, 0.7) : Theme.outlineVariant

                        Text {
                            id: label
                            anchors.centerIn: parent
                            text: chip.modelData === "material" ? qsTr("Material") : chip.modelData
                            color: chip.chosen ? Theme.text : Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.textSize - 1
                            font.weight: chip.chosen ? Font.DemiBold : Font.Normal
                        }
                        MouseArea {
                            id: mouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.theme.theme = chip.modelData
                        }
                    }
                }
            }

            Heading { text: qsTr("Mode") }

            Segmented {
                enabled: !root.theme.modeFixed
                labels: [qsTr("System"), qsTr("Light"), qsTr("Dark")]
                current: Math.max(0, root.modes.indexOf(root.theme.mode))
                onPicked: index => root.theme.mode = root.modes[index]
            }

            Heading { text: qsTr("Accent") }

            Flow {
                width: parent.width
                spacing: 3

                Repeater {
                    model: root.theme.accents

                    delegate: Item {
                        id: swatch
                        required property var modelData
                        readonly property bool chosen: root.theme.accent === modelData.id
                        width: 26
                        height: 26

                        Rectangle {
                            anchors.centerIn: parent
                            width: 26
                            height: 26
                            radius: 13
                            color: "transparent"
                            border.width: 2
                            border.color: swatch.chosen ? Theme.text : swatchMouse.containsMouse ? Theme.outline : "transparent"
                        }

                        Rectangle {
                            id: dot
                            anchors.centerIn: parent
                            width: 18
                            height: 18
                            radius: 9
                            color: swatch.modelData.color
                            border.width: 1
                            border.color: Qt.alpha(Theme.text, 0.25)
                        }

                        // Custom shows a hue ring around its colour so it reads as "pick your own".
                        Rectangle {
                            visible: swatch.modelData.id === "custom"
                            anchors.centerIn: parent
                            width: 18
                            height: 18
                            radius: 9
                            rotation: 45
                            gradient: Gradient {
                                GradientStop { position: 0; color: "#ff5f5f" }
                                GradientStop { position: 0.35; color: "#ffd54f" }
                                GradientStop { position: 0.65; color: "#4dd0c4" }
                                GradientStop { position: 1; color: "#b39ddb" }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                width: 10
                                height: 10
                                radius: 5
                                color: swatch.modelData.color
                            }
                        }

                        MouseArea {
                            id: swatchMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.theme.accent = swatch.modelData.id
                        }
                    }
                }
            }

            Text {
                text: {
                    const list = root.theme.accents;
                    for (let i = 0; i < list.length; ++i) {
                        if (list[i].id === root.theme.accent)
                            return list[i].label;
                    }
                    return "";
                }
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.smallSize
            }

            ColorPicker {
                visible: root.theme.accent === "custom"
                width: parent.width
                height: visible ? implicitHeight : 0
                color: root.theme.customAccent
                onEdited: value => root.theme.customAccent = value
            }
        }
    }
}
