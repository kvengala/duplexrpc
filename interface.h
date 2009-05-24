#pragma once

#include "rpc/rpc.hpp"
#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
using namespace std;
using namespace rpc;

#include "rpc/rpc_interface.hpp"

DRI_BEGIN(Interface)

DRI_SERVER_BEGIN
	DRI_SERVER_FUNC2(string, who_am_i, string, string)
	DRI_SERVER_FUNC0(string, get_my_name)
	DRI_SERVER_FUNC1(void, set_my_name, string)
DRI_SERVER_END

DRI_CLIENT_BEGIN
DRI_CLIENT_END

DRI_END