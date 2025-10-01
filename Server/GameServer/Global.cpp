#include "pch.h"
#include "Global.h"

const map<string, int32> GClassMappings = {
    make_pair("warrior", 1)
};

atomic<int64> GNextItemUID = 0;