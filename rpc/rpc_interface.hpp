// This file provides some convenient macros for building uniform interface that may be useful while using Duplex RPC.
// Содм(Yang, Xiao), 2009.3.18
#pragma once

//Duplex RPC Interface (DRI for short)

#define DRI_DECLARE_CALLBACK(f) void _ ## f() { if ( this->f ) this->f(); }

#define DRI_BEGIN(IName) \
class IName{\
	rpc::peer_weak_ptr wp;\
public:\
	typedef IName this_type;\
	typedef RPC_SHARED_PTR<this_type> this_type_shared_ptr;\
	RPC_FUNCTION_CLASS<void(void)> on_connection_in_place, on_connection_closed;\
private:\
	DRI_DECLARE_CALLBACK(on_connection_in_place);\
	DRI_DECLARE_CALLBACK(on_connection_closed);\
public:\
	IName(const rpc::peer_weak_ptr& the_peer):wp(the_peer){}\
	bool close()\
	{\
		rpc::peer_ptr p = wp.lock();\
		if(p) p->close();\
		return p;\
	}

#define DRI_SERVER_LISTEN_BEGIN \
	template<class ObjT> static void on_accepted( const RPC_SHARED_PTR<rpc::peer>& this_peer )\
	{\
		this_type_shared_ptr conn( new this_type( this_peer ) );\
		RPC_SHARED_PTR<ObjT> obj( new ObjT( conn ) );\
		rpc::function_set_ptr p_functions( new rpc::function_set() );\
		this_peer->on_peer_started = RPC_BIND_FUNCTION( &this_type::_on_connection_in_place, conn );\
		this_peer->on_peer_closed = RPC_BIND_FUNCTION( &this_type::_on_connection_closed, conn );

#define DRI_SERVER_LISTEN_END \
		this_peer->set_function_set( p_functions );\
	}\
	template <class ObjT> static void listen( int port )\
	{\
		rpc::peer::listen( port, rpc::function_set_ptr(),\
			RPC_BIND_FUNCTION( &this_type::on_accepted<ObjT>, RPC__1 )\
		);\
	}

#define DRI_CLIENT_CONNECT_BEGIN \
	template<class ObjT> static void on_connected( const boost::system::error_code& err, const rpc::peer_ptr& this_peer,\
		ObjT* obj )\
	{\
		if ( err )\
		{\
			obj->on_connected( err, this_type_shared_ptr() );\
		}\
		else\
		{\
			this_type_shared_ptr conn( new this_type( this_peer ) );\
			this_peer->on_peer_started = RPC_BIND_FUNCTION( &this_type::_on_connection_in_place, conn );\
			this_peer->on_peer_closed = RPC_BIND_FUNCTION( &this_type::_on_connection_closed, conn );\
			obj->on_connected( err, conn );\
		}\
	}\
	template <class ObjT> static void connect( const char* host, int port, ObjT* obj )\
	{\
		rpc::function_set_ptr p_functions( new rpc::function_set() );

#define DRI_CLIENT_CONNECT_END \
		rpc::peer::connect( host, port, p_functions, \
			RPC_BIND_FUNCTION( &this_type::on_connected<ObjT>, RPC__1, RPC__2, obj ) );\
	}

#define _DRI_INIT_R_(RetT, FuncName, FuncSig, AddSig)\
		p_functions->add<RetT AddSig>(#FuncName, \
			RPC_BIND_FUNCTION( &ObjT::FuncName, obj

#define DRI_INIT_R0(RetT, FuncName) \
	_DRI_INIT_R_(RetT, FuncName, (void), ())\
	) );
#define DRI_INIT_R1(RetT, FuncName, T1) \
	_DRI_INIT_R_(RetT, FuncName, (const T1&), (T1))\
	, RPC__1 ) );
#define DRI_INIT_R2(RetT, FuncName, T1, T2) \
	_DRI_INIT_R_(RetT, FuncName, (const T1&, const T2&), (T1, T2))\
	, RPC__1, RPC__2 ) );
#define DRI_INIT_R3(RetT, FuncName, T1, T2, T3) \
	_DRI_INIT_R_(RetT, FuncName, (const T1&, const T2&, const T3&), (T1, T2, T3))\
	, RPC__1, RPC__2, RPC__3 ) );
#define DRI_INIT_R4(RetT, FuncName, T1, T2, T3, T4) \
	_DRI_INIT_R_(RetT, FuncName, (const T1&, const T2&, const T3&, const T4&), (T1, T2, T3, T4))\
	, RPC__1, RPC__2, RPC__3, RPC__4 ) );
#define DRI_INIT_R5(RetT, FuncName, T1, T2, T3, T4, T5) \
	_DRI_INIT_R_(RetT, FuncName, (const T1&, const T2&, const T3&, const T4&, const T5&), (T1, T2, T3, T4, T5))\
	, RPC__1, RPC__2, RPC__3, RPC__4, RPC__5 ) );

