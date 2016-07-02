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

#include "QUANTAnet_rbudpBase_c.hxx"

// Instantiate these here.  Why we should ever need to instantiate constants
// beats me, but some linkers (ia64?) call them undefined if we use them
// in subclasses but not in this class, or something.
const int QUANTAnet_rbudpBase_c::SIZEOFFILENAME;
const int QUANTAnet_rbudpBase_c::UDPSOCKBUF;
const int QUANTAnet_rbudpBase_c::SUCCESS;
const int QUANTAnet_rbudpBase_c::FAILED;

static void checkbuf(int udpSockfd, int sockbufsize, int verbose) {
    int oldsend = -1, oldrecv = -1;
    int newsend = -1, newrecv = -1;
    socklen_t olen = sizeof(int);

    if (getsockopt(udpSockfd, SOL_SOCKET, SO_SNDBUF, &oldsend, &olen) < 0)
        perror("getsockopt: SO_SNDBUF");
    if (getsockopt(udpSockfd, SOL_SOCKET, SO_RCVBUF, &oldrecv, &olen) < 0)
        perror("getsockopt: SO_RCVBUF");
    if (sockbufsize > 0) {
        if (setsockopt(udpSockfd, SOL_SOCKET, SO_SNDBUF, &sockbufsize, sizeof(sockbufsize)) < 0)
            perror("setsockopt: SO_SNDBUF");
        if (setsockopt(udpSockfd, SOL_SOCKET, SO_RCVBUF, &sockbufsize, sizeof(sockbufsize)) < 0)
            perror("setsockopt: SO_RCVBUF");
        if (getsockopt(udpSockfd, SOL_SOCKET, SO_SNDBUF, &newsend, &olen) < 0)
            perror("getsockopt: SO_SNDBUF");
        if (getsockopt(udpSockfd, SOL_SOCKET, SO_RCVBUF, &newrecv, &olen) < 0)
            perror("getsockopt: SO_RCVBUF");
    }
    if (verbose)
        fprintf(stderr, "UDP sockbufsize was %d/%d now %d/%d (send/recv)\n",
                oldsend, oldrecv, newsend, newrecv);
}

