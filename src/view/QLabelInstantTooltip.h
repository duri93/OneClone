#pragma once
#include <QLabel>
#include <QEvent>
#include <QToolTip>

class QLabelInstantTooltip : public QLabel {
    Q_OBJECT

public:
    // Add this constructor accepting an optional parent pointer
    explicit QLabelInstantTooltip(QWidget *parent = nullptr) : QLabel(parent) {}
protected:
    bool event(QEvent *e) override {
        if (e->type() == QEvent::ToolTip) {
            return true; // Ignore default delayed tooltip trigger
        }
        return QLabel::event(e);
    }

    void enterEvent(QEnterEvent *e) override {
        QLabel::enterEvent(e);
        if (!toolTip().isEmpty()) {
            QToolTip::showText(QCursor::pos(), toolTip(), this, QRect(), -1);
        }
    }

    void leaveEvent(QEvent *e) override {
        QToolTip::hideText();
        QLabel::leaveEvent(e);
    }
};