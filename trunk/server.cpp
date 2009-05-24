#include "server.h"

#define DRI_IS_SERVER
#include "interface.h"
using namespace boost;

class Server : public rpc::ServerPeer<Interface>
{
	string clientName;
public:
	string who_am_i(const string& name, const string& adj)
	{
		return name+" «Õ∑"+adj+"÷Ì£°";
	}

	string get_my_name()
	{
		return clientName;
	}

	void set_my_name( const string& name )
	{
		clientName = name;
	}
};

void start_server()
{
	Interface::listen<Server>( 7844 );
}