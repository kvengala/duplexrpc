#pragma once
#include "../rpc/rpc.hpp"
#include "../rpc/rpc_interface.hpp"
#include <string>

DRI_BEGIN(LuguodeInterface)

DRI_SERVER_BEGIN
	DRI_SERVER_FUNC1( void, talk, std::wstring )
DRI_SERVER_END

DRI_CLIENT_BEGIN
	DRI_CLIENT_FUNC1( void, notify, std::wstring )
	DRI_CLIENT_FUNC1( void, talk, std::wstring )
	DRI_CLIENT_FUNC0( void, stranger_come_in )
DRI_CLIENT_END

DRI_END
