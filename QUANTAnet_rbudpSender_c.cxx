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

#include "QUANTAnet_rbudpSender_c.hxx"

#include <stdarg.h>

// If you want to output debug info on terminals when running, put
//  fprintf(stderr, __VA_ARGS__);\
//  fprintf(stderr, "\n");\
// into TRACE_DEBUG definition.
// If you want to log the output into a log file, uncomment DEBUG, and put
//  fprintf(log, __VA_ARGS__);\
//  fprintf(log, "\n");\
// into TRACE_DEBUG definition.

//#define DEBUG

/*#define TRACE_DEBUG(...) do { \
  fprintf(stderr, __VA_ARGS__);\
  fprintf(stderr, "\n");\
  }  while(0)
*/

inline void TRACE_DEBUG(char *format, ...) {
    va_list arglist;
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    fprintf(stderr, "\n");
    va_end(arglist);
}

void QUANTAnet_rbudpSender_c::send(void *buffer, int bufSize, int sendRate, int packetSize) {
    int done = 0;
    struct timeval curTime, startTime;
    double srate;
    gettimeofday(&curTime, NULL);
    startTime = curTime;
    initSendRudp(buffer, bufSize, sendRate, packetSize);
    while (!done) {
        // blast UDP packets
        if (verbose > 1) TRACE_DEBUG("sending UDP packets");
        reportTime(curTime);
        udpSend();

        srate = (double) remainNumberOfPackets * payloadSize * 8 / (double) reportTime(curTime);
        if (verbose > 1) TRACE_DEBUG("real sending rate in this send is %f", srate);

        // send end of UDP signal
        if (verbose > 1) TRACE_DEBUG("send to socket %d an end signal.", tcpSockfd);
        if (verbose > 1) fprintf(stderr, "write %d bytes.\n", sizeof(endOfUdp));
        writen(tcpSockfd, (char *) &endOfUdp, sizeof(endOfUdp));
        endOfUdp.round++;

        reportTime(curTime);
        gettimeofday(&curTime, NULL);
        if (verbose > 1) TRACE_DEBUG("Current time: %d %ld", curTime.tv_sec, curTime.tv_usec);

        // receive error list
        if (verbose > 1) TRACE_DEBUG("waiting for error bitmap");

        int n = readn(tcpSockfd, errorBitmap, sizeofErrorBitmap);
        if (n < 0) {
            perror("read");
            exit(1);
        }

        if ((unsigned char) errorBitmap[0] == 1) {
            done = 1;
            remainNumberOfPackets = 0;
            if (verbose > 1) TRACE_DEBUG("done.");
        }
        else {
            remainNumberOfPackets = updateHashTable();
        }

        if (isFirstBlast) {
            isFirstBlast = 0;
            double lossRate = (double) remainNumberOfPackets / (double) totalNumberOfPackets;
            //	if (remainNumberOfPackets > 0)
            //	    usecsPerPacket = (int) ((double)usecsPerPacket / (1.0 - lossRate - 0.05));
            if (verbose > 0) {
                float dt = (curTime.tv_sec - startTime.tv_sec)
                           + 1e-6 * (curTime.tv_usec - startTime.tv_usec);
                float mbps = 1e-6 * 8 * bufSize / (dt == 0 ? .01 : dt);
                TRACE_DEBUG("loss rate: %f  on %dK in %.3f seconds (%.2f Mbits/s)",
                            lossRate, (int) bufSize >> 10, dt, mbps);
                if (verbose > 1) TRACE_DEBUG("usecsPerPacket updated to %d", usecsPerPacket);
            }
        }
    }
    free(errorBitmap);
    free(hashTable);
}

void QUANTAnet_rbudpSender_c::udpSendWritev() {
    int i, done, actualPayloadSize;
    struct timeval start, now;

    done = 0;
    i = 0;
    gettimeofday(&start, NULL);
    while (!done) {
        gettimeofday(&now, NULL);
        if (USEC(&start, &now) < usecsPerPacket * i) {
            // busy wait or sleep
            //		usleep(1);
        }
        else {
            // last packet is probably smaller than regular packets
            // we have to pad the last packet, make it the same length
            // as regular packets
            if (hashTable[i] < totalNumberOfPackets - 1) {
                actualPayloadSize = payloadSize;
            }
            else {
                actualPayloadSize = lastPayloadSize;
            }
            sendHeader.seq = hashTable[i];

            iovSend[1].iov_base = (char *) mainBuffer + (sendHeader.seq * payloadSize);
            iovSend[1].iov_len = actualPayloadSize;
            if (verbose > 1) fprintf(stderr, "sent %d, %d %d\n", sendHeader.seq, now.tv_sec, now.tv_usec);
            if (sendmsg(udpSockfd, &msgSend, 0) < 0) {
                perror("sendmsg");
                exit(1);
            }
            i++;
            if (i >= remainNumberOfPackets)
                done = 1;
        }
    }
}

