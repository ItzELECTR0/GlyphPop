#include "glyphmodel.h"

int GlyphModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant GlyphModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &row = m_rows[index.row()];
    switch (role) {
    case HeaderRole:
        return row.header;
    case TitleRole:
        return row.title;
    case FirstCellRole:
        return row.first;
    case CategoryRole:
        return row.category;
    case GlyphsRole:
    case NamesRole: {
        QStringList out;
        for (int i = row.first; i < row.first + row.count; ++i)
            out.append(role == GlyphsRole ? m_cells[i].text : m_cells[i].name);
        return out;
    }
    }
    return {};
}

QHash<int, QByteArray> GlyphModel::roleNames() const
{
    return {{HeaderRole, "header"},       {TitleRole, "title"},       {GlyphsRole, "glyphs"},
            {NamesRole, "names"},         {FirstCellRole, "firstCell"}, {CategoryRole, "category"}};
}

void GlyphModel::reset(const QList<GlyphSection> &sections, int columns)
{
    beginResetModel();
    m_rows.clear();
    m_cells.clear();
    m_rowOfCell.clear();
    m_columns = columns;
    for (const GlyphSection &section : sections) {
        if (section.cells.isEmpty())
            continue;
        if (!section.title.isEmpty())
            m_rows.append({true, section.title, int(m_cells.size()), 0, section.category});
        for (int i = 0; i < section.cells.size(); i += columns) {
            const int count = qMin(columns, int(section.cells.size()) - i);
            m_rows.append({false, {}, int(m_cells.size()), count, section.category});
            for (int j = 0; j < count; ++j) {
                m_cells.append(section.cells[i + j]);
                m_rowOfCell.append(m_rows.size() - 1);
            }
        }
    }
    endResetModel();
    emit layoutReset();
}

int GlyphModel::rowOfCell(int flat) const
{
    return flat >= 0 && flat < m_rowOfCell.size() ? m_rowOfCell[flat] : -1;
}

int GlyphModel::rowOfCategory(int category) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].category == category)
            return i;
    }
    return -1;
}

int GlyphModel::categoryOfRow(int row) const
{
    return row >= 0 && row < m_rows.size() ? m_rows[row].category : -1;
}

int GlyphModel::firstCellOfCategory(int category) const
{
    const int row = rowOfCategory(category);
    return row < 0 ? 0 : m_rows[row].first;
}

int GlyphModel::verticalNeighbour(int flat, int direction) const
{
    const int rowIndex = rowOfCell(flat);
    if (rowIndex < 0)
        return -1;
    const int column = flat - m_rows[rowIndex].first;
    for (int r = rowIndex + direction; r >= 0 && r < m_rows.size(); r += direction) {
        const Row &row = m_rows[r];
        if (!row.header)
            return row.first + qMin(column, row.count - 1);
    }
    return -1;
}
