// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the License, or (at your option) any later            *
 * version.                                                                     *
 *                                                                              *
 ********************************************************************************/

#include "RuleLibraryPanel.h"

#include "Catalogue.h"
#include "../../components/Section.h"

#include <QAbstractItemView>
#include <QListWidget>
#include <QMimeData>
#include <QSet>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::rule {

namespace {

// The library is organised by rule family, in the fixed order of the
// engine registry (registry.py): the rules first, the IDS facets last.
QStringList familyOrder() {
    QStringList order = {QStringLiteral("OneObjectRule"), QStringLiteral("TwoObjectRule"),
                         QStringLiteral("ComplexRule"), QStringLiteral("IDS Facet")};
    for (const auto& definition : ruleCatalogue()) {
        if (!order.contains(definition.family)) order.append(definition.family);
    }
    return order;
}

// A stock QListWidget only puts the opaque internal item-model format into
// its drag payload; the Rule tree wants plain text to read the rule name.
class DraggableRuleList : public QListWidget {
public:
    using QListWidget::QListWidget;

protected:
    QMimeData* mimeData(const QList<QListWidgetItem*>& items) const override {
        QMimeData* data = QListWidget::mimeData(items);
        if (!items.isEmpty()) data->setText(items.first()->text());
        return data;
    }
};

} // namespace

RuleLibraryPanel::RuleLibraryPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    sections_layout_ = layout;
    rebuildSections();
}

void RuleLibraryPanel::rebuildSections() {
    // Drop the previous sections; the entries are rebuilt from the current
    // catalogue (the static mirror, then the engine registry once known).
    while (sections_layout_->count() > 0) {
        QLayoutItem* item = sections_layout_->takeAt(0);
        delete item->widget();
        delete item;
    }

    const QStringList order = familyOrder();
    for (const QString& family : order) {
        auto* section = new components::Section(family, components::SectionHeaderMode::Visible, this);
        auto* list = new DraggableRuleList(section);
        list->setSelectionMode(QAbstractItemView::SingleSelection);
        list->setDragEnabled(true);
        list->setDragDropMode(QAbstractItemView::DragOnly);
        list->setDefaultDropAction(Qt::CopyAction);
        int entries = 0;
        for (const auto& definition : ruleCatalogue()) {
            if (definition.family != family) continue;
            // The drag text is the definition's label; the tree resolves
            // it back to the stable catalogue name.
            new QListWidgetItem(definition.label, list);
            ++entries;
        }
        if (entries == 0) {
            delete section;
            continue;
        }
        connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem* clicked) {
            emit ruleDefinitionSelected(clicked->text());
        });
        section->addBodyWidget(list);
        sections_layout_->addWidget(section);
    }
    sections_layout_->addStretch(1);
}

} // namespace bonsaiviewer::modules::rule
