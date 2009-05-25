#include "stdafx.h"
#include "server.h"
#include "client.h"
#include <boost/thread.hpp>

void main()
{
	boost::thread th1(&start_server);
	boost::thread th2(&start_client);
	th1.join();
	th2.join();
	::system("pause");
}