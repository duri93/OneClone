#pragma once

#include "src/core/Job.h"

#include <QWidget>

namespace Ui {
class JobWidget;
}

// ---------------------------------------------------------------------------
// JobWidget
// A single row in the jobs list: shows a job's icon, status, and progress,
// and exposes controls to start/stop it, view its details, and reorder it
// via drag-and-drop.
// ---------------------------------------------------------------------------
class JobWidget : public QWidget
{
    Q_OBJECT

public:
    explicit JobWidget(Job* job);
    ~JobWidget() override;

    Job*    job () { return m_job; };
    const Job*    job () const { return m_job; };

    // Connected to Job's signals to keep the widget in sync
    void onSpecChange();
    void onStatusChange();
    void onProgress();
    void onWarning();

    const QPixmap getJobIcon() const;
    const QPixmap getStatusIcon() const;

signals:
    void openDetailsRequested(const QString& id);

private:
    Ui::JobWidget* ui;
    Job*       m_job;
    QPoint m_dragStartPos;

    void setProgressVisibility();
protected:
    bool jobInvert = false;
    void showConfirm();
    void hideConfirm();
    void confirmEvent();

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event)  override;

    bool eventFilter(QObject* watched, QEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
};
