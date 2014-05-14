
#include <unistd.h>
#include <sys/syscall.h>

#include "debugoutput.h"

// simple 
// setenv("QT_MESSAGE_PATTERN", "[%{type}] %{appname} (%{file}:%{line}) T%{threadid} %{function} - %{message} ", 1);

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    int tid = syscall(__NR_gettid);
    QDateTime now = QDateTime::currentDateTime();
    QString time_str = now.toString("yyyy-MM-dd hh:mm:ss"); // now.toString("yyyy-MM-dd hh:mm:ss.zzz");

    QStringList tlist = QString(context.file).split('/');
    QString hpath = tlist.takeAt(tlist.count() - 1);

    QString mfunc = QString(context.function);
    tlist = mfunc.split(' ');
    if (tlist.at(0) == "static" || tlist.at(0) == "virtual") {
        tlist = tlist.takeAt(2).split('(');
    } else {
        tlist = tlist.takeAt(1).split('(');
    }
    mfunc = tlist.takeAt(0);
    
    // static void StunClient::debugStunResponse(QByteArray)
    // void StunClient::debugStunResponse(QByteArray)
    // virtual void StunClinet::aaa()
    if (1) {
        fprintf(stderr, "[%s] T(%u) %s:%u %s - %s\n", time_str.toLocal8Bit().data(),  tid,
                hpath.toLocal8Bit().data(), context.line,
                mfunc.toLocal8Bit().data(), msg.toLocal8Bit().constData());
    } else {
        fprintf(stderr, "[%s] T(%u) %s:%u %s,%s - %s\n", time_str.toLocal8Bit().data(), tid,
                hpath.toLocal8Bit().data(), context.line,
                mfunc.toLocal8Bit().data(), context.function, msg.toLocal8Bit().constData());
    }
    return;

    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}