void QUANTAnet_rbudpSender_c::udpSend() {
    int i, done, actualPayloadSize;
    struct timeval start, now;
    char *msg = (char *) malloc(packetSize);

    done = 0;
    i = 0;
    gettimeofday(&start, NULL);
    while (!done) {
        gettimeofday(&now, NULL);
        if (USEC(&start, &now) < usecsPerPacket * i) {
            // busy wait or sleep
            //		usleep(1);
        }
        else {
            // last packet is probably smaller than regular packets
            // we have to pad the last packet, make it the same length
            // as regular packets
            if (hashTable[i] < totalNumberOfPackets - 1) {
                actualPayloadSize = payloadSize;
            }
            else {
                actualPayloadSize = lastPayloadSize;
            }
            sendHeader.seq = hashTable[i];
            bcopy(&sendHeader, msg, headerSize);
            bcopy((char *) ((char *) mainBuffer + (sendHeader.seq * payloadSize)), msg + headerSize, actualPayloadSize);

            //TRACE_DEBUG("sent %d, packet size: %d", sendHeader.seq, actualPayloadSize+headerSize);
            if (sendto(udpSockfd, msg, actualPayloadSize + headerSize, 0, (const struct sockaddr *) &udpServerAddr,
                       sizeof(udpServerAddr)) < 0) {
                perror("sendto");
                exit(1);
            }
            i++;
            if (i >= remainNumberOfPackets)
                done = 1;
        }
    }
    free(msg);
}

int QUANTAnet_rbudpSender_c::sendstream(int fromfd, int sendRate, int packetSize, int bufSize) {
    // Receive startup message
    char ack[1];
    int n = readn(tcpSockfd, ack, 1);
    if (n < 0) {
        fprintf(stderr, "stream ack read error.\n");
        return (FAILED);
    }


    // allocate buffer
    char *buf = (char *) malloc(bufSize);
    if (buf == 0) {
        fprintf(stderr, "QUANTAnet_rbudpSender_c::sendstream: Couldn't malloc %d bytes for buffer\n", bufSize);
        return (FAILED);
    }

    long long bytesread;

    while ((bytesread = readn(fromfd, buf, bufSize)) > 0) {
        long long nread = htonll(bytesread);
        if (writen(tcpSockfd, (char *) &nread, sizeof(nread)) != sizeof(nread)) {
            {
                fprintf(stderr, "tcp send failed.\n");
                free(buf);
                return (FAILED);
            }
        }
        if (verbose > 1) fprintf(stderr, "sending %lld bytes\n", bytesread);
        send(buf, bytesread, sendRate, packetSize);
    }
    ::close(fromfd);
    free(buf);
    return ((bytesread == 0) ? SUCCESS /*clean EOF*/ : FAILED /*error*/);
}

int QUANTAnet_rbudpSender_c::sendfile(int sendRate, int packetSize) {
    // Receive the getfile message
    char fname[SIZEOFFILENAME];
    int n = readn(tcpSockfd, fname, SIZEOFFILENAME);
    if (n < 0) {
        fprintf(stderr, "read error.\n");
        return (FAILED);
    }

    fprintf(stderr, "Send file %s\n", fname);

    struct stat filestat;
    if (stat(fname, &filestat) < 0) {
        fprintf(stderr, "stat error.\n");
        return (FAILED);
    }

    long long filesize = filestat.st_size;
    fprintf(stderr, "The size of the file is %lld\n", filesize);

    long long nfilesize = htonll(filesize);
    fprintf(stderr, "write %d bytess.\n", sizeof(nfilesize));

    // Send the file size to the receiver.
    if (writen(tcpSockfd, (char *) &nfilesize, sizeof(nfilesize)) != sizeof(nfilesize)) {
        {
            fprintf(stderr, "tcp send failed.\n");
            return (FAILED);
        }
    }

    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open file failed.\n");
        return (FAILED);
    }
    char *buf = (char *) mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);

    send(buf, filesize, sendRate, packetSize);

    munmap(buf, filesize);
    ::close(fd);

    return (SUCCESS);
}

