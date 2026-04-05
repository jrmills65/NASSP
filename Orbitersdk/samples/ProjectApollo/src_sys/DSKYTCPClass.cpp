#include "DSKYTCPClass.h"
#include "ioChannels.h"
#include <winsock.h>
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

DSKYTCP::DSKYTCP() : connected(false)
{
    int i;

    for (i = 0; i < 12; ++i) // 12 rows of data to drive DSKY display, PROG, VERB, NOUN & registers & some lights
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

    for (i = 0; i < 12; ++i)
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


    // Set up socket and listen
    // 
    //for the server, we only need to specify a port number
    if (port == NULL)
    {
        std::cerr << "Usage: port" << std::endl;
        exit(0);
    }

    //setup a socket and connection tools
    d_sockaddr.sin_family = AF_INET;
    d_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    d_sockaddr.sin_port = htons(PORT);

    //open stream oriented socket with internet address
    //also keep track of the socket descriptor
    d_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (d_socket < 0)
    {
        std::cerr << "Error establishing the server socket" << std::endl;
        exit(0);
    }

    //bind the socket to its local address
    int bindStatus = bind(d_socket, (struct sockaddr*)&d_sockaddr, sizeof(d_sockaddr));

    if (bindStatus < 0)
    {
        std::cerr << "Error binding socket to local address" << std::endl;
        exit(0);
    }

    std::cout << "Waiting for a client to connect..." << std::endl;

    //listen for up to 5 requests at a time
    listen(d_socket, 5);

    //receive a request from client using accept
    //we need a new address to connect with the client
    n_sockaddrlen = sizeof(n_sockaddr);

    //while (1)
    //{
    //    //receive a message from the client (listen)
    //    std::cout << "Awaiting client response..." << std::endl;
    //    memset(&msg, 0, sizeof(msg));//clear the buffer
    //    bytesRead += recv(n_socket, ( char* )&msg, sizeof(msg), 0);

    //    if (!strcmp(msg, "exit"))
    //    {
    //        std::cout << "Client has quit the session" << std::endl;
    //        break;
    //    }

    //    std::cout << "Client: " << msg << std::endl;
    //    std::cout << ">";

    //    std::string data;
    //    std::getline(std::cin, data);
    //    memset(&msg, 0, sizeof(msg)); //clear the buffer
    //    strcpy(msg, data.c_str());

    //    if (data == "exit")
    //    {
    //        //send to the client that server has closed the connection
    //        send(n_socket, ( char* )&msg, strlen(msg), 0);
    //        break;
    //    }

    //    //send the message to client
    //    bytesWritten += send(n_socket, ( char* )&msg, strlen(msg), 0);
    //}

    //we need to close the socket descriptors after we're all done
    //gettimeofday(&end1, NULL);
    //closesocket(n_socket);
    //closesocket(d_socket);

    //std::cout << "********Session********" << std::endl;
    //std::cout << "Bytes written: " << bytesWritten << " Bytes read: " << bytesRead << std::endl;
    ////cout << "Elapsed time: " << (end1.tv_sec - start1.tv_sec) << " secs" << endl;
    //std::cout << "Connection closed..." << std::endl;


    //******************************************************************

    //d_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //if (d_socket == INVALID_SOCKET)
    //{
    //    sprintf(wsk_emsg, "DSKY: Error at socket(): %ld", WSAGetLastError());
    //    WSACleanup();
    //    wsk_error = 1;
    //    return;
    //}

    //// Be nonblocking
    //int iMode = 1; // 0 = BLOCKING, 1 = NONBLOCKING

    //if (ioctlsocket(d_socket, FIONBIO, (u_long FAR*) & iMode) != 0)
    //{
    //    sprintf(wsk_emsg, "DSKY: ioctlsocket() failed: %ld", WSAGetLastError());
    //    wsk_error = 1;
    //    closesocket(d_socket);
    //    WSACleanup();
    //    return;
    //}

    //// Set up incoming options
    //d_sockaddr.sin_family = AF_INET;
    //d_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //d_sockaddr.sin_port = htons(port);

    //if (::bind(d_socket, ( SOCKADDR* )&d_sockaddr, sizeof(d_sockaddr)) == SOCKET_ERROR)
    //{
    //    sprintf(wsk_emsg, "Failed to start DSKY TCP interface.");
    //    wsk_error = 1;
    //    closesocket(d_socket);
    //    WSACleanup();
    //    return;
    //}

    //if (listen(d_socket, 1) == SOCKET_ERROR)
    //{
    //    wsk_error = 1;
    //    sprintf(wsk_emsg, "DSKY: listen() failed: %ld", WSAGetLastError());
    //    closesocket(d_socket);
    //    WSACleanup();
    //    return;
    //}
}

