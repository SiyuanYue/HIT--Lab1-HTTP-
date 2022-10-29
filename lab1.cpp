#include "HttpProxy.h"
#include "HttpHeader.h"

//Http ��Ҫͷ������
bool InitWinSock();
//����˿ں�
const int ProxyPort = 8080;
int main() {
    printf("�����������������\n");
    printf("��ʼ��...\n");
    if (!InitWinSock()) {
        printf("WinSock����ʧ��\n");
        return -1;
    }
    HttpProxy httpProxy(AF_INET, SOCK_STREAM, 0, ProxyPort);
    httpProxy.Listening();
    return 0;
}
bool InitWinSock() {
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    //�汾 2.2
    wVersionRequested = MAKEWORD(2, 2);
    //���� dll �ļ� Socket ��
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
    //�Ҳ��� winsock.dll
        printf("���� winsock ʧ�ܣ��������Ϊ: %d\n", WSAGetLastError());
        return FALSE;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        printf("�����ҵ���ȷ�� winsock �汾\n");
        WSACleanup();
        return FALSE;
    }
    return true;
}