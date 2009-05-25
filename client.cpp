#include "stdafx.h"
#include "client.h"
#include <random>
#include <ctime>

#define DRI_IS_CLIENT
#include "interface.h"
using namespace boost;

class Client : public rpc::ClientPeer<Interface>
{
	shared_ptr<Interface> conn;
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

	void on_connection_in_place()
	{
		conn->set_my_name( "挫比", &Client::on_set_my_name );
		conn->get_my_name( &Client::on_get_my_name );
		conn->set_my_name( "挫比1", &Client::on_set_my_name );
		conn->get_my_name( &Client::on_get_my_name );
		conn->set_my_name( "挫比2", &Client::on_set_my_name );
		conn->get_my_name( &Client::on_get_my_name );
		conn->set_my_name( "挫比3", &Client::on_set_my_name );
		conn->get_my_name( &Client::on_get_my_name );
		conn->set_my_name( "挫比4", &Client::on_set_my_name );
		conn->get_my_name( &Client::on_get_my_name );
		conn->set_my_name( "挫比5", &Client::on_set_my_name );
		conn->get_my_name( &Client::on_get_my_name );
		conn->who_am_i("abc", "123", &Client::on_who_am_i );
		conn->who_am_i("abcd", "1234", &Client::on_who_am_i );
		conn->who_am_i("abce", "12345", &Client::on_who_am_i );
		conn->who_am_i("abcf", "123456", &Client::on_who_am_i );
		conn->who_am_i("abcg", "1234567", &Client::on_who_am_i );
		conn->who_am_i("abch", "12345678", &Client::on_who_am_i );
	}

	static void on_set_my_name( const remote_call_error& err )
	{
		if (err)
		{
			cout<<"调用on_set_my_name失败："<<err.msg<<endl;
		}
		else
		{
			cout<<"调用on_set_my_name成功。"<<endl;
		}
	}

	static void on_get_my_name( const remote_call_error& err, const string& s )
	{
		if (err)
		{
			cout<<"调用on_get_my_name失败："<<err.msg<<endl;
		}
		else
		{
			cout<<"调用on_get_my_name成功，返回："<<endl<<s<<endl;
		}
	}

	static void on_who_am_i( const remote_call_error& err, const string& s)
	{
		if (err)
		{
			cout<<"调用who_am_i(\"abc\")失败："<<err.msg<<endl;
		}
		else
		{
			cout<<"调用who_am_i(\"abc\")成功，返回："<<endl<<s<<endl;
		}
	}

} ;


void start_client()
{
	Interface::connect<Client>( "localhost", 7844 );
}