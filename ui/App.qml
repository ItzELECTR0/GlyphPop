import QtQuick
import GlyphPop.Core

// C++ finds both windows by property name, so keep these names.
QtObject {
    property Window popupWindow: Window {
        flags: Qt.FramelessWindowHint
        color: "transparent"
        width: Theme.cardWidth + Theme.shadowMargin * 2
        height: Theme.cardHeight + Theme.shadowMargin * 2

        PickerCard {
            x: Theme.shadowMargin
            y: Theme.shadowMargin
            active: Picker.opened && !Picker.overlay
        }
    }

    property Window overlayWindow: Window {
        id: overlay

        flags: Qt.FramelessWindowHint
        color: "transparent"

        property bool placed: false

        function place(px, py) {
            const m = Theme.shadowMargin;
            card.x = Math.max(m, Math.min(px - 24, width - card.width - m));
            card.y = py + 20 + card.height + m <= height ? py + 20 : Math.max(m, py - 20 - card.height);
            placed = true;
        }

        Connections {
            target: Picker
            function onShown() {
                if (!Picker.overlay)
                    return;
                overlay.placed = false;
                placeFallback.restart();
            }
        }

        // The pointer's position arrives with its first hover event; without one, centre the card.
        Timer {
            id: placeFallback
            interval: 90
            onTriggered: if (!overlay.placed) overlay.place((overlay.width - card.width) / 2 + 24, (overlay.height - card.height) / 2 - 20)
        }

        HoverHandler {
            onPointChanged: if (!overlay.placed && Picker.overlay && point.position.x > 0) overlay.place(point.position.x, point.position.y)
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: Picker.close()
        }

        PickerCard {
            id: card
            opacity: overlay.placed ? 1 : 0
            active: Picker.opened && Picker.overlay
        }
    }
}
