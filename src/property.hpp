#ifndef __PROP_HPP__
#define __PROP_HPP__

// This code is constructed by reference to fabs_conf <ytakanoster@gmail.com>.

#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <sstream>

class property {

    public:

    property();
    virtual ~property();

    bool read(std::string filename);
    std::string get(const std::string& key);


    private:
    static std::string _trim(const std::string& str,
                      const char *trimCharList=" \t\v\r\n");
    static std::string _drop(const std::string& str,
                      const char *dropChar);

    std::map<std::string, std::string> property_map;

};

property::property() { }
property::~property() { }

bool
property::read(std::string filename)
{
    std::ifstream fs(filename);
    if (fs.fail()) {
        return false;
    }

    while (fs) {
        std::string line;
        std::getline(fs, line);
        line = _trim(line);
        line = _drop(line, " ");
        if (line.size() > 0 && line[0] == '#') continue;

        std::stringstream s1(line);
        std::getline(s1, line, '#');
        if (line.size() == 0) continue;

        //std::cout << line << std::endl;

        std::stringstream s2(line);
        std::string key;
        std::string value;
        std::getline(s2, key, '=');
        std::getline(s2, value);
        key = _trim(key);
        value = _trim(value);

        if (key.size() == 0 || value.size() == 0) continue;
        //std::cout << "key:" << key << ", value:" << value << std::endl;
        property_map[key] = value;
    }

    return true;
}

std::string
property::get(const std::string& key)
{
    return property_map[key];
}

std::string
property::_trim(const std::string& str, const char* trimCharList)
{
    std::string result;
    std::string::size_type left = str.find_first_not_of(trimCharList);

    if (left != std::string::npos) {
        std::string::size_type right = str.find_last_not_of(trimCharList);
        result = str.substr(left, right - left + 1);
    }

    return result;
}

std::string
property::_drop(const std::string& str, const char *dropChar)
{
    std::string retval = str;
    std::string::size_type i = retval.find(dropChar);

    while (i != std::string::npos) {
        retval.erase(i, strlen(dropChar));
        i = retval.find(dropChar, i);
    }
    return retval;
}

#endif // __PROP_HPP__
