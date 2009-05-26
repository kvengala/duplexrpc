#pragma once
#include "rpc_function_set.hpp"
#include <queue>
#include <boost/lexical_cast.hpp>
#include "connection.hpp"

/**
* 此命名空间包括了此远程过程调用(RPC)实现的所有代码。
*/
namespace rpc{
	/**
	* 此peer实现的细节。
	*/
	namespace detail{

		template<class T>
		struct result_handler_type
		{
			typedef RPC_FUNCTION_CLASS< void ( const remote_call_error&, T& ) > type;
			static void call_connection_been_closed_error( const type& func )
			{
				T t;
				func( remote_call_error(remote_call_error::connection_error, "Connection has been closed."), t );
			}
		} ;

		template<>
		struct result_handler_type<void>
		{
			typedef RPC_FUNCTION_CLASS< void ( const remote_call_error& ) > type;
			static void call_connection_been_closed_error( const type& func )
			{
				func( remote_call_error(remote_call_error::connection_error, "Connection has been closed.") );
			}
		} ;

		/**
		* 此类的子类将一个已序列化的串反序列化为它原本的类型。
		*/
		class RetValUnserializerBase : private boost::noncopyable{
		public:
			virtual void parsePacket( const packet& resultPak, iarchive& iar_result ) = 0;
			virtual void call_handler_with_error(const remote_call_error&  err) = 0;
			virtual ~RetValUnserializerBase(){}
		};

		/**
		* 此类将一个已序列化的串反序列化为它原本的类型。
		*/
		template<class RetType>
		class RetValUnserializer : public RetValUnserializerBase{
		public:
			RetValUnserializer( const typename result_handler_type<RetType>::type& hndler):handler(hndler){}

			/** 完成后的回调。 */
			typename result_handler_type<RetType>::type handler;

			virtual void parsePacket( const packet& resultPak, iarchive& iar_result )
			{
				RetType retVal;
				switch(resultPak.err.code)
				{
				case remote_call_error::succeeded:
					{
						try
						{
							//获得包头之后的返回值。
							iar_result>>retVal;
							handler(resultPak.err, retVal);
						}
						catch(const boost::archive::archive_exception& e)
						{
							RetType retVal;
							handler( remote_call_error(remote_call_error::return_type_mismatch, e.what()), retVal);
						}
					}
					break;
				default:
					handler(resultPak.err, retVal);
					break;
				}
			}
			virtual void call_handler_with_error(const remote_call_error& err)
			{
				RetType retVal;
				handler(err, retVal);
			}
		};

		template<>
		class RetValUnserializer<void> : public RetValUnserializerBase{
		public:
			RetValUnserializer( const result_handler_type<void>::type& hndler):handler(hndler){}

			/** 完成后的回调。 */
			result_handler_type<void>::type handler;

			virtual void parsePacket( const packet& resultPak, iarchive& iar_result )
			{
				switch(resultPak.err.code)
				{
				case remote_call_error::succeeded:
					{
						try
						{
							//获得包头之后的返回值。
							handler ( resultPak.err );
						}
						catch(const boost::archive::archive_exception& e)
						{
							handler( remote_call_error(remote_call_error::return_type_mismatch, e.what()) );
						}
					}
					break;
				default:
					handler( resultPak.err );
					break;
				}
			}
			virtual void call_handler_with_error(const remote_call_error& err)
			{
				handler( err );
			}
		};

		typedef RPC_SHARED_PTR<RetValUnserializerBase> RetValUnserializerPtr;
	}

	/**
	* TransmitterConcept: 
	* {
	*     void async_read( std::string& data, const function<void(const boost::system::error_code& error)>& handler );
	*     void async_write( const std::string& data, const function<void(const boost::system::error_code& error)>& handler );
	* }
	*/

