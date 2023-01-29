#include <regex>
#include <iostream>
#include <iomanip>


// #include "StringOperations.hh"

//WILL DELETE
#include "../include/StringOperations.hh"

#include "../include/tablet.h"

using namespace std;

bool isAddrString(string addrString) {
    regex ipFormat("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{1,5})");
    return regex_match(addrString, ipFormat);
}

std::string getIpAddr(std::string input) {
    int colonIdx = -1;
    for(int i = input.size() - 1; i >= 0; i--) {
        if(input[i] == ':') {
            colonIdx = i;
            break;
        }
    }
    return input.substr(0, colonIdx);
}

int getPort(std::string input) {
    int colonIdx = -1;
    for(int i = input.size() - 1; i >= 0; i--) {
        if(input[i] == ':') {
            colonIdx = i;
            break;
        }
    }
    return stoi(input.substr(colonIdx + 1, input.size() - colonIdx));
}


void printWithCRLF(string str) {
    for (char c : str) {
        if (c == '\r') {
            cout << "<CR>";
        }
        else if (c == '\n') {
            cout << "<LF>";
        }
        else {
            cout << c;
        }
    }
    cout << endl;
}

std::string getRowKey(std::string input) {
    int pos = input.find(";");
    string rowKey = input.substr(0, pos);
    return rowKey;
}

std::string getColKey(std::string input) {
    int pos1 = input.find(";");
    input = input.substr(pos1 + 1);
    int pos2 = input.find(";");
    string colKey = input.substr(0, pos2);
    return colKey;
}

std::string getValue(std::string input) {
    int pos1 = input.find(";");
    input = input.substr(pos1 + 1);
    int pos2 = input.find(";");
    string value = input.substr(pos2+1);
    return value;
}

std::string getFirstValue(std::string input) {
    int pos1 = input.find(";");
    input = input.substr(pos1 + 1);
    int pos2 = input.find(";");
    input = input.substr(pos2 + 1);
    int pos3 = input.find(";");
    string first_value = input.substr(0, pos3);
    return first_value;
}

std::string getSecondValue(std::string input) {
    int pos1 = input.find(";");
    input = input.substr(pos1 + 1);
    int pos2 = input.find(";");
    input = input.substr(pos2 + 1);
    int pos3 = input.find(";");
    string second_value = input.substr(pos3+1);
    return second_value;
}

std::string tabletToString(Tablet tablet) {
    map<string, map<string, vector<char>>> data = tablet.data;
    map<string, map<string, vector<char>>>::iterator it = data.begin();

    string res = "";
    while (it != data.end()) {
        string row_key = it->first;
        res += row_key + ":{";
        map<string, vector<char>> row = it->second;
        map<string, vector<char>>::iterator inner_it = row.begin();
        while (inner_it != row.end()) {
            string col_key = inner_it->first;
            res += col_key + ":";
            vector<char> value_charvec = inner_it->second;
            string value_str(value_charvec.begin(), value_charvec.end());
            res += value_str + ",";
            inner_it ++;
        }
        res += "};";
        it++;
    }
    return res;
}

Tablet stringToTablet(std::string tablet_string) {
    cout<<"tablet here"<<endl;
    Tablet tablet = Tablet();
    string rowkey;
    string row_content;
    
    while(tablet_string.find(":") != string::npos) {
        int pos1 = tablet_string.find(":");
        rowkey = tablet_string.substr(0, pos1);
        int pos2 = tablet_string.find("}");
        row_content = tablet_string.substr(pos1+2, pos2 - pos1 -1);
        tablet_string = tablet_string.substr(pos2+2, tablet_string.size() - pos2 - 1);
        
        string colkey;
        string value_str; 
        while(row_content.find(":") != string::npos) {
            int pos3 = row_content.find(":");
            colkey = row_content.substr(0, pos3);
            int pos4 = row_content.find(",");
            value_str = row_content.substr(pos3 + 1, pos4 - pos3 - 1);
            vector<char> value(value_str.begin(), value_str.end());
            tablet.data[rowkey][colkey] = value;
            row_content = row_content.substr(pos4+1, row_content.size() - pos4);
        }
    }    
    return tablet;
}