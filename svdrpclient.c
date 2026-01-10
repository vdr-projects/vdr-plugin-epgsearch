/*                                                                  -*- c++ -*-
Copyright (C) 2004-2013 Christian Wieninger

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

The author can be reached at cwieninger@gmx.de

The project's page is at http://winni.vdr-developer.org/epgsearch
*/

#include "svdrpclient.h"
#include <sys/socket.h>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#ifdef __FreeBSD__
#include <netinet/in.h>
#endif
#include <vdr/config.h>
#include "epgsearchcfg.h"
#include "epgsearchtools.h"
#include "log.h"

#define SVDRPCONNECT 220
#define SVDRPDISCONNECT 221
#define CMDSUCCESS 250

extern bool VDR_readyafterStartup;
static bool bFatal = false;

// --- cEpgsSVDRPClient -----------------------------------
cEpgsSVDRPClient::cEpgsSVDRPClient() {
    sock = 0;
    bConnected = false;
    if (bFatal) { // access denied needs VDR restart
        LogFile.Log(1, "SVDRP blocked check svdrphosts.conf");
        return;
    }
    sock  = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LogFile.eSysLog("error creating socket!");
        return;
    }
    struct sockaddr_in AdrSock;
    AdrSock.sin_family = AF_INET;
    AdrSock.sin_port   = htons(EPGSearchConfig.SVDRPPort);
    inet_aton("127.0.0.1", &AdrSock.sin_addr);
    memset(&(AdrSock.sin_zero), '\0', 8);

    struct timeval timeout;
    timeout.tv_sec = 5;  // after 5 seconds connect() will timeout
    timeout.tv_usec = 0; // should not happen for localhost
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (connect(sock, (struct sockaddr*)&AdrSock, sizeof(AdrSock)) == -1) {
        if (VDR_readyafterStartup)   // during startup svdrp might yet not be initialized
            LogFile.eSysLog("error connecting to socket!");
        return;
    }
    bConnected = (Receive() == SVDRPCONNECT);
    if (!bConnected) {
        if (VDR_readyafterStartup)
            LogFile.eSysLog("could not connect to VDR!");
    }
    else
        LogFile.Log(3, "SVDRP connected");
}

cEpgsSVDRPClient::~cEpgsSVDRPClient() {
    LogFile.Log(3, "SVDRP connection closed");
    if (sock > 0)
        close(sock);
}

bool cEpgsSVDRPClient::SendCmd(const char* cmd) {
    if (!bConnected)
        return false;

    LogFile.Log(3, "SVDRP send %s",cmd);
    cString szCmd = cString::sprintf("%s\r\n", cmd);
    if (!Send(*szCmd))
        return false;

    long rc = Receive();
    bool cmdret = (rc == CMDSUCCESS);
    if (!cmdret)
        LogFile.Log(2, "command %s got rc %ld", cmd, rc);
    if (rc < 0)
        return false;

    szCmd = cString::sprintf("QUIT\r\n");
    Send(szCmd);
    if (!Send(*szCmd))
        return false;
    if ((rc = Receive()) != SVDRPDISCONNECT)
        LogFile.eSysLog("could not disconnect (%ld)!", rc);

    close(sock);
    sock = -1;
    return cmdret;
}

bool cEpgsSVDRPClient::Send(const char* szSend) {
    int length = strlen(szSend);
    int sent = 0;
    do {
        sent += send(sock, szSend + sent, length - sent, 0);
        if (sent < 0) {
            LogFile.eSysLog("error sending command (%s)!", strerror(errno));
            return false;
        }
    } while (sent < length);
    return true;
}

long cEpgsSVDRPClient::Receive() {
    char* csResp = strdup("");
    char ch;
    long rc = 0;

    bool bCheckMultiLine = true;

    while (bCheckMultiLine) {
        while (strlen(csResp) < 2 || strcmp(csResp + strlen(csResp) - 2, "\r\n") != 0) {
            if (rc = recv(sock, &ch, 1, 0) <= 0) {
                if (rc == 0) { // unexpected but necessary to exit while loop
                    LogFile.Log(2, "SVDRP recv got eof or socket closed");
                    free(csResp);
                    return -1;
                }
                LogFile.eSysLog("error receiving response (%s)!",strerror(errno));
                free(csResp);
                return -1;
            }
            char* Temp = NULL;
            msprintf(&Temp, "%s%c", csResp, ch);
            free(csResp);
            csResp = Temp;
            if (!strcmp(Temp, "Access denied!")) {
                LogFile.Log(1, "Access denied");
                bFatal = true;
                return -1;
            }
        }
        if (csResp[3] == ' ') {
            bCheckMultiLine = false;
            rc = atol(csResp);
        }
        free(csResp);
        csResp = strdup("");
    }
    free(csResp);
    return rc;
}
