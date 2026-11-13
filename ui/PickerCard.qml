import QtQuick
import QtQuick.Effects
import GlyphPop.Core

Item {
    id: root

    property bool active: false

    width: Theme.cardWidth
    height: Theme.cardHeight

    function focusCurrent() {
        if (Picker.options)
            options.forceActiveFocus();
        else
            search.focusField();
    }

    function reset() {
        focusCurrent();
        grid.positionViewAtBeginning();
        toneMenu.open = false;
        appear.restart();
    }

    onActiveChanged: if (active) reset()

    Connections {
        target: Picker
        function onShown() {
            if (root.active)
                root.reset();
        }
        function onOptionsChanged() {
            if (root.active)
                root.focusCurrent();
        }
    }

    RectangularShadow {
        anchors.fill: body
        radius: Theme.radius
        blur: 22
        offset.y: 6
        color: Qt.alpha(Theme.shadow, Picker.theme.dark ? 0.5 : 0.16)
        opacity: body.opacity
        scale: body.scale
    }

    Item {
        id: body

        anchors.fill: parent
        transformOrigin: Item.Top

        ParallelAnimation {
            id: appear
            NumberAnimation { target: body; property: "opacity"; from: 0; to: 1; duration: 120; easing.type: Easing.OutCubic }
            NumberAnimation { target: body; property: "scale"; from: 0.97; to: 1; duration: 160; easing.type: Easing.OutCubic }
        }

        Rectangle {
            anchors.fill: parent
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.outlineVariant
        }

        // Keeps clicks on the card from reaching the overlay behind it, which closes on a click.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: toneMenu.open = false
        }

        Item {
            id: pickerView

            anchors.fill: parent
            visible: !Picker.options

            SectionSwitch {
                id: sections
                x: Theme.padding
                y: Theme.padding
            }

            IconButton {
                id: optionsButton
                anchors.right: parent.right
                anchors.rightMargin: Theme.padding
                anchors.verticalCenter: sections.verticalCenter
                onClicked: Picker.options = true
                TuneIcon {}
            }

            ToneButton {
                id: toneButton
                anchors.right: optionsButton.left
                anchors.rightMargin: 2
                anchors.verticalCenter: sections.verticalCenter
                visible: Picker.section === 0
                onClicked: toneMenu.open = !toneMenu.open
            }

            SearchField {
                id: search
                anchors.top: sections.bottom
                anchors.topMargin: 8
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Theme.padding
                anchors.rightMargin: Theme.padding
                onPageJump: direction => grid.jumpCategory(direction)
            }

            GlyphGrid {
                id: grid
                anchors.top: search.bottom
                anchors.topMargin: 6
                anchors.bottom: preview.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Theme.padding
                anchors.rightMargin: Theme.padding - 6
            }

            Item {
                id: preview

                anchors.bottom: divider.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Theme.padding + 4
                anchors.rightMargin: Theme.padding + 4
                height: 28

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    text: Picker.model.cellCount === 0 ? (Picker.query.length ? qsTr("Nothing found") : "") : Picker.selectedName
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.smallSize
                    elide: Text.ElideRight
                }
            }

            Rectangle {
                id: divider
                anchors.bottom: strip.top
                width: parent.width
                height: 1
                color: Theme.outlineVariant
            }

            CategoryStrip {
                id: strip
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 4
                current: grid.currentCategory
                onPicked: id => grid.showCategory(id)
            }

            ToneMenu {
                id: toneMenu
                anchors.top: toneButton.bottom
                anchors.topMargin: 6
                anchors.right: parent.right
                anchors.rightMargin: Theme.padding - 2
                z: 10
            }
        }

        OptionsPage {
            id: options
            anchors.fill: parent
            anchors.margins: Theme.padding
            visible: Picker.options
        }
    }
}
