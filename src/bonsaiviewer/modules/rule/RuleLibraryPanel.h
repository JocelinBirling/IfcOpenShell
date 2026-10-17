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

#ifndef IFCINTERFACE_MODULES_RULE_LIBRARY_PANEL_H
#define IFCINTERFACE_MODULES_RULE_LIBRARY_PANEL_H

#include <QWidget>

class QVBoxLayout;

namespace bonsaiviewer::modules::rule {

// The Rule Library: one collapsible section per family of the rule
// catalogue (OneObjectRule, TwoObjectRule, ComplexRule, IDS Facet), the
// entries being drag sources for the Rule panel tree. Rebuilt whenever
// the engine's registry replaces the static catalogue.
class RuleLibraryPanel : public QWidget {
    Q_OBJECT
public:
    explicit RuleLibraryPanel(QWidget* parent = nullptr);

    // Rebuilds the sections from the current rule catalogue (called when
    // the engine registry arrives).
    void rebuildSections();

signals:
    void ruleDefinitionSelected(const QString& name);

private:
    QVBoxLayout* sections_layout_ = nullptr;
};

} // namespace bonsaiviewer::modules::rule

#endif
