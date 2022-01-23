//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <QMessageBox>

#include "cb_Defines.h" 
#include "cb_emu_8080.h" 

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void CustomMessageHandler(const QtMsgType           Type, 
                          const QMessageLogContext& Context,
                          const QString&            Message) 
    {
    QString Severity;
    switch (Type)
        {
        case QtDebugMsg    : Severity = QObject::tr("Debug");    break;
        case QtInfoMsg     : Severity = QObject::tr("Info");     break;
        case QtWarningMsg  : Severity = QObject::tr("Warning");  break;
        case QtCriticalMsg : Severity = QObject::tr("Critical"); break;
        case QtFatalMsg    : Severity = QObject::tr("Fatal");    break;
        }
    QString LineMsg = QString::asprintf("(%24s:%5u) - %40s - %8s - %s\n",
                                        Context.file, 
                                        Context.line, 
                                        Context.function,
                                        C_STRING(Severity),
                                        C_STRING(Message));

    // fprintf(stdout, "%p\n", Emu8080); // Check that nullptr during startup.
    fprintf(stdout, C_STRING(LineMsg));
    fflush(stdout);

    if (Emu8080 and Emu8080->m_LogFile) 
        {
        fprintf(Emu8080->m_LogFile, C_STRING(LineMsg));
        fflush(Emu8080->m_LogFile);
        }

    if (Type == QtWarningMsg)
        {
        QMessageBox::warning(NULL, QObject::tr("Warning"), C_STRING(LineMsg));
        }

    if (Type == QtFatalMsg)
        {
        QMessageBox::warning(NULL, QObject::tr("Fatal"), C_STRING(LineMsg));
        Emu8080->OnActionFileExit();
        abort();
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int Argc, char* Argv[]) 
    {
    qRegisterMetaType <uint8_t> ("uint8_t");
    qInstallMessageHandler(CustomMessageHandler);
    qDebug() << "main: here we go!";
    new cb_emu_8080(Argc,Argv);
    Emu8080->exec();
    return (EXIT_SUCCESS);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
