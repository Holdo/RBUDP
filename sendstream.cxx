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

#include <getopt.h>
#include "QUANTAnet_rbudpSender_c.hxx"

char *prog;

int getnumber(char *str, char const *why, int vmin, int vmax, float dscale = 1, float vscale = 1.0f) {
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
    int sendRate;
    int port = -1;
    int verbose = 1;
    float tv = 0;
    int mtu;
    char suf = '\0';
    int c;
    int bufsize = 64 * 1024 * 1024;

    prog = argv[0];

    while ((c = getopt(argc, argv, "qvp:b:")) != EOF) {
        switch (c) {
            case 'v':
                verbose++;
                break;
            case 'q':
                verbose = 0;
                break;
            case 'p':
                port = getnumber(optarg, "UDP/TCP port number (default 38000)", 1, 65535);
                break;
            case 'b':
                bufsize = getnumber(optarg, "Data block size (default 64m)", 8, 1 << 30);
                break;
            default:
                argc = 0;
                break;
        }
    }

    if (argc < optind + 3) {
        fprintf(stderr, "Usage: somesource | sendstream [options] <receiver> <sending rate (Kbps)> <packetsize>\n\
Numbers may have 'k'/'m'/'g' suffix.  Example:\n\
    tar cf - somedirectory | sendstream  128.101.25.35  600m  1460\n\
Send rate in kbits/sec if no suffix given.\n\
\"packetsize\" is actually max UDP payload size;\n\
should be >= 40 bytes smaller than interface MTU, e.g. 1460 or 8800.\n\
Options:\n\
    -v		Verbose messages\n\
    -q		No messages (errors only)\n\
    -b <bufsize> Transfer in blocks of <bufsize>, default 64m bytes\n\
    -p <port>	Use UDP/TCP <port> rather than 38000.\n\
Firewall: sendstream listens on both UDP and TCP port 38000 (or <port>);\n\
	recvstream listens on UDP port 38001 (or <port>+1)\n\n\
Start \"sendstream\" first, then \"recvstream\".  Example:\n\
   on sourcehost:   tar cf - ... | sendstream destIPaddr 250m 1460\n\
then on desthost:   recvstream sourceIPaddr 1460 | tar xf -\n");
        exit(1);
    }

    sendRate = getnumber(argv[optind + 1], "sending bit rate (kbits/s unless suffix)", 0, 1 << 30, 1000, 0.001);

    mtu = getnumber(argv[optind + 2], "UDP payload data size (e.g. 1460, 8800)", 8, 100000);

    if (port <= 0)
        port = 38000;

    QUANTAnet_rbudpSender_c *mysender = new QUANTAnet_rbudpSender_c(port);
    mysender->init(argv[optind]);
    mysender->setverbose(verbose);

    mysender->sendstream(0, sendRate, mtu, bufsize);

    mysender->close();

    return 1;
}
