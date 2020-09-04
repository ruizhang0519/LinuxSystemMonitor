#include <string>

#include "format.h"

using std::string;

string Format::ElapsedTime(long seconds) { 
    long hours, minutes;

    hours = seconds / 3600;
    seconds %= 3600;
    minutes = seconds / 60;
    seconds %= 60;

    // take care of single digit
    string hour_str = std::to_string(hours);
    hour_str = hour_str.size() == 1 ? "0" + hour_str : hour_str;

    string min_str = std::to_string(minutes);
    min_str = min_str.size() == 1 ? "0" + min_str : min_str;

    string sec_str = std::to_string(seconds);
    sec_str = sec_str.size() == 1 ? "0" + sec_str : sec_str;

    return hour_str + ":" + min_str + ":" + sec_str; 
}