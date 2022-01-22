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

#include "cb_Model.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class cb_emu_8080 : public QApplication 
    {
    Q_OBJECT

    public:

        static QString ProgramName;
        static QString CompanyName;
        static QString DomainName;

        static QString BaseName(const char* FileName);

        cb_emu_8080(int& Argc, char* Argv[]);
        ~cb_emu_8080();

        class cb_MainWindow* m_MainWindow;
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
        void OnDiskImageLoaded(const bool Success, const int Drive, const QString& ImageFileName);
        void OnDiskActivity(const int Drive, const bool Write);
        void OnDiskIdle(const int Drive);
        void OnMemoryImageLoaded(const bool Success, const QString& ImageFileName);

    signals :

        void SignalRunModel(bool SingleStep);
        void SignalLoadDisk(const int Drive, const QString& ImageFileName);
        void SignalEjectDisk(const int Drive);

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
        void on_delay_cycles_changed(const int delay_cycles);

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

        class   cb_Model*        m_Model;

        QString     m_SettingsFileName;
        QString     m_BaseDir;

        QString                 m_ThemeFileName;
        QHash<QString,QString>  m_ConstantsInStyleSheet;

        int         m_DisplayMode;

        QIcon       m_Icon;
        QThread*    m_ModelThread;

        QTimer*     m_ConsoleBlinkTimer;
        QTimer*     m_ConsoleRefreshTimer;
        QTimer*     m_DiskActivityTimer[cb_Model::c_NoDrives];

        int         m_ConsoleRefreshRate;
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern cb_emu_8080* Emu8080;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
