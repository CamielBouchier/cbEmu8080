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
//
// IO module for managing an array of disks.
//
// The disks are mapped onto image files in the emulator.
//
// Programming interface. (I/O space).
//
// Addressed sector = Track * SPT + Sector. SPT is sector per tracks.
//
// 0 : Addressed drive. (0..15 on CP/M systems)
// 1 : FDC Sector within track addressed. 
//     (As custom for LCH : 1 based rather than 0 based).
// 2 : FDC Track addressed. Lower byte. 
// 3 : FDC Track addressed. Higher byte.
// 4 : FDC Command
//       write FDC command: transfer one sector in the wanted direction,
//       0 = read, 1 = write
//       read FDC command: check for IO completion,
//       0 = still in progress, 1 = IO done (will read always done)
// 5 : FDC status (reset on read)
//       The status byte of the FDC is set as follows:
//       0 - ok
//       1 - illegal drive
//       2 - illegal track
//       3 - illegal sector
//       4 - seek error
//       5 - read error
//       6 - write error
//       7 - illegal command to FDC
// 6 : DMA destination address low
// 7 : DMA destination address high
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class cb_DiskParms 
	{
    public:
        uint8_t  SectorSize;    // 128 on standard IBM-3740
        uint16_t SPT;           // Sectors per track. 26 on above IBM-3740
        uint16_t NrTracks;      // 77 on above.
        QString  ImageName;
        cb_DiskParms() 
            { 
            SectorSize = 128;
            SPT        = 26;
            NrTracks   = 77;
            }
    };

class cb_DiskArray : public QObject 
    {
    Q_OBJECT

    public:

        static const int c_load_ok_rw            = 0;
        static const int c_load_ok_ro            = 1;
        static const int c_load_wrong_format     = 2;
        static const int c_load_could_not_open   = 3;
        static const int c_load_no_image         = 4;

        static const int c_Status_OK             = 0;
        static const int c_Status_IllegalDrive   = 1;
        static const int c_Status_IllegalTrack   = 2;
        static const int c_Status_IllegalSector  = 3;
        static const int c_Status_SeekError      = 4;
        static const int c_Status_ReadError      = 5;
        static const int c_Status_WriteError     = 6;
        static const int c_Status_IllegalCommand = 7;

        cb_DiskArray(class cb_Model* Model, const QList <cb_DiskParms*> Parms);
        ~cb_DiskArray();

        // Programming interface.
        void    Write(const uint8_t Address, const uint8_t  Value);
        uint8_t Read (const uint8_t Address);

        void Reset();

        void OnLoad(const int Drive, const QString& ImageFileName);
        void OnEject(const int Drive);

    signals : 

        void signal_image_loaded(const int load_status, const int drive, const QString& msg);
        void SignalActivity(const int Drive, const bool Write);


    private:

        void DoCommand(const bool Write);
        
        int                  m_NrDrives;
        int                  m_Drive;
        uint8_t              m_Sector;
        uint16_t             m_Track;
        uint8_t              m_Status;
        uint16_t             m_AddrDMA;

        QList <cb_DiskParms*> m_Parms; 
        QList <QFile*>       m_Files;

        class cb_Model*       m_Model;
    };

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
