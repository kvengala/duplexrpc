#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>

#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <algorithm>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

/// The connection class provides serialization primitives on top of a socket.
/**
* Each message sent using this class consists of:
* @li An 8-byte header containing the length of the serialized data in
* hexadecimal.
* @li The serialized data.
*/
class connection : public ssl_socket, public boost::enable_shared_from_this<connection>
{
	typedef boost::archive::binary_oarchive oar;
	typedef boost::archive::binary_iarchive iar;
	typedef boost::shared_ptr<std::string> buffer_ptr;
	typedef boost::shared_ptr<connection> shared_this_type; 
public:
	typedef boost::function<void(const boost::system::error_code& error)> Handler;
	/// Constructor.
	connection(boost::asio::io_service& io_service, boost::asio::ssl::context& context)
		: ssl_socket(io_service, context),
		write_pending_(false),
		read_pending_(false)
	{
	}

	~connection()
	{
		assert (wop_buf_.empty() == true);
		assert (rop_buf_.empty() == true);
	}


	/// Asynchronously write a data structure to the socket.
	template <typename T>
	void async_write(const T& t, Handler handler)
	{
		boost::recursive_mutex::scoped_lock lck(wmtx_);

		buffer_ptr outbound_header_(new std::string);
		std::string outbound_data_;

		std::ostringstream out;
		{
			oar oa(out);
			oa << t;
		}// archive have to be closed before out.pop()

		outbound_data_ = out.str();

		// Format the header.
		std::ostringstream header_stream;
		header_stream << std::setw(header_length)
			<< std::hex << outbound_data_.size();
		if (!header_stream || header_stream.str().size() != header_length)
		{
			// Something went wrong, inform the caller.
			boost::system::error_code error(boost::asio::error::invalid_argument);
			handler(error);
			//io_service().post(boost::bind(handler, error));
			return;
		}

		//combine the header + data [size + actual data]
		*outbound_header_ = header_stream.str() + outbound_data_;

		if (write_pending_ == true)
		{
			wop_buf_.push_back(WOpElem(outbound_header_, handler));
		}
		else
		{
			write_pending_ = true;
			boost::asio::async_write(*this, boost::asio::buffer(*outbound_header_), 
				boost::bind(&connection::handle_async_write, shared_from_this(), boost::asio::placeholders::error,
				outbound_header_, handler));
		}
	}

private:
	/// free the data buffer after the async_write complete
	void handle_async_write(const boost::system::error_code& error,
		buffer_ptr dataPtr,
		Handler handler)
	{
		boost::recursive_mutex::scoped_lock lck(wmtx_);

		WOpContainer::iterator itor = std::find(wop_buf_.begin(), wop_buf_.end(), dataPtr);
		if (itor != wop_buf_.end())
			wop_buf_.erase (itor);

		handler(error);

		if (wop_buf_.empty())
		{
			write_pending_ = false;
		}
		else
		{
			write_pending_ = true;

			WOpContainer::iterator itor = wop_buf_.begin();

			boost::asio::async_write(*this, boost::asio::buffer(*(itor->dataPtr_)), 
				boost::bind(&connection::handle_async_write, shared_from_this(), boost::asio::placeholders::error,
				itor->dataPtr_, itor->opHandler_));
		}
	}

public:
	/// Asynchronously read a data structure from the socket.
	template <typename T>
	void async_read(T& t, Handler handler)
	{
		boost::recursive_mutex::scoped_lock lck(rmtx_);

		buffer_ptr inbound_header_(new std::string(header_length, 0));

		if (read_pending_ == true)
		{
			rop_buf_.push_back(ROpElem(inbound_header_, handler, reinterpret_cast<void*>(&t)));
		}
		else
		{
			read_pending_ = true;
			// Issue a read operation to read exactly the number of bytes in a header.
			boost::asio::async_read(*this, 
				boost::asio::buffer(const_cast<char*>(inbound_header_->c_str ()), header_length),
				boost::bind(&connection::handle_read_header<T>,
				shared_from_this(), boost::asio::placeholders::error, boost::ref(t), inbound_header_, handler));
		}
	}

private:
	/// Handle a completed read of a message header.
	template <typename T>
	void handle_read_header(const boost::system::error_code& err,
		T& t, buffer_ptr headPtr, Handler handler)
	{
		boost::recursive_mutex::scoped_lock lck(rmtx_);

		assert (read_pending_);

		if (err)
		{
			handler(err);

			freeReadOpAndStartNew<T>(headPtr);
		}
		else
		{
			// Determine the length of the serialized data.
			std::istringstream is(*headPtr);
			std::size_t inbound_data_size = 0;
			if (!(is >> std::hex >> inbound_data_size))
			{
				// Header doesn't seem to be valid. Inform the caller.
				boost::system::error_code error(boost::asio::error::invalid_argument);
				handler(error);
				freeReadOpAndStartNew<T>(headPtr);
				return;
			}

			// Start an asynchronous call to receive the data.
			// reuse the headPtr
			headPtr->resize(inbound_data_size);
			boost::asio::async_read(*this, 
				boost::asio::buffer(const_cast<char*>(headPtr->c_str ()),inbound_data_size),
				boost::bind(&connection::handle_read_data<T>, shared_from_this(),
				boost::asio::placeholders::error, boost::ref(t), headPtr, handler));
		}
	}

