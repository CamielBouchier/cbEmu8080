//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "cb_Defines.h"
#include "cb_DiskArray.h"
#include "cb_emu_8080.h"
#include "cb_Model.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cb_DiskArray::cb_DiskArray(cb_Model* Model, const QList <cb_DiskParms*>  Parms)
    {
    m_Model    = Model;
    m_Parms    = Parms;
    m_NrDrives = Parms.size();
    for (int Drive = 0; Drive < m_NrDrives; Drive++)
        {
        // Ensure files are existing in QList and "unitialized".
        m_Files << new QFile();
        }

    // Putting the signalling back here, ensures the initial load
    // in Reset() - based on user settings - is signalled.
    connect(this, &cb_DiskArray::SignalImageLoaded, Emu8080, &cb_emu_8080::OnDiskImageLoaded);

    Reset();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cb_DiskArray::~cb_DiskArray() 
    {
    qDebug("Start destructor");
    for (int i=0; i<m_NrDrives; i++) 
        {
        if (m_Files[i]->isOpen()) 
            {
            m_Files[i]->close();
            }
        }
    qDebug("End   destructor");
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_DiskArray::Write(const uint8_t Address, const uint8_t Data) 
    {
    if (Address == 0) 
        {
        m_Drive = Data;
        } 
    else if (Address == 1) 
        {
        m_Sector = Data - 1; // Account for the 1 based counting
        } 
    else if (Address == 2) 
        {
        m_Track = (m_Track & 0xFF00) | Data;
        } 
    else if (Address == 3) 
        {
        m_Track = (m_Track & 0x00FF) | (Data<<8);
        } 
    else if (Address == 4) 
        {
        if ( (Data == 0) or (Data == 1)) 
            {
            DoCommand(Data);
            } 
        else 
            {
            m_Status = c_Status_IllegalCommand;
            }
        } 
    else if (Address == 6) 
        {
        m_AddrDMA = (m_AddrDMA & 0XFF00) | Data;
        } 
    else if (Address == 0+7) 
        {
        m_AddrDMA = (m_AddrDMA & 0X00FF) | (Data<<8);
        } 
    else 
        {
        qFatal("cb_DiskArray::Write - Invalid address : %02X - Decoding violation.", Address);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

uint8_t cb_DiskArray::Read(const uint8_t Address) 
    {
    uint8_t Data = 0;

    if (Address == 0) 
        {
        Data = m_Drive;
        } 
    else if (Address == 1) 
        {
        Data = m_Sector + 1; // Account for the 1 based counting
        } 
    else if (Address == 2) 
        {
        Data = m_Track & 0xFF;
        } 
    else if (Address == 3) 
        {
        Data = m_Track >> 8;
        } 
    else if (Address == 4) 
        {
        Data = 1; // Per construction the IO will be done whenever reading.
        } 
    else if (Address == 5) 
        {
        Data     = m_Status;
        m_Status = c_Status_OK;
        } 
    else if (Address == 6) 
        {
        Data = m_AddrDMA & 0xFF;
        } 
    else if (Address == 7) 
        {
        Data = m_AddrDMA >> 8;
        } 
    else 
        {
        qFatal("cb_DiskArray::Read - Invalid address : %02X - Decoding violation.", Address);
        }

    return Data;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_DiskArray::Reset() 
    {
    for (int i=0; i<m_NrDrives; i++) 
        {
        // Close maybe open files.
        if (m_Files[i]->isOpen()) 
            {
            m_Files[i]->close();
            }
        // Open the files from the images.
        if (m_Parms[i]->ImageName.size() != 0) 
            {
            m_Files[i]->setFileName(m_Parms[i]->ImageName);
            QFileInfo FInfo(m_Parms[i]->ImageName);
            bool Writeable = FInfo.isWritable();
            m_Files[i]->open(Writeable ? QIODevice::ReadWrite : QIODevice::ReadOnly);
            bool Success = m_Files[i]->isOpen(); 
            SignalImageLoaded(Success, i, m_Parms[i]->ImageName);
            if (not Success)
                {
                m_Parms[i]->ImageName.clear();
                }
            }
        else
            {
            SignalImageLoaded(false, i, "");
            }
        }
    m_Status = c_Status_OK;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_DiskArray::DoCommand(const bool Write) 
    {
    m_Status = c_Status_OK;

    // Drive no check.
    if (m_Drive >= m_NrDrives) 
        {
        m_Status = c_Status_IllegalDrive;
        return;
        }

    // Sector check.
    if (m_Sector >= m_Parms[m_Drive]->SPT) 
        {
        m_Status = c_Status_IllegalSector;
        return;
        }

    // Track check.
    if (m_Track >= m_Parms[m_Drive]->NrTracks) 
        {
        m_Status = c_Status_IllegalTrack;
        return;
        }
 
    // File open check.
    if (not m_Files[m_Drive]->isOpen()) 
        {
        m_Status = c_Status_SeekError;
        return;
        }

    uint8_t SectorSize = m_Parms[m_Drive]->SectorSize;

    // Set the file on the right position..
    bool Result = m_Files[m_Drive]->
        seek((qint64)(m_Track * m_Parms[m_Drive]->SPT + m_Sector)*SectorSize);

    if (not Result) 
        {
        qWarning("Drive %d - Could not seek '%s'", 
                 m_Drive,
                 C_STRING(m_Parms[m_Drive]->ImageName));
        m_Status = c_Status_SeekError;
        return;
        }

    // Create buffer.
    char* Buffer;
    CB_CALLOC(Buffer, char*, SectorSize, 1);

    // And write or read from it.
    SignalActivity(m_Drive, Write);
    if (Write) 
        {
        for (int i=0; i<SectorSize; i++) 
            {
            Buffer[i] = m_Model->Read(m_AddrDMA+i);
            }

        qint64 Result = m_Files[m_Drive]->write(Buffer,SectorSize);

        if (Result != SectorSize) 
            {
            qWarning("Drive %d - Could not write '%s'",
                     m_Drive,
                     C_STRING(m_Parms[m_Drive]->ImageName));
            m_Status = c_Status_WriteError;
            return;
            }

        } 
    else 
        {
        qint64 Result = m_Files[m_Drive]->read(Buffer,SectorSize);

        if (Result != SectorSize) 
            {
            qWarning("Drive %d - Could not read '%s' - Track %d - Sector %d",
                     m_Drive,
                     C_STRING(m_Parms[m_Drive]->ImageName),
                     m_Track,
                     m_Sector);
            m_Status = c_Status_ReadError;
            return;
            }

        for (int i=0; i<SectorSize; i++) 
            {
            m_Model->Write(m_AddrDMA+i, Buffer[i]);
            }
        }
  
    CB_FREE(Buffer);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_DiskArray::OnLoad(const int Drive, const QString& ImageFileName) 
    {
    m_Parms[Drive]->ImageName = ImageFileName;
    Reset(); // Will close all and reopen all now with correct.
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cb_DiskArray::OnEject(const int Drive)
    {
    m_Parms[Drive]->ImageName.clear();
    Reset(); // Will close all and reopen all now with correct.
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
