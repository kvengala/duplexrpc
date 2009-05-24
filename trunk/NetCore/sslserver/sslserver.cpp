#include "stdafx.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <vector>
#include "sslconnection.hpp" // Must come before boost/serialization headers.
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
		boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
		context_(io_service, boost::asio::ssl::context::sslv23)
	{
		context_.set_options(
			boost::asio::ssl::context::default_workarounds
			| boost::asio::ssl::context::no_sslv2
			| boost::asio::ssl::context::single_dh_use);
		context_.set_password_callback(boost::bind(&server::get_password, this));
		context_.use_certificate_chain_file("server.pem");
		context_.use_private_key_file("server.pem", boost::asio::ssl::context::pem);
		context_.use_tmp_dh_file("dh512.pem");


		// Start an accept operation for a new connection.
		connection_ptr new_conn(new connection(acceptor_.io_service(), context_));
		acceptor_.async_accept(new_conn->lowest_layer (),
			boost::bind(&server::handle_accept, this,
			boost::asio::placeholders::error, new_conn));
	}

	std::string get_password() const
	{
		return "test";
	}

	/// Handle completion of a accept operation.
	void handle_accept(const boost::system::error_code& error, connection_ptr conn)
	{
		if (!error)
		{
			conn->async_handshake(boost::asio::ssl::stream_base::server, 
				boost::bind(&server::handle_handshake, this,
				boost::asio::placeholders::error, conn));

			// Start an accept operation for a new connection.
			connection_ptr new_conn(new connection(acceptor_.io_service(), context_));
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

	void handle_handshake(const boost::system::error_code& error, connection_ptr conn)
	{
		if (!error)
		{
			// Successfully accepted a new connection. Send the list of stocks to the
			// client. The connection::async_write() function will automatically
			// serialize the data structure for us.
			conn->async_write(g_stocks,
				boost::bind(&server::handle_write, this,
				boost::asio::placeholders::error, conn));
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
		// Nothing to do. The socket will be closed automatically when the last
		// reference to the connection object goes away.
	}

private:
	/// The acceptor object used to accept incoming socket connections.
	boost::asio::ip::tcp::acceptor acceptor_;

	boost::asio::ssl::context context_;
};
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
		if (argc != 2)
		{
			std::cerr << "Usage: server <port>" << std::endl;
			return 1;
		}

		init_stocks();

		unsigned short port = boost::lexical_cast<unsigned short>(argv[1]);

		boost::asio::io_service io_service;
		server svr(io_service, port);
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
