#include "stdafx.h"
#include "server.h"
#include "client.h"
#include <boost/thread.hpp>

ResultsType results;
boost::mutex results_write_mutex;

void main()
{
	boost::thread th1(&start_server);
	boost::thread th2(&start_client);
	th1.join();
	th2.join();
	::system("pause");
}