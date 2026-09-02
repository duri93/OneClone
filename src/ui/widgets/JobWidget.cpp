#include "JobWidget.h"

#include "src/common/Config.h"
#include "src/ui/widgets/ui_JobWidget.h"

#include <QApplication>
#include <QDrag>
#include <QEnterEvent>
#include <QFont>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QSvgRenderer>

// ---------------------------------------------------------------------------
// Constructor and helpers
// ---------------------------------------------------------------------------
JobWidget::JobWidget(Job *job) : QWidget(nullptr), ui(new Ui::JobWidget) {
    ui->setupUi(this);
    m_job = job;

    // populate widget with data
    onSpecChange();
    onStatusChange();
    hideConfirm();

    // enable mouse tracking and intercept double-click
    setMouseTracking(true);
    installEventFilter(this);

    // edit font size
    QFont small("Arial", Config::SMALL_FONT_SIZE);
    ui->bytes  ->setFont(small);
    ui->speed  ->setFont(small);
    ui->percent->setFont(small);
    ui->eta    ->setFont(small);

    // connect events
    connect(job, &Job::specsChanged,    this, &JobWidget::onSpecChange);
    connect(job, &Job::statusChanged,   this, &JobWidget::onStatusChange);
    connect(job, &Job::progressUpdated, this, &JobWidget::onProgress);
    connect(job, &Job::warning,         this, &JobWidget::onWarning);

    connect(ui->button1, &QPushButton::clicked, this, [this]() { buttonClicked(false); });
    connect(ui->button2, &QPushButton::clicked, this, [this]() { buttonClicked(true);  });
    connect(ui->cancel, &QPushButton::clicked, this, &JobWidget::hideConfirm);
    connect(ui->confirm, &QPushButton::clicked, this, &JobWidget::confirmEvent);
}
JobWidget::~JobWidget() {
    delete ui;
}


// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------
bool JobWidget::eventFilter(QObject* watched, QEvent* event)
{
    if( event->type() != QEvent::MouseButtonDblClick &&
        event->type() != QEvent::MouseButtonRelease){
        return QWidget::eventFilter(watched, event);
    }

    auto* me = static_cast<QMouseEvent*>(event);

    // name doubleclick: open job details
    if( event->type() == QEvent::MouseButtonDblClick &&
        ui->name->geometry().contains(me->pos()) ){
        emit openDetailsRequested(m_job->id());
        return true;
    }

    // icon doubleclick: open local folder
    if( event->type() == QEvent::MouseButtonRelease &&
        ui->jobIcon->geometry().contains(me->pos()) ){
        m_job->openLocalFolder();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}
void JobWidget::enterEvent(QEnterEvent* event)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window,
                 QApplication::palette().color(QPalette::Base).lighter(115));
    setPalette(pal);
    QWidget::enterEvent(event);
}
void JobWidget::leaveEvent(QEvent* event)
{
    setAutoFillBackground(false);
    setPalette(QApplication::palette());
    QWidget::leaveEvent(event);
}


// ---------------------------------------------------------------------------
// Job spec, status and progress
// ---------------------------------------------------------------------------
void JobWidget::onSpecChange(){
    ui->name->setText(m_job->name());

    ui->button2->setVisible(m_job->type() != "mount");

    bool validType = m_job->type() != "";
    ui->button1->setEnabled(validType);
    ui->button2->setEnabled(validType);

    onStatusChange();
}
void JobWidget::onStatusChange(){
    bool active = m_job->active();

    // status label
    ui->jobIcon->setPixmap(getJobIcon());
    ui->statusIcon->setIcon(QIcon(getStatusIcon()));

    // button labels
    if(m_job->type() == "mount"){
        ui->button1->setText(active ? "Stop" : "Mount");
    }else if(m_job->type() == "copy"){
        ui->button1->setText(active ? "Stop" : "Copy ▲");
        ui->button2->setText(active ? "Stop" : "Copy ▼");

        ui->button2->setVisible(!active);
    }else if(m_job->type() == "sync"){
        ui->button1->setText(active ? "Stop" : "Sync ▲");
        ui->button2->setText(active ? "Stop" : "Sync ▼");

        ui->button2->setVisible(!active);
    }

    if(m_job->status() == JobStatus::Starting){
        ui->statusIcon->setToolTip("");
    }

    // show progress if copy/sync is running
    setProgressVisibility();
}

