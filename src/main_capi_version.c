#include "rtc/rtc.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv)
{
    int peerConnectionID;
    rtcConfiguration * mainRTCConf;
    // const char mainStunServer[] = "stun.l.google.com:19302"; // Null character will be added automatically at the end.
    // const char ** stunServerList;
    // stunServerList = malloc(sizeof(char*)*1);
    // stunServerList[0] = mainStunServer;
    // mainRTCConf->iceServers = &stunServerList;
    mainRTCConf ->iceServersCount = 1;
    mainRTCConf ->iceServers[0] = "stun.l.google.com:19302";
    rtcPreload();
    peerConnectionID = rtcCreatePeerConnection(mainRTCConf);
    printf("%d\n", peerConnectionID);
    //printf("%d\n", RTC_ERR_SUCCESS);
    printf("%d\n", rtcClosePeerConnection(peerConnectionID));
    printf("%d\n", rtcDeletePeerConnection(peerConnectionID));
    rtcCleanup();
    // for (int i = 0; i < 24; i++)
    // {
    //     printf("%c\n", mainStunServer[i]);
    // }
    return 0;
}