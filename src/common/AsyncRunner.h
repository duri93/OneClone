#pragma once

#include <QObject>
#include <QThread>

// ---------------------------------------------------------------------------
// AsyncRunner
// Owns a background QThread together with a worker QObject moved onto it,
// and the moveToThread()/start()/quit()/wait()/deleteLater() bookkeeping
// that goes with running one there. Covers both ways this codebase drives
// blocking RCloneProvider calls off the UI thread:
//
//  - Persistent worker (e.g. RCloneConfigWorker in SetupWizardPage2):
//    construct once and keep the AsyncRunner around; connect request
//    signals to the worker's slots as needed (Qt marshals each call across
//    the thread boundary via a queued connection automatically). The
//    thread is stopped when the AsyncRunner is destroyed.
//
//  - One-shot worker (e.g. RemotesLookupWorker in RemotesAutocompleter):
//    connect backgroundThread()'s started() signal to the worker's entry
//    point, and the worker's own "done" signal to backgroundThread()'s
//    quit() slot, so the task runs once as soon as the thread starts and
//    the thread stops itself as soon as it's done — no need to wait for
//    the AsyncRunner itself to be destroyed.
//
// Either way, the worker is always destroyed (via deleteLater, on its own
// thread) once the background thread stops, and callers never touch the
// QThread's start/quit/wait or the worker's moveToThread/deleteLater
// bookkeeping directly.
// ---------------------------------------------------------------------------
class AsyncRunner : public QObject
{
    Q_OBJECT

public:
    // Takes ownership of `worker`: it must not already have a parent (Qt
    // refuses to moveToThread() a parented object). Moves it onto a new
    // background thread, which starts immediately.
    explicit AsyncRunner(QObject* worker, QObject* parent = nullptr);
    ~AsyncRunner() override;

    // The worker, living on the background thread. Connect to/from its
    // signals and slots as usual.
    QObject* worker() const { return m_worker; }

    // The background thread itself, for wiring up started()/quit() as
    // described above.
    QThread& backgroundThread() { return m_workerThread; }

private:
    QThread  m_workerThread;
    QObject* m_worker;
};
