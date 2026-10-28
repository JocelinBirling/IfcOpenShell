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

#include "ValueEditor.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStringListModel>
#include <QToolButton>
#include <QVBoxLayout>

#include <optional>

namespace bonsaiviewer::modules::filters {

using bonsaiviewer::services::IdsValue;

namespace {

constexpr int kValueRole = Qt::UserRole;

enum PatternPreset {
    kStartsWith = 0,
    kContains,
    kEndsWith,
    kFreeExpression,
};

struct MethodEntry {
    IdsValue::Kind kind;
    int flag;
    const char* label;
    const char* tooltip;
};

// Button order = page order.
const MethodEntry kMethods[] = {
    {IdsValue::Kind::Simple, ValueEditor::AllowSimple, "Value", "One exact value (empty = no constraint)"},
    {IdsValue::Kind::Enumeration, ValueEditor::AllowEnumeration, "List", "One of several values"},
    {IdsValue::Kind::Pattern, ValueEditor::AllowPattern, "Pattern", "Text matching a pattern"},
    {IdsValue::Kind::Range, ValueEditor::AllowRange, "Range", "A number between bounds"},
    {IdsValue::Kind::Length, ValueEditor::AllowLength, "Length", "A number of characters"},
};

int methodFlag(IdsValue::Kind kind) {
    for (const MethodEntry& method : kMethods) {
        if (method.kind == kind) return method.flag;
    }
    return 0;
}

int pageIndex(IdsValue::Kind kind) {
    for (int i = 0; i < static_cast<int>(std::size(kMethods)); ++i) {
        if (kMethods[i].kind == kind) return i;
    }
    return 0;
}

// XSD regular expression metacharacters (no anchors in XSD: "^" and "$"
// are plain characters outside character classes).
const QString& metacharacters() {
    static const QString chars = QStringLiteral(".\\?*+{}()[]|");
    return chars;
}

QString escapeXsd(const QString& text) {
    QString escaped;
    escaped.reserve(text.size() * 2);
    for (const QChar c : text) {
        if (metacharacters().contains(c)) escaped += '\\';
        escaped += c;
    }
    return escaped;
}

// The literal text of an escaped body, or nothing when the body holds a
// real regular expression construct.
std::optional<QString> unescapeXsd(const QString& body) {
    QString text;
    for (int i = 0; i < body.size(); ++i) {
        const QChar c = body[i];
        if (c == '\\') {
            if (i + 1 >= body.size() || !metacharacters().contains(body[i + 1])) return std::nullopt;
            text += body[++i];
        } else if (metacharacters().contains(c)) {
            return std::nullopt;
        } else {
            text += c;
        }
    }
    return text;
}

QStringList splitList(const QString& text) {
    static const QRegularExpression separators(QStringLiteral("[\\r\\n;,\\t]+"));
    QStringList values;
    for (const QString& part : text.split(separators, Qt::SkipEmptyParts)) {
        const QString value = part.trimmed();
        if (!value.isEmpty() && !values.contains(value)) values << value;
    }
    return values;
}

bool isNumber(const QString& text) {
    bool ok = false;
    QString(text).replace(',', '.').toDouble(&ok);
    return ok;
}

QWidget* makePage(QStackedWidget* pages) {
    auto* page = new QWidget(pages);
    pages->addWidget(page);
    return page;
}

} // namespace

ValueEditor::ValueEditor(int allowed_methods, QWidget* parent)
    : QWidget(parent),
      allowed_methods_(allowed_methods)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    // Method buttons: a compact segmented row, hidden when only one method
    // makes sense for the field.
    auto* header = new QHBoxLayout();
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(0);
    method_buttons_ = new QButtonGroup(this);
    method_buttons_->setExclusive(true);
    int shown = 0;
    for (const MethodEntry& method : kMethods) {
        auto* button = new QToolButton(this);
        button->setText(method.label);
        button->setToolTip(method.tooltip);
        button->setCheckable(true);
        button->setAutoRaise(false);
        button->setVisible(allowed_methods & method.flag);
        shown += (allowed_methods & method.flag) ? 1 : 0;
        method_buttons_->addButton(button, static_cast<int>(method.kind));
        header->addWidget(button);
    }
    if (shown < 2) {
        for (auto* button : method_buttons_->buttons()) button->hide();
    }
    header->addStretch(1);
    reload_button_ = new QToolButton(this);
    reload_button_->setText("Load from model");
    reload_button_->setToolTip("Read the values present in the active model");
    reload_button_->setAutoRaise(true);
    reload_button_->setVisible(false);
    header->addWidget(reload_button_);
    layout->addLayout(header);

