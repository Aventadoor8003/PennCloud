#include "Html.h"
#include <fstream>
#include "iostream"
using namespace std;
Html::Html(string filePath) {
    filePath_ = filePath;
    ifstream ifs;
    ifs.open(filePath_, ios::in);
    if (!ifs.is_open()) {
        cout << "open error" <<endl;
        return;
    }
    string ret;
    string buf;
    while (getline(ifs,buf))
    {
         ret += buf;
    }
    rawData_ = ret;
}

string Html::GetRawData() {
    return rawData_;
}

void Html::Replace(string signal,string source){
    auto startPos = rawData_.find(signal);
    auto endPos = startPos + signal.size();
    auto left = rawData_.substr(0,startPos);
    auto right = rawData_.substr(endPos);
    rawData_ = left + source + right;
}

