//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "ui_cbMainWindow.h"

#include <QLabel>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class cbMainWindow : public QMainWindow, public Ui::cbMainWindow 
	{
    Q_OBJECT

    public:
        QLabel* m_SpeedLabel;

        cbMainWindow(const QString& Title);
        ~cbMainWindow();

        QAction* m_ActionLoadMemory;
        QAction* m_ActionDumpMemory;
        QAction* m_ActionLoadDisk;
        QAction* m_ActionEjectDisk;

    protected:
        void closeEvent(QCloseEvent* Event);
        void OnMemoryContextMenu(const QPoint& Pos);
        void OnDiskContextMenu(const QPoint& Pos);
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
