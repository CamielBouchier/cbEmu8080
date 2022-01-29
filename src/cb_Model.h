//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <cstdint>
#include <QtCore>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class cb_Model : public QObject
    {
    Q_OBJECT

    public :

        cb_Model();
        ~cb_Model();

        void    Run(bool SingleStep);
        void    Reset(const bool Hard);
        void    Stop();

        uint8_t Read   (const uint16_t Address);
        void    Write  (const uint16_t Address, const uint8_t Data);
        uint8_t ReadIO (const uint8_t  Address);
        void    WriteIO(const uint8_t  Address, const uint8_t Data);

        class cb_8080*    m_8080;
        class cb_Memory*  m_Memory;
        class cb_Console* m_Console;

        static const int c_NoDrives = 6;

        QList <class cb_DiskParms*> m_DiskParms;
        class cb_DiskArray*         m_DiskArray;
        
        void on_jiffy_tick(); 

        void set_jiffy_period(const int period /*ms*/);
        void set_target_frequency(const int frequency /*MHz*/);
        
    private :

        float m_jiffy_period;       /* sec */
        float m_target_frequency;   /* Hz  */

        bool    m_single_stepping;
        bool    m_Running;
        bool    m_RequestStop;
        bool    m_RequestReset;
        bool    m_HardReset;
        bool    m_HaveError;
        QString m_ErrorMsg;

        QElapsedTimer m_frequency_measure_timer;
        uint64_t      m_frequency_measure_ticks;

        void    OnStop();
        void    OnError(const QString& ErrMsg);

    
    signals :
        void SignalReportSpeed(const float MHz);
        void SignalReportHalted(const QString& ErrMsg);
        void signal_report_stress(const QString& message);
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
