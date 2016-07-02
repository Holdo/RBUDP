CXXFLAGS= -O3 

PROGRAMS = sendfile recvfile usend urecv sendfilelist recvfilelist sendstream recvstream

all: ${PROGRAMS}


sendfile: sendfile.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpSender_c.cxx
	g++ ${CXXFLAGS} -o sendfile sendfile.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpSender_c.cxx

recvfile: recvfile.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpReceiver_c.cxx
	g++ ${CXXFLAGS} -o recvfile recvfile.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpReceiver_c.cxx

sendfilelist: sendfilelist.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpSender_c.cxx
	g++ ${CXXFLAGS} -o sendfilelist sendfilelist.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpSender_c.cxx

recvfilelist: recvfilelist.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpReceiver_c.cxx
	g++ ${CXXFLAGS} -o recvfilelist recvfilelist.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpReceiver_c.cxx

usend: usend.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpSender_c.cxx
	g++ ${CXXFLAGS} -o usend usend.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpSender_c.cxx -lpthread

urecv: urecv.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpReceiver_c.cxx
	g++ ${CXXFLAGS} -o urecv urecv.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpReceiver_c.cxx

sendstream: sendstream.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpSender_c.cxx
	g++ ${CXXFLAGS} -o sendstream sendstream.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpSender_c.cxx

recvstream: recvstream.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpReceiver_c.cxx
	g++ ${CXXFLAGS} -o recvstream recvstream.cxx QUANTAnet_rbudpBase_c.cxx QUANTAnet_rbudpReceiver_c.cxx

clean:
	rm -f *.o ${PROGRAMS}


