
#include <unistd.h>
#include <sys/syscall.h>

#include "debugoutput.h"

// simple 
// setenv("QT_MESSAGE_PATTERN", "[%{type}] %{appname} (%{file}:%{line}) T%{threadid} %{function} - %{message} ", 1);

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    int tid = syscall(__NR_gettid);
    QDateTime now = QDateTime::currentDateTime();
    QString time_str = now.toString("yyyy-MM-dd hh:mm:ss.zzz");
    QStringList paths = QString(context.file).split('/');
    paths.removeFirst(); paths.removeFirst();
    QString hpath = paths.join('/');

    fprintf(stderr, "[] (%s:%u) T(%u) %s - %s", hpath.toLocal8Bit().data(), context.line, tid,
            context.function, msg.toLocal8Bit().constData());
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