void QUANTAnet_rbudpBase_c::passiveUDP(char *host) {
    struct sockaddr_in cliaddr;
    struct hostent *phe;

    // Create a UDP sink
    if ((udpSockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    bzero((char *) &udpServerAddr, sizeof(udpServerAddr));
    udpServerAddr.sin_family = AF_INET;
    udpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpServerAddr.sin_port = htons(udpLocalPort);

    if ((bind(udpSockfd, (struct sockaddr *) &udpServerAddr, sizeof(udpServerAddr))) < 0) {
        perror("UDP bind error");
        exit(1);
    }

    // Use connected UDP to receive only from a specific host and port.
    bzero(&cliaddr, sizeof(cliaddr));
    if (phe = gethostbyname(host))
        memcpy(&cliaddr.sin_addr, phe->h_addr, phe->h_length);
    else if ((cliaddr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        perror("can't get host entry");
        exit(1);
    }

    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(udpRemotePort);
    if (connect(udpSockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0) {
        perror("connect() error");
        exit(1);
    }

    checkbuf(udpSockfd, udpSockBufSize, verbose);
}

int QUANTAnet_rbudpBase_c::connectTCP(char *host) {
    static struct sockaddr_in tcpServerAddr;
    int retval = 0;
    struct hostent *phe;
    struct timeval start, now;

    /*Create a TCP connection */
    bzero((char *) &tcpServerAddr, sizeof(tcpServerAddr));
    tcpServerAddr.sin_family = AF_INET;
    if (phe = gethostbyname(host))
        memcpy(&tcpServerAddr.sin_addr, phe->h_addr, phe->h_length);
    else if ((tcpServerAddr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        perror("can't get host entry");
        exit(1);
    }
    tcpServerAddr.sin_port = htons(tcpPort);

    if ((tcpSockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    printf("try to conn.\n");
    gettimeofday(&start, NULL);
    do {
        retval = connect(tcpSockfd, (struct sockaddr *) &tcpServerAddr, sizeof(tcpServerAddr));
        gettimeofday(&now, NULL);
    }
    while ((retval < 0) && (USEC(&start, &now) < 5000000));
    return retval;
}

void QUANTAnet_rbudpBase_c::connectUDP(char *host) {
    static struct sockaddr_in udpClientAddr;
    struct hostent *phe;

    // Fill in the structure with the address of the server that we want to send to
    // udpServerAddr is class global variable, will be used to send data
    bzero(&udpServerAddr, sizeof(udpServerAddr));
    udpServerAddr.sin_family = AF_INET;
    if (phe = gethostbyname(host))
        memcpy(&udpServerAddr.sin_addr, phe->h_addr, phe->h_length);
    else if ((udpServerAddr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        perror("can't get host entry");
        exit(1);
    }
    udpServerAddr.sin_port = htons(udpRemotePort);

    /* Open a UDP socket */
    if ((udpSockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    // Allow the port to be reused.
    int yes = 1;
    if (setsockopt(udpSockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    /* Bind any local address for us */
    bzero(&udpClientAddr, sizeof(udpClientAddr));
    udpClientAddr.sin_family = AF_INET;
    udpClientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpClientAddr.sin_port = htons(udpLocalPort);
    if ((bind(udpSockfd, (struct sockaddr *) &udpClientAddr, sizeof(udpClientAddr))) < 0) {
        perror("UDP client bind error");
        exit(1);
    }

    checkbuf(udpSockfd, udpSockBufSize, verbose);
}

void QUANTAnet_rbudpBase_c::initTCPServer() {
    struct sockaddr_in tcpServerAddr;

    // Create TCP connection as a server in order to transmit the error list
    // Open a TCP socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    int yes = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    /* Bind our local address so that the client can send to us */
    bzero(&tcpServerAddr, sizeof(tcpServerAddr));
    tcpServerAddr.sin_family = AF_INET;
    tcpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcpServerAddr.sin_port = htons(tcpPort);

    if ((bind(listenfd, (struct sockaddr *) &tcpServerAddr, sizeof(tcpServerAddr))) < 0) {
        perror("bind error");
        exit(1);
    }

    if ((listen(listenfd, 5)) < 0) {
        perror("listen error");
        exit(1);
    }
}

void QUANTAnet_rbudpBase_c::listenTCPServer() {
    struct sockaddr_in tcpClientAddr;
    socklen_t clilen;

    clilen = sizeof(tcpClientAddr);
    tcpSockfd = accept(listenfd, (struct sockaddr *) &tcpClientAddr, &clilen);

    if (tcpSockfd < 0) {
        perror("accept error");
        exit(1);
    }
}

int QUANTAnet_rbudpBase_c::readn(register int fd, register char *ptr, register int nbytes) {
    int nleft, nread;
    nleft = nbytes;
    while (nleft > 0) {
        nread = read(fd, ptr, nleft);
        if (nread < 0)
            return (nread); /*error */
        else if (nread == 0)
            break;  /* EOF */

        nleft -= nread;
        ptr += nread;
    }
    return (nbytes - nleft);
}

int QUANTAnet_rbudpBase_c::writen(register int fd, register char *ptr, register int nbytes) {
    int nleft, nwritten;

    nleft = nbytes;
    while (nleft > 0) {
        nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0)
            return (nwritten); /* error */
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (nbytes - nleft);
}

int QUANTAnet_rbudpBase_c::reportTime(struct timeval &start) {
    struct timeval end;
    int usecs;
    gettimeofday(&end, NULL);
    usecs = 1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    start.tv_sec = end.tv_sec;
    start.tv_usec = end.tv_usec;
    return usecs;
}


void QUANTAnet_rbudpBase_c::initErrorBitmap() {
    int i;
    // the first byte is 0 if there is error.  1 if all done.
    int startOfLastByte = totalNumberOfPackets - (sizeofErrorBitmap - 2) * 8;
    char bits[8] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080};

    /* The first byte is for judging all_done */
    for (i = 0; i < sizeofErrorBitmap; i++)
        errorBitmap[i] = 0;
    /* Preset those bits unused */
    for (i = startOfLastByte; i < 8; i++)
        errorBitmap[sizeofErrorBitmap - 1] |= bits[i];

    /* Hack: we're not sure whether the peer is the same
     * endian-ness we are, and the RBUDP protocol doesn't specify.
     * Let's assume that it's like us, but keep a flag
     * to set if we see unreasonable-looking sequence numbers.
     */
    peerswap = false;
}

int  QUANTAnet_rbudpBase_c::ptohseq(int origseq) {
    int seq = origseq;

    if (peerswap)
        seq = swab32(origseq);

    if (seq < 0 || (seq >> 3) >= sizeofErrorBitmap - 1) {
        if (!peerswap) {
            peerswap = true;
            if (verbose) fprintf(stderr, "peer has different endian-ness from ours\n");
            return ptohseq(seq);
        } else {
            fprintf(stderr, "Unreasonable RBUDP sequence number %d = %x\n",
                    origseq, origseq);
            return 0;
        }
    }
    return seq;
}


void QUANTAnet_rbudpBase_c::updateErrorBitmap(long long seq) {
    long long index_in_list, offset_in_index;
    char bits[8] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080};
    if (peerswap)
        seq = swab32(seq);
    if (seq < 0 || (seq >> 3) >= sizeofErrorBitmap - 1) {
        if (!peerswap) {
            peerswap = true;
            if (verbose) fprintf(stderr, "peer has opposite endian-ness to ours, swapping seqno bytes\n");
        }
        seq = swab32(seq);
    }
    if (seq < 0 || (seq >> 3) >= sizeofErrorBitmap - 1) {
        fprintf(stderr, "sequence number 0x%llx out of range 0..%d\n",
                seq, sizeofErrorBitmap * 8 - 1);
        return;
    }
    // seq starts with 0
    index_in_list = seq >> 3;
    index_in_list++;
    offset_in_index = seq % 8;
    errorBitmap[index_in_list] |= bits[offset_in_index];
}


// return the count of errors
// The first byte is reserved to indicate if any packet's missing
int QUANTAnet_rbudpBase_c::updateHashTable() {
    int count = 0;
    int i, j;
    char bits[8] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080};

    for (i = 1; i < sizeofErrorBitmap; i++) {
        for (j = 0; j < 8; j++) {
            if ((errorBitmap[i] & bits[j]) == 0) {
                hashTable[count] = (i - 1) * 8 + j;
                count++;
            }
        }
    }
// set the first byte to let the sender know "all done"
    if (count == 0)
        errorBitmap[0] = 1;
    return count;
}

// Utility functions

/* Unconditionally swap bytes in a 32-bit value */
int QUANTAnet_rbudpBase_c::swab32(int val) {
    return (((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00)
            | ((val & 0xFF00) << 8) | ((val & 0xFF) << 24));
}

long long QUANTAnet_rbudpBase_c::htonll(long long lll) {
    long long nll = 0;
    unsigned char *cp = (unsigned char *) &nll;

    cp[0] = (lll >> 56) & 0xFF;
    cp[1] = (lll >> 48) & 0xFF;
    cp[2] = (lll >> 40) & 0xFF;
    cp[3] = (lll >> 32) & 0xFF;
    cp[4] = (lll >> 24) & 0xFF;
    cp[5] = (lll >> 16) & 0xFF;
    cp[6] = (lll >> 8) & 0xFF;
    cp[7] = (lll >> 0) & 0xFF;

    return nll;
}

long long QUANTAnet_rbudpBase_c::ntohll(long long nll) {
    unsigned char *cp = (unsigned char *) &nll;

    return ((long long) cp[0] << 56) |
           ((long long) cp[1] << 48) |
           ((long long) cp[2] << 40) |
           ((long long) cp[3] << 32) |
           ((long long) cp[4] << 24) |
           ((long long) cp[5] << 16) |
           ((long long) cp[6] << 8) |
           ((long long) cp[7] << 0);
}
