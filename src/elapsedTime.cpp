//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "elapsedTime.hpp"
#ifdef SAPF_MACH_TIME
#include <mach/mach_time.h>
#else
#include <chrono>
#endif // SAPF_MACH_TIME
#include <pthread.h>

extern "C" {

static double gHostClockFreq;

void initElapsedTime()
{
#ifdef SAPF_MACH_TIME
	struct mach_timebase_info info;
	mach_timebase_info(&info);
	gHostClockFreq = 1e9 * ((double)info.numer / (double)info.denom);
#else
        gHostClockFreq = 0.0;
#endif // SAPF_MACH_TIME
}

double elapsedTime()
{
#ifdef SAPF_MACH_TIME
	return (double)mach_absolute_time() / gHostClockFreq;
#else
        return (double) std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count() / 10e9;
#endif // SAPF_MACH_TIME
}

}
