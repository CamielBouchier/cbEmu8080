//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense#
//
// $EndLicense#
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <QApplication>
#include <QIcon>
#include <QSettings>
#include <QThread>

#include "cbModel.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class cbEmu8080 : public QApplication 
    {
    Q_OBJECT

    public:

        static QString ProgramName;
        static QString CompanyName;
        static QString DomainName;

        static QString BaseName(const char* FileName);

        cbEmu8080(int& Argc, char* Argv[]);
        ~cbEmu8080();

        class cbMainWindow* m_MainWindow;
        QSettings*          m_UserSettings;
        QString             m_DataLocation;
        FILE*               m_LogFile = nullptr;

        void OnActionFileExit();
        void OnActionLoadMemory();
        void OnActionDumpMemory();
        void OnActionLoadDisk();
        void OnActionEjectDisk();
        void OnReportSpeed(const float MHz);
        void OnReportHalted(const QString& Msg);
        void on_disk_image_loaded(const int load_status, const int drive, const QString& message);
        void OnDiskActivity(const int Drive, const bool Write);
        void OnDiskIdle(const int Drive);
        void OnMemoryImageLoaded(const bool Success, const QString& ImageFileName);
        void on_report_stress(const QString& message);

    signals :

        void SignalRunModel(bool SingleStep);
        void SignalLoadDisk(const int Drive, const QString& ImageFileName);
        void SignalEjectDisk(const int Drive);
        void signal_set_jiffy_period(const int period);
        void signal_set_target_frequency(const int frequency);

    private :

        void OnActionTheme();
        void OnActionDisplayBinary(bool);
        void OnActionDisplayOctal(bool);
        void OnActionDisplayHex(bool);
        void OnActionDisplayDecimal(bool);
        void OnReset(const bool Hard);
        void OnRun();
        void OnStop();
        void OnSingleStep();
        void OnSelectTraceFile();
        void OnCBDoTrace(const bool Enabled);
        void OnConsoleRefreshRateChanged(const int Rate);  // In Hz.

        static const int c_DisplayMode_Bin = 0;
        static const int c_DisplayMode_Oct = 1;
        static const int c_DisplayMode_Hex = 2;
        static const int c_DisplayMode_Dec = 3;

        void RecursiveCopy(const QString& OrgDirName, const QString& DstDirName);
        void Installize();
        void InstallTheme();
        void CreateMainWindow();
        void InstallModel();
        void InitUserSettings();
        void InitConnects();
        void DisplayRegisters();
        void SetTraceFileName(const QString& FileName);
        void SetTraceEnabled(const bool Enabled);
        void SetTraceEnabled();

        class   cbModel*        m_Model;

        QString     m_SettingsFileName;
        QString     m_BaseDir;

        QString                 m_ThemeFileName;
        QHash<QString,QString>  m_ConstantsInStyleSheet;

        int         m_DisplayMode;

        QIcon       m_Icon;
        QThread*    m_ModelThread;

        QTimer*     m_jiffy_timer;
        QTimer*     m_ConsoleBlinkTimer;
        QTimer*     m_ConsoleRefreshTimer;
        QTimer*     m_DiskActivityTimer[cbModel::c_NoDrives];

        int         m_ConsoleRefreshRate;
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern cbEmu8080* Emu8080;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
