//
// Created by me on 2022/10/14.
//

#include <cassert>
#include "HttpProxy.h"
namespace fs = std::filesystem;
std::string& replace_all(std::string& src, const std::string& old_value, const std::string& new_value);
static BOOL ConnectToServer(SOCKET *serverSocket, char *host);
static std::set<std::string> UserFilters{};
static std::set<std::string> WebFilters{};
static std::unordered_map<std::string,std::string> FishFilters{};
static void setfish()
{
    FishFilters["www.hit.edu.cn"]="oa.hit.edu.cn";
    FishFilters["jwts.hit.edu.cn"]="news.hit.edu.cn";
}
static void setFilters()
{
    WebFilters.insert("today.hit.edu.cn");
}
[[noreturn]] void HttpProxy::Listening() const {
    ProxyParam *lpProxyParam;
    SOCKET clientSocket;
    while (true) {
        clientSocket = accept(this->ProxySever.proxyseversocket, nullptr, nullptr);
        lpProxyParam = new ProxyParam();
        lpProxyParam->clientSocket = clientSocket;
        //�½��߳�
        HANDLE createThread = CreateThread(
                nullptr,
                0,
                Proxythread,
                (void *) lpProxyParam,
                0, //��������������
                nullptr);
        CloseHandle(createThread);
        Sleep(100);
    }
    closesocket(this->ProxySever.proxyseversocket);
    WSACleanup();
}

DWORD WINAPI HttpProxy::Proxythread(void *lpproxypram) {
    //TODO cache �����������������
    setfish();
    setFilters();
    char buffer[65536];
    memset(buffer, 0, 65536);
    auto *tlpproxypram = (ProxyParam *) lpproxypram;
    int recvSize = recv(tlpproxypram->clientSocket, buffer, 65535, 0);
    if (recvSize < 0) {
        std::cout << "recv client len <0" << std::endl;
        closesocket(tlpproxypram->clientSocket);
        closesocket(tlpproxypram->serverSocket);
        _endthreadex(1);
    }
    std::string buffer2(buffer);
    auto *http_header = new HttpHeader();
    HttpProxy::ParseHttpHead((char *) buffer2.c_str(), http_header);
    if(websitefilter(http_header))
    {
        printf("�ر��׽���\n");
        closesocket(tlpproxypram->clientSocket);
        closesocket(tlpproxypram->serverSocket);
        delete tlpproxypram;
        _endthreadex(1);
    }
    //FISHING
    std::string rebuffer(buffer);
    //std::cout<<FishFilters.contains(std::string(http_header->host))<<"  strlen(host)= "<<strlen(http_header->host)<<std::endl;
    if(FishFilters.contains(http_header->host))
    {
        std::string oldhost(http_header->host);
        std::string newhost(FishFilters[http_header->host]);
        strcpy(http_header->host,newhost.c_str());
        replace_all(rebuffer,oldhost,newhost);
    }
    if (!ConnectToServer(&tlpproxypram->serverSocket, http_header->host)) {
        printf("�ر��׽���\n");
        closesocket(tlpproxypram->clientSocket);
        closesocket(tlpproxypram->serverSocket);
        delete tlpproxypram;
        _endthreadex(1);
    }
    printf("������������ %s �ɹ�\n", http_header->host);
    HttpProxy::cache(tlpproxypram,http_header,rebuffer.c_str());
////���ͻ��˷��͵� HTTP ���ݱ���ֱ��ת����Ŀ�������
//    send(tlpproxypram->serverSocket, buffer, (int)strlen(buffer) + 1, 0);
////�ȴ�Ŀ���������������
//    recvSize = recv(tlpproxypram->serverSocket, buffer, 65536, 0);
//    if (recvSize < 0) {
//        goto error;
//    }
////��Ŀ����������ص�����ֱ��ת�����ͻ���
//    send(tlpproxypram->clientSocket, buffer, recvSize, 0);//recvSize!!!
////������
//    error:
    printf("�ر��׽���\n");
    Sleep(100);
    closesocket(tlpproxypram->clientSocket);
    closesocket(tlpproxypram->serverSocket);
    delete tlpproxypram;
    _endthreadex(0);
}

