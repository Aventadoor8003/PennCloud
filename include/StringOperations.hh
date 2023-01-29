#ifndef _STRING_OPS_HH_
#define _STRING_OPS_HH_
#include <string>
#include "tablet.h"

/// @brief Check if a string is a correct ip address with port number
/// @param addrString 
bool isAddrString(std::string addrString);

/// @brief Split a ip address from an input string
/// @param input 
/// @return ip address string
std::string getIpAddr(std::string input);

/// @brief Get a port number from input
/// @param input 
/// @return port number
int getPort(std::string input);


/// @brief Print string while printing CR and LF
/// @param str 
void printWithCRLF(std::string str);

/// @brief Split row key from message
/// @param input 
/// @return row key string
std::string getRowKey(std::string input);

/// @brief Split column key from message
/// @param input 
/// @return column key string
std::string getColKey(std::string input);

/// @brief Split value from message
/// @param input 
/// @return value string
std::string getValue(std::string input);

/// @brief Split the first value from message
/// @param input 
/// @return value string
std::string getFirstValue(std::string input);

/// @brief Split the second value from message
/// @param input 
/// @return value string
std::string getSecondValue(std::string input);

/// @brief Convert tablet to string
/// @param input 
/// @return tablet string
std::string tabletToString(Tablet tablet);

/// @brief Convert string to tablet
/// @param input 
/// @return Tablet
Tablet stringToTablet(std::string tablet_string);

#endif