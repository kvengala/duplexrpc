// LuguodePeer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#define DRI_IS_SERVER
#include "LuguodeInterface.h"
#include <list>
using namespace std;
using namespace boost;

class LuguodePeer : public enable_shared_from_this<LuguodePeer>, public rpc::ServerPeer<LuguodeInterface>
{
private:
	static mutex static_mtx;
	// 连接。
	typedef shared_ptr<LuguodePeer> LuguodePeerPtr;
	typedef weak_ptr<LuguodePeer> LuguodePeerWeakPtr;
	typedef list< LuguodePeerPtr > ConnectionListType;
	static ConnectionListType waiting_connections;
private:
	shared_ptr<LuguodeInterface> client_connection;
	LuguodePeerWeakPtr stranger;
	bool talking;
public:
	void on_connected( const shared_ptr<LuguodeInterface>& conn )
	{
		talking = false;
		client_connection = conn;
	}

	void talk( const std::wstring& msg )
	{
		if ( talking )
		{
			LuguodePeerPtr p = stranger.lock();
			if ( p )
			{
				p->client_connection->talk( msg,
					bind( &LuguodePeer::close_on_error, this, _1 ) );
			}
		}
		else
		{
			client_connection->notify( L"服务器正在为你连接到某个路人甲...请稍等。",
					bind( &LuguodePeer::close_on_error, this, _1 ) );
		}
	}

	void on_connection_in_place()
	{
		{
			mutex::scoped_lock lck(static_mtx);

			//看看是否还有人在等待。
			if ( waiting_connections.empty() )
			{
				//Add to waiting_connections list.
				waiting_connections.push_back( shared_from_this() );
			}
			else
			{
				LuguodePeerPtr another_server = waiting_connections.front();
				waiting_connections.pop_front();

				stranger = another_server;
				another_server->stranger = shared_from_this();

				talking = true;
				client_connection->stranger_come_in( bind( &LuguodePeer::close_on_error, this, _1 ) );
				another_server->talking = true;
				another_server->client_connection->stranger_come_in( bind( &LuguodePeer::close_on_error, another_server.get(), _1 ) );
			}
		}
	}

	void on_connection_closed()
	{
		if ( !talking )
		{
			waiting_connections.remove( shared_from_this() );
		}
		else
		{
			LuguodePeerPtr p = stranger.lock();
			if ( p )
			{
				p->client_connection->notify( L"路人甲已经断开连接。你可以重新连接来继续和另一位路人甲聊天。",
					bind( &LuguodePeer::close_on_call_completed, this, _1 ) );
				p->client_connection->close();
			}
		}
	}

private:
	void close_on_error( const rpc::remote_call_error& err )
	{
		if ( err )
		{
			if ( err.code != rpc::remote_call_error::connection_error )
			{
				client_connection->close();
			}
			output_msg( err.what() );
		}
	}
	
	void close_on_call_completed( const rpc::remote_call_error& err )
	{
		if ( err.code != rpc::remote_call_error::connection_error )
		{
			client_connection->close();
		}
	}

	static void output_msg( const string& msg )
	{
		mutex::scoped_lock lck( cout_mtx );
		cout << msg << endl;
	}

	static void output_msg( const wstring& msg )
	{
		mutex::scoped_lock lck( cout_mtx );
		wcout << msg << endl;
	}

	static mutex cout_mtx;
} ;

mutex LuguodePeer::static_mtx, LuguodePeer::cout_mtx;
LuguodePeer::ConnectionListType LuguodePeer::waiting_connections;

int _tmain(int argc, _TCHAR* argv[])
{
	int port = 9125;
	cout << "开始监听" << port << "端口。" << endl << flush;
	LuguodeInterface::listen<LuguodePeer>( port );
	return 0;
}

