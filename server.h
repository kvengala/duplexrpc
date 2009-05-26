#pragma once
#include <map>
#include <string>
#include <boost/thread.hpp>

struct TestResult
{
	TestResult():successCount(0), failureCount(0){}
	int successCount, failureCount;
};
typedef std::map<std::string, TestResult> ResultsType;
extern ResultsType results;
extern boost::mutex results_write_mutex;

void start_server();