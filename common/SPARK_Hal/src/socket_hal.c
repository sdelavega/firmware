

#include "socket_hal.h"
#include "cc3000_spi.h"
#include "socket.h"

int32_t socket_connect(sock_handle_t sd, const sockaddr *addr, long addrlen)
{
    return connect(sd, addr, addrlen);
}

sock_result_t socket_reset_blocking_call() 
{
    //Work around to exit the blocking nature of socket calls
    tSLInformation.usEventOrDataReceived = 1;
    tSLInformation.usRxEventOpcode = 0;
    tSLInformation.usRxDataPending = 0;
    return 0;
}

sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout)
{
  timeval timeout;
  _types_fd_set_cc3000 readSet;
  int bytes_received = 0;
  int num_fds_ready = 0;

  // reset the fd_set structure
  FD_ZERO(&readSet);
  FD_SET(sd, &readSet);

  // tell select to timeout after the minimum 5000 microseconds
  // defined in the SimpleLink API as SELECT_TIMEOUT_MIN_MICRO_SECONDS
  timeout.tv_sec = _timeout/1000;    
  timeout.tv_usec = (_timeout%1000)*1000;
  if (timeout.tv_sec==0 && timeout.tv_usec<5000)
      timeout.tv_usec = 5000;
  
  num_fds_ready = select(sd + 1, &readSet, NULL, NULL, &timeout);

  if (0 < num_fds_ready)
  {
    if (FD_ISSET(sd, &readSet))
    {
      // recv returns negative numbers on error
      bytes_received = recv(sd, buffer, len, 0);
      DEBUG("bytes_received %d",bytes_received);
    }
  }
  else if (0 > num_fds_ready)
  {
    // error from select
    DEBUG("select Error %d",num_fds_ready);
    return num_fds_ready;
  }
  return bytes_received;
}

sock_result_t socket_create_nonblocking_server(sock_handle_t sock, uint16_t port) {
    long optval = SOCK_ON;
    sock_result_t retVal;
    sockaddr tServerAddr;

    tServerAddr.sa_family = AF_INET;
    tServerAddr.sa_data[0] = (port & 0xFF00) >> 8;
    tServerAddr.sa_data[1] = (port & 0x00FF);
    tServerAddr.sa_data[2] = 0;
    tServerAddr.sa_data[3] = 0;
    tServerAddr.sa_data[4] = 0;
    tServerAddr.sa_data[5] = 0;

    ((retVal=setsockopt(sock, SOL_SOCKET, SOCKOPT_ACCEPT_NONBLOCK, &optval, sizeof(optval))) >= 0) &&
        ((retVal=bind(sock, (sockaddr*)&tServerAddr, sizeof(tServerAddr))) >= 0) &&
            ((retVal=listen(sock, 0)) >=0);
    return retVal;
}

sock_result_t socket_receivefrom(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr* addr, socklen_t* addrsize) {
    _types_fd_set_cc3000 readSet;
    timeval timeout;

    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);

    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;

    int ret = -1;
    if (select(sock + 1, &readSet, NULL, NULL, &timeout) > 0)
    {
        if (FD_ISSET(sock, &readSet))
        {
            ret = socket_receivefrom(sock, buffer, bufLen, 0, addr, addrsize);
        }
    }
    return ret;
}

sock_result_t socket_bind(sock_handle_t sock, uint16_t port) 
{           
    sockaddr tUDPAddr;
    memset(&tUDPAddr, 0, sizeof(tUDPAddr));
    tUDPAddr.sa_family = AF_INET;
    tUDPAddr.sa_data[0] = (port & 0xFF00) >> 8;
    tUDPAddr.sa_data[1] = (port & 0x00FF);

    return bind(sock, &tUDPAddr, sizeof(tUDPAddr));
}

sock_result_t socket_accept(sock_handle_t sock) 
{    
    sockaddr tClientAddr;
    socklen_t tAddrLen = sizeof(tClientAddr);
    return accept(sock, &tClientAddr, &tAddrLen);
}


const sock_handle_t SOCKET_MAX = (sock_handle_t)8;
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;