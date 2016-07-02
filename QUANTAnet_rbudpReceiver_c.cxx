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

#include "QUANTAnet_rbudpReceiver_c.hxx"

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

void QUANTAnet_rbudpReceiver_c::receive(void *buffer, int bufSize, int packetSize) {
    int done = 0;
    struct timeval curTime, startTime;

    gettimeofday(&curTime, NULL);
    startTime = curTime;
    initReceiveRudp(buffer, bufSize, packetSize);
    initErrorBitmap();
    while (!done) {
        // receive UDP packetsq		TRACE_DEBUG("receiving UDP packets");
        reportTime(curTime);

        udpReceive();

        reportTime(curTime);

        gettimeofday(&curTime, NULL);
        if (verbose > 1) TRACE_DEBUG("Current time: %d %ld", curTime.tv_sec, curTime.tv_usec);

        if (updateHashTable() == 0)
            done = 1;

        if (verbose) {
            float dt = (curTime.tv_sec - startTime.tv_sec) +
                       1e-6 * (curTime.tv_usec - startTime.tv_usec);
            int nerrors = updateHashTable();
            int got = packetSize * (totalNumberOfPackets - nerrors);
            float mbps = 1e-6 * 8 * got / (dt == 0 ? .01 : dt);
            TRACE_DEBUG("Error: %d, Loss rate: %.5f Got %d/%dK in %.2fs (%g Mbit/s)",
                        nerrors,
                        (double) nerrors / (double) totalNumberOfPackets,
                        (int) (got >> 10), (int) (bufSize >> 10), dt, mbps);
        }

        // Send back Error bitmap
        if (writen(tcpSockfd, errorBitmap, sizeofErrorBitmap) != sizeofErrorBitmap) {
            perror("tcp send");
            exit(1);
        }
    }
    free(errorBitmap);
    free(hashTable);
}

void QUANTAnet_rbudpReceiver_c::udpReceiveReadv() {
    int done;
    long long seqno, packetno;
    struct timeval timeout;
    fd_set rset;
    int maxfdpl;
    done = 0;
    packetno = 0;

    timeout.tv_sec = 100;
    timeout.tv_usec = 0;
#define QMAX(x, y) ((x)>(y)?(x):(y))
    maxfdpl = QMAX(udpSockfd, tcpSockfd) + 1;
    FD_ZERO(&rset);
    while (!done) {
        // These two FD_SET cannot be put outside the while, don't why though
        FD_SET(udpSockfd, &rset);
        FD_SET(tcpSockfd, &rset);
        select(maxfdpl, &rset, NULL, NULL, &timeout);

        // receiving a packet
        if (FD_ISSET(udpSockfd, &rset)) {
            // set seqno to expected sequence number
            seqno = hashTable[packetno];
            //fprintf(log, "expect %d\t", seqno);
            iovRecv[1].iov_base = (char *) mainBuffer + (seqno * payloadSize);
            iovRecv[1].iov_len = payloadSize;
            if (recvmsg(udpSockfd, &msgRecv, 0) < 0) {
                perror("recvmsg error\n");
                exit(1);
            }

            int rcvseq = ptohseq(recvHeader.seq);
            // make sure it is the expected packet
            if (rcvseq == seqno) {
                updateErrorBitmap(rcvseq);
                // next expected packet
                packetno++;
            }
                // if some packets lost before this packet, move this packet backward
            else if (rcvseq > seqno) {
                bcopy(iovRecv[1].iov_base, (char *) mainBuffer + (rcvseq * payloadSize), iovRecv[1].iov_len);
                updateErrorBitmap(rcvseq);
                // missed some packets, next expected packet
                do packetno++;
                while ((rcvseq != hashTable[packetno]) && (packetno <= remainNumberOfPackets));
                if (packetno > remainNumberOfPackets) {
                    fprintf(stderr, "recv error\n");
                }
                packetno++;
            }
            //TRACE_DEBUG("received %d -> %d", recvHeader.seq, rcvseq);
        }
            //receive end of UDP signal
        else if (FD_ISSET(tcpSockfd, &rset)) {
            done = 1;
            if (verbose) TRACE_DEBUG("received TCP signal");
            readn(tcpSockfd, (char *) &endOfUdp, sizeof(struct _endOfUdp));
        }
        else // time out
        {
            done = 1;
        }
    }
}

