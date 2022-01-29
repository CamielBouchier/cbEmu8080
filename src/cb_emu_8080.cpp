//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>

#include "cb_8080.h"
#include "cb_Console.h"
#include "cb_Defines.h"
#include "cb_DiskArray.h"
#include "cb_emu_8080.h"
#include "cb_MainWindow.h"
#include "cb_Memory.h"
#include "cb_Model.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QString cb_emu_8080::ProgramName = QString("cb_emu_8080");
QString cb_emu_8080::CompanyName = QString("cb_soft");
QString cb_emu_8080::DomainName  = QString("www.bouchier.be");

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QString cb_emu_8080::BaseName(const char* FileName) 
    {
    QFileInfo FileInfo(FileName);
    return FileInfo.fileName();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cb_emu_8080::cb_emu_8080(int& Argc, char* Argv[]) : QApplication(Argc, Argv)
    {
    Emu8080 = this;
    
    m_DisplayMode = c_DisplayMode_Bin;

    Installize();
    InstallTheme();
    CreateMainWindow();
    InstallModel();
    InitConnects();

    // Last: some signals ought to be triggered ...
    InitUserSettings();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::InitConnects()
    {
    // Aestethics
    cb_MainWindow* W = m_MainWindow;
    cb_emu_8080*    T = this;

    connect(W->ActionFileExit,       &QAction::triggered,   T, &cb_emu_8080::OnActionFileExit);
    connect(W->ActionFileTheme,      &QAction::triggered,   T, &cb_emu_8080::OnActionTheme);
    connect(W->ActionDisplayBinary,  &QAction::triggered,   T, &cb_emu_8080::OnActionDisplayBinary);
    connect(W->ActionDisplayOctal ,  &QAction::triggered,   T, &cb_emu_8080::OnActionDisplayOctal);
    connect(W->ActionDisplayHex,     &QAction::triggered,   T, &cb_emu_8080::OnActionDisplayHex);
    connect(W->ActionDisplayDecimal, &QAction::triggered,   T, &cb_emu_8080::OnActionDisplayDecimal);
    connect(W->PB_Run,               &QPushButton::clicked, T, &cb_emu_8080::OnRun);
    connect(W->PB_Stop,              &QPushButton::clicked, T, &cb_emu_8080::OnStop);
    connect(W->PB_SingleStep,        &QPushButton::clicked, T, &cb_emu_8080::OnSingleStep);
    connect(W->CB_DoTrace,           &QCheckBox::clicked,   T, &cb_emu_8080::OnCBDoTrace);
    connect(W->TB_SelectTraceFile,   &QToolButton::clicked, T, &cb_emu_8080::OnSelectTraceFile);
    connect(W->PB_HardReset,         &QPushButton::clicked, T, [this] { OnReset(true);  } );
    connect(W->PB_SoftReset,         &QPushButton::clicked, T, [this] { OnReset(false); } );

    connect(m_jiffy_timer,
            &QTimer::timeout,         
            m_Model,
            &cb_Model::on_jiffy_tick);

    connect(m_MainWindow->sb_jiffy_period,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [this](int value) 
                {
                m_UserSettings->setValue("emulator/jiffy_period", value);
                signal_set_jiffy_period(value);
                m_jiffy_timer->start(value);
                });

    connect(m_MainWindow->sb_target_frequency,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [this](int value) 
                {
                m_UserSettings->setValue("emulator/target_frequency", value);
                signal_set_target_frequency(value);
                });

    connect(T, &cb_emu_8080::signal_set_jiffy_period, m_Model, &cb_Model::set_jiffy_period);
    connect(T, &cb_emu_8080::signal_set_target_frequency, m_Model, &cb_Model::set_target_frequency);

    //
    // Memory related
    //

    connect(W->m_ActionLoadMemory,   &QAction::triggered,   T, &cb_emu_8080::OnActionLoadMemory);
    connect(W->m_ActionDumpMemory,   &QAction::triggered,   T, &cb_emu_8080::OnActionDumpMemory);

    //
    // Disk Related
    //

    connect(W->m_ActionLoadDisk,     &QAction::triggered,   T, &cb_emu_8080::OnActionLoadDisk);
    connect(W->m_ActionEjectDisk,    &QAction::triggered,   T, &cb_emu_8080::OnActionEjectDisk);

    connect(T, &cb_emu_8080::SignalLoadDisk,  m_Model->m_DiskArray, &cb_DiskArray::OnLoad);
    connect(T, &cb_emu_8080::SignalEjectDisk, m_Model->m_DiskArray, &cb_DiskArray::OnEject);

    connect(m_Model->m_DiskArray, &cb_DiskArray::SignalActivity, T, &cb_emu_8080::OnDiskActivity);
    for (int Drive=0; Drive<cb_Model::c_NoDrives; Drive++)
        {
        connect(m_DiskActivityTimer[Drive], &QTimer::timeout, T, [this, Drive]{OnDiskIdle(Drive);});
        }

    //
    // Console related. 
    // Runs in main thread. No Signal need.
    //

    cb_Console* C = m_Model->m_Console;

    connect(m_ConsoleRefreshTimer,&QTimer::timeout,         C, &cb_Console::OnUpdateConsole);
    connect(m_ConsoleBlinkTimer,  &QTimer::timeout,         C, &cb_Console::OnBlink);
    connect(W->QCB_FontModeVt220, &QCheckBox::stateChanged, C, &cb_Console::OnFontModeVt220Changed);
    connect(W->QCB_BlankInterline,&QCheckBox::stateChanged, C, &cb_Console::OnBlankInterlineChanged);
    connect(W->QCB_UnderlineRow9, &QCheckBox::stateChanged, C, &cb_Console::OnUnderlineRowChanged);
    connect(W->QS_Intensity,      &QSlider::valueChanged,   C, &cb_Console::OnIntensityChanged);
    connect(W->ActionVTTest,      &QAction::triggered,      C, &cb_Console::OnTest);

    connect(W->QCB_ConsoleColorScheme, 
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            C, &cb_Console::OnColorSchemeChanged);

    connect(W->SliderConsoleRefreshRate, 
            static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged),
            T, &cb_emu_8080::OnConsoleRefreshRateChanged);

    //
    // Model related.
    //

    connect(m_ModelThread,  &QThread::finished,           m_Model, &QObject::deleteLater);
    connect(T,              &cb_emu_8080::SignalRunModel,   m_Model, &cb_Model::Run);
    connect(m_Model,        &cb_Model::SignalReportSpeed,  T,       &cb_emu_8080::OnReportSpeed);
    connect(m_Model,        &cb_Model::SignalReportHalted, T,       &cb_emu_8080::OnReportHalted);

    connect(m_Model,
            &cb_Model::signal_report_stress, 
            T,       
            &cb_emu_8080::on_report_stress);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Account for copying local data to user after installation and 
// adapting some paths accordingly.

void cb_emu_8080::RecursiveCopy(const QString& OrgDirName, const QString& DstDirName)
    {
    QFileInfo OrgInfo(OrgDirName);
    if (!OrgInfo.isDir()) 
        {
        qFatal("'%s' is no directory.", C_STRING(OrgDirName));
        }
    QDir OrgDir(OrgDirName);
    QDir DstDir(DstDirName);
    if (not DstDir.exists()) 
        {
        DstDir.mkpath(DstDirName);
        }
    QStringList FileList = OrgDir.entryList(QDir::Files);
    foreach(QString File, FileList) 
        {
        // Note this doesn't overwrite.
        QFile::copy(OrgDir.filePath(File), DstDir.filePath(File));
        }
    QStringList DirList = OrgDir.entryList(QDir::Dirs);
    foreach(QString Dir, DirList) 
        {
        if (Dir != "." and Dir != "..")
            {
            RecursiveCopy(OrgDir.filePath(Dir), DstDir.filePath(Dir));
            }
        }
    }

void cb_emu_8080::Installize() 
    {
    // Persistent settings.
    QCoreApplication::setOrganizationName(CompanyName);
    QCoreApplication::setOrganizationDomain(DomainName);
    QCoreApplication::setApplicationName(ProgramName);
    // I strongly prefer ini files above register values as they are readable and editeable.
    // We don't want something in a windows registry, do we ?
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Datalocation. Plus create it in case it isn't there yet.
    m_DataLocation  = QDir(QDir::homePath()).filePath(ProgramName);
    QDir DataDir(m_DataLocation);
    if (not DataDir.exists()) 
        {
        DataDir.mkpath(m_DataLocation);
        }
    // Log file used in the MessageHandler. (qDebug and friends go to it)
    m_LogFile = fopen( C_STRING(QDir(m_DataLocation).filePath(ProgramName + ".log")), "wt" );
    qDebug() << "DataLocation:" << m_DataLocation;

    //
    m_UserSettings =new QSettings( QDir(m_DataLocation).filePath(ProgramName + ".ini") ,
                                   QSettings::IniFormat);
    m_SettingsFileName = m_UserSettings->fileName();
    QFileInfo FileInfo(m_SettingsFileName);
    qDebug() << "Settingsfile:" << m_SettingsFileName;


    // currentPath is also under Windows installation the install dir, even 
    // if the executable is bin. At least when we start from the menu.
    // (the shortcut sets the start directory)
    QDir MyDir(QDir::currentPath());
    m_BaseDir = MyDir.canonicalPath();
    qDebug() << "BaseDir:" << m_BaseDir;

    // Stuff to copy to the DataLocation
    QStringList ToCopyList;
    ToCopyList  
        << "charsets"
        << "memory_images"
        << "disk_images"
	    << "themes"
        ;

    foreach(QString ToCopy, ToCopyList) 
        {
        QString   Org = QDir(m_BaseDir).filePath(ToCopy);
        QString   Dst = QDir(m_DataLocation).filePath(ToCopy);
        RecursiveCopy(Org, Dst);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::InstallTheme()
    {
    const char* DT = C_STRING(QDir(m_DataLocation).filePath("themes/standard.dlt"));
    m_ThemeFileName = m_UserSettings->value("ThemeFileName", DT).toString();
    qDebug() << "ThemeFileName:" << m_ThemeFileName;

    // Some init values, knowing we could be called multiple times.
    m_ConstantsInStyleSheet.clear();

    QFile StyleSheetFile(m_ThemeFileName);

    if (not StyleSheetFile.open(QIODevice::ReadOnly | QIODevice::Text)) 
        {
        qWarning("%s", C_STRING(tr("Could not open themefile '%1'").arg(m_ThemeFileName)));
        }

    QTextStream StyleSheetStream(&StyleSheetFile);
    QString     StyleSheet;
    while (not StyleSheetStream.atEnd()) 
        {
        QString Line = StyleSheetStream.readLine();
        // Remove spaces at beginning.
        while (Line.size() and Line.at(0).isSpace()) 
            {
            Line.remove(0,1);
            }
        // Line starting with ; will be considered comment.
        if (Line.startsWith(";")) 
            {
            continue;
            }
        // Line starting with @ will be considered constant defintion
        if (Line.startsWith("@")) 
            {
            // Split on = and remove blanks around.
            QStringList Definition = Line.split("=");
            QString     Identifier = Definition[0];
            while (Identifier.at(Identifier.size() - 1).isSpace()) 
                {
                Identifier.chop(1);
                }
            QString Value = Definition[1];
            while (Value.at(Value.size() - 1).isSpace()) 
                {
                Value.chop(1);
                }
            while (Value.at(0).isSpace()) 
                {
                Value.remove(0,1);
                }
            m_ConstantsInStyleSheet[Identifier] = Value;
            continue;
            }

        // Now check for each of our Identifiers in m_ConstantsInStyleSheet if
        // they are part of the line and thus should be expanded.
        // This is not the most efficient way of doing, but it serves this
        // purpose. And the check on @ before makes it acceptable.
        if (Line.contains("@")) 
            {
            QHashIterator <QString,QString> i(m_ConstantsInStyleSheet);
            while (i.hasNext()) 
                {
                i.next();
                Line.replace(i.key(),i.value());
                }
            }
        // So now we have a new line for our StyleSheet
        StyleSheet += Line + " \n";
        }
    StyleSheetFile.close();

    // See if our theme has defined a mandatory style.
    if (m_ConstantsInStyleSheet.contains("@MANDATORY_STYLE")) 
        {
        setStyle(m_ConstantsInStyleSheet.value("@MANDATORY_STYLE"));
        }

    // Debug option
    // printf("%s",C_STRING(StyleSheet));

    // And now also apply the stylesheet
    setStyleSheet(StyleSheet);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::CreateMainWindow()
    {
    m_Icon.addPixmap(QPixmap(":/cb_emu_8080/icons/cb_emu_8080.png"));    

    m_MainWindow = new cb_MainWindow(ProgramName);
    m_MainWindow->setWindowIcon(m_Icon);

    // Calculate a nice position.
    // Persistent settings.

    QPoint MainWindowPos;
    QSize  MainWindowSize;
    QRect  DesktopRect = (QApplication::desktop())->screenGeometry(m_MainWindow);

    MainWindowPos = m_UserSettings->value
        (
        "MainWindowPos",
        QPoint(DesktopRect.width() / 10, DesktopRect.height() / 10)
        ).toPoint();
    MainWindowSize = m_UserSettings->value
        (
        "MainWindowSize",
        QSize(DesktopRect.width() * 8 / 10, DesktopRect.height() * 8 / 10)
        ).toSize();

    m_MainWindow->MainSplitter->restoreState(m_UserSettings->value("MainSplitter").toByteArray());

    m_MainWindow->resize(MainWindowSize);
    m_MainWindow->move(MainWindowPos);
    m_MainWindow->show();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::InstallModel()
    {
    m_Model = new cb_Model();

    m_jiffy_timer = new QTimer(this);

    // Console Blinking clock. Keep timers in GUI thread
    m_ConsoleBlinkTimer = new QTimer(this);
    m_ConsoleBlinkTimer->start(500);

    // Console refresh clock. Keep timers in GUI thread
    m_ConsoleRefreshTimer = new QTimer(this);
    m_ConsoleRefreshRate = m_UserSettings->value("Console/RefreshRate",20).toInt();
    OnConsoleRefreshRateChanged(m_ConsoleRefreshRate);
    m_ConsoleRefreshTimer->start(m_ConsoleRefreshRate);

    // 
    for (int Drive=0; Drive<cb_Model::c_NoDrives; Drive++)
        {
        m_DiskActivityTimer[Drive] = new QTimer(this);
        m_DiskActivityTimer[Drive]->setSingleShot(true);
        }

    //
    m_ModelThread = new QThread();
    m_Model->moveToThread(m_ModelThread);

    m_ModelThread->start(QThread::HighestPriority);

    DisplayRegisters();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnConsoleRefreshRateChanged(const int ConsoleRefreshRate) 
    {
    m_ConsoleRefreshRate = ConsoleRefreshRate;
    m_UserSettings->setValue("Console/RefreshRate", m_ConsoleRefreshRate);
    m_MainWindow->LabelConsoleRefreshRate->setText(tr("Console : %1 Hz").arg(m_ConsoleRefreshRate));
    m_ConsoleRefreshTimer->stop();
    m_ConsoleRefreshTimer->start(1000.0/m_ConsoleRefreshRate);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::InitUserSettings()
    {
    SetTraceFileName(m_UserSettings->value("TraceFileName", "").toString());
    SetTraceEnabled(m_UserSettings->value("TraceEnabled", false).toBool());

    QString S1 = "emulator/jiffy_period";
    m_UserSettings->setValue(S1, m_UserSettings->value(S1, 20).toInt());
    auto jiffy_period =m_UserSettings->value(S1).toInt();
    m_MainWindow->sb_jiffy_period->setValue(jiffy_period);
    signal_set_jiffy_period(jiffy_period);
    m_jiffy_timer->start(jiffy_period);
    
    QString S2 = "emulator/target_frequency";
    m_UserSettings->setValue(S2, m_UserSettings->value(S2, 2).toInt());
    auto target_frequency = m_UserSettings->value(S2).toInt();
    m_MainWindow->sb_target_frequency->setValue(target_frequency);
    signal_set_target_frequency(target_frequency);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionFileExit()
    {
    // TODO Do we need some blabla before exiting ?
    qInfo() << "That's all folks ...";

    // Early in the process exit situation.
    if (not m_MainWindow) 
        {
        ::exit(EXIT_FAILURE); 
        }

    // Store some stuff.
    m_UserSettings->setValue("MainSplitter",    m_MainWindow->MainSplitter->saveState());
    m_UserSettings->setValue("MainWindowPos",   m_MainWindow->pos());
    m_UserSettings->setValue("MainWindowSize",  m_MainWindow->size());
    m_UserSettings->setValue("DisplayMode",     m_DisplayMode);
    m_UserSettings->sync();

    m_Model->Stop();

    m_ModelThread->quit();  // Note this will call the m_Model destructor !
    m_ModelThread->wait();

    delete m_ModelThread;
    m_MainWindow->deleteLater();

    exit(EXIT_SUCCESS);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionTheme()
    {
    QString OpenFileName = QFileDialog::getOpenFileName(
        NULL,
        tr("Select theme file"),
        m_ThemeFileName,
        tr("cb_emu_8080 Memory File (*.dlt)"));

    // Operation cancelled.
    if (OpenFileName.size() == 0) 
        {
        return;
        }
    m_UserSettings->setValue("ThemeFileName", OpenFileName);
    InstallTheme();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionDisplayBinary(bool) 
    {
    m_MainWindow->ActionDisplayBinary->setChecked(true);
    m_MainWindow->ActionDisplayOctal->setChecked(false);
    m_MainWindow->ActionDisplayHex->setChecked(false);
    m_MainWindow->ActionDisplayDecimal->setChecked(false);
    m_DisplayMode = c_DisplayMode_Bin;
    m_UserSettings->setValue("DisplayMode", m_DisplayMode);
    DisplayRegisters();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionDisplayOctal(bool) 
    {
    m_MainWindow->ActionDisplayBinary->setChecked(false);
    m_MainWindow->ActionDisplayOctal->setChecked(true);
    m_MainWindow->ActionDisplayHex->setChecked(false);
    m_MainWindow->ActionDisplayDecimal->setChecked(false);
    m_DisplayMode = c_DisplayMode_Oct;
    m_UserSettings->setValue("DisplayMode", m_DisplayMode);
    DisplayRegisters();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionDisplayHex(bool) 
    {
    m_MainWindow->ActionDisplayBinary->setChecked(false);
    m_MainWindow->ActionDisplayOctal->setChecked(false);
    m_MainWindow->ActionDisplayHex->setChecked(true);
    m_MainWindow->ActionDisplayDecimal->setChecked(false);
    m_DisplayMode = c_DisplayMode_Hex;
    m_UserSettings->setValue("DisplayMode", m_DisplayMode);
    DisplayRegisters();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionDisplayDecimal(bool) 
    {
    m_MainWindow->ActionDisplayBinary->setChecked(false);
    m_MainWindow->ActionDisplayOctal->setChecked(false);
    m_MainWindow->ActionDisplayHex->setChecked(false);
    m_MainWindow->ActionDisplayDecimal->setChecked(true);
    m_DisplayMode = c_DisplayMode_Dec;
    m_UserSettings->setValue("DisplayMode", m_DisplayMode);
    DisplayRegisters();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionLoadMemory()
    {
    QString memory_images_dir = 
        m_UserSettings->value("Memory/ImagesDir",
                              QDir(m_DataLocation).filePath("memory_images")).toString();
    QString OpenFileName = QFileDialog::getOpenFileName(
        NULL,
        tr("Select memory file"),
        memory_images_dir,
        tr("cb_emu_8080 Memory File (*.hex)"));
    // Operation cancelled.
    if (OpenFileName.size() == 0) 
        {
        return;
        }
    m_UserSettings->setValue("Memory/ImageName", OpenFileName);
    m_UserSettings->setValue("Memory/ImagesDir", QFileInfo(OpenFileName).absolutePath());
    m_Model->m_Memory->LoadFromFile(OpenFileName);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionDumpMemory()
    {
    QString SaveFileName = QFileDialog::getSaveFileName(
        NULL,
        tr("Select memory file"),
        QDir(m_DataLocation).filePath("memory_images"),
        tr("cb_emu_8080 Memory File (*.hex)"));
    // Operation cancelled.
    if (SaveFileName.size() == 0) 
        {
        return;
        }
    m_Model->m_Memory->SaveToFile(SaveFileName);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnRun()
    {
    SetTraceEnabled();
    SignalRunModel(false);
    m_MainWindow->PB_Run->setEnabled(false);
    m_MainWindow->PB_Stop->setEnabled(true);
    m_MainWindow->PB_SingleStep->setEnabled(false);
    m_MainWindow->CB_DoTrace->setEnabled(false);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnSingleStep()
    {
    SetTraceEnabled();
    SignalRunModel(true);
    m_MainWindow->PB_Run->setEnabled(false);
    m_MainWindow->PB_Stop->setEnabled(true);
    m_MainWindow->PB_SingleStep->setEnabled(false);
    m_MainWindow->CB_DoTrace->setEnabled(false);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnReset(const bool Hard)
    {
    m_Model->Reset(Hard); // Async vs thread !
    DisplayRegisters();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnStop()
    {
    m_Model->Stop(); // Async vs thread !
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionLoadDisk()
    {
    int Drive = qobject_cast<QAction*>(sender())->data().toInt();
    QString disk_images_dir = 
        m_UserSettings->value("Disk/ImagesDir",
                              QDir(m_DataLocation).filePath("disk_images")).toString();
    QString ImageFileName = 
        QFileDialog::getOpenFileName(NULL,
                                     tr("Select disk image file"),
                                     disk_images_dir,
                                     tr("CP/M image (*.cpm)"));
    if (ImageFileName.size() == 0) 
        {
        return;
        }
    m_UserSettings->setValue(QString("Disk/%1/ImageName").arg(Drive), ImageFileName);
    m_UserSettings->setValue("Disk/ImagesDir", QFileInfo(ImageFileName).absolutePath());
    SignalLoadDisk(Drive, ImageFileName);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnActionEjectDisk()
    {
    int Drive = qobject_cast<QAction*>(sender())->data().toInt();
    m_UserSettings->setValue(QString("Disk/%1/ImageName").arg(Drive), "");
    SignalEjectDisk(Drive);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnDiskImageLoaded(const bool     Success,
                                  const int      Drive,
                                  const QString& ImageFileName)
    {
    QPixmap Pixmap;
    Pixmap.load(Success ? ":/cb_emu_8080/icons/circle_blue_13x13.png" : 
                          ":/cb_emu_8080/icons/circle_gray_13x13.png");

    QString LabelName = QString("QL_Disk_%1").arg(Drive);
    QLabel* Label = m_MainWindow->findChild <QLabel*> (LabelName);
    Label->setPixmap(Pixmap);

    QString ButtonName = QString("TB_Disk_%1").arg(Drive);
    QToolButton* Button = m_MainWindow->findChild <QToolButton*> (ButtonName);

    QString Msg = tr("Right click for options.");
    if (Success)
        {
        Msg += tr("\nCurrently loaded : %1").arg(ImageFileName);
        }
    Button->setToolTip(Msg);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnDiskActivity(const int Drive, const bool Write)
    {
    if (not m_DiskActivityTimer[Drive]->isActive())
        {
        QPixmap Pixmap;
        Pixmap.load(Write ? ":/cb_emu_8080/icons/circle_red_13x13.png" : 
                            ":/cb_emu_8080/icons/circle_green_13x13.png");

        QString LabelName = QString("QL_Disk_%1").arg(Drive);
        QLabel* Label = m_MainWindow->findChild <QLabel*> (LabelName);
        Label->setPixmap(Pixmap);
        }
    m_DiskActivityTimer[Drive]->start(500);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnDiskIdle(const int Drive)
    {
    QPixmap Pixmap;
    Pixmap.load(":/cb_emu_8080/icons/circle_blue_13x13.png");
    QString LabelName = QString("QL_Disk_%1").arg(Drive);
    QLabel* Label = m_MainWindow->findChild <QLabel*> (LabelName);
    Label->setPixmap(Pixmap);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnMemoryImageLoaded(const bool Success, const QString& ImageFileName)
    {
    QPixmap Pixmap;
    Pixmap.load(Success ? ":/cb_emu_8080/icons/circle_blue_13x13.png" : 
                          ":/cb_emu_8080/icons/circle_gray_13x13.png");
    m_MainWindow->QL_Memory->setPixmap(Pixmap);
    QString Msg = tr("Right click for options.");
    if (Success)
        {
        Msg += tr("\nCurrently loaded : %1").arg(ImageFileName);
        }
    m_MainWindow->TB_Memory->setToolTip(Msg);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnSelectTraceFile()
    {
    QString FileName = 
        m_UserSettings->value("TraceFileName", m_DataLocation + "/Traces").toString();

    QString SaveFileName = QFileDialog::getSaveFileName(
        m_MainWindow,
        tr("Select trace file"),
        FileName,
        tr("cb_emu_8080 trace File (*.log)"));

    // Operation cancelled.
    if (SaveFileName.size() == 0) 
        {
        return;
        }

    m_UserSettings->setValue("TraceFileName", SaveFileName);

    SetTraceFileName(SaveFileName);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::SetTraceFileName(const QString& FileName)
    {
    m_MainWindow->LE_TraceFileName->setText(FileName);
    m_Model->m_8080->m_TraceFileName = FileName;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Only on user clicks. Not on programmatic.

void cb_emu_8080::OnCBDoTrace(const bool Enabled)
    {
    m_UserSettings->setValue("TraceEnabled", Enabled);
    SetTraceEnabled(Enabled);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::SetTraceEnabled(const bool Enabled)
    {
    m_MainWindow->CB_DoTrace->setChecked(Enabled);
    m_Model->m_8080->SetDoTrace(Enabled);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::SetTraceEnabled()
    {
    m_Model->m_8080->SetDoTrace(m_MainWindow->CB_DoTrace->isChecked());
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnReportHalted(const QString& ErrMsg)
    {
    m_Model->m_8080->FlushTrace();
    if (ErrMsg.size())
        {
        QMessageBox::warning(m_MainWindow, 
                             tr("Message from model"),
                             tr("Message from model:\n%1").arg(ErrMsg));
        }
    DisplayRegisters();
    m_MainWindow->PB_Run->setEnabled(true);
    m_MainWindow->PB_Stop->setEnabled(false);
    m_MainWindow->PB_SingleStep->setEnabled(true);
    m_MainWindow->CB_DoTrace->setEnabled(true);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::OnReportSpeed(const float MHz)
    {
    m_MainWindow->m_SpeedLabel->setText(tr("Speed : %1 MHz").arg(MHz,4,'f',1));
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::on_report_stress(const QString& message)
    {
    if (message.size())
        {
        QMessageBox::warning(m_MainWindow, 
                             tr("Error"),
                             tr("Error in model : %1").arg(message));
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cb_emu_8080::~cb_emu_8080()
    {
    qDebug() << "Start destructor";
    qDebug() << "End destructor";
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_emu_8080::DisplayRegisters()
    {
    // Registers from 8080
    int Base           = 0;
    int ByteFieldWidth = 0;

    if (m_DisplayMode == c_DisplayMode_Bin) 
        {
        Base = 2;
        ByteFieldWidth = 8;
        } 
    else if (m_DisplayMode == c_DisplayMode_Oct) 
        {
        Base = 8;
        ByteFieldWidth = 3;
        } 
    else if (m_DisplayMode == c_DisplayMode_Hex) 
        {
        Base = 16;
        ByteFieldWidth = 2;
        } 
    else if (m_DisplayMode == c_DisplayMode_Dec) 
        {
        Base = 10;
        ByteFieldWidth = 3;
        } 
    else 
        {
        qFatal("%s : Unknown m_DisplayMode : %d", __PRETTY_FUNCTION__, m_DisplayMode);
        }

    QString Text;

    Text = QString("%1").arg(m_Model->m_8080->RegA,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_A->setText(Text.toUpper());
    m_MainWindow->LE_A->setReadOnly(true);
 

    Text = QString("%1").arg(m_Model->m_8080->RegB,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_B->setText(Text.toUpper());
    m_MainWindow->LE_B->setReadOnly(true);

    Text = QString("%1").arg(m_Model->m_8080->RegC,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_C->setText(Text.toUpper());
    m_MainWindow->LE_C->setReadOnly(true);
  
    Text = QString("%1").arg(m_Model->m_8080->RegD,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_D->setText(Text.toUpper());
    m_MainWindow->LE_D->setReadOnly(true);
  
    Text = QString("%1").arg(m_Model->m_8080->RegE,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_E->setText(Text);
    m_MainWindow->LE_E->setReadOnly(true);
  
    Text = QString("%1").arg(m_Model->m_8080->RegH,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_H->setText(Text.toUpper());
    m_MainWindow->LE_H->setReadOnly(true);
  
    Text = QString("%1").arg(m_Model->m_8080->RegL,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_L->setText(Text.toUpper());
    m_MainWindow->LE_L->setReadOnly(true);
  
    Text = QString("%1").arg(m_Model->m_8080->RegSPH,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_SPH->setText(Text.toUpper());
    m_MainWindow->LE_SPH->setReadOnly(true);
  
    Text = QString("%1").arg(m_Model->m_8080->RegSPL,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_SPL->setText(Text.toUpper());
    m_MainWindow->LE_SPL->setReadOnly(true);
  
    Text = QString("%1").arg(m_Model->m_8080->RegPCH,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_PCH->setText(Text.toUpper());
    m_MainWindow->LE_PCH->setReadOnly(true);
  
    Text = QString("%1").arg(m_Model->m_8080->RegPCL,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_PCL->setText(Text.toUpper());
    m_MainWindow->LE_PCL->setReadOnly(true);
  
    Text = QString("%1").arg(m_Model->m_8080->m_IR,ByteFieldWidth,Base,QChar('0'));
    m_MainWindow->LE_IR->setText(Text.toUpper());
    m_MainWindow->LE_IR->setReadOnly(true);

    Text = m_Model->m_8080->Disassemble(m_Model->m_8080->RegPC);
    m_MainWindow->LE_DIS->setText(Text);
    m_MainWindow->LE_DIS->setReadOnly(true);

    // Flags
    m_MainWindow->F_Z->setChecked(m_Model->m_8080->m_FlagZ);
    m_MainWindow->F_C->setChecked(m_Model->m_8080->m_FlagC);
    m_MainWindow->F_P->setChecked(m_Model->m_8080->m_FlagP);
    m_MainWindow->F_S->setChecked(m_Model->m_8080->m_FlagS);
    m_MainWindow->F_A->setChecked(m_Model->m_8080->m_FlagA);

    m_MainWindow->F_Z->setEnabled(false);
    m_MainWindow->F_C->setEnabled(false);
    m_MainWindow->F_P->setEnabled(false);
    m_MainWindow->F_S->setEnabled(false);
    m_MainWindow->F_A->setEnabled(false);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cb_emu_8080* Emu8080 = nullptr;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