	/// Handle a completed read of message data.
	template <typename T>
	void handle_read_data(const boost::system::error_code& err,
		T& t, buffer_ptr dataPtr, Handler handler)
	{
		boost::recursive_mutex::scoped_lock lck(rmtx_);

		if (err)
		{
			handler(err);

			freeReadOpAndStartNew<T>(dataPtr);
		}
		else
		{
			// Extract the data structure from the data just received.
			try
			{
				std::istringstream in(*dataPtr);
				{
					iar ia(in);
					ia >> t;
				}
			}
			catch (std::exception& /*e*/)
			{
				// Unable to decode data.
				boost::system::error_code error(boost::asio::error::invalid_argument);
				handler(error);
				freeReadOpAndStartNew<T>(dataPtr);
				return;
			}

			freeReadOpAndStartNew<T>(dataPtr);
		}
		// Inform caller that data has been received ok.
		handler(err);
		//io_service().post(boost::bind(handler, err));
	}

	template <typename T>
	void freeReadOpAndStartNew( buffer_ptr dataPtr)
	{
		//free the read op
		ROpContainer::iterator itor = std::find(rop_buf_.begin(), rop_buf_.end(), dataPtr);
		if (itor != rop_buf_.end())
			rop_buf_.erase (itor);

		if (rop_buf_.empty() == false)
		{//发起下一个读取
			read_pending_ = true;

			itor = rop_buf_.begin();

			// Issue a read operation to read exactly the number of bytes in a header.
			boost::asio::async_read(*this, 
				boost::asio::buffer(const_cast<char*>(itor->dataPtr_->c_str ()), header_length),
				boost::bind(&connection::handle_read_header<T>,
				shared_from_this(), boost::asio::placeholders::error, 
				boost::ref(*reinterpret_cast<T*>(itor->dataPos_)),
				itor->dataPtr_, itor->opHandler_));
		}
		else
		{
			read_pending_ = false;
		}
	}

private:
	/// The size of a fixed length header.
	enum { header_length = 8 };

	/// make async_write thread safe
	boost::recursive_mutex wmtx_;

	/// make async_read thread safe
	boost::recursive_mutex rmtx_;

	/// write_buf
	struct WOpElem{
		WOpElem(const buffer_ptr& ptr, const Handler& handler):dataPtr_(ptr), opHandler_(handler){}
		buffer_ptr dataPtr_;
		Handler opHandler_;
		bool operator ==(buffer_ptr const& _Right) const
		{
			return dataPtr_ == _Right;
		}
	};

	/// read_buf
	struct ROpElem{
		ROpElem(const buffer_ptr& ptr, const Handler& handler, void* posPtr):
			dataPtr_(ptr), opHandler_(handler), dataPos_(posPtr){}
		buffer_ptr dataPtr_;
		Handler opHandler_;
		void* dataPos_;
		bool operator ==(buffer_ptr const& _Right) const
		{
			return dataPtr_ == _Right;
		}
	};
	typedef std::list<WOpElem> WOpContainer;
	//pending write operation
	WOpContainer wop_buf_;

	typedef std::list<ROpElem> ROpContainer;
	//pending read operation
	ROpContainer rop_buf_;

	/// is there any pending write
	bool write_pending_;

	/// is there any pending read
	bool read_pending_;
};

typedef boost::shared_ptr<connection> connection_ptr;
