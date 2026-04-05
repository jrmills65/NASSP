#pragma once

#if !defined(DSKYTCPCLASS_H_INCLUDED)
#define DSKYTCPCLASS_H_INCLUDED

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>

constexpr auto PORT = 14246;

class DSKYTCP
{
public:

    // Singleton creator
    static DSKYTCP& GetReference()
    {
        static DSKYTCP instance;
        return instance;
    }

    // Singleton creator
    static DSKYTCP* GetPtr()
    {
        return &GetReference();
    }

    SOCKET d_socket;             // DSKY TCP socket
    sockaddr_in d_sockaddr;       // DSKY SOCKADDR_IN

    SOCKET n_socket;
    sockaddr_in n_sockaddr;
    SOCKET AcceptSocket;
    int n_sockaddrlen;

    int conn_state;

    const u_short port = PORT;

    int bytesRead = 0;
    int bytesWritten = 0;

    //buffer to send and receive messages with
    char msg[32];

    // Error control
    int wsk_error;                  // Winsock error
    char wsk_emsg[256];             // Winsock error message

    // Check for available characters (RX)
    int Available();

    // Read data in a buffer, if nbChar is greater than the
    // maximum number of bytes available, it will return only the
    // bytes available. The function returns -1 when nothing could
    // be read, the number of bytes actually read.
    int ReadData(char* buffer, unsigned int nbChar);

    // Writes data from a buffer through the Serial connection
    // return true on success.
    bool WriteData(const unsigned char* buffer, unsigned int nbChar);

    // Check if we are actually connected
    bool IsConnected() const;

    // Transmit (buffered) data to arduino
    bool Transmit();

private:

    DSKYTCP();

    ~DSKYTCP();

    bool connected; // Connection status

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

#endif // DSKYTCPCLASS_H_INCLUDED