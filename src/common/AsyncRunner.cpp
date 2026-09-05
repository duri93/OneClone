#include "AsyncRunner.h"

AsyncRunner::AsyncRunner(QObject* worker, QObject* parent)
    : QObject(parent), m_worker(worker)
{
    m_worker->moveToThread(&m_workerThread);

    // The worker is only ever destroyed once its thread has actually
    // stopped running it — whichever of the two usage styles described in
    // the header ends up stopping the thread.
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread.start();
}

AsyncRunner::~AsyncRunner()
{
    m_workerThread.quit();
    m_workerThread.wait();
}
