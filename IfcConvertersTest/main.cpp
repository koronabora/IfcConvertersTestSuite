#include <QApplication>
#include <QtConcurrent>
#include "mainwindow.h"
#include "Worker.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    ThreadInstance w1, w2, w3;
    w1.set(new Worker(QString("C:\\")), QThread::Priority::LowPriority);
    QObject::connect(w1.get<Worker>(), &Worker::log, &w, &MainWindow::log, Qt::ConnectionType::QueuedConnection);
    //w2.set(new Worker(QString("D:\\")), QThread::Priority::LowPriority);
    //QObject::connect(w2.get<Worker>(), &Worker::log, &w, &MainWindow::log, Qt::ConnectionType::QueuedConnection);
    w3.set(new Worker(QString("F:\\")), QThread::Priority::LowPriority);
    QObject::connect(w3.get<Worker>(), &Worker::log, &w, &MainWindow::log, Qt::ConnectionType::QueuedConnection);

    w.show();
    
    int res = a.exec();
     w1.stop();
    //w2.stop();
    w3.stop();
    return res;
}
