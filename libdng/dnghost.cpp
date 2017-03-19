/* This file is part of the dngconvert project
   Copyright (C) 2011 Jens Mueller <tschensensinger at gmx dot de>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public   
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
   
   This file uses code from dng_threaded_host.cpp -- Sandy McGuffog CornerFix utility
   (http://sourceforge.net/projects/cornerfix, sandy dot cornerfix at gmail dot com),
   dng_threaded_host.cpp is copyright 2007-2011, by Sandy McGuffog and Contributors.
*/

#include "dnghost.h"
#include "dng_abort_sniffer.h"
#include "dng_area_task.h"
#include "dng_rect.h"

#ifndef kLocalUseThreads
#define kLocalUseThreads 1
#endif

#if kLocalUseThreads
#include <pthread.h>

enum threadVals { kMaxLocalThreads = 16 };

// areaThread uses pthread to execute a single area task
class areaThread {
    static void* thread_func(void *d) { ((areaThread*)d)->run(); return NULL; }

public:
    areaThread(dng_area_task &task, uint32 threadIndex, const dng_rect &threadArea, const dng_point &tileSize, dng_abort_sniffer *sniffer) :
    	fTask(task), fThreadIndex(threadIndex), fThreadArea(threadArea), fTileSize(tileSize), fSniffer(sniffer) {
        // Make very sure the thread is joinable. This is the default on most systems, but you never know
        // Initialize and set thread joinable attribute
        pthread_attr_init(&threadAttr);
        pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_JOINABLE);
        if (stackSize() != 0) pthread_attr_setstacksize(&threadAttr, stackSize());
    }
    ~areaThread() { pthread_attr_destroy(&threadAttr); }

    size_t stackSize() { return 0; } // 0 gives the default size for the platform; override if we know something better

    void run() { fTask.ProcessOnThread(fThreadIndex, fThreadArea, fTileSize, fSniffer); }
    int start() { return pthread_create(&thread, &threadAttr, thread_func, (void*)this); }
    int wait()  { return pthread_join(thread, NULL); }

private:
    pthread_attr_t threadAttr;
    pthread_t thread;

    dng_area_task &fTask;
    uint32 fThreadIndex;
    dng_rect fThreadArea;
    dng_point fTileSize;
    dng_abort_sniffer *fSniffer;
};

#endif

void DngHost::PerformAreaTask(dng_area_task &task, const dng_rect &area) {
#if kLocalUseThreads

    dng_point tileSize(task.FindTileSize (area));

    // Now we need to do some resource allocation
    // We start by assuming one tile per thread, and work our way up
    // Note that a thread pool is technically a better idea, but very complex
    // in a Mac/Windows environment; this is the cheap and cheerful solution
    int vTilesPerThread = 1;
    int hTilesPerThread = 1;

    int vTilesinArea = area.H() / tileSize.v;
    vTilesinArea = ((vTilesinArea * ((int) tileSize.v)) < ((int) area.H())) ? vTilesinArea + 1 : vTilesinArea;

    int hTilesinArea = area.W() / tileSize.h;
    hTilesinArea = ((hTilesinArea * ((int) tileSize.h)) < ((int) area.W())) ? hTilesinArea + 1 : hTilesinArea;

    int32 tileWidth  = tileSize.h;
    int32 tileHeight = tileSize.v;

    // Ensure we don't exceed maxThreads for this task
    while (((vTilesinArea + vTilesPerThread - 1) / vTilesPerThread) * ((hTilesinArea + hTilesPerThread - 1) / hTilesPerThread) > 
           Min_int32(task.MaxThreads (), kMaxLocalThreads)) {
        // Here we want to increase the number of tiles per thread
        // So do we do that in the V or H dimension?
        if ((vTilesinArea / vTilesPerThread) > (hTilesinArea / hTilesPerThread)) {
            vTilesPerThread++;
            tileHeight += tileSize.v;
        }
        else {
            hTilesPerThread++;
            tileWidth += tileSize.h;
        }
    }

    areaThread *localThreads[kMaxLocalThreads];

    task.Start(Min_uint32(task.MaxThreads (), kMaxLocalThreads), tileSize, &Allocator (), Sniffer ());

    int threadIndex = 0;
    dng_rect threadArea(area.t, area.l, area.t + tileHeight, area.l + tileWidth);

    for (int vIndex = 0; vIndex < vTilesinArea; vIndex += vTilesPerThread) {
        threadArea.l = area.l;
        threadArea.r = area.l + tileWidth;

        for (int hIndex = 0; hIndex < hTilesinArea; hIndex += hTilesPerThread) {
            localThreads[threadIndex] = new areaThread(task, threadIndex, threadArea, tileSize, Sniffer ());
            if (localThreads[threadIndex] != NULL)
                if (localThreads[threadIndex]->start() != 0) {
                    delete localThreads[threadIndex];
                    localThreads[threadIndex] = NULL;
                }

            // If something goes bad, do this non-threaded
            if (localThreads[threadIndex] == NULL) task.ProcessOnThread(threadIndex, threadArea, tileSize, Sniffer ());

            threadIndex++;

            threadArea.l = threadArea.r;
            threadArea.r = Min_int32(threadArea.r + tileWidth, area.r);
        }
        threadArea.t = threadArea.b;
        threadArea.b = Min_int32(threadArea.b + tileHeight, area.b);
    }
    for (int i = 0; i < threadIndex; i++) {
        if (localThreads[i] != NULL) {
            // There's nothing we can do if join fails; just hope for the best(!)
            localThreads[i]->wait();
            delete localThreads[i];
        }
    }
    task.Finish(Min_uint32(task.MaxThreads(), kMaxLocalThreads));

#else // kLocalUseThreads
    dng_area_task::Perform(task, area, &Allocator (), Sniffer ());
#endif
}
