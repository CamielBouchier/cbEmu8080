//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "ui_cb_MainWindow.h"

#include <QLabel>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class cb_MainWindow : public QMainWindow, public Ui::cb_MainWindow 
	{
    Q_OBJECT

    public:
        QLabel* m_SpeedLabel;

        cb_MainWindow(const QString& Title);
        ~cb_MainWindow();

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
