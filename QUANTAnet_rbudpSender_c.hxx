/******************************************************************************
 * QUANTA - A toolkit for High Performance Data Sharing
 * Copyright (C) 2003 Electronic Visualization Laboratory,  
 * University of Illinois at Chicago
 *
 * This library is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either Version 2.1 of the License, or 
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public 
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Direct questions, comments etc about Quanta to cavern@evl.uic.edu
 *****************************************************************************/

#ifndef _QUANTAPLUS_RBUDPSENDER_C

#define _QUANTAPLUS_RBUDPSENDER_C

#include "QUANTAnet_rbudpBase_c.hxx"

/**
RBUDP Sender class.   This class implements the sender part of RBUDP protocol. First, instantiate the QUANTAnet_rbudpSender_c class. Then
call this object's init() method with the host name or IP address of the
receiver as parameter. This is a blocking call that waits for the receiver
to make to init() call to connect to the sender.  After that, you can call
this object's send(buffer, sizeofBuffer, sendRate, sizeofPacket) to send
a buffer.  After you finish sending all buffers, you can call this object's
close() to close this session.
 
*/

class QUANTAnet_rbudpSender_c : public QUANTAnet_rbudpBase_c {
private:
    struct msghdr msgSend;
    struct iovec iovSend[2];
    struct _rbudpHeader sendHeader;

    void udpSendWritev();

    void udpSend();

    int initSendRudp(void *buffer, int bufSize, int sRate, int pSize);

public:
    /** Constructor by telling which TCP and UDP ports we are going to use.
            @ param port the TCP server port and UDP local and remote ports will be calculated based on it.
    */
    QUANTAnet_rbudpSender_c(int port = 38000) {
        tcpPort = port;
        udpLocalPort = port;
        udpRemotePort = port + 1;
        hasTcpSock = 0;
    }

    /** Constructor when we want to reuse exising TCP socket.
            @ param tcpsock the TCP socket we are going to use in RBUDP.
            @ param port UDP local and remote ports will be calculated based on it.
    */
    QUANTAnet_rbudpSender_c(int tcpsock, int port) {
        hasTcpSock = 1;
        tcpSockfd = tcpsock;
        udpLocalPort = port;
        udpRemotePort = port;
    }

    ~QUANTAnet_rbudpSender_c() { };

    /** Send a memory block using RBUDP protocol
        @param buffer the pointer of the buffer you want to send.
        @param bufSize the size of the buffer you want to send.
        @param sendRate the first-pass UDP blasting rate in Kbps, should be decided based on the actual available bandwidth.
        @param packetSize payload size of each UDP packet, suggest 1460 considering the total plusing the header not exceeding the Ethernet MTU 1500.
    */
    void send(void *buffer, int bufSize, int sendRate, int packetSize);


    /** Send a file to the receiver using RBUDP protocol.
        @param sendRate the first-pass UDP blasting rate in Kbps.
    @param packetSize payload size of each UDP packet.
*/
    int sendfile(int sendRate, int packetSize);

    /** Send a set of files to the receiver using RBUDP protocol.
        @param sendRate the first-pass UDP blasting rate in Kbps.
    @param packetSize payload size of each UDP packet.
*/
    int sendfilelist(int sendRate, int packetSize);

    /** Send a data stream from a UNIX file descriptor
    to the receiver using RBUDP protocol.
    @param fromfd   the file descriptor to fetch data from
        @param sendRate the first-pass UDP blasting rate in Kbps.
    @param packetSize payload size of each UDP packet.
    @param bufSize  buffer size -- data is sent in this-size chunks
*/
    int sendstream(int fromfd, int sendRate, int packetSize, int bufSize = 64 * 1024 * 1024);

    /** Initialize a RBUDP session
        @param remoteHost the name of the receiving host
    */
    void init(char *remoteHost);

    /** As part of initialization, open TCP and UDP sockets as needed
        @param remoteHost the name of receiving host
    */
    void openSockets(char *remoteHost);

    /// As part of initialization, TCP socket listens and initialize members
    void listenAndInit();

    /// Close the RBUDP session
    void close();
};

#endif