void QUANTAnet_rbudpReceiver_c::udpReceive() {
    int done, actualPayloadSize, retval;
    long long seqno;
    struct timeval start;
    char *msg = (char *) malloc(packetSize);
    struct timeval timeout;
    fd_set rset;
    int maxfdpl;
    float prog;
    int oldprog = 0;
    done = 0;
    seqno = 0;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
#define QMAX(x, y) ((x)>(y)?(x):(y))
    maxfdpl = QMAX(udpSockfd, tcpSockfd) + 1;
    FD_ZERO(&rset);
    gettimeofday(&start, NULL);
    while (!done) {
        // These two FD_SET cannot be put outside the while, don't why though
        FD_SET(udpSockfd, &rset);
        FD_SET(tcpSockfd, &rset);
        retval = select(maxfdpl, &rset, NULL, NULL, &timeout);

        // receiving a packet
        if (FD_ISSET(udpSockfd, &rset)) {
            int temp = recv(udpSockfd, msg, packetSize, 0);
            bcopy(msg, &recvHeader, sizeof(struct _rbudpHeader));
            seqno = ptohseq(recvHeader.seq);

            // If the packet is the last one,
            if (seqno < totalNumberOfPackets - 1) {
                actualPayloadSize = payloadSize;
            }
            else {
                actualPayloadSize = lastPayloadSize;
            }

            bcopy(msg + headerSize, (char *) mainBuffer + (seqno * payloadSize), actualPayloadSize);

            updateErrorBitmap(seqno);

            receivedNumberOfPackets++;
            prog = (float) receivedNumberOfPackets / (float) totalNumberOfPackets * 100;
            if ((int) prog > oldprog) {
                oldprog = (int) prog;
                if (oldprog > 100) oldprog = 100;
                if (progress != 0) {
                    fseek(progress, 0, SEEK_SET);
                    fprintf(progress, "%d\n", oldprog);
                }
            }

        }
            //receive end of UDP signal
        else if (FD_ISSET(tcpSockfd, &rset)) {
            done = 1;
            readn(tcpSockfd, (char *) &endOfUdp, sizeof(struct _endOfUdp));
        }
        else // time out
        {
            //done = 1;
        }
    }
    free(msg);

}

int QUANTAnet_rbudpReceiver_c::getstream(int tofd, int packetSize) {
    // Let sender know we're ready.
    if (writen(tcpSockfd, "\0", 1) != 1) {
        perror("tcp send");
        exit(1);
    }

    long long curSize = -1;
    char *buf = 0;
    int ok = SUCCESS;

    for (; ;) {

        long long nbufSize, bufSize;
        int n = readn(tcpSockfd, (char *) &nbufSize, sizeof(nbufSize));
        if (n < 0) {
            fprintf(stderr, "read error.\n");
            return (FAILED);
        }

        bufSize = ntohll(nbufSize);

        if (bufSize <= 0)
            break;

        if (verbose > 1) fprintf(stderr, "accepting %lld byte chunk\n", bufSize);

        if (buf == 0 && bufSize != curSize) {
            if (buf) free(buf);
            buf = (char *) malloc(bufSize);
            if (buf == 0) {
                fprintf(stderr, "QUANTAnet_rbudpReceiver_c::getstream: Couldn't malloc %lld bytes for buffer\n",
                        bufSize);
                ok = FAILED;
                break;
            }
            curSize = bufSize;
        }

        receive(buf, bufSize, packetSize);

        if (write(tofd, buf, bufSize) < bufSize) {
            fprintf(stderr, "QUANTAnet_rbudpReceiver_c::getstream: couldn't write %lld bytes\n", bufSize);
            ok = FAILED;
            break;
        }
    }
    if (buf != 0)
        free(buf);
    ::close(tofd);
    return (ok);
}

int QUANTAnet_rbudpReceiver_c::getfile(char *origFName, char *destFName, int packetSize) {
    // Send getfile message.
    if (writen(tcpSockfd, origFName, SIZEOFFILENAME) != SIZEOFFILENAME) {
        perror("tcp send");
        exit(1);
    }

    long long filesize;
    int n = readn(tcpSockfd, (char *) &filesize, sizeof(filesize));
    if (n < 0) {
        fprintf(stderr, "read error.\n");
        return (FAILED);
    }

    /* Can't use ntohl() on long longs! */
    filesize = ntohll(filesize);
    fprintf(stderr, "The size of the file is %lld.\n", filesize);

    int fd = open(destFName, O_RDWR | O_CREAT | O_TRUNC, 0666);
    ftruncate(fd, filesize);

    char *buf;
    buf = (char *) mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf == MAP_FAILED) {
        fprintf(stderr, "mmap failed.\n");
        return FAILED;
    }

    receive(buf, filesize, packetSize);

    munmap(buf, filesize);
    ::close(fd);

    return SUCCESS;
}

