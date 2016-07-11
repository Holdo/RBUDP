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
#include "QUANTAnet_rbudpReceiver_c.hxx"

char *prog;

int getnumber(char *str, char const *why, int vmin, int vmax, int dscale = 1) {
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
    struct timeval start, end;
    double throughput;
    int port = -1;
    int verbose = 1;
    int mtu;
    char c;

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
            default:
                argc = 0;
                break;
        }
    }

    if (argc < optind + 2) {
        fprintf(stderr, "Usage: recvstream [-v] [-q] [-p <port>] <receiver> <packetsize>\n\
Receive a stream of RBUDP data sent by \"sendstream\", write it to standard output.\n\
The <packetsize> (UDP payload data size) must be same as given to sendstream.\n\
Use \"-v\" for verbose messaging, \"-q\" for quiet (errors-only),\n\
  \"-p <port>\" to set port number (default 38000).\n\
Firewalling: sendstream listens on both UDP and TCP port 38000 (or <port>);\n\
	recvstream listens on UDP port 38001 (or <port>+1)\n\n\
Start \"sendstream\" first, then \"recvstream\".  Example, sending at 250 Mbit/s:\n\
   on sourcehost:   tar cf - ... | sendstream destIPaddr 250m 1460\n\
then on desthost:   recvstream sourceIPaddr 1460 | tar xf -\n");
        exit(1);
    }

    mtu = getnumber(argv[optind + 1], "UDP data payload size (bytes, e.g. 1460 or 8800)", 8, 100000);
    if (port <= 0)
        port = 38000;

    QUANTAnet_rbudpReceiver_c *myreceiver = new QUANTAnet_rbudpReceiver_c(port);

    myreceiver->init(argv[optind]);
    myreceiver->setverbose(verbose);

    if (verbose > 0)
        fprintf(stderr, "init finishes\n");
    gettimeofday(&start, NULL);
    myreceiver->getstream(1, mtu);
    gettimeofday(&end, NULL);
    myreceiver->close();

    if (verbose > 0)
        fprintf(stderr, "time consumed: %d microseconds\n", USEC (&start, &end));
    return 0;
}
