import QtQuick
import GlyphPop.Core

Rectangle {
    id: root

    signal pageJump(int direction)

    function focusField() {
        input.forceActiveFocus();
    }

    height: 34
    radius: Theme.radiusSmall + 2
    color: Theme.containerLow
    border.width: 1
    border.color: input.activeFocus ? Theme.accent : Theme.outlineVariant

    Behavior on border.color { ColorAnimation { duration: 120 } }

    // A drawn magnifier, so the field does not depend on an icon theme.
    Item {
        id: glass
        x: 10
        anchors.verticalCenter: parent.verticalCenter
        width: 14
        height: 14

        Rectangle {
            width: 10
            height: 10
            radius: 5
            color: "transparent"
            border.width: 1.6
            border.color: Theme.textMuted
        }
        Rectangle {
            x: 8.2
            y: 9.4
            width: 5.4
            height: 1.8
            radius: 0.9
            rotation: 45
            transformOrigin: Item.Left
            color: Theme.textMuted
        }
    }

    TextInput {
        id: input

        anchors.left: glass.right
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.text
        selectionColor: Theme.selection
        selectedTextColor: Theme.text
        font.family: Theme.fontFamily
        font.pixelSize: Theme.textSize
        clip: true
        selectByMouse: true
        cursorDelegate: Rectangle {
            width: 1.5
            color: Theme.accent
            visible: input.cursorVisible
        }

        onTextEdited: Picker.query = text

        Connections {
            target: Picker
            function onQueryChanged() {
                if (input.text !== Picker.query)
                    input.text = Picker.query;
            }
        }

        Keys.onPressed: event => {
            const keep = (event.modifiers & Qt.ShiftModifier) !== 0;
            switch (event.key) {
            case Qt.Key_Escape:
                Picker.close();
                break;
            case Qt.Key_Return:
            case Qt.Key_Enter:
                Picker.insert(Picker.selected, keep);
                break;
            case Qt.Key_Up:
                Picker.move(0, -1);
                break;
            case Qt.Key_Down:
                Picker.move(0, 1);
                break;
            case Qt.Key_Left:
                Picker.move(-1, 0);
                break;
            case Qt.Key_Right:
                Picker.move(1, 0);
                break;
            case Qt.Key_Tab:
                Picker.section = (Picker.section + 1) % 3;
                break;
            case Qt.Key_Backtab:
                Picker.section = (Picker.section + 2) % 3;
                break;
            case Qt.Key_PageUp:
                root.pageJump(-1);
                break;
            case Qt.Key_PageDown:
                root.pageJump(1);
                break;
            default:
                return;
            }
            event.accepted = true;
        }

        Text {
            anchors.fill: parent
            verticalAlignment: Text.AlignVCenter
            visible: !input.text.length
            text: [qsTr("Search emoji"), qsTr("Search kaomoji"), qsTr("Search symbols")][Picker.section]
            color: Theme.textMuted
            font: input.font
            elide: Text.ElideRight
        }
    }
}
