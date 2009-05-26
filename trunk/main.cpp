#include "stdafx.h"
#include "server.h"
#include "client.h"
#include <boost/thread.hpp>

ResultsType results;
boost::mutex results_write_mutex;

void main()
{
	boost::thread_group threads;
	threads.create_thread ( &start_server );

	for (int i = 0; i != 5; ++i)
	{
		threads.create_thread ( &start_client );
	}

	threads.join_all();
	::system("pause");
}