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
	DRI_SERVER_FUNC0(string, echo0)
	DRI_SERVER_FUNC1(string, echo1, string)
	DRI_SERVER_FUNC2(string, echo2, string, string)
	DRI_SERVER_FUNC3(string, echo3, string, string, string)
	DRI_SERVER_FUNC4(string, echo4, string, string, string, string)
	DRI_SERVER_FUNC5(string, echo5, string, string, string, string, string)
	DRI_SERVER_FUNC0(void, void0)
	DRI_SERVER_FUNC1(void, void1, string)
	DRI_SERVER_FUNC2(void, void2, string, string)
	DRI_SERVER_FUNC3(void, void3, string, string, string)
	DRI_SERVER_FUNC4(void, void4, string, string, string, string)
	DRI_SERVER_FUNC5(void, void5, string, string, string, string, string)
DRI_SERVER_END

DRI_CLIENT_BEGIN
	DRI_CLIENT_FUNC2(void, got_void_string, int, string)
DRI_CLIENT_END

DRI_END