#include "stdafx.h"
#include "server.h"
#include <vector>

#define DRI_IS_SERVER
#include "interface.h"
using namespace boost;

class Server : public rpc::ServerPeer<Interface>
{
	shared_ptr<Interface> conn;
public:
	virtual void on_connection_init( const shared_ptr<Interface>& conn_ )
	{
		conn = conn_;
	}

	Server(){}

	string echo0()
	{
		return "echo0";
	}
	string echo1(string s1)
	{
		return s1;
	}
	string echo2(string s1, string s2)
	{
		return s1+s2;
	}
	string echo3(string s1, string s2, string s3)
	{
		return s1+s2+s3;
	}
	string echo4(string s1, string s2, string s3, string s4)
	{
		return s1+s2+s3+s4;
	}
	string echo5(string s1, string s2, string s3, string s4, string s5)
	{
		return s1+s2+s3+s4+s5;
	}
	void void0()
	{
		string s = echo0();
		int i = 0;
		conn->got_void_string( i, s, bind( &Server::check_client_call, this, _1, i, s ) );
	}
	void void1(string s1)
	{
		string s = echo1(s1);
		int i = 1;
		conn->got_void_string( i, s, bind( &Server::check_client_call, this, _1, i, s ) );
	}
	void void2(string s1, string s2)
	{
		string s = echo2(s1, s2);
		int i = 2;
		conn->got_void_string( i, s, bind( &Server::check_client_call, this, _1, i, s ) );
	}
	void void3(string s1, string s2, string s3)
	{
		string s = echo3(s1, s2, s3);
		int i = 3;
		conn->got_void_string( i, s, bind( &Server::check_client_call, this, _1, i, s ) );
	}
	void void4(string s1, string s2, string s3, string s4)
	{
		string s = echo4(s1, s2, s3, s4);
		int i = 4;
		conn->got_void_string( i, s, bind( &Server::check_client_call, this, _1, i, s ) );
	}
	void void5(string s1, string s2, string s3, string s4, string s5)
	{
		string s = echo5(s1, s2, s3, s4, s5);
		int i = 5;
		conn->got_void_string( i, s, bind( &Server::check_client_call, this, _1, i, s ) );
	}
	void check_client_call( const remote_call_error& err, int i, const string& sent_content )
	{
		mutex::scoped_lock lck(results_write_mutex);
		static const char* t = "Client function call";
		if ( err )
		{
			++ results[t].failureCount;
			cout << "Client function call got_void_string( " << i << ", " << sent_content << " ) failed." << endl;
		}
		else
		{
			++ results[t].successCount;
		}
		cout << t << ": " << results[t].successCount <<  " OK, " << results[t].failureCount << " failed." << endl;
	}
};

void start_server()
{
	Interface::listen<Server>( 7844 );
}