    completion_model_ = new QStringListModel(this);
    pages_ = new QStackedWidget(this);
    buildSimplePage();
    buildEnumerationPage();
    buildPatternPage();
    buildRangePage();
    buildLengthPage();
    layout->addWidget(pages_);

    connect(method_buttons_, &QButtonGroup::idClicked, this, [this](int id) {
        switchTo(static_cast<Kind>(id));
    });
    connect(reload_button_, &QToolButton::clicked, this, &ValueEditor::reloadRequested);

    // Default: the first allowed method (Value for every field).
    for (const MethodEntry& method : kMethods) {
        if (allowed_methods & method.flag) {
            showPage(method.kind);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Pages
// ---------------------------------------------------------------------------

void ValueEditor::buildSimplePage() {
    auto* layout = new QHBoxLayout(makePage(pages_));
    layout->setContentsMargins(0, 0, 0, 0);
    simple_combo_ = new QComboBox(pages_);
    simple_combo_->setEditable(true);
    simple_combo_->setInsertPolicy(QComboBox::NoInsert);
    simple_combo_->setMaxVisibleItems(15);
    simple_combo_->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    simple_combo_->completer()->setFilterMode(Qt::MatchContains);
    simple_combo_->completer()->setCompletionMode(QCompleter::PopupCompletion);
    simple_combo_->lineEdit()->setPlaceholderText("any value");
    layout->addWidget(simple_combo_, 1);
    connect(simple_combo_, &QComboBox::currentTextChanged, this, [this](const QString&) { emitChanged(); });
}

void ValueEditor::buildEnumerationPage() {
    auto* page = makePage(pages_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    enumeration_list_ = new QListWidget(page);
    enumeration_list_->setMinimumHeight(90);
    enumeration_list_->setToolTip("Checked values are accepted");
    layout->addWidget(enumeration_list_);

    auto* add_row = new QHBoxLayout();
    enumeration_add_edit_ = new QLineEdit(page);
    enumeration_add_edit_->setPlaceholderText("Add a value… (Enter; several separated by ';')");
    auto* completer = new QCompleter(completion_model_, enumeration_add_edit_);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    enumeration_add_edit_->setCompleter(completer);
    auto* add_button = new QPushButton("Add", page);
    auto* paste_button = new QPushButton("Paste list", page);
    paste_button->setToolTip("Add the values of the clipboard: one per line, or separated by ';' or ','");
    auto* all_button = new QToolButton(page);
    all_button->setText("All");
    all_button->setAutoRaise(true);
    auto* none_button = new QToolButton(page);
    none_button->setText("None");
    none_button->setAutoRaise(true);
    add_row->addWidget(enumeration_add_edit_, 1);
    add_row->addWidget(add_button);
    add_row->addWidget(paste_button);
    add_row->addWidget(all_button);
    add_row->addWidget(none_button);
    layout->addLayout(add_row);

    enumeration_info_ = new QLabel(page);
    enumeration_info_->setProperty("textRole", "secondary");
    enumeration_info_->setWordWrap(true);
    layout->addWidget(enumeration_info_);

    const auto add_typed = [this]() {
        for (const QString& value : splitList(enumeration_add_edit_->text())) {
            addEnumerationValue(value, true);
        }
        enumeration_add_edit_->clear();
        emitChanged();
    };
    connect(enumeration_add_edit_, &QLineEdit::returnPressed, this, add_typed);
    connect(add_button, &QPushButton::clicked, this, add_typed);
    // Picking a completion adds it right away (queued: the completer writes
    // the text into the edit after this signal).
    connect(completer, qOverload<const QString&>(&QCompleter::activated), this, [this](const QString& value) {
        QMetaObject::invokeMethod(this, [this, value]() {
            addEnumerationValue(value, true);
            enumeration_add_edit_->clear();
            emitChanged();
        }, Qt::QueuedConnection);
    });
    connect(paste_button, &QPushButton::clicked, this, [this]() {
        const QStringList values = splitList(QApplication::clipboard()->text());
        for (const QString& value : values) addEnumerationValue(value, true);
        enumeration_info_->setText(QString("%1 value(s) pasted").arg(values.size()));
        emitChanged();
    });
    const auto check_all = [this](bool checked) {
        {
            const QSignalBlocker blocker(enumeration_list_);
            for (int i = 0; i < enumeration_list_->count(); ++i) {
                enumeration_list_->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
            }
        }
        emitChanged();
    };
    connect(all_button, &QToolButton::clicked, this, [check_all]() { check_all(true); });
    connect(none_button, &QToolButton::clicked, this, [check_all]() { check_all(false); });
    connect(enumeration_list_, &QListWidget::itemChanged, this, [this](QListWidgetItem*) { emitChanged(); });
}

void ValueEditor::buildPatternPage() {
    auto* page = makePage(pages_);
    auto* layout = new QFormLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* row = new QHBoxLayout();
    pattern_preset_ = new QComboBox(page);
    pattern_preset_->addItems({"starts with", "contains", "ends with", "regular expression"});
    pattern_text_ = new QLineEdit(page);
    row->addWidget(pattern_preset_);
    row->addWidget(pattern_text_, 1);
    layout->addRow("Text", row);

    pattern_preview_ = new QLabel(page);
    pattern_preview_->setProperty("textRole", "secondary");
    pattern_preview_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addRow("XSD pattern", pattern_preview_);

    auto* test_row = new QHBoxLayout();
    pattern_test_ = new QLineEdit(page);
    pattern_test_->setPlaceholderText("Type a value to test the pattern");
    pattern_test_result_ = new QLabel(page);
    test_row->addWidget(pattern_test_, 1);
    test_row->addWidget(pattern_test_result_);
    layout->addRow("Test", test_row);

    connect(pattern_preset_, &QComboBox::currentIndexChanged, this, [this](int) {
        updatePatternPreview();
        emitChanged();
    });
    connect(pattern_text_, &QLineEdit::textChanged, this, [this](const QString&) {
        updatePatternPreview();
        emitChanged();
    });
    connect(pattern_test_, &QLineEdit::textChanged, this, [this](const QString&) { updatePatternPreview(); });
}

void ValueEditor::buildRangePage() {
    auto* page = makePage(pages_);
    auto* layout = new QFormLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* min_row = new QHBoxLayout();
    range_min_ = new QLineEdit(page);
    range_min_->setPlaceholderText("no minimum");
    range_min_inclusive_ = new QCheckBox("included", page);
    range_min_inclusive_->setChecked(true);
    min_row->addWidget(range_min_, 1);
    min_row->addWidget(range_min_inclusive_);
    layout->addRow("Minimum", min_row);

    auto* max_row = new QHBoxLayout();
    range_max_ = new QLineEdit(page);
    range_max_->setPlaceholderText("no maximum");
    range_max_inclusive_ = new QCheckBox("included", page);
    range_max_inclusive_->setChecked(true);
    max_row->addWidget(range_max_, 1);
    max_row->addWidget(range_max_inclusive_);
    layout->addRow("Maximum", max_row);

    range_base_ = new QComboBox(page);
    range_base_->addItem("Decimal number", "double");
    range_base_->addItem("Integer", "integer");
    layout->addRow("Type", range_base_);

    for (auto* edit : {range_min_, range_max_}) {
        connect(edit, &QLineEdit::textChanged, this, [this](const QString&) { emitChanged(); });
    }
    for (auto* check : {range_min_inclusive_, range_max_inclusive_}) {
        connect(check, &QCheckBox::toggled, this, [this](bool) { emitChanged(); });
    }
    connect(range_base_, &QComboBox::currentIndexChanged, this, [this](int) { emitChanged(); });
}

void ValueEditor::buildLengthPage() {
    auto* page = makePage(pages_);
    auto* layout = new QFormLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    length_min_ = new QLineEdit(page);
    length_min_->setPlaceholderText("no minimum");
    length_min_->setValidator(new QIntValidator(0, 100000, length_min_));
    length_max_ = new QLineEdit(page);
    length_max_->setPlaceholderText("no maximum");
    length_max_->setValidator(new QIntValidator(0, 100000, length_max_));
    layout->addRow("Minimum", length_min_);
    layout->addRow("Maximum", length_max_);
    auto* hint = new QLabel("Number of characters. The same minimum and maximum is an exact length.", page);
    hint->setProperty("textRole", "secondary");
    hint->setWordWrap(true);
    layout->addRow(hint);
    for (auto* edit : {length_min_, length_max_}) {
        connect(edit, &QLineEdit::textChanged, this, [this](const QString&) { emitChanged(); });
    }
}

// ---------------------------------------------------------------------------
// Method switching
// ---------------------------------------------------------------------------

IdsValue::Kind ValueEditor::currentKind() const {
    return kind_;
}

void ValueEditor::showPage(Kind kind) {
    kind_ = kind;
    if (auto* button = method_buttons_->button(static_cast<int>(kind))) {
        const QSignalBlocker blocker(method_buttons_);
        button->setChecked(true);
    }
    // A QStackedWidget is as tall as its tallest page: let the hidden pages
    // be ignored so a one-line value does not reserve the list's height.
    const int current = pageIndex(kind);
    pages_->setCurrentIndex(current);
    for (int i = 0; i < pages_->count(); ++i) {
        const auto policy = i == current ? QSizePolicy::Preferred : QSizePolicy::Ignored;
        pages_->widget(i)->setSizePolicy(QSizePolicy::Preferred, policy);
    }
    pages_->adjustSize();
    updateGeometry();
}

QString ValueEditor::typedText(Kind kind) const {
    switch (kind) {
    case Kind::Simple:
        return simple_combo_->currentText().trimmed();
    case Kind::Enumeration: {
        const QStringList values = checkedEnumerationValues();
        return values.isEmpty() ? QString() : values.front();
    }
    case Kind::Pattern:
        return pattern_text_->text().trimmed();
    case Kind::Range:
        return range_min_->text().trimmed().isEmpty() ? range_max_->text().trimmed()
                                                      : range_min_->text().trimmed();
    case Kind::Any:
    case Kind::Length:
        break;
    }
    return {};
}

void ValueEditor::switchTo(Kind kind) {
    if (kind == kind_) return;
    // Carry what was typed over to the new method when it is still empty.
    const QString seed = typedText(kind_);
    loading_ = true;
    if (!seed.isEmpty()) {
        switch (kind) {
        case Kind::Simple:
            if (simple_combo_->currentText().trimmed().isEmpty()) simple_combo_->setEditText(seed);
            break;
        case Kind::Enumeration:
            if (checkedEnumerationValues().isEmpty()) addEnumerationValue(seed, true);
            break;
        case Kind::Pattern:
            if (pattern_text_->text().trimmed().isEmpty()) pattern_text_->setText(seed);
            break;
        case Kind::Range:
            if (range_min_->text().isEmpty() && range_max_->text().isEmpty() && isNumber(seed)) {
                range_min_->setText(seed);
            }
            break;
        case Kind::Any:
        case Kind::Length:
            break;
        }
    }
    loading_ = false;
    updatePatternPreview();
    showPage(kind);
    emitChanged();
}

// ---------------------------------------------------------------------------
// Value in / out
// ---------------------------------------------------------------------------

void ValueEditor::setValue(const IdsValue& value) {
    loading_ = true;
    simple_combo_->setEditText(value.kind == Kind::Simple ? value.simple : QString());
    rebuildEnumerationList(value.kind == Kind::Enumeration ? value.values : QStringList());

    pattern_preset_->setCurrentIndex(kStartsWith);
    pattern_text_->clear();
    if (value.kind == Kind::Pattern) {
        const QString& pattern = value.pattern;
        static const QString any = QStringLiteral(".*");
        std::optional<QString> literal;
        int preset = kFreeExpression;
        if (pattern.size() >= 4 && pattern.startsWith(any) && pattern.endsWith(any) &&
            (literal = unescapeXsd(pattern.mid(2, pattern.size() - 4)))) {
            preset = kContains;
        } else if (pattern.endsWith(any) && (literal = unescapeXsd(pattern.left(pattern.size() - 2)))) {
            preset = kStartsWith;
        } else if (pattern.startsWith(any) && (literal = unescapeXsd(pattern.mid(2)))) {
            preset = kEndsWith;
        }
        // ".*" alone has no literal part: a free expression, not an empty "starts with".
        if (preset != kFreeExpression && literal->isEmpty()) preset = kFreeExpression;
        pattern_preset_->setCurrentIndex(preset);
        pattern_text_->setText(preset == kFreeExpression ? pattern : *literal);
    }
    updatePatternPreview();

    const bool range = value.kind == Kind::Range;
    range_min_->setText(range ? value.min : QString());
    range_max_->setText(range ? value.max : QString());
    range_min_inclusive_->setChecked(!range || value.min_inclusive);
    range_max_inclusive_->setChecked(!range || value.max_inclusive);
    const int base_index = range_base_->findData(range ? value.base : QStringLiteral("double"));
    range_base_->setCurrentIndex(base_index < 0 ? 0 : base_index);

    const bool length = value.kind == Kind::Length;
    length_min_->setText(length ? value.min : QString());
    length_max_->setText(length ? value.max : QString());
    loading_ = false;

    // No constraint is an empty Value; a method this field does not offer
    // falls back to Value too.
    Kind kind = value.kind == Kind::Any ? Kind::Simple : value.kind;
    if ((allowed_methods_ & methodFlag(kind)) == 0) kind = Kind::Simple;
    showPage(kind);
}

IdsValue ValueEditor::value() const {
    IdsValue value;
    value.kind = kind_;
    switch (kind_) {
    case Kind::Any:
        break;
    case Kind::Simple:
        value.simple = simple_combo_->currentText().trimmed();
        if (value.simple.isEmpty()) value.kind = Kind::Any;
        break;
    case Kind::Enumeration:
        value.values = checkedEnumerationValues();
        break;
    case Kind::Pattern:
        value.pattern = currentPattern();
        break;
    case Kind::Range:
        value.min = range_min_->text().trimmed().replace(',', '.');
        value.max = range_max_->text().trimmed().replace(',', '.');
        value.min_inclusive = range_min_inclusive_->isChecked();
        value.max_inclusive = range_max_inclusive_->isChecked();
        value.base = range_base_->currentData().toString();
        break;
    case Kind::Length:
        value.min = length_min_->text().trimmed();
        value.max = length_max_->text().trimmed();
        break;
    }
    return value;
}

// ---------------------------------------------------------------------------
// Suggestions
// ---------------------------------------------------------------------------

void ValueEditor::setModelValues(const QList<QPair<QString, int>>& values, int missing) {
    model_values_ = values;
    missing_ = missing;
    rebuildEnumerationList(checkedEnumerationValues());
    refreshSimpleItems();
}

void ValueEditor::setSuggestions(const QStringList& values) {
    suggestions_ = values;
    rebuildEnumerationList(checkedEnumerationValues());
    refreshSimpleItems();
}

void ValueEditor::setCompletions(const QStringList& values) {
    completions_ = values;
    refreshSimpleItems();
}

void ValueEditor::clearSuggestions() {
    model_values_.clear();
    suggestions_.clear();
    missing_ = -1;
    rebuildEnumerationList(checkedEnumerationValues());
    refreshSimpleItems();
}

void ValueEditor::refreshSimpleItems() {
    QStringList all;
    for (const auto& [value, count] : model_values_) {
        static_cast<void>(count);
        all << value;
    }
    for (const QString& value : suggestions_ + completions_) {
        if (!all.contains(value)) all << value;
    }
    completion_model_->setStringList(all);

    const QString text = simple_combo_->currentText();
    const QSignalBlocker blocker(simple_combo_);
    simple_combo_->clear();
    for (const auto& [value, count] : model_values_) {
        simple_combo_->addItem(value);
        simple_combo_->setItemData(simple_combo_->count() - 1, QString("%1 element(s)").arg(count),
                                   Qt::ToolTipRole);
    }
    for (const QString& value : suggestions_ + completions_) {
        if (simple_combo_->findText(value) < 0) simple_combo_->addItem(value);
    }
    simple_combo_->setEditText(text);
}

void ValueEditor::setRangeBase(const QString& base) {
    const int index = range_base_->findData(base);
    if (index >= 0 && kind_ != Kind::Range) {
        const QSignalBlocker blocker(range_base_);
        range_base_->setCurrentIndex(index);
    }
}

void ValueEditor::setReloadVisible(bool visible) {
    reload_button_->setVisible(visible);
}

void ValueEditor::setPlaceholder(const QString& text) {
    simple_combo_->lineEdit()->setPlaceholderText(text);
}

// ---------------------------------------------------------------------------
// List
// ---------------------------------------------------------------------------

void ValueEditor::addEnumerationValue(const QString& value, bool checked) {
    for (int i = 0; i < enumeration_list_->count(); ++i) {
        QListWidgetItem* item = enumeration_list_->item(i);
        if (item->data(kValueRole).toString().compare(value, Qt::CaseInsensitive) == 0) {
            if (checked) item->setCheckState(Qt::Checked);
            return;
        }
    }
    // Use the known spelling when there is one (ifcwall -> IfcWall).
    QString spelled = value;
    for (const QString& known : suggestions_ + completions_) {
        if (known.compare(value, Qt::CaseInsensitive) == 0) {
            spelled = known;
            break;
        }
    }
    QString label = spelled;
    for (const auto& [model_value, count] : model_values_) {
        if (model_value == spelled) {
            label = QString("%1    — %2 element(s)").arg(spelled).arg(count);
            break;
        }
    }
    auto* item = new QListWidgetItem(label, enumeration_list_);
    item->setData(kValueRole, spelled);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
    item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

void ValueEditor::rebuildEnumerationList(const QStringList& checked) {
    {
        const QSignalBlocker blocker(enumeration_list_);
        enumeration_list_->clear();
        // Model values first (most frequent first), then the short known
        // lists, then the checked values found nowhere else.
        for (const auto& [value, count] : model_values_) {
            static_cast<void>(count);
            addEnumerationValue(value, checked.contains(value));
        }
        for (const QString& value : suggestions_) addEnumerationValue(value, checked.contains(value));
        for (const QString& value : checked) addEnumerationValue(value, true);
    }
    QString info;
    if (!model_values_.isEmpty()) {
        info = QString("%1 distinct value(s) in the model").arg(model_values_.size());
        if (missing_ > 0) info += QString(", %1 element(s) without this property").arg(missing_);
        info += ".";
    } else if (missing_ >= 0) {
        info = "No value found in the model.";
    }
    enumeration_info_->setText(info);
}

QStringList ValueEditor::checkedEnumerationValues() const {
    QStringList values;
    for (int i = 0; i < enumeration_list_->count(); ++i) {
        const QListWidgetItem* item = enumeration_list_->item(i);
        if (item->checkState() == Qt::Checked) values << item->data(kValueRole).toString();
    }
    return values;
}

// ---------------------------------------------------------------------------
// Pattern
// ---------------------------------------------------------------------------

QString ValueEditor::currentPattern() const {
    const QString text = pattern_text_->text();
    if (text.isEmpty()) return {};
    switch (pattern_preset_->currentIndex()) {
    case kStartsWith: return escapeXsd(text) + ".*";
    case kContains: return ".*" + escapeXsd(text) + ".*";
    case kEndsWith: return ".*" + escapeXsd(text);
    default: return text;
    }
}

void ValueEditor::updatePatternPreview() {
    const QString pattern = currentPattern();
    pattern_preview_->setText(pattern.isEmpty() ? QStringLiteral("—") : pattern);
    const QString sample = pattern_test_->text();
    if (sample.isEmpty() || pattern.isEmpty()) {
        pattern_test_result_->clear();
        return;
    }
    // XSD patterns always match the whole value.
    const QRegularExpression expression(QString("^(?:%1)$").arg(pattern));
    if (!expression.isValid()) {
        pattern_test_result_->setText("invalid pattern");
        return;
    }
    pattern_test_result_->setText(expression.match(sample).hasMatch() ? "✓ matches" : "✗ no match");
}

void ValueEditor::emitChanged() {
    if (!loading_) emit changed();
}

} // namespace bonsaiviewer::modules::filters
