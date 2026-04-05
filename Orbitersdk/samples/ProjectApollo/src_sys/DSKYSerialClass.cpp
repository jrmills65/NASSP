#include "DSKYSerialClass.h"
#include "ioChannels.h"
#include <iostream>
#include <string>
#include <stdio.h>
#include <inaddr.h>
#include <errhandlingapi.h>
#include <winerror.h>
#include <winnt.h>
#include <handleapi.h>
#include <minwindef.h>
#include <fileapi.h>

DSKYSerial::DSKYSerial (const char *portName /*= DEFAULT_COM */) : hDSKYSerial(INVALID_HANDLE_VALUE), connected(false)
{
    int i;

    for(i = 0; i < 12; ++i) // 12 rows of data to drive DSKY display, PROG, VERB, NOUN & registers & some lights
    { 
        memset(channel_010[i], 0, 3);
        channel_010[i][0] = 010; 
    }

    memset(channel_011, 0, 3);
    channel_011[0] = 011;

    memset(channel_013, 0, 3);
    channel_013[0] = 013;

    memset(channel_0163, 0, 3);
    channel_0163[0] = 0163;

    for(i = 0; i < 12; ++i) 
    { 
        memset(_channel_010[i], 0, 3);
        channel_010[i][0] = 010; 
    }

    memset(_channel_011, 0, 3);
    channel_011[0] = 011;

    memset(_channel_013, 0, 3);
    channel_013[0] = 013;

    memset(_channel_0163, 0, 3);
    channel_0163[0] = 0163;

    // Try to connect to the given port through CreateFile
    this->hDSKYSerial = CreateFile( portName,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    // Check if the connection was successfull
    if (this->hDSKYSerial==INVALID_HANDLE_VALUE)
    {
        // If not success full display an Error
        if (GetLastError()==ERROR_FILE_NOT_FOUND)
        {
            // Print Error if neccessary
            printf("ERROR: Handle was not attached. Reason: %s not available.\n", portName);
        }
        else
        {
            printf("ERROR!!!");
        }
    }
    else
    {
        // If connected we try to set the comm parameters
        DCB dcbSerialParams = {0};
        //dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        // Try to get the current
        if (!GetCommState(this->hDSKYSerial, &dcbSerialParams))
        {
            // If impossible, show an error
            printf("failed to get current serial parameters!");
        }
        else
        {
            // Define serial connection parameters for the arduino board
            dcbSerialParams.BaudRate=CBR_57600;
            dcbSerialParams.ByteSize=8;
            dcbSerialParams.StopBits=ONESTOPBIT;
            dcbSerialParams.Parity=NOPARITY;

            // Set the parameters and check for their proper application
            if(!SetCommState(this->hDSKYSerial, &dcbSerialParams))
            {
                printf("ALERT: Could not set Serial Port parameters");
            }
            else
            {
                // If everything went fine we're connected
                this->connected = true;
                // We wait some time as the arduino board will be reseting
                Sleep(ARDUINO_WAIT_TIME);
            }
        }
    }

}

DSKYSerial::~DSKYSerial ()
{
    // Check if we are connected before trying to disconnect
    if (this->connected)
    {
        // We're no longer connected
        this->connected = false;

        // Close the serial handler
        if (this->hDSKYSerial != INVALID_HANDLE_VALUE) 
        {
            CloseHandle(this->hDSKYSerial);
	        this->hDSKYSerial = INVALID_HANDLE_VALUE;
        }
    }
}

int DSKYSerial::Available () 
{
    return this->status.cbInQue;
}

int DSKYSerial::ReadData (char *buffer, unsigned int nbChar)
{
    // Number of bytes we'll have read
    DWORD bytesRead;

    // Number of bytes we'll really ask to read
    unsigned int toRead;

    // Use the ClearCommError function to get status info on the Serial port
    ClearCommError(this->hDSKYSerial, &this->errors, &this->status);

    // Check if there is something to read
    if (this->status.cbInQue > 0)
    {
        // If there is we check if there is enough data to read the required number
        // of characters, if not we'll read only the available characters to prevent
        // locking of the application.
        if (this->status.cbInQue > nbChar) 
        {
            toRead = nbChar;
        } 
        else 
        {
            toRead = this->status.cbInQue;
        }

        // Try to read the require number of chars, and return the number of read bytes on success
        if (ReadFile(this->hDSKYSerial, buffer, toRead, &bytesRead, NULL) && bytesRead != 0)
        {
            return bytesRead;
        }

    }

    //If nothing has been read, or that an error was detected return -1
    return -1;
}

bool DSKYSerial::WriteData (const unsigned char *buffer, unsigned int nbChar)
{
    union 
    {
        ChannelValue10 s;
        unsigned char bytes[2];
    } out_val;

    if (buffer[0] == 010)
    {
        out_val.bytes[0] = buffer[2];
        out_val.bytes[1] = buffer[1];

        if (out_val.s.Bits.a < 12) 
        {
            memcpy(this->channel_010[out_val.s.Bits.a], buffer, 3);
        }
        // DEBUG
        else 
        {
	        int dummy = 1;
        }
    }
    else if (buffer[0] == 011)
    {
        memcpy(this->channel_011, buffer, 3);
    }
    else if (buffer[0] == 013)
    {
        memcpy(this->channel_013, buffer, 3);
    }
    else if (buffer[0] == 0163)
    {
        memcpy(this->channel_0163, buffer, 3);
    }
    else
    {
        return false;
    }

    return true;
}

bool DSKYSerial::IsConnected () const 
{
      return this->connected; // Simply return the connection status
}

bool DSKYSerial::Transmit ()
{
    DWORD bytesSend;
    static int forceCount = 1; // 'forced' update ASAP

    //Try to write the buffers on the Serial port
    bool forceUpdate = false;

    if (!--forceCount)
    {
        forceCount = 100;
        forceUpdate = true;
    }

    //
    // Channel 010
    //
    int i;

    for (i = 0; i < 12; ++i)
    {
        if (forceUpdate ||this->_channel_010[i][1] != this->channel_010[i][1]
                        ||this->_channel_010[i][2] != this->channel_010[i][2])
        {
            if(!WriteFile(this->hDSKYSerial, (void *)this->channel_010[i], 3, &bytesSend, 0))
            {
                //In case it don't work get comm error and return false
                ClearCommError(this->hDSKYSerial, &this->errors, &this->status);
                return false;
            }

            this->_channel_010[i][1] = this->channel_010[i][1];
            this->_channel_010[i][2] = this->channel_010[i][2];
        }
    }

    //
    // Channel 011
    //
    if (forceUpdate ||this->_channel_011[1] != this->channel_011[1]
                    ||this->_channel_011[2] != this->channel_011[2])
    {
        if(!WriteFile(this->hDSKYSerial, (void *)this->channel_011, 3, &bytesSend, 0))
        {
            //In case it don't work get comm error and return false
            ClearCommError(this->hDSKYSerial, &this->errors, &this->status);
            return false;
        }

        this->_channel_011[1] = this->channel_011[1];
        this->_channel_011[2] = this->channel_011[2];
    }

    //
    // Channel 013
    //
    if (forceUpdate ||this->_channel_013[1] != this->channel_013[1]
                    ||this->_channel_013[2] != this->channel_013[2])
    {
        if(!WriteFile(this->hDSKYSerial, (void *)this->channel_013, 3, &bytesSend, 0))
        {
            //In case it don't work get comm error and return false
            ClearCommError(this->hDSKYSerial, &this->errors, &this->status);
            return false;
        }

        this->_channel_013[1] = this->channel_013[1];
        this->_channel_013[2] = this->channel_013[2];
    }

    //
    // Channel 0163
    //
    if (forceUpdate || this->_channel_0163[1] != this->channel_0163[1]
                    || this->_channel_0163[2] != this->channel_0163[2])
    {
        if (!WriteFile(this->hDSKYSerial, (void*)this->channel_0163, 3, &bytesSend, 0))
        {
            //In case it don't work get comm error and return false
            ClearCommError(this->hDSKYSerial, &this->errors, &this->status);
            return false;
        }

        this->_channel_0163[1] = this->channel_0163[1];
        this->_channel_0163[2] = this->channel_0163[2];
    }

    return true;
}


static char ValueChar(unsigned val)
{
    switch (val)
    {
        case 21:    return '0';
        case 3:     return '1';
        case 25:    return '2';
        case 27:    return '3';
        case 15:    return '4';
        case 30:    return '5';
        case 28:    return '6';
        case 19:    return '7';
        case 29:    return '8';
        case 31:    return '9';

        default:    return ' ';
    }

    return ' ';
}

static void decodeChannel10(FILE* fp, unsigned char* buffer)
{
    static unsigned char
        Prog[2] = { ' ', ' ' },
        Verb[2] = { ' ', ' ' },
        Noun[2] = { ' ', ' ' },
        R1[6] = { ' ', ' ', ' ', ' ', ' ', ' ' },
        R2[6] = { ' ', ' ', ' ', ' ', ' ', ' ' },
        R3[6] = { ' ', ' ', ' ', ' ', ' ', ' ' };

    union
    {
        ChannelValue10 s;
        unsigned char bytes[2];
    } out_val;

    unsigned char C1, C2;

    //out_val.Value = value;
    out_val.bytes[0] = buffer[1];
    out_val.bytes[1] = buffer[0];

    C1 = ValueChar(out_val.s.Bits.c);
    C2 = ValueChar(out_val.s.Bits.d);

    //fprintf(fp, "[%c][%c]", C1, C2);

    bool display = true;

    switch (out_val.s.Bits.a)
    {
        case 11:
            Prog[0] = C1;
            Prog[1] = C2;
            break;

        case 10:
            Verb[0] = C1;
            Verb[1] = C2;
            break;

        case 9:
            Noun[0] = C1;
            Noun[1] = C2;
            break;

        case 8:
            R1[1] = C2;
            break;

        case 7:
            R1[2] = C1;
            R1[3] = C2;
            if (out_val.s.Bits.b)
            {
                R1[0] = '+';
            }
            else if (R1[0] == '+')
            {
                R1[0] = ' ';
            }
            break;

        case 6:
            R1[4] = C1;
            R1[5] = C2;
            if (out_val.s.Bits.b)
            {
                R1[0] = '-';
            }
            else if (R1[0] == '-')
            {
                R1[0] = ' ';
            }
            break;

        case 5:
            R2[1] = C1;
            R2[2] = C2;
            if (out_val.s.Bits.b)
            {
                R2[0] = '+';
            }
            else if (R2[0] == '+')
            {
                R2[0] = ' ';
            }
            break;

        case 4:
            R2[3] = C1;
            R2[4] = C2;
            if (out_val.s.Bits.b)
            {
                R2[0] = '-';
            }
            else if (R2[0] == '-')
            {
                R2[0] = ' ';
            }
            break;

        case 3:
            R2[5] = C1;
            R3[1] = C2;
            break;

        case 2:
            R3[2] = C1;
            R3[3] = C2;
            if (out_val.s.Bits.b)
            {
                R3[0] = '+';
            }
            else if (R3[0] == '+')
            {
                R3[0] = ' ';
            }
            break;

        case 1:
            R3[4] = C1;
            R3[5] = C2;
            if (out_val.s.Bits.b)
            {
                R3[0] = '-';
            }
            else if (R3[0] == '-')
            {
                R3[0] = ' ';
            }
            break;

        // 12 - set light states.
        case 12:
            display = false;
            break;

        default:
            display = false;
    }

    //
    if (display)
    {
        fprintf(fp, "[%c%c] [%c%c] [%c%c]  [%c%c%c%c%c%c][%c%c%c%c%c%c][%c%c%c%c%c%c]",
            Prog[0], Prog[1],
            Verb[0], Verb[1],
            Noun[0], Noun[1],
            R1[0], R1[1], R1[2], R1[3], R1[4], R1[5],
            R2[0], R2[1], R2[2], R2[3], R2[4], R2[5],
            R3[0], R3[1], R3[2], R3[3], R3[4], R3[5]);
    }
    else
    {
        fprintf(fp, "?");
    }

}
