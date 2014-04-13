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

//Application entry point.

#include <exception>
#include <iostream>

#include "../core/commons.h"
#include "../core/main.h"

namespace ns = synbin;

int main(int argc, const char* const argv[]) try {
	int result = ns::main(argc, argv);
	return result;
} catch (std::exception& e) {
	std::cerr << "Error: " << e.what() << "\n";
	return 1;
} catch (ns::Exception& e) {
	std::cerr << "Error: ";
	e.print(std::cerr);
	std::cerr << "\n";
	return 1;
} catch (const char* msg) {
	std::cerr << "Error: \"" << msg << "\"\n";
	return 1;
} catch (...) {
	std::cerr << "Error. Unhandled exception\n";
	return 1;
}
