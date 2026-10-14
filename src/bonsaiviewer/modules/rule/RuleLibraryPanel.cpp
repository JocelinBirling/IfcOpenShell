// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or           *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                     *
 * but WITHOUT ANY WARRANTY; without the implied warranty of                     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 ********************************************************************************/

#include "RuleLibraryPanel.h"

#include "../../components/Section.h"

#include <QAbstractItemView>
#include <QListWidget>
#include <QMimeData>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::rule {

namespace {

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

    // The library is organised by rule family. Each Section collapses with
    // its header arrow; the entries are drag sources for the Rule panel tree.
    const auto make_entries = [this](components::Section* section, std::initializer_list<QString> names) {
        auto* list = new DraggableRuleList(section);
        list->setSelectionMode(QAbstractItemView::SingleSelection);
        list->setDragEnabled(true);
        list->setDragDropMode(QAbstractItemView::DragOnly);
        list->setDefaultDropAction(Qt::CopyAction);
        for (const QString& name : names) new QListWidgetItem(name, list);
        connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem* clicked) {
            emit ruleDefinitionSelected(clicked->text());
        });
        section->addBodyWidget(list);
    };

    auto* one_object = new components::Section("OneObjectRule", components::SectionHeaderMode::Visible, this);
    make_entries(one_object, {"Name Contains", "Minimum Volume", "Maximum Height"});

    auto* two_object = new components::Section("TwoObjectRule", components::SectionHeaderMode::Visible, this);
    make_entries(two_object, {"Minimum Distance", "Overlap Check"});

    auto* complex = new components::Section("ComplexRule", components::SectionHeaderMode::Visible, this);
    make_entries(complex, {"Rule Chain", "Group Filter"});

    auto* ids_facet = new components::Section("IDS Facet", components::SectionHeaderMode::Visible, this);
    make_entries(ids_facet, {"Entity Facet", "Attribute Facet", "Property Facet", "Classification Facet", "Material Facet"});

    layout->addWidget(one_object);
    layout->addWidget(two_object);
    layout->addWidget(complex);
    layout->addWidget(ids_facet);
    layout->addStretch(1);
}

} // namespace bonsaiviewer::modules::rule