int QUANTAnet_rbudpSender_c::sendfilelist(int sendRate, int packetSize) {
    // Receive the getfile message
    char fname[SIZEOFFILENAME];
    int isFinished = 0;

    while (!isFinished) {
        int n = readn(tcpSockfd, fname, SIZEOFFILENAME);
        if (n <= 0) {
            fprintf(stderr, "read error.\n");
            return (FAILED);
        }

        // If not "finish" signal (All zero), continue to send.
        char test[SIZEOFFILENAME];
        bzero((void *) test, SIZEOFFILENAME);
        if (strcmp(test, fname) != 0) {

            fprintf(stderr, "Send file %s\n", fname);

            struct stat filestat;
            if (stat(fname, &filestat) < 0) {
                fprintf(stderr, "stat error.\n");
                return (FAILED);
            }

            long long filesize = filestat.st_size;
            fprintf(stderr, "The size of the file is %d\n", filesize);

            long long nfilesize = htonll(filesize);

            // Send the file size to the receiver.
            if (writen(tcpSockfd, (char *) &nfilesize, sizeof(nfilesize)) != sizeof(nfilesize)) {
                {
                    fprintf(stderr, "tcp send failed.\n");
                    return (FAILED);
                }
            }

            int fd = open(fname, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "open file failed.\n");
                return (FAILED);
            }
            char *buf = (char *) mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);

            send(buf, filesize, sendRate, packetSize);

            munmap(buf, filesize);
            ::close(fd);
        }
        else
            isFinished = 1;
    }
    return (SUCCESS);
}

/* sendRate: Kbps */
int QUANTAnet_rbudpSender_c::initSendRudp(void *buffer, int bufSize, int sRate, int pSize) {
    int i;
    mainBuffer = (char *) buffer;
    dataSize = bufSize;
    sendRate = sRate;
    payloadSize = pSize;
    headerSize = sizeof(struct _rbudpHeader);
    packetSize = payloadSize + headerSize;
    usecsPerPacket = 8 * payloadSize * 1000 / sendRate;
    isFirstBlast = 1;

    if (dataSize % payloadSize == 0) {
        totalNumberOfPackets = dataSize / payloadSize;
        lastPayloadSize = payloadSize;
    }
    else {
        totalNumberOfPackets = dataSize / payloadSize + 1; /* the last packet is not full */
        lastPayloadSize = dataSize - payloadSize * (totalNumberOfPackets - 1);
    }

    remainNumberOfPackets = totalNumberOfPackets;
    sizeofErrorBitmap = totalNumberOfPackets / 8 + 2;
    errorBitmap = (char *) malloc(sizeofErrorBitmap);
    hashTable = (long long *) malloc(totalNumberOfPackets * sizeof(long long));

    endOfUdp.round = 0;
    strncpy(endOfUdp.end, "EEE", 3);

    if (verbose > 1) TRACE_DEBUG("totalNumberOfPackets: %d", totalNumberOfPackets);
    if (verbose > 1) TRACE_DEBUG("usecsPerPacket: %d", usecsPerPacket);

    if (errorBitmap == NULL) {
        fprintf(stderr, "malloc errorBitmap failed\n");
        return (-1);
    }
    if (hashTable == NULL) {
        fprintf(stderr, "malloc hashTable failed\n");
        return (-1);
    }

    /* Initialize the hash table */
    for (i = 0; i < totalNumberOfPackets; i++) {
        hashTable[i] = i;
    }
    return 0;
}


void QUANTAnet_rbudpSender_c::init(char *remoteHost) {
    openSockets(remoteHost);
    listenAndInit();
}

void QUANTAnet_rbudpSender_c::openSockets(char *remoteHost) {
#ifdef DEBUG
    log = fopen("rbudpsend.log", "w");
#endif
    connectUDP(remoteHost);
    if (!hasTcpSock) {
        initTCPServer();
    }
}

void QUANTAnet_rbudpSender_c::listenAndInit() {
    if (!hasTcpSock) {
        listenTCPServer();
    }

#if defined (__sgi)
    //	msgSend.msg_name = (caddr_t)&udpServerAddr.sin_addr;
            msgSend.msg_name = (caddr_t)&udpServerAddr;
#else
//        msgSend.msg_name = (void *)&udpServerAddr.sin_addr;
    msgSend.msg_name = reinterpret_cast<char *>(&udpServerAddr);
#endif
    msgSend.msg_namelen = sizeof(udpServerAddr);
    msgSend.msg_iov = iovSend;
    msgSend.msg_iovlen = 2;

    iovSend[0].iov_base = reinterpret_cast<char *>(&sendHeader);
    iovSend[0].iov_len = sizeof(struct _rbudpHeader);
}

void QUANTAnet_rbudpSender_c::close() {
    if (!hasTcpSock) {
        ::close(tcpSockfd);
        ::close(listenfd);
    }
    ::close(udpSockfd);
#ifdef DEBUG
    fclose(log);
#endif
}

