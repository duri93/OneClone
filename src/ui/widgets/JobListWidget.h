#pragma once

#include <QWidget>

// ---------------------------------------------------------------------------
// JobListWidget
// Container for the JobWidget rows, supporting drag-and-drop reordering
// with a visual insertion-point indicator.
// ---------------------------------------------------------------------------
class JobListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit JobListWidget(QWidget* parent = nullptr);

signals:
    void jobMoved(const QString& id, int newIndex);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent*  event) override;
    void dropEvent(QDropEvent*          event) override;
    void paintEvent(QPaintEvent*        event) override;

private:
    int m_dropLine = -1;   // y-position of the insertion indicator, -1 = hidden

    int insertionIndexAt(const QPoint& pos) const;
};