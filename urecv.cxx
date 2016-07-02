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

char *prog;

int getnumber(char *str, char *why, int vmin, int vmax) {
    int iv;
    float v;
    int ok = 1;
    int c = '\0';
    if (sscanf(str, "%f%c", &v, &c) > 0) {
        switch (c) {
            case 'k':
            case 'K':
                iv = (int) (v * (1 << 10));
                break;
            case 'm':
            case 'M':
                iv = (int) (v * (1 << 20));
                break;
            case 'g':
            case 'G':
                iv = (int) (v * (1 << 30));
                break;
            case '\0':
                iv = (int) v;
                break;
            default:
                ok = 0;
                break;
        }
    } else {
        ok = 0;
    }
    if (ok) {
        if (vmin < vmax && !(vmin <= iv && iv <= vmax)) {
            fprintf(stderr, "%s: %s: value '%s' = %d out of range %d..%d\n",
                    prog, why, str, iv, vmin, vmax);
            exit(1);
        } else {
            return iv;
        }
    }
    fprintf(stderr, "%s: %s: expected number (possibly with k/m/g suffix), not '%s'\n",
            prog, why, str);
    exit(1);
}

int
main(int argc, char **argv) {
    char *buf;
    int i;
    double totlen;
    struct timeval start, end;
    double throughput;
    int verbose = 1;
    prog = argv[0];

    if (argc > 5 && 0 == strcmp(argv[1], "-q"))
        verbose = 0, argc--, argv++;

    if (argc > 5 && 0 == strcmp(argv[1], "-v"))
        verbose++, argc--, argv++;

    if (argc < 5) {
        printf
                ("Usage: urecv [-q|-v] <sender> <bytes-to-transfer[k|m|g]> <packetsize> <ITER> [<port>(38000)]\n\
Test memory-to-memory network performance of RBUDP transfers.\n\
Soak up RBUDP test data sent by \"usend\".\n\
Start \"usend\" first, then \"urecv\".  \"-q\"=> quiet, \"-v\"=>verbose.  E.g.:\n\
  (on hostA)   usend  hostB  256m  800m  1460  4\n\
  (on hostB)   urecv  hostA  256m  1460  4\n\
transfers a 256-megabyte block at 800 megabits/sec, four times,\n\
using 1460-byte UDP packets (to fit in 1500-byte Ethernet MTU),\n\
averaging network performance across the four attempts.\n");
        exit(1);
    }


    char *sender = argv[1];
    int bigsize = getnumber(argv[2], "transfer size (bytes)", 8, (((unsigned int) 1) << 31) - 1);
    int mtu = getnumber(argv[3], "UDP packet payload size (bytes)", 8, 32768);
    int iter = getnumber(argv[4], "Number of iterations", 1, 1 << 30);
    int port = (argc <= 5) ? 38000 : getnumber(argv[5], "TCP/UDP port number", 1, 65535);

    buf = (char *) malloc(bigsize);
    if (buf == NULL) {
        printf("urecv: memory allocation fail.\n");
        exit(1);
    }

    QUANTAnet_rbudpReceiver_c *myreceiver =
            new QUANTAnet_rbudpReceiver_c(port);
    myreceiver->init(sender);
    myreceiver->setverbose(verbose);

    printf("init finishes\n");
    gettimeofday(&start, NULL);
    for (int j = 0; j < iter; j++)
        myreceiver->receive(buf, bigsize, mtu);
    gettimeofday(&end, NULL);
    myreceiver->close();
    totlen = bigsize * 8.0 * iter;

    /*
       // verify if the protocol transferred the exact data
       for (i=0;i<size/4;i++)
       if (*(int*)(buf+i*4) != i)
       {
       printf("Verification failed on %d: %d!\n",i, *(int*)(buf+i*4));
       exit(1);
       }
     */



    throughput = ((double) (totlen) / (double) USEC (&start, &end));
    printf("throughput: %f Mbps\n", throughput);
    return 0;
}
