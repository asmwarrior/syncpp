/*
 * Copyright 2014 Anton Karmanov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//Platform-dependent definitions.

#ifndef SYNSAMPLE_CORE_PLATFORM_H_INCLUDED
#define SYNSAMPLE_CORE_PLATFORM_H_INCLUDED

#ifdef _MSC_VER 
#include <Windows.h>
#endif

#include "stringex__dec.h"

#ifdef _MSC_VER 
#define PLATFORM__THREAD_LOCAL __declspec(thread)
#else
#define PLATFORM__THREAD_LOCAL __thread
#endif


namespace syn_script {

	namespace platform {

#ifdef _MSC_VER
//Visual C++.
		typedef DWORD Tick_t;
		const Tick_t GC_SYNC_INTERVAL = 2;
		typedef DWORD TimeMs_t;

		const bool IS_WINDOWS = true;
#else
//Linux.
		typedef unsigned long long Tick_t;
		const Tick_t GC_SYNC_INTERVAL = 2;
		typedef unsigned long long TimeMs_t;

		const bool IS_WINDOWS = false;
#endif

		struct DateTime {
			int m_year;
			int m_month;
			int m_day;
			int m_hour;
			int m_minute;
			int m_second;
		};

		//A very fast, platform-dependent function that returns the current tick count.
		//Used to measure time intervals for GC synchronization.
		Tick_t get_current_tick_count();

		TimeMs_t get_current_time_millis();

		void get_current_time(DateTime& time);

	}

}

#endif//SYNSAMPLE_CORE_PLATFORM_H_INCLUDED
