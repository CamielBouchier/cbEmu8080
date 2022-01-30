//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "cb8080.h"
#include "cbConsole.h"
#include "cbDiskArray.h"
#include "cbEmu8080.h"
#include "cbMemory.h"
#include "cbModel.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbModel::cbModel() 
    {
    m_Running      = false;
    m_single_stepping      = false;
    m_RequestStop  = false;
    m_RequestReset = false;
    m_HaveError    = false;

    QSettings* UserSettings = Emu8080->m_UserSettings;

    m_8080    = new cb8080(this);
    m_Memory  = new cbMemory(UserSettings->value("Memory/ImageName", "").toString());
    m_Console = new cbConsole();

    for (int Drive = 0; Drive<c_NoDrives; Drive++) 
        {
        cbDiskParms* Parms = new cbDiskParms;
        switch (Drive) 
            {
            case 0 :
                // IBM-3740
                Parms->SPT       = 26;
                Parms->NrTracks  = 77;
                Parms->ImageName = UserSettings->value("Disk/0/ImageName", "").toString();
                break;
            case 1 :
                // IBM-3740
                Parms->SPT       = 26;
                Parms->NrTracks  = 77;
                Parms->ImageName = UserSettings->value("Disk/1/ImageName", "").toString();
                break;
            case 2 :
                // IBM-3740
                Parms->SPT       = 26;
                Parms->NrTracks  = 77;
                Parms->ImageName = UserSettings->value("Disk/2/ImageName", "").toString();
                break;
            case 3 :
                // IBM-3740
                Parms->SPT       = 26;
                Parms->NrTracks  = 77;
                Parms->ImageName = UserSettings->value("Disk/3/ImageName", "").toString();
                break;
            case 4 :
                // 4MB-HD
                Parms->SPT       = 32;
                Parms->NrTracks  = 1024;
                Parms->ImageName = UserSettings->value("Disk/4/ImageName", "").toString();
                break;
            case 5 :
                // 4MB-HD
                Parms->SPT       = 32;
                Parms->NrTracks  = 1024;
                Parms->ImageName = UserSettings->value("Disk/5/ImageName", "").toString();
                break;
            }
        m_DiskParms << Parms;
        }
    m_DiskArray = new cbDiskArray(this, m_DiskParms);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbModel::~cbModel() 
    {
    qDebug("Start destructor");
    delete m_DiskArray;
    delete m_Console;
    delete m_Memory;
    delete m_8080;
    qDebug("End   destructor");
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbModel::set_jiffy_period(const int period /*ms*/)  
    { 
    if (QThread::currentThread() == qApp->thread())
        {
        qFatal("Model supposed to run in separate thread.");
        }
    m_jiffy_period = period * 1.0E-3; 
    m_frequency_measure_timer.start();
    m_frequency_measure_ticks = 0;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbModel::set_target_frequency(const int frequency /*MHz*/) 
    { 
    if (QThread::currentThread() == qApp->thread())
        {
        qFatal("Model supposed to run in separate thread.");
        }
    m_target_frequency = frequency * 1.0E6; 
    m_frequency_measure_timer.start();
    m_frequency_measure_ticks = 0;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbModel::on_jiffy_tick()
    {
    if (QThread::currentThread() == qApp->thread())
        {
        qFatal("Model supposed to run in separate thread.");
        }

    int   elapsed_ms         = m_frequency_measure_timer.elapsed();
    float actual_frequency   = float(m_frequency_measure_ticks) / (elapsed_ms / 1.0E3);

    float missed_ticks  = m_target_frequency * (elapsed_ms * 1.0E-3) - m_frequency_measure_ticks;
    float nr_ticks_per_jiffy = m_jiffy_period * m_target_frequency;
    if (m_Running and not m_single_stepping and missed_ticks > 10 * nr_ticks_per_jiffy)
        {
        m_Running = false;
        QString message = "Too many missed_ticks: target_frequency too high";
        qDebug() << message;
        qDebug() << "m_jiffy_period:" << m_jiffy_period;
        qDebug() << "nr_ticks_per_jiffy:" << nr_ticks_per_jiffy;
        qDebug() << "missed_ticks:" << missed_ticks;
        qDebug() << "actual_frequency:" << actual_frequency;
        qDebug() << "target_frequency:" << m_target_frequency;
        signal_report_stress(message);
        }
    nr_ticks_per_jiffy += missed_ticks;

    for (uint32_t tick = 0; m_Running and tick < nr_ticks_per_jiffy; tick++)
        {
        m_8080->clock_tick();
        if (m_RequestStop)
            {
            m_Running = false;
            }
        if (m_8080->m_Halted)
            {
            m_Running = false;
            QString msg;
            if (m_8080->m_IR == 0166)
                { 
                msg = QString::asprintf("HLT at PC=%04XH.", m_8080->RegPC);
                }
            else
                { 
                msg = QString::asprintf("Illegal instruction: PC=%04XH:%03oQ. Halted.",
                                        m_8080->RegPC,m_8080->m_IR);
                }
            OnError(msg);
            }
        if (m_single_stepping and (m_8080->m_ProcessorState == m_8080->m_IStates - 1) )
            {
            m_Running = false;
            }
        m_frequency_measure_ticks++;
        }

    if (m_Running)
        {
        SignalReportSpeed(actual_frequency/1.0E6);
        }
    else
        {
        SignalReportSpeed(0);
        SignalReportHalted(QString());
        }

    if (m_RequestStop)
        {
        OnStop();
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbModel::Run(bool SingleStep)
    {
    if (QThread::currentThread() == qApp->thread())
        {
        qFatal("Model supposed to run in separate thread.");
        }
    m_Running = true;
    m_single_stepping = SingleStep;
    m_frequency_measure_timer.start();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Async (vs thread) part.
void cbModel::Stop()
    {
    m_RequestStop = true;
    if (!m_Running)
        {
        OnStop();
        }
    }

// Async (vs thread) part.
void cbModel::Reset(const bool Hard)
    {
    m_RequestReset = true;
    m_HardReset = Hard;
    Stop();
    }

// Sync part
void cbModel::OnStop()
    {
    m_RequestStop = false;
    if (m_RequestReset)
        {
        m_RequestReset = false;
        m_8080->Reset();
        m_Console->Reset();
        if (m_HardReset)
            {
            m_Memory->Reset();
            }
        }
    SignalReportSpeed(0);
    QString Msg;
    if (m_HaveError)
        {
        Msg = m_ErrorMsg;
        m_HaveError = false;
        }
    SignalReportHalted(Msg);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbModel::OnError(const QString& Msg)
    {
    m_HaveError   = true;
    m_ErrorMsg    = Msg;
    m_RequestStop = true;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

uint8_t cbModel::Read(const uint16_t Address) 
    {
    return m_Memory->Read(Address);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbModel::Write(const uint16_t Address, const uint8_t Data) 
    {
    m_Memory->Write(Address, Data);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

uint8_t cbModel::ReadIO(const uint8_t Address)
    {
    if ( Address < 0x08 )
        {
        return m_DiskArray->Read(Address);
        }
    else if (Address >= 0x40 and Address <= 0x42)
        {
        return m_Console->Read(Address-0x40);
        }
    else
        {
        QString Message = QString::asprintf("This model does not support ReadIO on Address %04x.", 
                                            Address);
        OnError(Message);
        return 0;
        }
    return 0;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbModel::WriteIO(const uint8_t Address, const uint8_t Data)
    {
    if ( Address < 0x08 )
        {
        m_DiskArray->Write(Address, Data);
        }
    else if (Address >= 0x40 and Address <= 0x42)
        {
        m_Console->Write(Address-0x40, Data);
        }
    else
        {
        QString Message = QString::asprintf("This model does not support WriteIO on Address %04x.",                                             Address);
        OnError(Message);
        return;
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
