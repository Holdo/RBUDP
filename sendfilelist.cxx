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

int main(int argc, char **argv) {
    int sendRate;

    if (argc < 5) {
        printf
                ("Usage: sendfilelist <receiver> <sending rate (Kbps)> <port> <MTU> \n");
        exit(1);
    }
    int port = atoi(argv[3]);
    sendRate = atoi(argv[2]);
    int packetsize = atoi(argv[4]);

    QUANTAnet_rbudpSender_c *mysender = new QUANTAnet_rbudpSender_c(port);
    mysender->init(argv[1]);

    mysender->sendfilelist(sendRate, packetsize);

    mysender->close();

    return 1;
}
