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
 ********************************************************************************/

#include "Panel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

namespace bonsaiviewer::modules::text_digger {

TextDiggerPanel::TextDiggerPanel(QWidget* parent)
    : components::Panel("Ifc Text Digger", nullptr, parent, false, false)
{
    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(8, 8, 8, 8);

    auto* toolbar = new QHBoxLayout();
    reset_button_ = new QPushButton("Reset", container);
    inspect_button_ = new QPushButton("Inspect", container);
    toolbar->addWidget(reset_button_);
    toolbar->addWidget(inspect_button_);
    toolbar->addStretch(1);

    status_label_ = new QLabel(container);
    status_label_->setObjectName("panelSectionEmptyLabel");

    browser_ = new QTextBrowser(container);
    // Links are followed by TextDiggerView, never opened by the browser.
    browser_->setOpenLinks(false);
    browser_->setOpenExternalLinks(false);

    layout->addLayout(toolbar);
    layout->addWidget(status_label_);
    layout->addWidget(browser_, 1);

    connect(reset_button_, &QPushButton::clicked, this, &TextDiggerPanel::resetRequested);
    connect(inspect_button_, &QPushButton::clicked, this, &TextDiggerPanel::inspectRequested);
    connect(browser_, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
        emit hashtagClicked(url.toString());
    });

    addBodyWidget(container);
}

void TextDiggerPanel::setHtml(const QString& html) {
    browser_->setHtml(html);
}

void TextDiggerPanel::setStatus(const QString& text) {
    status_label_->setText(text);
}

} // namespace bonsaiviewer::modules::text_digger