#define _DRI_CALL_R_BEGIN(FuncName) \
	bool FuncName(

#define _DRI_CALL_R_MID(FuncName, RetT) \
	const rpc::detail::result_handler_type<RetT>::type& on_ ## FuncName )\
	{\
		rpc::peer_ptr p = wp.lock();\
		if(p) p->remote_call<RetT>( #FuncName, 

#define _DRI_CALL_R_END(FuncName) \
		on_ ## FuncName );\
		return p;\
	}

#define DRI_CALL_R0(RetT, FuncName) _DRI_CALL_R_BEGIN(FuncName)\
	\
	_DRI_CALL_R_MID(FuncName, RetT) \
	\
	_DRI_CALL_R_END(FuncName)
#define DRI_CALL_R1(RetT, FuncName, T1) _DRI_CALL_R_BEGIN(FuncName)\
	const T1& t1, \
	_DRI_CALL_R_MID(FuncName, RetT) \
	t1, \
	_DRI_CALL_R_END(FuncName)
#define DRI_CALL_R2(RetT, FuncName, T1, T2) _DRI_CALL_R_BEGIN(FuncName)\
	const T1& t1, const T2& t2, \
	_DRI_CALL_R_MID(FuncName, RetT) \
	t1, t2, \
	_DRI_CALL_R_END(FuncName)
#define DRI_CALL_R3(RetT, FuncName, T1, T2, T3) _DRI_CALL_R_BEGIN(FuncName)\
	const T1& t1, const T2& t2, const T3& t3, \
	_DRI_CALL_R_MID(FuncName, RetT) \
	t1, t2, t3, \
	_DRI_CALL_R_END(FuncName)
#define DRI_CALL_R4(RetT, FuncName, T1, T2, T3, T4) _DRI_CALL_R_BEGIN(FuncName)\
	const T1& t1, const T2& t2, const T3& t3, const T4& t4, \
	_DRI_CALL_R_MID(FuncName, RetT) \
	t1, t2, t3, t4, \
	_DRI_CALL_R_END(FuncName)
#define DRI_CALL_R5(RetT, FuncName, T1, T2, T3, T4, T5) _DRI_CALL_R_BEGIN(FuncName)\
	const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5, \
	_DRI_CALL_R_MID(FuncName, RetT) \
	t1, t2, t3, t4, t5, \
	_DRI_CALL_R_END(FuncName)

#define DRI_END \
};

#ifdef DRI_IS_SERVER
# define DRI_SERVER_BEGIN DRI_SERVER_LISTEN_BEGIN
# define DRI_SERVER_END DRI_SERVER_LISTEN_END
# define DRI_CLIENT_BEGIN
# define DRI_CLIENT_END
# define DRI_SERVER_FUNC0 DRI_INIT_R0
# define DRI_CLIENT_FUNC0 DRI_CALL_R0
# define DRI_SERVER_FUNC1 DRI_INIT_R1
# define DRI_CLIENT_FUNC1 DRI_CALL_R1
# define DRI_SERVER_FUNC2 DRI_INIT_R2
# define DRI_CLIENT_FUNC2 DRI_CALL_R2
# define DRI_SERVER_FUNC3 DRI_INIT_R3
# define DRI_CLIENT_FUNC3 DRI_CALL_R3
# define DRI_SERVER_FUNC4 DRI_INIT_R4
# define DRI_CLIENT_FUNC4 DRI_CALL_R4
# define DRI_SERVER_FUNC5 DRI_INIT_R5
# define DRI_CLIENT_FUNC5 DRI_CALL_R5
#elif defined(DRI_IS_CLIENT)
# define DRI_SERVER_BEGIN 
# define DRI_SERVER_END 
# define DRI_CLIENT_BEGIN DRI_CLIENT_CONNECT_BEGIN
# define DRI_CLIENT_END DRI_CLIENT_CONNECT_END
# define DRI_CLIENT_FUNC0 DRI_INIT_R0
# define DRI_SERVER_FUNC0 DRI_CALL_R0
# define DRI_CLIENT_FUNC1 DRI_INIT_R1
# define DRI_SERVER_FUNC1 DRI_CALL_R1
# define DRI_CLIENT_FUNC2 DRI_INIT_R2
# define DRI_SERVER_FUNC2 DRI_CALL_R2
# define DRI_CLIENT_FUNC3 DRI_INIT_R3
# define DRI_SERVER_FUNC3 DRI_CALL_R3
# define DRI_CLIENT_FUNC4 DRI_INIT_R4
# define DRI_SERVER_FUNC4 DRI_CALL_R4
# define DRI_CLIENT_FUNC5 DRI_INIT_R5
# define DRI_SERVER_FUNC5 DRI_CALL_R5
#else
# error Must define one of following 2 macros: DRI_IS_SERVER, or DRI_IS_CLIENT.
#endif