DSKYTCP::~DSKYTCP()
{
    if (d_socket != INVALID_SOCKET)
    {
        shutdown(d_socket, 2); // Shutdown both streams
        closesocket(d_socket);
    }
}

int DSKYTCP::Available()
{
    return 0;
}

int DSKYTCP::ReadData(char* buffer, unsigned int nbChar)
{
    // Number of bytes we'll have read
    DWORD bytesRead = 0;

    // Number of bytes we'll really ask to read
    unsigned int toRead = 0;

    // Use the ClearCommError function to get status info on the Serial port
    //ClearCommError(this->hDSKYSerial, &this->errors, &this->status);

    // Check if there is something to read
    //if (this->status.cbInQue > 0)
    //{
    //    // If there is we check if there is enough data to read the required number
    //    // of characters, if not we'll read only the available characters to prevent
    //    // locking of the application.
    //    if (this->status.cbInQue > nbChar)
    //    {
    //        toRead = nbChar;
    //    }
    //    else
    //    {
    //        toRead = this->status.cbInQue;
    //    }

    //    // Try to read the require number of chars, and return the number of read bytes on success
    //    if (ReadFile(this->hDSKYSerial, buffer, toRead, &bytesRead, NULL) && bytesRead != 0)
    //    {
    //        return bytesRead;
    //    }

    //}

    //If nothing has been read, or that an error was detected return -1
    return -1;
}

bool DSKYTCP::WriteData(const unsigned char* buffer, unsigned int nbChar)
{
    return false;
}

bool DSKYTCP::IsConnected() const
{
    return false;
}

bool DSKYTCP::Transmit()
{
    //DWORD bytesSend;
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
        if (forceUpdate || this->_channel_010[i][1] != this->channel_010[i][1]
            || this->_channel_010[i][2] != this->channel_010[i][2])
        {
            if (!send(AcceptSocket, (char*)this->channel_010[i], 3, 0))
            {
                //In case it don't work get comm error and return false
                //ClearCommError(this->hDSKYSerial, &this->errors, &this->status);
                return false;
            }

            this->_channel_010[i][1] = this->channel_010[i][1];
            this->_channel_010[i][2] = this->channel_010[i][2];
        }
    }

    //
    // Channel 011
    //
    if (forceUpdate || this->_channel_011[1] != this->channel_011[1]
        || this->_channel_011[2] != this->channel_011[2])
    {
        if (!send(AcceptSocket, (char*)this->channel_011[i], 3, 0))
        {
            //In case it don't work get comm error and return false
            //ClearCommError(this->hDSKYSerial, &this->errors, &this->status);
            return false;
        }

        this->_channel_011[1] = this->channel_011[1];
        this->_channel_011[2] = this->channel_011[2];
    }

    //
    // Channel 013
    //
    if (forceUpdate || this->_channel_013[1] != this->channel_013[1]
        || this->_channel_013[2] != this->channel_013[2])
    {
        if (!send(AcceptSocket, (char*)this->channel_013[i], 3, 0))
        {
            //In case it don't work get comm error and return false
            //ClearCommError(this->hDSKYSerial, &this->errors, &this->status);
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
        if (!send(AcceptSocket, (char*)this->channel_0163[i], 3, 0))
        {
            //In case it don't work get comm error and return false
            //ClearCommError(this->hDSKYSerial, &this->errors, &this->status);
            return false;
        }

        this->_channel_0163[1] = this->channel_0163[1];
        this->_channel_0163[2] = this->channel_0163[2];
    }

    return true;

    // **********************************************************************************
    //switch (conn_state)
    //{
    //    case 0: // Uninitialised
    //        break;

    //    case 1: // Listening???
    //        // Try to accept
    //        AcceptSocket = accept(d_socket, NULL, NULL);

    //        if (AcceptSocket != INVALID_SOCKET)
    //        {
    //            conn_state = 2; // Accept this!
    //            wsk_error = 0; // For now
    //        }

    //        break;

    //    case 2: // Connected
    //        int bytesSent, bytesRecv;

    //        bytesSent = send(AcceptSocket, ( char* )val, 3, 0);

    //        break;
    //}



    //return true;
}
