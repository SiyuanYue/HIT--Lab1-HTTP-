//
// Created by me on 2022/10/14.
//

#include "ProxyServerSocket.h"

bool ProxyServerSocket::setProxyAddr(short sinfamily, short addrs) {
    ProxySeverAddr.sin_family = sinfamily;
    ProxySeverAddr.sin_port = htons(this->proxyport);
    ProxySeverAddr.sin_addr.S_un.S_addr = addrs;
    if(bind(this->proxyseversocket, (SOCKADDR*)&ProxySeverAddr, sizeof(SOCKADDR)) == SOCKET_ERROR){
        printf("���׽���ʧ��\n");
        return FALSE;
    }
    return true;
}

void ProxyServerSocket::listenclient() const {
    if(listen(this->proxyseversocket, SOMAXCONN) == SOCKET_ERROR){
        printf("�����˿�%d ʧ��",this->proxyseversocket);
        exit(1);
    }
}
