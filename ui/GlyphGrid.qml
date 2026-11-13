import QtQuick
import GlyphPop.Core

ListView {
    id: list

    readonly property bool wide: Picker.model.columns < 6
    readonly property real cellWidth: (width - 8) / Picker.model.columns
    readonly property int cellHeight: wide ? 36 : Theme.cell
    readonly property int currentCategory: {
        const row = indexAt(8, contentY + 4);
        return row < 0 ? -1 : Picker.model.categoryOfRow(row);
    }

    function showCategory(id) {
        const row = Picker.model.rowOfCategory(id);
        if (row >= 0)
            positionViewAtIndex(row, ListView.Beginning);
    }

    function jumpCategory(direction) {
        const ids = Picker.categories.map(c => c.id);
        const at = ids.indexOf(currentCategory);
        const next = ids[Math.max(0, Math.min(ids.length - 1, at + direction))];
        if (next !== undefined) {
            showCategory(next);
            Picker.selected = Picker.model.firstCellOfCategory(next);
        }
    }

    clip: true
    model: Picker.model
    boundsBehavior: Flickable.StopAtBounds
    reuseItems: true
    cacheBuffer: 320
    flickDeceleration: 4000
    maximumFlickVelocity: 3200

    Connections {
        target: Picker
        function onSelectedChanged() {
            const row = Picker.model.rowOfCell(Picker.selected);
            if (row >= 0 && !mouseMoved.active)
                list.positionViewAtIndex(row, ListView.Contain);
        }
        function onSectionChanged() { list.positionViewAtBeginning(); }
        function onQueryChanged() { list.positionViewAtBeginning(); }
    }

    // Hovering also moves the selection; this keeps that from scrolling the view under the pointer.
    Timer { id: mouseMoved; property bool active: running; interval: 60 }

    delegate: Item {
        id: row

        required property bool header
        required property string title
        required property var glyphs
        required property var names
        required property int firstCell

        width: list.width - 8
        height: header ? 30 : list.cellHeight

        Text {
            visible: row.header
            x: 4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 6
            text: row.title
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.smallSize
            font.weight: Font.DemiBold
        }

        Row {
            visible: !row.header

            Repeater {
                model: row.header ? [] : row.glyphs

                delegate: Item {
                    id: cell

                    required property string modelData
                    required property int index
                    readonly property int flat: row.firstCell + index
                    readonly property bool selected: Picker.selected === flat

                    width: list.cellWidth
                    height: list.cellHeight

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        radius: Theme.radiusSmall
                        color: mouse.pressed ? Theme.pressed : cell.selected ? Theme.selection : "transparent"
                        border.width: cell.selected ? 1 : 0
                        border.color: Qt.alpha(Theme.accent, 0.55)
                    }

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 6
                        horizontalAlignment: Text.AlignHCenter
                        text: cell.modelData
                        color: Theme.text
                        font.family: Picker.section === 0 ? Theme.emojiFamily : Theme.fontFamily
                        font.pixelSize: list.wide ? Theme.textSize : Picker.section === 0 ? Theme.glyphSize : 20
                        elide: Text.ElideRight
                        scale: mouse.pressed ? 0.9 : 1
                        Behavior on scale { NumberAnimation { duration: 80 } }
                    }

                    MouseArea {
                        id: mouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: {
                            mouseMoved.restart();
                            Picker.selected = cell.flat;
                        }
                        onClicked: event => Picker.insert(cell.flat, (event.modifiers & Qt.ShiftModifier) !== 0)
                    }
                }
            }
        }
    }

    Rectangle {
        parent: list
        anchors.right: parent.right
        anchors.rightMargin: 1
        y: list.visibleArea.yPosition * list.height
        width: 4
        height: Math.max(24, list.visibleArea.heightRatio * list.height)
        radius: 2
        color: Theme.outline
        opacity: list.moving || scrollHover.hovered ? 0.8 : list.visibleArea.heightRatio < 1 ? 0.35 : 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }

    HoverHandler { id: scrollHover }
}
