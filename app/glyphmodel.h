#pragma once

#include <QAbstractListModel>
#include <QStringList>

struct GlyphCell {
    QString text;
    QString name;
    // What Recent remembers: the untoned emoji, so a later tone change still applies.
    QString key;
};

struct GlyphSection {
    QString title;
    QList<GlyphCell> cells;
    int category = -1;
};

// Sections laid out as header rows and rows of up to `columns` cells. Cells are also addressed by a flat index.
class GlyphModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int columns READ columns NOTIFY layoutReset)
    Q_PROPERTY(int cellCount READ cellCount NOTIFY layoutReset)

public:
    enum Role { HeaderRole = Qt::UserRole + 1, TitleRole, GlyphsRole, NamesRole, FirstCellRole, CategoryRole };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void reset(const QList<GlyphSection> &sections, int columns);
    int columns() const { return m_columns; }
    int cellCount() const { return m_cells.size(); }
    const GlyphCell &cell(int flat) const { return m_cells[flat]; }

    Q_INVOKABLE int rowOfCell(int flat) const;
    Q_INVOKABLE int rowOfCategory(int category) const;
    Q_INVOKABLE int categoryOfRow(int row) const;
    Q_INVOKABLE int firstCellOfCategory(int category) const;
    // The cell straight above or below, clamped to the shorter row; -1 at the edges.
    int verticalNeighbour(int flat, int direction) const;

signals:
    void layoutReset();

private:
    struct Row {
        bool header = false;
        QString title;
        int first = 0;
        int count = 0;
        int category = -1;
    };

    QList<Row> m_rows;
    QList<GlyphCell> m_cells;
    QList<int> m_rowOfCell;
    int m_columns = 8;
};
