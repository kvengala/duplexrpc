#include "stdafx.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <vector>
#include "connection.hpp" // Must come before boost/serialization headers.
#include <boost/serialization/vector.hpp>
#include "stock.hpp"

/// The data to be sent to each client.
std::vector<stock> g_stocks;

/// Serves stock quote information to any client that connects to it.
class server
{
public:
	/// Constructor opens the acceptor and starts waiting for the first incoming
	/// connection.
	server(boost::asio::io_service& io_service, unsigned short port)
		: acceptor_(io_service,
		boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	{
		// Start an accept operation for a new connection.
		connection_ptr new_conn(new connection(io_service));
		acceptor_.async_accept(new_conn->lowest_layer (),
			boost::bind(&server::handle_accept, this,
			boost::asio::placeholders::error, new_conn));
	}

	/// Handle completion of a accept operation.
	void handle_accept(const boost::system::error_code& error, connection_ptr conn)
	{
		if (!error)
		{
			// Successfully accepted a new connection. Send the list of stocks to the
			// client. The connection::async_write() function will automatically
			// serialize the data structure for us.
			conn->async_write(g_stocks,
				boost::bind(&server::handle_write, this,
				boost::asio::placeholders::error, conn));

			// Start an accept operation for a new connection.
			connection_ptr new_conn(new connection(acceptor_.io_service()));
			acceptor_.async_accept(new_conn->lowest_layer (),
				boost::bind(&server::handle_accept, this,
				boost::asio::placeholders::error, new_conn));
		}
		else
		{
			// An error occurred. Log it and return. Since we are not starting a new
			// accept operation the io_service will run out of work to do and the
			// server will exit.
			std::cerr << error.message() << std::endl;
		}
	}

	/// Handle completion of a write operation.
	void handle_write(const boost::system::error_code& e, connection_ptr conn)
	{
		if(e)
		{
			std::cerr << e.message() << std::endl;
			return;
		}

		conn->async_write(g_stocks,
			boost::bind(&server::handle_write, this,
			boost::asio::placeholders::error, conn));
		// Nothing to do. The socket will be closed automatically when the last
		// reference to the connection object goes away.
	}

private:
	/// The acceptor object used to accept incoming socket connections.
	boost::asio::ip::tcp::acceptor acceptor_;
};

boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
	switch (ctrl_type)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		console_ctrl_function();
		return TRUE;
	default:
		return FALSE;
	}
}

void init_stocks()
{
	// Create the data to be sent to each client.
	stock s;
	s.code = "ABC";
	s.name = "A Big Company";
	s.open_price = 4.56;
	s.high_price = 5.12;
	s.low_price = 4.33;
	s.last_price = 4.98;
	s.buy_price = 4.96;
	s.buy_quantity = 1000;
	s.sell_price = 4.99;
	s.sell_quantity = 2000;
	g_stocks.push_back(s);
	s.code = "DEF";
	s.name = "Developer Entertainment Firm";
	s.open_price = 20.24;
	s.high_price = 22.88;
	s.low_price = 19.50;
	s.last_price = 19.76;
	s.buy_price = 19.72;
	s.buy_quantity = 34000;
	s.sell_price = 19.85;
	s.sell_quantity = 45000;
	g_stocks.push_back(s);
}

int main(int argc, char* argv[])
{
	try
	{
		// Check command line arguments.
		if (argc != 3)
		{
			std::cerr << "Usage: server <threads> <port>" << std::endl;
			return 1;
		}

		init_stocks();

		unsigned short threadcount = boost::lexical_cast<unsigned short>(argv[1]);
		unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);

		boost::asio::io_service io_service;
		server svr(io_service, port);

		// Set console control handler to allow server to be stopped.
		console_ctrl_function = boost::bind(&boost::asio::io_service::stop, boost::ref(io_service));
		SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

		boost::thread_group threads_;
		for (int i = 0; i!= threadcount; ++i)
			threads_.create_thread ( boost::bind( &boost::asio::io_service::run, boost::ref(io_service)) );
		threads_.join_all();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
