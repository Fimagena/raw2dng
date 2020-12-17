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

#if !kLocalUseThreads

void DngHost::PerformAreaTask(dng_area_task &task, const dng_rect &area) { 
   dng_area_task::Perform(task, area, &Allocator (), Sniffer ());
}

#else 

#include <thread>
#include <vector>
#include <exception>
#include "dng_sdk_limits.h"

static std::exception_ptr threadException = nullptr;

static void executeAreaThread(std::reference_wrapper<dng_area_task> task, uint32 threadIndex, const dng_rect &threadArea, const dng_point &tileSize, dng_abort_sniffer *sniffer) {
   try { task.get().ProcessOnThread(threadIndex, threadArea, tileSize, sniffer, NULL); }
   catch (...) { threadException = std::current_exception(); }
}


void DngHost::PerformAreaTask(dng_area_task &task, const dng_rect &area, dng_area_task_progress *progress) {
    dng_point tileSize(task.FindTileSize(area));

    // Now we need to do some resource allocation
    // We start by assuming one tile per thread, and work our way up
    uint32 vTilesinArea = area.H() / tileSize.v; if ((area.H() - (vTilesinArea * tileSize.v)) > 0) vTilesinArea++;
    uint32 hTilesinArea = area.W() / tileSize.h; if ((area.W() - (hTilesinArea * tileSize.h)) > 0) hTilesinArea++;

    int vTilesPerThread = 1, hTilesPerThread = 1;
    // Ensure we don't exceed maxThreads for this task
    while (((vTilesinArea + vTilesPerThread - 1) / vTilesPerThread) * ((hTilesinArea + hTilesPerThread - 1) / hTilesPerThread) > kMaxMPThreads) {
        // Here we want to increase the number of tiles per thread; so do we do that in the V or H dimension?
        if ((vTilesinArea / vTilesPerThread) > (hTilesinArea / hTilesPerThread)) vTilesPerThread++;
        else hTilesPerThread++;
    }

    task.Start(Min_uint32(task.MaxThreads (), kMaxMPThreads), area, tileSize, &Allocator (), Sniffer ());

    std::vector<std::thread> areaThreads;
    threadException = nullptr;

    dng_rect threadArea(area.t, area.l, area.t + (vTilesPerThread * tileSize.v), area.l + (hTilesPerThread * tileSize.h));
    for (uint32 vIndex = 0; vIndex < vTilesinArea; vIndex += vTilesPerThread) {

        for (uint32 hIndex = 0; hIndex < hTilesinArea; hIndex += hTilesPerThread) {
            try { areaThreads.push_back(std::thread(executeAreaThread, std::ref(task), areaThreads.size(), threadArea, tileSize, Sniffer ())); }
            catch (...) { executeAreaThread(task, areaThreads.size(), threadArea, tileSize, Sniffer ()); }

            threadArea.l = threadArea.r;
            threadArea.r = Min_int32(threadArea.r + (hTilesPerThread * tileSize.h), area.r);
        }

        threadArea.t = threadArea.b;
        threadArea.l = area.l;
        threadArea.b = Min_int32(threadArea.b + (vTilesPerThread * tileSize.v), area.b);
        threadArea.r = area.l + (hTilesPerThread * tileSize.h);
    }

   for (auto& areaThread : areaThreads) areaThread.join();
   if (threadException) std::rethrow_exception(threadException);

   task.Finish(Min_uint32(task.MaxThreads(), kMaxMPThreads));
}

#endif