void HttpProxy::ParseHttpHead(char *buffer, HttpHeader *httpHeader) {
    char *p;
    char *ptr;
    const char *delim = "\r\n";
    p = strtok_s(buffer, delim, &ptr);//��ȡ��һ��
    if(p== nullptr)
    {
        return;
    }
    //printf("%s\n", p);
    if (p[0] == 'G') {//GET ��ʽ
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) - 13);
    } else if (p[0] == 'P') {//POST ��ʽ
        memcpy(httpHeader->method, "POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }
    //printf("%s\n", httpHeader->url);
    p = strtok_s(nullptr, delim, &ptr);
    while (p) {
        switch (p[0]) {
            case 'H'://Host
                memcpy(httpHeader->host, &p[6], strlen(p) - 6);
                break;
            case 'C'://Cookie
                if (strlen(p) > 8) {
                    char header[8];
                    ZeroMemory(header, sizeof(header));
                    memcpy(header, p, 6);
                    if (!strcmp(header, "Cookie")) {
                        memcpy(httpHeader->cookie, &p[8], strlen(p) - 8);
                    }
                }
                break;
            default:
                break;
        }
        p = strtok_s(nullptr, delim, &ptr);
    }
}

void HttpProxy::cache(ProxyParam *proxyParam, HttpHeader *httpHeader, const char *request_buffer) {
    //std::cout<<"cache:"<<std::endl;
    //std::cout<<absolute(fs::path("."))<<std::endl;
    fs::path Cachehostpath(cachepath+std::string("\\")+ std::string(httpHeader->host));
    if (!fs::exists(Cachehostpath)) {
        if (!fs::create_directory(Cachehostpath)) {
            std::cout << "��ǰhost�Ļ����ļ��д���ʧ��" << std::endl;
        }
    }
    auto file_cut = std::to_string(std::hash<std::string>{}(std::string(httpHeader->url)));
    auto filename = cachepath +std::string ("\\")+ std::string(httpHeader->host)+std::string("\\") + file_cut +".txt";
    std::ofstream fileout;
    if (!fs::exists(fs::path(filename)))//�ļ�������
    {
        //����
//        bool delflag;
//        std::string deletefile=checkCacheLeft(delflag);
//        if(delflag)
//            std::cout<< deletefile <<"��ɾ��"<<std::endl;
        // �������ת��ԭ����
        send(proxyParam->serverSocket, request_buffer, (int)strlen(request_buffer) + 1, 0);
        char buffer[65536];char cachebuffer[65536];ZeroMemory(buffer,65536);ZeroMemory(cachebuffer,65536);
        int recvSize = recv(proxyParam->serverSocket, buffer, 65535, 0);
        if (recvSize < 0||recvSize >65535 ) {
            std::cout << "����size<0,����" << std::endl;
            _endthreadex(0);
        }
        memcpy(cachebuffer,buffer,recvSize);
        send(proxyParam->clientSocket, buffer, recvSize, 0);
        fileout.open(filename,std::ios::binary|std::ios::out);
        //printf("%s",cachebuffer);
        //BUG!!!!!
        if(fileout){
            fileout.write(cachebuffer,recvSize);
        }else{
            std::cout<<"�ļ���ʧ��"<<std::endl;
        }
        fileout.close();
        std::cout<<"�½������ļ�: "<<filename<<std::endl;
    }
    else//�Ѵ��ڣ��ж��Ƿ���Ҫ����
    {
        auto ft =fs::last_write_time(fs::path(filename) ) ;
        std::time_t t= std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ft));
        char* asctime=std::asctime(std::localtime(&t));
        std::istringstream its(asctime);
        std::string dayname,month,day,minutes,year;
        its>>dayname>>month>>day>>minutes>>year;
        std::ostringstream fmt;
        fmt<<"\r\nIf-Modified-Since: "<<dayname<<", "<<day<<", "<<month<<", "<<year<<", "<<minutes<<" GMT\r\n";
        std::string gram=fmt.str();
        send(proxyParam->serverSocket,gram.c_str(),(int)gram.size(),0);
        char buffer[65536];
        ZeroMemory(buffer,65536);
        int recvSize=recv(proxyParam->serverSocket,buffer,65535,0);
        if (recvSize < 0) {
            std::cout << "����size<0,����" << std::endl;
            closesocket(proxyParam->clientSocket);
            closesocket(proxyParam->serverSocket);
            delete proxyParam;
            _endthreadex(0);
        }
        std::string resgram(buffer);
        //�ж��Ƿ�304 �������
        char *ptr,*phead;//printf("%s\r\n",buffer);
        phead=strtok_s(buffer, "\r\n", &ptr);
        std::string _phead(phead);
        if(_phead.find("304"))
        {//cache hit
            std::cout<<"cache hit"<<filename<<std::endl;
            std::ifstream cachefilestram(filename,std::ios::binary);
            if(!cachefilestram.is_open())
            {
                std::cout << "�����ȡʧ�ܣ��޷������ļ���" << std::endl;
                closesocket(proxyParam->clientSocket);
                closesocket(proxyParam->serverSocket);
                delete proxyParam;
                _endthreadex(0);
            }
            std::stringstream res;
            res<<cachefilestram.rdbuf();
            std::string resstr(res.str());
            cachefilestram.close();
            send(proxyParam->clientSocket,resstr.c_str(),(int)strlen(resstr.c_str()),0);
        }else if(_phead.find("200"))
        {//cache update
            std::cout<<"cache update"<<filename<<std::endl;
            send(proxyParam->clientSocket,resgram.c_str(),(int)resgram.size(),0);//ת��
            std::ofstream cachefilestram(filename,std::ios::binary|std::ios::out);
            if(!cachefilestram.is_open()) {
                std::cout << "�������ʧ�ܣ��޷������ļ���" << std::endl;
                closesocket(proxyParam->clientSocket);
                closesocket(proxyParam->serverSocket);
                delete proxyParam;
                _endthreadex(0);
            }
            cachefilestram<<resgram;
            cachefilestram.close();
        }
    }
}