int QUANTAnet_rbudpReceiver_c::getfilelist(char *fileList, int packetSize) {
    FILE *fp = fopen(fileList, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error open file!\n");
        return -1;
    }

    char str[SIZEOFFILENAME];
    char *origFName, *destFName;
    while (fgets(str, SIZEOFFILENAME, fp)) {
        puts(str);
        origFName = strtok(str, " ");
        destFName = strtok(NULL, " ");
        if ((origFName != NULL) && (destFName != NULL)) {

            // Send getfile message.
            if (writen(tcpSockfd, origFName, SIZEOFFILENAME) != SIZEOFFILENAME) {
                perror("tcp send");
                exit(1);
            }

            long long filesize;
            int n = readn(tcpSockfd, (char *) &filesize, sizeof(filesize));
            if (n < 0) {
                fprintf(stderr, "read error.\n");
                return (FAILED);
            }

            filesize = ntohll(filesize);
            fprintf(stderr, "The size of the file is %lld.\n", filesize);

            int fd = open(destFName, O_RDWR | O_CREAT | O_TRUNC, 0666);
            ftruncate(fd, filesize);

            char *buf;
            buf = (char *) mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (buf == MAP_FAILED) {
                fprintf(stderr, "mmap failed.\n");
                return FAILED;
            }

            receive(buf, filesize, packetSize);

            munmap(buf, filesize);
            ::close(fd);
        }
    }

    // send all zero block to end the sender.
    bzero(str, SIZEOFFILENAME);
    if (writen(tcpSockfd, str, SIZEOFFILENAME) != SIZEOFFILENAME) {
        perror("Error Tcp Send");
        exit(1);
    }

    fclose(fp);
    return SUCCESS;
}

/* sendRate: Mbps */
int QUANTAnet_rbudpReceiver_c::initReceiveRudp(void *buffer, int bufSize, int pSize) {
    int i;
    mainBuffer = (char *) buffer;
    dataSize = bufSize;
    payloadSize = pSize;
    headerSize = sizeof(struct _rbudpHeader);
    packetSize = payloadSize + headerSize;
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
    receivedNumberOfPackets = 0;
    sizeofErrorBitmap = totalNumberOfPackets / 8 + 2;
    errorBitmap = (char *) malloc(sizeofErrorBitmap);
    hashTable = (long long *) malloc(totalNumberOfPackets * sizeof(long long));

    if (verbose > 1) TRACE_DEBUG("totalNumberOfPackets: %d", totalNumberOfPackets);

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


void QUANTAnet_rbudpReceiver_c::init(char *remoteHost) {
    int n;

#ifdef DEBUG
    log = fopen("rbudprecv.log","w");
#endif
    if (verbose > 2)
        progress = fopen("progress.log", "w");
    else
        progress = 0;

    passiveUDP(remoteHost);

    if (!hasTcpSock) {
        if (verbose) TRACE_DEBUG("try to connect the sender via TCP ...");
        n = connectTCP(remoteHost);
        if (n < 0) {
            fprintf(stderr, "connecting TCP failed, make sure the sender has been started\n");
            exit(1);
        }
        if (verbose) TRACE_DEBUG("tcp connected.");
    }

    msgRecv.msg_name = NULL;
    msgRecv.msg_namelen = 0;
    msgRecv.msg_iov = iovRecv;
    msgRecv.msg_iovlen = 2;

    iovRecv[0].iov_base = reinterpret_cast<char *>(&recvHeader);
    iovRecv[0].iov_len = sizeof(struct _rbudpHeader);
}


void QUANTAnet_rbudpReceiver_c::close() {
    if (!hasTcpSock)
        ::close(tcpSockfd);
    ::close(udpSockfd);
#ifdef DEBUG
    fclose(log);
#endif
    if (progress != 0)
        fclose(progress);
}
