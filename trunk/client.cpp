#include "stdafx.h"
#include "client.h"
#include <random>
#include <ctime>
#include "server.h"

#define DRI_IS_CLIENT
#include "interface.h"
using namespace boost;

class Client : public rpc::ClientPeer<Interface>
{
	shared_ptr<Interface> conn;

	string expecteds[6];
public:
	void on_connected( const boost::system::error_code& err, const shared_ptr<Interface>& conn_ )
	{
		if (err)
		{
			cout<<"调用出错："<<boost::system::system_error(err).what()<<endl;
		}
		else
		{
			cout<<"连接成功。"<<endl;
			conn = conn_;
		}
	}

	void got_void_string( int i, const string& s )
	{
		mutex::scoped_lock lck(results_write_mutex);
		static const char* t = "got_void_string";

		if ( i<0 || i>5 || expecteds[i]!=s )
		{
			++ results[t].failureCount;
			cout << "got_void_string( " << i << ", " << s << " ) is different from expected." << endl;
		}
		else
		{
			++ results[t].successCount;
		}

		cout << t << ": " << results[t].successCount << " OK, " << results[t].failureCount << " failed." << endl;
	}

#define PARAMS 

#define VOID_CALL(W) {\
			const char* ss[] = { PARAMS };\
			int c = sizeof(ss)/sizeof(ss[0]);\
			for ( int i=0; i<c; ++i )\
			{\
				expecteds[c] += ss[i];\
			}\
\
			conn->void ## W( PARAMS, bind( &Client::check_void_call, this, _1 ) );\
		}

	void voidCall0()
	{
		expecteds[0] = "echo0";
		conn->void0( bind( &Client::check_void_call, this, _1 ) );
	}

	void voidCall1()
	{
#undef PARAMS
#define PARAMS "猪"
	VOID_CALL(1);
	}

	void voidCall2()
	{
#undef PARAMS
#define PARAMS "你就是", "猪"
	VOID_CALL(2);
	}

	void voidCall3()
	{
#undef PARAMS
#define PARAMS "你就是", "一头", "猪"
	VOID_CALL(3);
	}

	void voidCall4()
	{
#undef PARAMS
#define PARAMS "你就是", "一头", "小笨", "猪"
	VOID_CALL(4);
	}

	void voidCall5()
	{
#undef PARAMS
#define PARAMS "你就是", "一头", "挫到不行的", "小笨", "猪"
	VOID_CALL(5);
	}

	void on_connection_in_place()
	{
		thread_group threads;
		threads.create_thread( bind( &Client::voidCall0, this ) );
		threads.create_thread( bind( &Client::voidCall1, this ) );
		threads.create_thread( bind( &Client::voidCall2, this ) );
		threads.create_thread( bind( &Client::voidCall3, this ) );
		threads.create_thread( bind( &Client::voidCall4, this ) );
		threads.create_thread( bind( &Client::voidCall5, this ) );

		threads.join_all();
	}

	void check_void_call( const remote_call_error& err )
	{
		mutex::scoped_lock lck(results_write_mutex);
		static const char* t = "Server function call (void)";
		if ( err )
		{
			++ results[t].failureCount;
			cout << "Client function call (void) failed." << endl;
		}
		else
		{
			++ results[t].successCount;
		}
		cout << t << ": " << results[t].successCount << " OK, " << results[t].failureCount << " failed." << endl;
	}

} ;


void start_client()
{
	Interface::connect<Client>( "localhost", 7844 );
}