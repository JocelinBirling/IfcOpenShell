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

#ifndef IFCINTERFACE_MODULES_RULE_CLASHTOPICDIALOG_H
#define IFCINTERFACE_MODULES_RULE_CLASHTOPICDIALOG_H

#include "ClashTopic.h"
#include "Results.h"

#include "../../ports/bcf/Bcf.h"

#include <QDialog>
#include <QString>
#include <QStringList>

#include <optional>
#include <vector>

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QRadioButton;

namespace bonsaiviewer::modules::rule {

// What the user chose in the dialog.
struct ClashTopicOptions {
    bool per_result = false;     // one topic per result, else one for all
    QString title;               // a pattern: {rule} {source} {target} {measure} {n}
    QString type, status, assigned_to, stage;
    QString priority;            // empty: from the criticity of each topic's results
    QStringList labels;          // one topic: its labels; one per result: added to each topic's own
    QString due_date;            // xs:date, empty: none
    QString description;         // one topic, edited by the user: used as is (empty: generated)
    QString comment;             // the first comment, empty: none
    bool mark_valid = true;
    bool context = false;        // the saved context visible in the viewpoints
    bool snapshot = true;
    bool lines = true;           // the closest points of the results as lines
};

// "Create a BCF Topic": the fields of the topics made from the selected
// results, the generated description as a preview, the options of the
// viewpoints.
class ClashTopicDialog : public QDialog {
    Q_OBJECT

public:
    ClashTopicDialog(std::vector<const ResultRow*> rows, bool per_result, const ports::bcf::Extensions& extensions,
                     const topic::PriorityTable& priorities, bool has_context, const QString& bcf_name,
                     QWidget* parent = nullptr);
    ClashTopicOptions options() const;

private:
    void modeChanged();

    std::vector<const ResultRow*> rows_;
    topic::PriorityTable priorities_;
    QString bcf_name_;
    QString generated_; // the generated description of the selection
    QRadioButton* one_ = nullptr;
    QRadioButton* each_ = nullptr;
    QLineEdit* title_ = nullptr;
    QComboBox* type_ = nullptr;
    QComboBox* status_ = nullptr;
    QComboBox* priority_ = nullptr;
    QLineEdit* labels_ = nullptr;
    QLabel* labels_label_ = nullptr;
    QComboBox* assigned_ = nullptr;
    QComboBox* stage_ = nullptr;
    QCheckBox* has_due_ = nullptr;
    QDateEdit* due_ = nullptr;
    QPlainTextEdit* description_ = nullptr;
    QPlainTextEdit* comment_ = nullptr;
    QCheckBox* mark_valid_ = nullptr;
    QCheckBox* context_ = nullptr;
    QCheckBox* snapshot_ = nullptr;
    QCheckBox* lines_ = nullptr;
    QLabel* summary_ = nullptr;
};

} // namespace bonsaiviewer::modules::rule

#endif // IFCINTERFACE_MODULES_RULE_CLASHTOPICDIALOG_H
