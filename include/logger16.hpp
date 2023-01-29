#pragma once

#include <memory>
#include <string>
#include <stdexcept>
#include <iostream>

//A macro to check verbose mode
#define CHECK_VERBOSE() if(!vflag) return;

/**
 * A simple logging library provided basic log printing functions
 * 
 * How to use:
 * 1. set vflag to 1 if you are in verbose mode
 * 2. call PrintLog function anywhere
 */


namespace log16 {


/// @brief Format a C style string with signs to a std string
template<typename ... Args>
inline std::string FormatString( const std::string& format, Args ... args ) {
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; 
    if( size_s <= 0 ) { 
        throw std::runtime_error( "Error during formatting." ); 
    }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); 
}

/// @brief A print function. Use the same grammer as printf
template<typename ... Args>
inline void PrintLog(int vflag, const std::string text, Args ... args) {
    CHECK_VERBOSE()
    std::cout << FormatString(text, args ...) << std::endl;
}

/// @brief Print pure text of C string
inline void PrintLog(int vflag, const char* content) {
    CHECK_VERBOSE()
    std::cout << content << std::endl;
}

// @brief Print pure text of std string
inline void PrintLog(int vflag, std::string content) {
    CHECK_VERBOSE()
    std::cout << content << std::endl;
}

}