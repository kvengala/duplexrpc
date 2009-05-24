#include "stdafx.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <vector>
#include "connection.hpp" // Must come before boost/serialization headers.
#include <boost/serialization/vector.hpp>
#include "stock.hpp"

/// Downloads stock quote information from a server.
class client
{
public:
  /// Constructor starts the asynchronous connect operation.
  client(boost::asio::io_service& io_service,
      const std::string& host, const std::string& service)
    : connection_(io_service)
  {
    // Resolve the host name into an IP address.
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(host, service);
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator =
      resolver.resolve(query);
    boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;

    // Start an asynchronous connect operation.
    connection_.lowest_layer().async_connect(endpoint,
        boost::bind(&client::handle_connect, this,
          boost::asio::placeholders::error, ++endpoint_iterator));
  }

  /// Handle completion of a connect operation.
  void handle_connect(const boost::system::error_code& error,
      boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
  {
    if (!error)
    {
			// Successfully established connection. Start operation to read the list
			// of stocks. The connection::async_read() function will automatically
			// decode the data that is read from the underlying socket.
			connection_.async_read(stocks_,
				boost::bind(&client::handle_read, this,
				boost::asio::placeholders::error));
    }
    else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
    {
      // Try the next endpoint.
      connection_.lowest_layer().close();
      boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
      connection_.lowest_layer().async_connect(endpoint,
          boost::bind(&client::handle_connect, this,
            boost::asio::placeholders::error, ++endpoint_iterator));
    }
    else
    {
      // An error occurred. Log it and return. Since we are not starting a new
      // operation the io_service will run out of work to do and the client will
      // exit.
      std::cerr << error.message() << std::endl;
    }
  }

  /// Handle completion of a read operation.
  void handle_read(const boost::system::error_code& error)
  {
    if (!error)
    {
      // Print out the data that was received.
      for each (stock st in stocks_)
				std::cout<< st;

			std::cout<<"  TID: "<<boost::this_thread::get_id()<< std::endl;

			connection_.async_read(stocks_,
				boost::bind(&client::handle_read, this,
				boost::asio::placeholders::error));
    }
    else
    {
      // An error occurred.
      std::cerr << error.message() << std::endl;
    }

    // Since we are not starting a new operation the io_service will run out of
    // work to do and the client will exit.
  }

private:
  /// The connection to the server.
  connection connection_;

  /// The data received from the server.
  std::vector<stock> stocks_;
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

int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 4)
    {
      std::cerr << "Usage: client <threads> <host> <port>" << std::endl;
      return 1;
    }

		unsigned short threadcount = boost::lexical_cast<unsigned short>(argv[1]);

    boost::asio::io_service io_service;

    client clnt(io_service, argv[2], argv[3]);

		// Set console control handler to allow client to be stopped.
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
