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

#include "cbDefines.h"
#include "cbDiskArray.h"
#include "cbEmu8080.h"
#include "cbModel.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbDiskArray::cbDiskArray(cbModel* Model, const QList <cbDiskParms*>  Parms)
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
    connect(this, &cbDiskArray::signal_image_loaded, Emu8080, &cbEmu8080::on_disk_image_loaded);

    Reset();
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbDiskArray::~cbDiskArray() 
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

void cbDiskArray::Write(const uint8_t Address, const uint8_t Data) 
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
        qFatal("cbDiskArray::Write - Invalid address : %02X - Decoding violation.", Address);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

uint8_t cbDiskArray::Read(const uint8_t Address) 
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
        qFatal("cbDiskArray::Read - Invalid address : %02X - Decoding violation.", Address);
        }

    return Data;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbDiskArray::Reset() 
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
            QFileInfo file_info(m_Parms[i]->ImageName);
            uint32_t expected_imagesize = 
                m_Parms[i]->SectorSize * m_Parms[i]->SPT * m_Parms[i]->NrTracks;
            if (expected_imagesize != file_info.size())
                {
                QString message;
                message  = tr("Disk: %1\n").arg(QChar('A' + i));
                message += tr("Image: %1\n").arg(m_Parms[i]->ImageName);
                message += tr("Expected imagesize: %1\n").arg(expected_imagesize);
                message += tr("Mounted imagesize: %1").arg(file_info.size());
                signal_image_loaded(c_load_wrong_format, i, message);
                continue;
                }
            bool is_writeable = file_info.isWritable();
            m_Files[i]->open(is_writeable ? QIODevice::ReadWrite : QIODevice::ReadOnly);
            bool is_open = m_Files[i]->isOpen(); 
            if (not is_open)
                {
                QString message;
                message  = tr("Disk: %1\n").arg(QChar('A' + i));
                message += tr("Image: %1\n").arg(m_Parms[i]->ImageName);
                message += tr("Could not open image");
                signal_image_loaded(c_load_could_not_open, i, m_Parms[i]->ImageName);
                m_Parms[i]->ImageName.clear();
                continue;
                }
            else
                {
                signal_image_loaded(is_writeable?c_load_ok_rw : c_load_ok_ro, 
                                    i, 
                                    m_Parms[i]->ImageName);
                }
            }
        else
            {
            signal_image_loaded(c_load_no_image, i, "");
            }
        }
    m_Status = c_Status_OK;
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbDiskArray::DoCommand(const bool Write) 
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
            goto label_return;
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
            goto label_return;
            }

        for (int i=0; i<SectorSize; i++) 
            {
            m_Model->Write(m_AddrDMA+i, Buffer[i]);
            }
        }
  
label_return:
    CB_FREE(Buffer);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbDiskArray::OnLoad(const int Drive, const QString& ImageFileName) 
    {
    m_Parms[Drive]->ImageName = ImageFileName;
    Reset(); // Will close all and reopen all now with correct.
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbDiskArray::OnEject(const int Drive)
    {
    m_Parms[Drive]->ImageName.clear();
    Reset(); // Will close all and reopen all now with correct.
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
