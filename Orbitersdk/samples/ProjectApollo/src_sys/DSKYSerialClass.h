#pragma once

#if !defined(DSKYSERIALCLASS_H_INCLUDED)
#define DSKYSERIALCLASS_H_INCLUDED

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// Select appropriate com port ***********************************
// 
//constexpr auto DEFAULT_COM = "\\\\.\\COM4"; // UNO R3
constexpr auto DEFAULT_COM = "\\\\.\\COM6"; // r4WiFi
//constexpr auto DEFAULT_COM = "\\\\.\\COM15"; // MEGA2560
//constexpr auto DEFAULT_COM = "\\\\.\\COM20"; // NANO
// 
// ***************************************************************

// Select time [ms] to wait for the arduino board to reset. ******
// 
constexpr auto ARDUINO_WAIT_TIME = 0; // Original value
//constexpr auto ARDUINO_WAIT_TIME = 1000; // Optional test
//
// ***************************************************************

class DSKYSerial
{
  public:
	// Singleton creator
    static DSKYSerial& GetReference ()
    {
        static DSKYSerial instance;
        return instance;
    }

	// Singleton creator
    static DSKYSerial* GetPtr () 
    {
        return &GetReference();
    }

    // Check for available characters (RX)
    int Available ();

    // Read data in a buffer, if nbChar is greater than the
    // maximum number of bytes available, it will return only the
    // bytes available. The function returns -1 when nothing could
    // be read, the number of bytes actually read.
    int ReadData (char *buffer, unsigned int nbChar);

    // Writes data from a buffer through the Serial connection
    // return true on success.
    bool WriteData (const unsigned char *buffer, unsigned int nbChar);

    // Check if we are actually connected
    bool IsConnected () const;

    // Transmit (buffered) data to arduino
    bool Transmit ();


  private:
    // Private ctor
	// Initialize Serial communication with the given COM port
    DSKYSerial (const char *portName = DEFAULT_COM);

    // Private dtor.
	// Close the connection
    ~DSKYSerial ();

    HANDLE  hDSKYSerial;   // Serial comm (file-)handle
    bool    connected; // Connection status
    COMSTAT status;    // Various information about the connection
    DWORD   errors;    // Keeps track of last error

    // AGC channels and their 'mirror' values (for change detection)
	unsigned char channel_010[12][3];
    unsigned char channel_011[3];
    unsigned char channel_013[3];
    unsigned char channel_0163[3];
    unsigned char _channel_010[12][3];
    unsigned char _channel_011[3];
    unsigned char _channel_013[3];
    unsigned char _channel_0163[3];
};

#endif // DSKYSERIALCLASS_H_INCLUDED