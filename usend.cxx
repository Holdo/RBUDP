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

char *prog;

int getnumber(char *str, char *why, int vmin, int vmax, float dscale = 1, float vscale = 1.0f) {
    int iv;
    float v;
    int ok = 1;
    int c = '\0';
    if (sscanf(str, "%f%c", &v, &c) > 0) {
        v *= vscale;
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
                iv = (int) (v * dscale);
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
    int verbose = 1;
    prog = argv[0];

    if (argc > 5 && 0 == strcmp(argv[1], "-q"))
        verbose = 0, argc--, argv++;

    if (argc > 5 && 0 == strcmp(argv[1], "-v"))
        verbose++, argc--, argv++;

    if (argc < 6) {
        printf
                ("Usage: usend [-q|-v] <receiver> <bytes-to-transfer[k|m|g]> <send-rate(Kbit/s unless k|m|g suffix)> <packetsize> <numITERs> [<port>]\n\
Tests memory-to-memory speed of RBUDP block network transfers.\n\
Start \"usend\" first, then \"urecv\".  \"-q\"=> quiet, \"-v\"=>verbose.  E.g.:\n\
  (on hostA)   usend  hostB  256m  800m  1460  4\n\
  (on hostB)   urecv  hostA  256m  1460  4\n\
transfers a 256-megabyte block at 800 megabits/sec, four times,\n\
using 1460-byte UDP packets (to fit in 1500-byte Ethernet MTU),\n\
averaging network performance across the four attempts.\n");
        exit(1);
    }

    int size = getnumber(argv[2], "transfer size (bytes)", 8, ((unsigned int) 1) << 31 - 1);
    int sendRate = getnumber(argv[3], "sending bit rate (kbits/s unless k/m/g suffix)", 0, 1 << 30, 1000, 0.001);
    int mtu = getnumber(argv[4], "UDP payload data size (e.g. 1460 or 8800)", 8, 100000);
    int iter = getnumber(argv[5], "repetition count (sends count * transfersize bytes total)", 0, 1 << 30);
    int port = argc <= 6 ? 38000 : getnumber(argv[6], "UDP/TCP port (default 38000)", 1, 65535);
    buf = (char *) malloc(size);
    if (buf == NULL) {
        fprintf(stderr, "usend: can't allocate %d bytes for buffer.\n", size);
        exit(1);
    }
    // Initialize the buffer content in order to verify the protocol
    for (int i = 0; i < size / sizeof(int); i++)
        ((int *) buf)[i] = i;

    printf(" Creating sender Obj\n");

    QUANTAnet_rbudpSender_c *mysender = new QUANTAnet_rbudpSender_c(port);
    printf(" Sender To bbe initialized \n");
    mysender->init(argv[1]);
    mysender->setverbose(verbose);

    printf(" Sender has initialized \n");

    for (int j = 0; j < iter; j++)
        mysender->send(buf, size, sendRate, mtu);

    mysender->close();
    return 1;
}
