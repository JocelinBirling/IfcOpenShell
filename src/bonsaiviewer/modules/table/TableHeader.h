// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                           *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCINTERFACE_MODULES_TABLE_TABLEHEADER_H
#define IFCINTERFACE_MODULES_TABLE_TABLEHEADER_H

#include <QHeaderView>
#include <QSet>

namespace bonsaiviewer::modules::table {

// BonsaiViewer07 phase 3 — the column header of the Table panel. Each
// section shows, right of its title, a sort arrow (down: A to Z, up: Z to
// A; grey while the column is not sorted) and a funnel (accent colour while
// the column is filtered). Clicking them emits the requests; the rest of
// the section keeps the header's behaviour (right click menu, drag to move).
class TableHeader : public QHeaderView {
    Q_OBJECT

public:
    explicit TableHeader(QWidget* parent = nullptr);

    // -1 = no sorted column.
    void setSortState(int section, Qt::SortOrder order);
    void setFiltered(int section, bool filtered);
    void clearStates();

signals:
    void sortRequested(int section);
    void filterRequested(int section, const QPoint& global_pos);

protected:
    void paintSection(QPainter* painter, const QRect& rect, int logical_index) const override;
    void mousePressEvent(QMouseEvent* event) override;
    QSize sectionSizeFromContents(int logical_index) const override;

private:
    QRect sortRect(const QRect& section) const;
    QRect filterRect(const QRect& section) const;
    QRect sectionRect(int logical_index) const;

    int sort_section_ = -1;
    Qt::SortOrder sort_order_ = Qt::AscendingOrder;
    QSet<int> filtered_;
};

} // namespace bonsaiviewer::modules::table

#endif // IFCINTERFACE_MODULES_TABLE_TABLEHEADER_H
