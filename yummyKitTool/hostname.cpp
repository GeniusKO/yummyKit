#include "hostname.h"
#include <QCoreApplication>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <netdb.h>

QStringList host_ipList, hostName;
u_char *ip;

void host_filter(u_char*);
QString getHex2String(u_char *s);

hostname::hostname(QObject *parent) : QThread(parent)
{
    std::ifstream fin;
    this->host_stop = false;
    system("sudo nbtscan | grep Usage > ./nbtscan_log.txt");
    char buf[10];
    fin.open("./nbtscan_log.txt");
    while(fin >> buf) {}
    fin.close();
    if(strncmp(buf, "Usage:", 6)) {
        memset(buf, 0, 10);
        system("sudo apt-get install -y nbtscan >/dev/null && echo success > ./nbtscan_log.txt");
        fin.open("./nbtscan_log.txt");
        while(fin >> buf) {}
        fin.close();
        if(strncmp(buf, "success", 7)) {
            system("echo 'You must run yummyKit with root. Please re-run.' > ./nbtscan_log.txt");
            this->host_stop = true;
        }
    }
}

void hostname::run() {
    std::ifstream fin;
    QString nbt;
    int i = 1;
    char buf[256];
    while(1) {
        if(!host_ipList.isEmpty() && host_ipList.length() == 1) {
            hostName << "Router";
            emit setHostName(hostName);
        } else if (!host_ipList.isEmpty() && host_ipList.length() > 1) {
            memset(buf, 0, 256);
            nbt.append("sudo nbtscan ");
            nbt.append(host_ipList.at(i));
            nbt.append(" | grep -a '.' | awk '{ print $2 }' > ./nbtscan_log.txt");
            system(nbt.toStdString().c_str());
            system("iconv -c -f euc-kr -t utf-8 ./nbtscan_log.txt > ./nbtscan.txt");
            fin.open("./nbtscan.txt");
            while(fin >> buf) {}
            fin.close();
            if(!strncmp(buf, "address", 7)) host_filter(ip);
            else hostName << buf;
            emit setHostName(hostName);
            nbt.clear();
            i++;
            if(i == host_ipList.length() && this->host_stop) break;
            else if(i == host_ipList.length()) {
                sleep(15);
                if(i == host_ipList.length()) {
                    this->host_stop = true;
                    break;
                }
            }
        }
    }
    system("sudo rm ./nbtscan_log.txt ./nbtscan.txt");
}

void host_filter(u_char *ip) {
    struct sockaddr_in host_addr;
    char host_buf[NI_MAXHOST] = {0,};
//    struct hostent *hptr;
    char host_tmp[15] = {0,};
    for(int i = 0, j = 0; i < 4; i++) {
        if((ip[i] / 100) != 0) {
            host_tmp[j] = (ip[i] / 100) + '0';
            host_tmp[j+1] = (ip[i] / 10) - ((ip[i] / 100) * 10) + '0';
            host_tmp[j+2] = ip[i] - ((ip[i] / 10) * 10) + '0';
            host_tmp[j+3] = '.';
            j += 4;
        }
        else if((ip[i] / 10) != 0) {
            host_tmp[j] = ip[i] / 10 + '0';
            host_tmp[j+1] = ip[i] - ((ip[i] / 10) * 10) + '0';
            host_tmp[j+2] = '.';
            j += 3;
        }
        else {
            host_tmp[j] = ip[i] + '0';
            if(i != 3) host_tmp[j+1] = '.';
            j += 2;
        }
    }
/*
    // I wanna get NetBIOS name. But it is very difficult work!
    // I try to use many function such as gethostbyaddr() / getaddrinfo() / getnameinfo() / gethostname() / gethostbyname()
    // And I try to use many argument such as NI_NAMEREQD / NI_NOFQDN / NI_IDN / NI_DGRAM / NI_NUMERICHOST / AF_NETBEUI / AF_LLC
    memset(&host_addr, 0, sizeof(host_addr));
    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = inet_addr(host_tmp);
    hptr = gethostbyaddr((char *)&host_addr.sin_addr, 4, AF_INET);
    if(hptr == NULL) {
        switch (h_errno) {
                case HOST_NOT_FOUND:
                    hostName << "HOST_NOT_FOUND";
                    break;
                case NO_ADDRESS:
                    hostName << "NO_ADDRESS";
                    break;
                case NO_RECOVERY:
                    hostName << "NO_RECOVERY";
                    break;
                case TRY_AGAIN:
                    hostName << "TRY_AGAIN";
                    break;
        }
    }
    else hostName << hptr->h_name;
*/
    memset(&host_addr, 0, sizeof(host_addr));
    inet_pton(AF_INET, host_tmp, &(host_addr.sin_addr));
    host_addr.sin_family = AF_INET;
//    host_addr.sin_addr.s_addr = inet_addr(host_tmp);
//    host_addr.sin_port = 80;
    int test;
    if((test = getnameinfo((struct sockaddr *)&host_addr, sizeof(host_addr), host_buf, sizeof(host_buf), NULL, 0, NI_NAMEREQD)) != 0) {
//        qDebug() <<  gai_strerror(test);
        switch (h_errno) {
                case HOST_NOT_FOUND:
                    hostName << "HOST_NOT_FOUND";
                    break;
                case NO_ADDRESS:
                    hostName << "NO_ADDRESS";
                    break;
                case NO_RECOVERY:
                    hostName << "NO_RECOVERY";
                    break;
                case TRY_AGAIN:
                    hostName << "TRY_AGAIN";
                    break;
        }
    }
    else hostName << host_buf;
}

QString getHex2String(u_char *s) {
    QString a, str;
    for(int i = 0; i < 4; i++) {
        a = QString("%1").arg(s[i], 0, 10);
        if(i < 3) a += ".";
        str.append(a);
    }
    a.clear();
    return str;
}

void hostname::hostStop(bool s) {
    QMutex mut;
    mut.lock();
    this->host_stop = s;
    mut.unlock();
}

void hostname::getArgu(u_char *list) {
    host_ipList << getHex2String(list);
    ip = list;
}