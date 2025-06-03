// 자주 사용하는 헤더는 여기에 포함.
#pragma once

/*-----------------
	   Core
------------------*/
#include "Types.h"
#include "CoreMacro.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"


/*-----------------
	   Debug
------------------*/
#include <iostream>
#include <assert.h>


/*-----------------
	STL Container
------------------*/
#include <array>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>


/*-----------------
	Window Socket 
------------------*/
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


/*-----------------
	   Network
------------------*/
#include "SendBuffer.h"
#include "Session.h"


/*-----------------
	    Etc
------------------*/
#include "JobQueue.h"

using namespace std;

