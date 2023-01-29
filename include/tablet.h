#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <map>


class Tablet{
    public:
    Tablet() {};
    ~Tablet() {};
    /* data: {rowkey:{colkey:value}} */
    std::map<std::string, std::map<std::string, std::vector<char>>> data;
};