void JobWidget::onProgress(){
    ui->bytes->setText(m_job->progress().bytes);
    ui->speed->setText(m_job->progress().speed);
    ui->percent->setText(QString::number(m_job->progress().percent) + "%");
    ui->eta->setText(m_job->progress().eta);

    ui->progress->setValue(m_job->progress().percent);
}
void JobWidget::onWarning(){

    if(ui->statusIcon->toolTip().isEmpty()){
         ui->statusIcon->setIcon(QIcon(getStatusIcon()));
    }

    QString str = m_job->warnings().join("\n");

    ui->statusIcon->setToolTip(str);
}

const QPixmap JobWidget::getJobIcon() const{
    // svg resource
    QString str = QString(":/icons/%1_%2.svg")
        .arg(m_job->type(), m_job->active() ? "active" : "inactive");
    str = str.toLower();

    // size
    int h = ui->jobIcon->size().height();

    // actually get
    return QPixmap(str).scaled(h, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
const QPixmap JobWidget::getStatusIcon() const{
    int h = ui->statusIcon->size().height();

    // base icon
    QString baseIcon = QString(":/icons/%1.svg").arg(m_job->statusString());
    baseIcon = baseIcon.toLower();

    QPixmap pixmap(baseIcon);
    pixmap = pixmap.scaled(h, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // warning badge
    if(!m_job->warnings().isEmpty()){
        QString warnIcon = QString(":/icons/warning.svg");
        QSvgRenderer warnRenderer(warnIcon);

        int warnSize = h / 3;

        QRect warnRect(
            h - warnSize,
            h - warnSize,
            warnSize,
            warnSize);

        QPainter painter(&pixmap);
        warnRenderer.render(&painter, warnRect);
    }

    // set icon
    return pixmap;
}
void JobWidget::setProgressVisibility(){
    bool active = m_job->active();

    if(m_job->type() != "mount" && active){
        ui->progressFrame->show();
    }else{
        ui->bytes->setText("");
        ui->speed->setText("");
        ui->percent->setText("");
        ui->eta->setText("");
        ui->progress->setValue(0);

        ui->progressFrame->hide();
    }
}

// ---------------------------------------------------------------------------
// Click events
// ---------------------------------------------------------------------------
void JobWidget::buttonClicked(bool swap){
    this->jobInvert = swap;

    if(m_job->type() == "mount" || m_job->active()){
        this->confirmEvent();
    }else{
        this->showConfirm();
    }
}
void JobWidget::showConfirm(){
    QString label = "Confirm starting %1 job?\nFrom: %2\nTo: %3\n";
    label = label.arg(m_job->type(),
                      jobInvert ? m_job->remote() : m_job->local(),
                      jobInvert ? m_job->local()  : m_job->remote());

    ui->confirmLabel->setText(label);

    ui->confirmFrame->show();
}

void JobWidget::hideConfirm(){
    ui->confirmFrame->hide();
}
void JobWidget::confirmEvent(){
    hideConfirm();

    m_job->toggle(jobInvert);
}
// ---------------------------------------------------------------------------
// Drag / drop jobs
// ---------------------------------------------------------------------------
void JobWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton
        && ui->jobIcon->geometry().contains(event->pos()))
    {
        m_dragStartPos = event->pos();
    } else {
        m_dragStartPos = QPoint();  // null — drag won't start
    }
    QWidget::mousePressEvent(event);
}
void JobWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton) || m_dragStartPos.isNull()) return;

    // don't start a drag if the press was on a button
    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
        return;

    QDrag*     drag = new QDrag(this);
    QMimeData* mime = new QMimeData;
    mime->setText(m_job->id());           // carry the job ID
    drag->setMimeData(mime);

    // semi-transparent thumbnail of the widget as the drag pixmap
    QPixmap px = grab();
    drag->setPixmap(px.scaledToWidth(px.width() / devicePixelRatioF(),
                                     Qt::SmoothTransformation));
    drag->setHotSpot(event->pos());

    drag->exec(Qt::MoveAction);
}