std::string& replace_all(std::string& src, const std::string& old_value, const std::string& new_value) {
    // ÿ�����¶�λ��ʼλ�ã���ֹ�����滻����ַ����γ��µ�old_value
    for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
        if ((pos = src.find(old_value, pos)) != std::string::npos) {
            src.replace(pos, old_value.length(), new_value);
        }
        else break;
    }
    return src;
}

bool HttpProxy::websitefilter(HttpHeader *httpHeader){
    if(WebFilters.contains(httpHeader->host))
    {
        std::cout<<"��վ���ˣ����������"<<httpHeader->host<<std::endl;
        return true;
    }
//    if(){
//
//    }
    return false;
}


std::string HttpProxy::checkCacheLeft(bool& delflag) {
    std::vector<fs::path> filelist;
    delflag=false;
    for(auto& p: fs::directory_iterator(fs::path(cachepath)))//��û�Ŀ¼��Ŀ¼�б�
    {
        if (fs::directory_entry(p).status().type()== fs::file_type::directory)
            filelist.push_back(p.path());
        else
            std::cout<<"checkCacheLeft wrong"<<std::endl;
    }
    if(filelist.empty())
        return "";
    int count=0;
    std::time_t todeletetime=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    fs::directory_entry toDelfile;
    for(auto &str:filelist)//��ÿ��hostĿ¼
    {
        if(!str.string().starts_with("."))
        {
            for(auto &file_name : fs::directory_iterator(str))
            {
                count++;
                std::time_t fileLastmodifytime=std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(fs::last_write_time(file_name)));
                if(fileLastmodifytime<todeletetime)
                {
                    todeletetime=fileLastmodifytime;
                    toDelfile=file_name;
                    delflag=true;
                }
            }
        }
    }
    if(count < cacheSize )
    {
        delflag=false;
        return "";
    }
    else if(delflag) // >=cachesize &&delflag
    {
        if(!fs::remove(toDelfile))
            std::cout<<"ɾ��ʧ��"<<std::endl;
        return toDelfile.path().string();
    }
    return "";
}

BOOL ConnectToServer(SOCKET *serverSocket, char *host) {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(HTTP_PORT);
    HOSTENT *hostent = gethostbyname(host);//DNS?
    if (!hostent) {
        return FALSE;
    }
    in_addr Inaddr = *((in_addr *) *hostent->h_addr_list);
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverSocket == INVALID_SOCKET) {
        return FALSE;
    }
    if (connect(*serverSocket, (SOCKADDR *) &serverAddr, sizeof(serverAddr))
        == SOCKET_ERROR) {
        closesocket(*serverSocket);
        return FALSE;
    }
    return TRUE;
}