	/**
	* RPC连接的一端(peer)。每一个RPC连接都对应于两个peer。
	* 每一个peer既可以向对方提供RPC函数调用服务，也可以请求调用对方提供的RPC函数。
	* 在外部不可以保存peer的shared_ptr，因为这会破坏peer在连接中断自动析构的特性。
	* @param TransmitterType A type of TransmitterConcept.
	* concept TransmitterConcept : public boost::asio::ip::tcp::socket
	* {
	*     void async_read( buffer_type& data, const function<void(const boost::system::error_code& error)>& handler );
	*     void async_write( const buffer_type& data, const function<void(const boost::system::error_code& error)>& handler );
	* }
	*/
	template< class TransmitterType = connection >
	class peer : public boost::noncopyable, public RPC_ENABLE_SHARED_FROM_THIS<peer<TransmitterType> >
	{
		typedef std::string buffer_type;
		typedef RPC_SHARED_PTR<std::string> buffer_ptr;
		typedef RPC_SHARED_PTR<function_set> function_set_ptr;
		function_set_ptr p_func_set;
		typedef std::queue<detail::RetValUnserializerPtr> ser_queue_type;
		typedef TransmitterType transmitter_type;
		typedef boost::asio::ip::tcp::acceptor acceptor_type;
		typedef boost::asio::ip::tcp::endpoint endpoint_type;

		ser_queue_type serializerqueue;
		volatile bool peer_started;
		buffer_type read_buffer;
	public:
		typedef peer<TransmitterType> this_type;
		typedef RPC_SHARED_PTR<this_type> peer_ptr;
		typedef RPC_WEAK_PTR<this_type> peer_weak_ptr;
		/**
		* 异常：peer已经开始。
		*/
		class peer_already_started : public std::runtime_error
		{
		public:
			peer_already_started():std::runtime_error("The peer has been started. A peer could be started only once."){}
		};
		/**
		* 异常：request包与result包不匹配。
		*/
		class request_result_not_match : public std::runtime_error
		{
		public:
			request_result_not_match():std::runtime_error("Fatal error: request packets do not match result packets."){}
		};

		typedef RPC_FUNCTION_CLASS<void(const peer_ptr&)> on_accepted_type;

	public:
		/* Callback functions: */
		RPC_FUNCTION_CLASS<void(void)> on_peer_started, on_peer_closed;
	private:
		transmitter_type socket_;
	private:

		/**
		* 构造函数。
		* @param p_connection 此peer使用的连接。
		* @param other_peer_to_share_functions 要与之共享RPC函数的另一peer。为空则自己新建一个function_set。
		* @see set_connection
		*/
		peer(boost::asio::io_service& acceptor_io_service, const function_set_ptr& p_functions)
			:socket_(acceptor_io_service), p_func_set(p_functions), peer_started(false)
		{
		}

		transmitter_type& socket()
		{
			return socket_;
		}

		class listener{
			acceptor_type acceptor_;
			function_set_ptr p_functions_;
			on_accepted_type on_accepted_;
		public:
			listener( boost::asio::io_service& io_service_, const function_set_ptr& p_functions, int port,
				const on_accepted_type& on_accepted
				)
				:acceptor_(io_service_, endpoint_type(boost::asio::ip::tcp::v4(), port)), p_functions_(p_functions),
				on_accepted_(on_accepted)
			{
				start_accept();
			}
			void start_accept()
			{
				peer_ptr p(new this_type(acceptor_.get_io_service(), p_functions_));
				acceptor_.async_accept(p->socket(),
					RPC_BIND_FUNCTION(&listener::handle_accept, this, p, RPC__1));
			}
			void handle_accept(const peer_ptr& new_peer, const boost::system::error_code& error)
			{
				if (!error)
				{
					if(on_accepted_)
						on_accepted_(new_peer);
					new_peer->start();
				}
				start_accept();
			}
		};

	public:

		/**
		* 开始监听。将阻塞此线程。
		* @param port 端口号。
		* @param p_functions 
		* @param on_accepted 在接受了一个连接、但还未调用peer::start()之前，调用此回调函数。
		*/
		static void listen(int port, const function_set_ptr& p_functions = function_set_ptr(),
			const on_accepted_type& on_accepted = on_accepted_type()
			)
		{
			boost::asio::io_service io_service_;
			listener listener_( io_service_, p_functions, port, on_accepted );
			io_service_.run();
		}

