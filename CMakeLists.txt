cmake_minimum_required(VERSION 3.5)
project(RBUDP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    QUANTAnet_rbudpBase_c.cxx
    QUANTAnet_rbudpBase_c.hxx
    QUANTAnet_rbudpReceiver_c.cxx
    QUANTAnet_rbudpReceiver_c.hxx
    QUANTAnet_rbudpSender_c.cxx
    QUANTAnet_rbudpSender_c.hxx
    recvfile.cxx
    recvfilelist.cxx
    recvstream.cxx
    sendfile.cxx
    sendfilelist.cxx
    sendstream.cxx
    urecv.cxx
    usend.cxx)

add_executable(RBUDP ${SOURCE_FILES})