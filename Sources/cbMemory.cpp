//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "cbDefines.h"
#include "cbEmu8080.h"
#include "cbMemory.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbMemory::cbMemory(const QString& ImageName) 
    {
    Reset();
    // Putting the signalling back here, ensures the initial load
    // - based on user settings - is signalled.
    connect(this, &cbMemory::SignalImageLoaded, Emu8080, &cbEmu8080::OnMemoryImageLoaded);
    if (ImageName.size())
        {
        LoadFromFile(ImageName); 
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbMemory::~cbMemory() 
    {
    qDebug("Start destructor");
    qDebug("End   destructor");
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Reset
// Fill with "DEADBEEF" pattern.
// If FileName, then load as Intel hex.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbMemory::Reset() 
    {
    for (uint32_t Address = 0; Address < 0X10000; Address++) 
        {
        switch (Address % 4) 
            {
            case 0 : m_Memory[Address] = 0xDE; break;
            case 1 : m_Memory[Address] = 0xAD; break;
            case 2 : m_Memory[Address] = 0xBE; break;
            case 3 : m_Memory[Address] = 0xEF; break;
            }
        }
    if (m_ImageFileName.size())
        {
        LoadFromFile(m_ImageFileName);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void cbMemory::SaveToFile(const QString& FileName) 
    {
    FILE* Output = fopen(C_STRING(FileName), "wt");
    if (not Output) 
        {
        qWarning("Could not open file '%s' for writing", C_STRING(FileName));
        return;
        }

    for (uint32_t Address = 0; Address < 0X10000;) 
        {
        uint8_t CheckSum = 0x20; // ByteCount
        CheckSum += (uint8_t) (Address >>8);
        CheckSum += (uint8_t) (Address & 0xFF);
        fprintf(Output,":20%04X00",Address);
        for (int i=0; i<32; i++) {
            uint8_t Data = m_Memory[Address];
            CheckSum += Data;
            fprintf(Output,"%02X",Data);
            Address++;
        }
        CheckSum = ~CheckSum + 1;
        fprintf(Output,"%02X\n",CheckSum);
        }

    fprintf(Output,":00000001FF\n");
    fclose(Output);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Helper.

uint32_t HexExtract(char *p, int Count) 
    {
    uint32_t Data = 0;
    while (Count--) 
        {
        uint8_t c = *(p++);
        Data <<= 4;
        if ((c >= '0') and (c <= '9')) 
            {
            Data += (c - '0');
            } 
        else if ((c >= 'A') and (c <= 'F')) 
            {
            Data += (c + 10 - 'A');
            } 
        else if ((c >= 'a') && (c <= 'f')) 
            {
            Data += (c + 10 - 'a');
            }
        }
    return Data;
    }

void cbMemory::LoadFromFile(const QString& FileName) 
    {
    FILE* Input = fopen(C_STRING(FileName), "rt");
    if (not Input) 
        {
        qWarning("Could not open file '%s' for reading", C_STRING(FileName));
        SignalImageLoaded(false, FileName);
        return;
        }

    char Buffer[257];
    while(fgets(Buffer,sizeof(Buffer),Input)) 
        {
        if (Buffer[0] != ':') continue;
        uint8_t  ByteCount = HexExtract(&Buffer[1],2);
        uint16_t Address   = HexExtract(&Buffer[3],4);
        uint8_t  RecType   = HexExtract(&Buffer[7],2);
        uint8_t  CheckSum  = ByteCount;
        CheckSum += (uint8_t) (Address >>8);
        CheckSum += (uint8_t) (Address & 0xFF);
        CheckSum += RecType;
  
        switch(RecType) 
            { 
            case 0x00 :
                for (int i = 0; i < ByteCount; i++) 
                    {
  	                uint8_t Data = HexExtract(&Buffer[9 + 2 * i],2);
                    m_Memory[Address + i] = Data;
                    CheckSum += Data;
                    }
                break;
            case 0x01 : // End of file record.
                break;
            default :
                qWarning("Invalid RecordType %d", RecType);
                fclose(Input);
                SignalImageLoaded(false, FileName);
                return;
            }
    
        CheckSum = ~CheckSum + 1;
        uint8_t ShouldCheckSum = HexExtract(&Buffer[9+2*ByteCount],2);
        if (CheckSum != ShouldCheckSum) 
            {
            qWarning("Checksum %02X instead of %02X", CheckSum, ShouldCheckSum);
            fclose(Input);
            SignalImageLoaded(false, FileName);
            return;
            }
        }

    fclose(Input);
    m_ImageFileName = FileName;
    SignalImageLoaded(true, FileName);
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
