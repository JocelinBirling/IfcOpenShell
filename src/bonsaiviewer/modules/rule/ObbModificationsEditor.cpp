// This file was generated with the assistance of an AI coding tool.
/********************************************************************************
 *                                                                              *
 * This file is part of Bonsai.                                                 *
 *                                                                              *
 * Bonsai is free software: you can redistribute it and/or modify               *
 * it under the terms of the GNU General Public License as published by         *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * Bonsai is distributed in the hope that it will be useful,                    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * GNU General Public License for more details.                                 *
 *                                                                              *
 * You should have received a copy of the GNU General Public License            *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "ObbModificationsEditor.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <tuple>

namespace bonsaiviewer::modules::rule {

namespace {

// A value typed with a decimal point or a comma (the editor writes 0.5 in the JSON).
class DecimalSpinBox : public QDoubleSpinBox {
public:
    explicit DecimalSpinBox(QWidget* parent) : QDoubleSpinBox(parent) {
        setLocale(QLocale::c());
        setRange(-1e6, 1e6);
        setDecimals(4);
        setKeyboardTracking(false);
    }

protected:
    QValidator::State validate(QString& text, int& pos) const override {
        text.replace(',', '.');
        return QDoubleSpinBox::validate(text, pos);
    }
    double valueFromText(const QString& text) const override {
        QString t = text;
        return QDoubleSpinBox::valueFromText(t.replace(',', '.'));
    }
    // 0.5, not 0.5000.
    QString textFromValue(double value) const override { return QString::number(value, 'g', 10); }
};

const QString kBadFrame = QStringLiteral("QFrame#obbRow { border: 1px solid #E69500; border-radius: 3px; }");
const QString kRawFrame = QStringLiteral("QFrame#obbRow { border: 1px solid #E6194B; border-radius: 3px; }");

} // namespace

ObbModificationsEditor::ObbModificationsEditor(const QStringList& operations, const QJsonArray& entries, QWidget* parent)
    : QWidget(parent) {
    for (const QString& op : operations) {
        if (op != QLatin1String("NEW_OBB")) operations_ << op;
    }
    rows_ = obb::rowsFromJson(entries);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    auto* list_host = new QWidget(this);
    list_ = new QVBoxLayout(list_host);
    list_->setContentsMargins(0, 0, 0, 0);
    list_->setSpacing(2);
    layout->addWidget(list_host);
    auto* buttons = new QHBoxLayout();
    auto* add_step = new QPushButton("+ Step", this);
    add_step->setToolTip("Add a step at the end of the last zone (extend_up:0 to set).");
    auto* add_zone = new QPushButton("+ New zone", this);
    add_zone->setToolTip("Close the zone and start a new one from the OBB of the source (NEW_OBB), with a first step.");
    buttons->addWidget(add_step);
    buttons->addWidget(add_zone);
    buttons->addStretch(1);
    layout->addLayout(buttons);
    // Under the list: the zones obtained, or what the engine would refuse.
    message_ = new QLabel(this);
    message_->setWordWrap(true);
    layout->addWidget(message_);
    connect(add_step, &QPushButton::clicked, this, [this]() {
        rows_.push_back(obb::Row{});
        rebuild();
        emitChanged();
    });
    connect(add_zone, &QPushButton::clicked, this, [this]() {
        obb::Row zone;
        zone.kind = obb::Row::Kind::NewZone;
        rows_.push_back(zone);
        rows_.push_back(obb::Row{});
        rebuild();
        emitChanged();
    });
    rebuild();
}

void ObbModificationsEditor::emitChanged() {
    emit changed(obb::rowsToJson(rows_));
}

void ObbModificationsEditor::moveRow(int index, int delta) {
    const int to = index + delta;
    if (to < 0 || to >= static_cast<int>(rows_.size())) return;
    std::swap(rows_[static_cast<size_t>(index)], rows_[static_cast<size_t>(to)]);
    rebuild();
    emitChanged();
}

void ObbModificationsEditor::removeRow(int index) {
    if (index < 0 || index >= static_cast<int>(rows_.size())) return;
    rows_.erase(rows_.begin() + index);
    rebuild();
    emitChanged();
}

QWidget* ObbModificationsEditor::rowButtons(int index, QWidget* parent) {
    auto* box = new QWidget(parent);
    auto* layout = new QHBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);
    for (const auto& [text, tip, action] : std::vector<std::tuple<QString, QString, int>>{
             {QStringLiteral("↑"), QStringLiteral("Move up"), -1},
             {QStringLiteral("↓"), QStringLiteral("Move down"), 1},
             {QStringLiteral("✕"), QStringLiteral("Remove"), 0}}) {
        auto* button = new QToolButton(box);
        button->setText(text);
        button->setToolTip(tip);
        button->setAutoRaise(true);
        // Deferred: the row (and this button) is rebuilt.
        connect(button, &QToolButton::clicked, this, [this, index, action = action]() {
            QTimer::singleShot(0, this, [this, index, action]() {
                if (action == 0) removeRow(index);
                else moveRow(index, action);
            });
        });
        layout->addWidget(button);
    }
    return box;
}

QWidget* ObbModificationsEditor::stepRow(int index) {
    obb::Row& row = rows_[static_cast<size_t>(index)];
    auto* frame = new QFrame(this);
    frame->setObjectName("obbRow");
    auto* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(4, 2, 2, 2);
    auto* op = new QComboBox(frame);
    op->addItems(operations_);
    if (!operations_.contains(row.op)) op->addItem(row.op);
    op->setCurrentText(row.op);
    auto* value = new DecimalSpinBox(frame);
    value->setValue(row.value);
    auto* unit = new QComboBox(frame);
    unit->addItems({"m", "%"});
    unit->setCurrentIndex(row.percent ? 1 : 0);
    unit->setToolTip("m: metres; %: a percentage of the zone (its height, or its widths for expand_sides)");
    auto* text = new QLabel(obb::describeStep(row.op, row.value, row.percent), frame);
    text->setWordWrap(true);
    text->setProperty("textRole", "secondary");
    layout->addWidget(op);
    layout->addWidget(value);
    layout->addWidget(unit);
    layout->addWidget(text, 1);
    layout->addWidget(rowButtons(index, frame));
    // A value change keeps the row (no rebuild): the description and the list follow.
    const auto update = [this, index, op, value, unit, text]() {
        obb::Row& r = rows_[static_cast<size_t>(index)];
        r.op = op->currentText();
        r.value = value->value();
        r.percent = unit->currentIndex() == 1;
        text->setText(obb::describeStep(r.op, r.value, r.percent));
        emitChanged();
    };
    connect(op, &QComboBox::currentTextChanged, this, update);
    connect(value, &QDoubleSpinBox::valueChanged, this, update);
    connect(unit, &QComboBox::currentIndexChanged, this, update);
    return frame;
}

QWidget* ObbModificationsEditor::zoneRow(int index, int zone) {
    auto* frame = new QFrame(this);
    frame->setObjectName("obbRow");
    auto* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(4, 2, 2, 2);
    auto* label = new QLabel(QString("<b>New zone — Zone %1</b> (NEW_OBB: starts again from the OBB of the source)").arg(zone), frame);
    label->setWordWrap(true);
    layout->addWidget(label, 1);
    layout->addWidget(rowButtons(index, frame));
    return frame;
}

QWidget* ObbModificationsEditor::rawRow(int index) {
    const obb::Row& row = rows_[static_cast<size_t>(index)];
    auto* frame = new QFrame(this);
    frame->setObjectName("obbRow");
    frame->setStyleSheet(kRawFrame);
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(4, 4, 2, 4);
    layout->setSpacing(2);
    auto* line_row = new QHBoxLayout();
    auto* edit = new QLineEdit(row.raw, frame);
    edit->setMinimumHeight(edit->sizeHint().height());
    edit->setToolTip("An entry the engine cannot read: correct it (it becomes a step) or remove it.");
    line_row->addWidget(edit, 1);
    line_row->addWidget(rowButtons(index, frame));
    layout->addLayout(line_row);
    // The cause, short (the expected names are in the tooltip and under the list).
    QString cause = row.error;
    const int expected = cause.indexOf(" (expected");
    if (expected > 0) cause = cause.left(expected);
    auto* error = new QLabel(cause, frame);
    error->setToolTip(row.error);
    error->setStyleSheet("color: #E6194B;");
    layout->addWidget(error);
    frame->setMinimumHeight(frame->sizeHint().height());
    connect(edit, &QLineEdit::editingFinished, this, [this, index, edit]() {
        if (edit->text() == rows_[static_cast<size_t>(index)].raw) return;
        // Read again: a corrected entry becomes a step (or a separator).
        const auto read = obb::rowsFromJson(QJsonArray{edit->text()});
        rows_[static_cast<size_t>(index)] = read.front();
        if (read.front().kind == obb::Row::Kind::Raw) rows_[static_cast<size_t>(index)].raw = edit->text();
        QTimer::singleShot(0, this, [this]() {
            rebuild();
            emitChanged();
        });
    });
    return frame;
}

void ObbModificationsEditor::rebuild() {
    while (QLayoutItem* item = list_->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
    const obb::Check check = obb::check(rows_);
    int zone = 1;
    auto* first = new QLabel(QStringLiteral("<b>Zone 1</b>"), this);
    list_->addWidget(first);
    if (rows_.empty()) {
        auto* empty = new QLabel("No step: the zone is the OBB of the source.", this);
        empty->setProperty("textRole", "secondary");
        list_->addWidget(empty);
    }
    for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
        const obb::Row& row = rows_[static_cast<size_t>(i)];
        QWidget* widget = nullptr;
        switch (row.kind) {
        case obb::Row::Kind::Step: widget = stepRow(i); break;
        case obb::Row::Kind::NewZone: widget = zoneRow(i, ++zone); break;
        case obb::Row::Kind::Raw: widget = rawRow(i); break;
        }
        if (row.kind != obb::Row::Kind::Raw &&
            std::find(check.bad_rows.begin(), check.bad_rows.end(), i) != check.bad_rows.end()) {
            widget->setStyleSheet(kBadFrame);
        }
        list_->addWidget(widget);
    }
    if (check.message.isEmpty()) {
        message_->setStyleSheet(QString());
        message_->setText(QString("%1 zone%2").arg(check.zones).arg(check.zones == 1 ? "" : "s"));
    } else {
        message_->setStyleSheet("color: #E69500;");
        message_->setText("The engine would refuse the list: " + check.message);
    }
}

} // namespace bonsaiviewer::modules::rule
