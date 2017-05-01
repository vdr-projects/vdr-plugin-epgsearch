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

#ifndef VDR_STATUS_THREAD_H
#define VDR_STATUS_THREAD_H

#include <vdr/thread.h>
#include <vdr/status.h>
#include "epgsearchext.h"
#include "recdone.h"
#include "epgsearchtools.h"
#include "log.h"

extern int gl_StatusProgged;

typedef enum
{
    StatusThreadReady,
    StatusThreadWorking,
    StatusThreadError,
    StatusThreadDone
} StatusThreadStatus;

class cStatusThread: public cThread {
private:
        static cStatusThread *m_Instance;
	const cDevice * m_device;
	const char * m_name;
	const char * m_filename;
	bool m_on;
	static StatusThreadStatus m_Status;
protected:
        virtual void Action(void);
        void Stop(void);
public:
        bool m_Active;
	StatusThreadStatus GetStatus() { return cStatusThread::m_Status; }
	void SetStatus(StatusThreadStatus Status) { LogFile.eSysLog("%d", int(Status)); cStatusThread::m_Status = Status; }
        cStatusThread();
        virtual ~cStatusThread();
        static void Init(const cDevice *Device, const char *Name, const char *Filename, bool On);
        void Exit(void);
};

#endif
