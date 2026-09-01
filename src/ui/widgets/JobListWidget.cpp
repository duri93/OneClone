#include "JobListWidget.h"
#include "JobWidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLayout>
#include <QMimeData>
#include <QPainter>

JobListWidget::JobListWidget(QWidget* parent) : QWidget(parent)
{
    setAcceptDrops(true);
}

// Returns which layout index the drop should land at, based on cursor y
int JobListWidget::insertionIndexAt(const QPoint& pos) const
{
    QLayout* l = layout();
    if (!l) return 0;

    for (int i = 0; i < l->count(); ++i) {
        QWidget* w = l->itemAt(i)->widget();
        if (!w) continue;
        if (pos.y() < w->geometry().center().y())
            return i;
    }
    return l->count();
}

void JobListWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasText())
        event->acceptProposedAction();
}

void JobListWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (!event->mimeData()->hasText()) return;

    // find the y-position between items where we'd insert
    int idx = insertionIndexAt(event->position().toPoint());
    QLayout* l = layout();

    if (l && l->count() > 0) {
        if (idx < l->count()) {
            QWidget* w = l->itemAt(idx)->widget();
            m_dropLine = w ? w->geometry().top() : -1;
        } else {
            QWidget* w = l->itemAt(l->count() - 1)->widget();
            m_dropLine = w ? w->geometry().bottom() : -1;
        }
    }

    update();   // repaint the drop indicator
    event->acceptProposedAction();
}

void JobListWidget::dropEvent(QDropEvent* event)
{
    m_dropLine = -1;
    update();

    if (!event->mimeData()->hasText()) return;

    const QString id  = event->mimeData()->text();
    int targetIndex   = insertionIndexAt(event->position().toPoint());

    // find source index for the adjustment
    QLayout* l = layout();
    for (int i = 0; i < l->count(); i++) {
        QWidget* w = l->itemAt(i)->widget();
        if (w && qobject_cast<JobWidget*>(w)->job()->id() == id) {
            if (targetIndex > i) targetIndex--;
            break;
        }
    }
    targetIndex--;

    // sync the new order to the model
    emit jobMoved(id, targetIndex);
    event->acceptProposedAction();
}

// Draw a horizontal line at the drop position
void JobListWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (m_dropLine < 0) return;

    QPainter p(this);
    p.setPen(QPen(palette().highlight().color(), 2));
    p.drawLine(0, m_dropLine, width(), m_dropLine);
}