		/**
		* 连接。将阻塞此线程。
		*/
		static void connect(const std::string& host, int port, const function_set_ptr& p_functions,
			RPC_FUNCTION_CLASS<void(const boost::system::error_code&, const peer_ptr&)> on_connected)
		{
			using namespace boost::asio;
			using namespace boost::asio::ip;
			using namespace boost;

			io_service io_service_;

			tcp::resolver resolver(io_service_);
			tcp::resolver::query query(ip::tcp::v4(), host, lexical_cast<std::string>(port));
			tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
			tcp::resolver::iterator end;

			peer_ptr new_peer(new this_type(io_service_, p_functions));
			boost::system::error_code error = boost::asio::error::host_not_found;
			while (error && endpoint_iterator != end)
			{
				(new_peer->socket()).close();
				(new_peer->socket()).connect(*endpoint_iterator++, error);
			}

			if (error)
			{
				on_connected(error, peer_ptr());
			}
			else
			{
				on_connected(error, new_peer);
				new_peer->start();
			}

			io_service_.run();
		}

		/**
		* 获得此peer所使用函数集总共被使用的次数.
		* @return 此peer所使用函数集总共被使用的次数.
		*/
		long get_func_set_use_count()
		{
			return p_func_set.use_count();
		}

		/**
		* 设置此peer使用的RPC函数集。
		* @param p_func_set RPC函数集。
		*/
		void set_function_set(function_set_ptr p_fnc_set)
		{
			boost::recursive_mutex::scoped_lock lck(mtx);
			p_func_set = p_fnc_set;
		}

		/**
		* 获得此peer使用的RPC函数集。
		* @return RPC函数集。
		*/
		function_set_ptr get_function_set()
		{
			return p_func_set;
		}

		~peer()
		{
			close();
			if ( on_peer_closed ) on_peer_closed();
		}

		/**
		* 关闭此连接.
		*/
		void close()
		{
			boost::recursive_mutex::scoped_lock lck(mtx);
			peer_started = false;
			socket_.close();		//socket_关闭后，writing_thread也会自动返回。
			on_socket_closed();
		}

		/**
		* 开始此peer的监听循环。不会阻塞。
		*/
		void start()
		{
			if (peer_started)
			{
				throw peer_already_started();
			}
			peer_started = true;

			void* ptr = &read_buffer;

			//异步读。一个包接一个包地读。
			socket_.async_read( read_buffer,
				RPC_BIND_FUNCTION( &peer::on_received_packet, shared_from_this(), RPC__1 ) );

			// call back.
			if ( on_peer_started ) on_peer_started();
		}

	private:
		boost::recursive_mutex mtx;

		void put_buffer_into_write_queue(buffer_ptr buf)
		{
			socket_.async_write( *buf,
				RPC_BIND_FUNCTION( &this_type::on_write_finished, shared_from_this(), RPC__1 ) );
		}

		void on_received_packet( const boost::system::error_code& err )
		{
			boost::recursive_mutex::scoped_lock lck(mtx);
			if (err)
			{
				close();
				return;
			}

			std::istringstream istrm(read_buffer);
			iarchive iar(istrm, detail::archive_flags);
			packet buff;
			iar>>buff;


			if (buff.is_request)
			{
				buffer_ptr write_buf_ptr(new buffer_type());
				std::ostringstream ostrm( std::ios::out | std::ios::binary );
				oarchive oar(ostrm, detail::archive_flags);
				if (p_func_set)
				{
					p_func_set->invoke(buff.func_id, iar, oar);
				}
				else
				{
					oar<<packet(false, 
						remote_call_error ( remote_call_error::no_rpc_service, "There's no RPC service on this peer." ),
						std::string() );
				}

				*write_buf_ptr = ostrm.str();

				put_buffer_into_write_queue(write_buf_ptr);
			}
			else
			{
				if ( !serializerqueue.empty() )
				{
					serializerqueue.front()->parsePacket( buff, iar );
					if(!serializerqueue.empty())
						serializerqueue.pop();
				}
				else
				{
					throw request_result_not_match();
				}
			}

			socket_.async_read( read_buffer,
				RPC_BIND_FUNCTION( &peer::on_received_packet, shared_from_this(), RPC__1 ) );
		}

