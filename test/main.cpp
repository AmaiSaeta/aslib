#include <iostream>
#include <string>
#include <strstream>
#include <typeinfo>

std::ostream &operator<<(std::ostream &stream, const std::type_info &type) { return stream << type.name(); }

#define  BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#ifdef WIN32
#	include <windows.h>
#endif

struct GlobalConfiguration {
	GlobalConfiguration() {
		if(isDebuggerPresent()) boost::unit_test::unit_test_log.set_stream(stream_);
	}

	~GlobalConfiguration() {
		if(isDebuggerPresent()) {
			std::string result;

			while( std::getline(stream_, result)) {
				result += "\n";
				print4Debugger(result);
			}
			boost::unit_test::unit_test_log.set_stream(std::cout);
		}
	}

	static
	void print4Debugger(const std::string &str) {
#if WIN32
		::OutputDebugStringA(str.c_str());
#else
		std::cout << str << std::endl;
#endif
	}

	static
	bool isDebuggerPresent() {
		return
#if WIN32
			::IsDebuggerPresent() == TRUE;
#else
			false;
#endif
	}

	std::stringstream stream_;
};

BOOST_GLOBAL_FIXTURE(GlobalConfiguration);
