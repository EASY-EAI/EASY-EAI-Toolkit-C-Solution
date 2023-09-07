#include <string>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "info.h"

static int32_t get_local_Ip(const char *device, char *ip, int ip_len)
{
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    memset(ip, 0, ip_len);
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd) {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, device, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0) {
        printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    sprintf(ip, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;
}

static int32_t get_local_NetMask(const char *device, char *netMask, int netMask_len)
{ 
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;
	
    memset(netMask, 0, netMask_len);
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd) {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }
    
    ifr.ifr_addr.sa_family = AF_INET;
    
    strncpy(ifr.ifr_name, device, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(sd, SIOCGIFNETMASK, &ifr) < 0) {
        printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    sprintf(netMask, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
	return 0;
}
/*
static int32_t get_local_GateWay(const char *device, char *gateWay, int gateWay_len)
{
    memset(gateWay, 0, gateWay_len);
	
	char cmd[128] = {0};
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd,"route -n|grep %s|grep 0.0.0.0|awk \'NR==1 {print $2}\'", device);
    FILE* fp = popen( cmd, "r" );
    if(NULL == fp) {
        printf("popen error: %s\n", strerror(errno));
        return -1;
    }
    
	while ( NULL != fgets(gateWay, gateWay_len, fp)) {
        if(gateWay[strlen(gateWay)-1] == '\n') {
           gateWay[strlen(gateWay)-1] = 0;
        }
        break;
    }
    pclose(fp);
	return 0;
}
*/
std::string devInfo()
{
    std::string strInfo;
    strInfo.clear();

    char devName[32] = {0};
    gethostname(devName, sizeof(devName));
    
    char ipAddr[32]  = {0};
    get_local_Ip("eth0", ipAddr, sizeof(ipAddr));
    
    char netMask[32] = {0};
    get_local_NetMask("eth0", netMask, sizeof(netMask));
    
    // make JsonStr
    strInfo += "{";
    // ========================
    // devName:
    strInfo += "\"devName\" : ";
    strInfo += "\"";
    strInfo += devName;
    strInfo += "\", ";
    // ipAddr
    strInfo += "\"ip\" : ";
    strInfo += "\"";
    strInfo += ipAddr;
    strInfo += "\", ";
    // netMask
    strInfo += "\"mask\" : ";
    strInfo += "\"";
    strInfo += netMask;
    strInfo += "\"";
    // ========================    
    strInfo += "}";


    return strInfo;
}


