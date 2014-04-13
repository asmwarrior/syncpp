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

//Linux platform-dependent functions.

#include <chrono>
#include <ctime>

#include "platform.h"

namespace ss = syn_script;
namespace pf = ss::platform;

pf::Tick_t pf::get_current_tick_count() {
	return get_current_time_millis();
}

pf::TimeMs_t pf::get_current_time_millis() {
	auto now = std::chrono::system_clock::now();
	auto time = now.time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
}

void pf::get_current_time(DateTime& date_time) {
	std::time_t time = std::time(nullptr);

	//TODO This function is not thread-safe. Add synchronization.
	struct std::tm* tm = localtime(&time);

	date_time.m_year = tm->tm_year + 1900;
	date_time.m_month = tm->tm_mon;
	date_time.m_day = tm->tm_mday;
	date_time.m_hour = tm->tm_hour;
	date_time.m_minute = tm->tm_min;
	date_time.m_second = tm->tm_sec;
}