		void on_socket_closed()
		{
			while ( !serializerqueue.empty() )
			{
				serializerqueue.front()->call_handler_with_error(remote_call_error(remote_call_error::connection_error, 
					"The connection has been closed."));
				serializerqueue.pop();
			}
		}

		void on_write_finished( const boost::system::error_code& err )
		{
			boost::recursive_mutex::scoped_lock lck(mtx);
			if (err)
			{
				close();
				return;
			}
		}

	public:

		/**
		* 试图发起远程调用。
		* @param funcId 要调用的函数id.
		* @param handler 远程调用过程完成后将调用此回调。
		* @exception boost::archive::archive_exception 序列化参数时出错。
		*/
		template<class RetType>
		void remote_call(const std::string& func_id, const typename detail::result_handler_type<RetType>::type& handler)
			throw(boost::archive::archive_exception)
		{
			boost::recursive_mutex::scoped_lock lck(mtx);
			if(socket_.is_open())
			{
				buffer_ptr write_buf_ptr( new buffer_type() );
				std::ostringstream strm;
				oarchive strm_buf_oar( strm, detail::archive_flags );
				strm_buf_oar << packet(true, remote_call_error(), func_id);
				serializerqueue.push(detail::RetValUnserializerPtr( new detail::RetValUnserializer<RetType>( handler ) ) );
				*write_buf_ptr = strm.str();
				put_buffer_into_write_queue( write_buf_ptr );
			}
			else
			{
				detail::result_handler_type<RetType>::call_connection_been_closed_error( handler );
			}
		}

#define remoteCall_BEGIN \
	template<class RetType, 
#define remoteCall_BEGIN2 >\
	void remote_call(const std::string& func_id,
#define remoteCall_BEGIN3 \
	const typename detail::result_handler_type<RetType>::type& handler)\
	throw(boost::archive::archive_exception)\
		{\
		boost::recursive_mutex::scoped_lock lck(mtx);\
		if(socket_.is_open()){\
		buffer_ptr write_buf_ptr(new buffer_type());\
		std::ostringstream strm;\
		oarchive oar(strm, detail::archive_flags);\
		packet pak(true, remote_call_error(), func_id);\
		oar<<pak;
#define remoteCall_END \
	serializerqueue.push(detail::RetValUnserializerPtr(new detail::RetValUnserializer<RetType>(handler)));\
	*write_buf_ptr = strm.str();\
	put_buffer_into_write_queue(write_buf_ptr);\
		}else{\
		detail::result_handler_type<RetType>::call_connection_been_closed_error( handler );\
		}\
		}

		//remoteCall 1 param:
		remoteCall_BEGIN class T1 remoteCall_BEGIN2 const T1& t1, remoteCall_BEGIN3
			oar<<t1;
		remoteCall_END;

		//remoteCall 2 param:
		remoteCall_BEGIN class T1, class T2 remoteCall_BEGIN2 const T1& t1, const T2& t2, remoteCall_BEGIN3
			oar<<t1;
		oar<<t2;
		remoteCall_END;

		//remoteCall 3 param:
		remoteCall_BEGIN class T1, class T2, class T3 remoteCall_BEGIN2 const T1& t1, const T2& t2, const T3& t3, remoteCall_BEGIN3
			oar<<t1;
		oar<<t2;
		oar<<t3;
		remoteCall_END;

		//remoteCall 4 param:
		remoteCall_BEGIN class T1, class T2, class T3, class T4 remoteCall_BEGIN2 const T1& t1, const T2& t2, const T3& t3, const T4& t4, remoteCall_BEGIN3
			oar<<t1;
		oar<<t2;
		oar<<t3;
		oar<<t4;
		remoteCall_END;

		//remoteCall 5 param:
		remoteCall_BEGIN class T1, class T2, class T3, class T4, class T5 remoteCall_BEGIN2 const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, remoteCall_BEGIN3
			oar<<t1;
		oar<<t2;
		oar<<t3;
		oar<<t4;
		oar<<t5;
		remoteCall_END;

	};
}