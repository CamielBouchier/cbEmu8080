//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "cbMainWindow.h"

#include <QPushButton>
#include <QCloseEvent>

#include "cbDefines.h"
#include "cbEmu8080.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbMainWindow::cbMainWindow(const QString& Title) : QMainWindow(NULL) 
    {
    // Setup from the Gui builder.
    setupUi(this);
    setWindowTitle(Title);

    QIcon LoadIcon;
    LoadIcon.addPixmap(QPixmap(":/cbEmu8080/Icons/reload.png"));    
    TB_SelectTraceFile->setIcon(LoadIcon);
    
    m_SpeedLabel = new QLabel(this);
    m_SpeedLabel->setToolTip(tr("Current speed of processor"));
    StatusBar->addPermanentWidget(m_SpeedLabel);

    QPixmap GrayPixmap;
    GrayPixmap.load(":/cbEmu8080/Icons/circle_gray_13x13.png");

    QIcon RamIcon;
    RamIcon.addPixmap(QPixmap(":/cbEmu8080/Icons/ram.png"));
    TB_Memory->setIcon(RamIcon);
    TB_Memory->setIconSize(QSize(48,48));
    TB_Memory->setContextMenuPolicy(Qt::CustomContextMenu);
    TB_Memory->setToolTip(tr("Right click for options"));
    QL_Memory->setPixmap(GrayPixmap);

    // Really no point in moving this connect to cbEmu8080.
    // Source and target are here.
    connect(TB_Memory, 
            &QToolButton::customContextMenuRequested, 
            this, 
            &cbMainWindow::OnMemoryContextMenu);

    QIcon DiskIcon;
    DiskIcon.addPixmap(QPixmap(":/cbEmu8080/Icons/disk.png"));    

    for (int Drive=0; Drive<6; Drive++)
        {  
        QString ButtonName = QString("TB_Disk_%1").arg(Drive);
        QToolButton* Button = findChild <QToolButton*> (ButtonName);
        if (not Button) 
            {
            qFatal("Button %s not found.", C_STRING(ButtonName));
            }
        Button->setIcon(DiskIcon);
        Button->setIconSize(QSize(48,48));
        Button->setToolTip(tr("Right click for options"));

        QString LabelName = QString("QL_Disk_%1").arg(Drive);
        QLabel* Label = findChild <QLabel*> (LabelName);
        Label->setPixmap(GrayPixmap);

        Button->setContextMenuPolicy(Qt::CustomContextMenu);
        // Really no point in moving this connect to cbEmu8080.
        // Source and target are here.
        connect(Button, 
                &QToolButton::customContextMenuRequested, 
                this, 
                &cbMainWindow::OnDiskContextMenu);
        }

    m_ActionLoadMemory = new QAction(tr("Load memory image"));
    m_ActionDumpMemory = new QAction(tr("Dump memory image"));
    m_ActionLoadDisk   = new QAction(tr("Load disk image"));
    m_ActionEjectDisk  = new QAction(tr("Eject disk image"));

    #ifndef CB_WITH_VTTEST
        actionVTTest->setVisible(false);
    #endif
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Intercept close event and translate to a FileExit.

void cbMainWindow::closeEvent(QCloseEvent* Event) 
    {
    Event->ignore();
    Emu8080->OnActionFileExit();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbMainWindow::OnMemoryContextMenu(const QPoint& Pos)
    {
    QMenu Menu(this);
    Menu.addAction(m_ActionLoadMemory);
    Menu.addAction(m_ActionDumpMemory);
    Menu.exec(TB_Memory->mapToGlobal(Pos));
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbMainWindow::OnDiskContextMenu(const QPoint& Pos)
    {
    QString Sender = sender()->objectName();
    int Disk = Sender.replace("TB_Disk_","").toInt();
    QToolButton* TB = dynamic_cast<QToolButton*>(sender());
    m_ActionLoadDisk->setData(Disk);
    m_ActionEjectDisk->setData(Disk);
    QMenu Menu(this);
    Menu.addAction(m_ActionLoadDisk);
    Menu.addAction(m_ActionEjectDisk);
    Menu.exec(TB->mapToGlobal(Pos));
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbMainWindow::~cbMainWindow() 
    {
    qDebug("Start destructor");
    delete m_ActionLoadMemory;
    delete m_ActionDumpMemory;
    delete m_ActionLoadDisk;
    delete m_ActionEjectDisk;
    qDebug("End   destructor");
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
