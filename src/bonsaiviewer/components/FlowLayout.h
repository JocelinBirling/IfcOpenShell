// This file was generated with the assistance of an AI coding tool.
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

#ifndef IFCINTERFACE_COMPONENTS_FLOWLAYOUT_H
#define IFCINTERFACE_COMPONENTS_FLOWLAYOUT_H

#include <QFrame>
#include <QLayout>
#include <QList>

namespace bonsaiviewer::components {

// A layout that places its items in rows, from left to right, and starts a
// new row when the width runs out (the Qt "flow layout" example). Its size
// hint is one row; its height follows the width (heightForWidth).
class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent = nullptr, int spacing = 0);
    ~FlowLayout() override;
    // Off: one row whatever the width (the nested flows too).
    void setWrapping(bool wrapping);
    bool wrapping() const { return wrapping_; }

    void addItem(QLayoutItem* item) override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize minimumSize() const override;
    QSize sizeHint() const override;
    void setGeometry(const QRect& rect) override;

private:
    int doLayout(const QRect& rect, bool test_only) const;
    QList<QLayoutItem*> items_;
    int spacing_ = 0;
    bool wrapping_ = true;
};

// A frame holding a FlowLayout whose minimum height follows its width: in a
// narrow dock panel, the rows it needs stay visible (the dock layouts do not
// use heightForWidth themselves).
class FlowFrame : public QFrame {
public:
    explicit FlowFrame(QWidget* parent = nullptr) : QFrame(parent) {}
    // Wrapping of its flow and of the flows inside it (a dock on a side or
    // floating: on; on the top or the bottom: off, one row).
    void setWrapping(bool wrapping);

protected:
    void resizeEvent(QResizeEvent* event) override;
};

} // namespace bonsaiviewer::components

#endif // IFCINTERFACE_COMPONENTS_FLOWLAYOUT_H
