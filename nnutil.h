#ifndef NNUTIL_H
#define NNUTIL_H
#pragma once

#include <string>
#include <sstream>
#include <experimental/filesystem>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cmath>
#include <inttypes.h>
#include <unordered_set>
#include <unordered_map>
#inlcude <memory>
namespace fs = std::experimental::filesystem;

class stringbuilder
{
protected:

    std::stringstream builtstring;

public:

    void append(const std::string& value)
    {
        builtstring << value;
    }

/*
 *     void appendnws(const std::string& value)
    {
        builtstring << value.replace(value.begin(), value.end(), ' ', '');
    }
*/
    void append(double value)
    {
        builtstring << value;
    }

    void append(int64_t value)
    {
        builtstring << value;
    }

    void semicolon()
    {
        builtstring << ";";
    }

    void comma()
    {
        builtstring << ",";
    }

    void oxml(const std::string& value)
    {
        builtstring << "<" << value << ">";
    }

    void cxml(const std::string& value)
    {
        builtstring << "</" << value << ">";
    }

    void ltype(const std::string& value)
    {
        builtstring << " [" << value << "] ";
    }

    void lcall(const std::string& value)
    {
        builtstring << "<" << value << "> : ";
    }

    void nl()
    {
        builtstring << std::endl;
    }

    std::string value()
    {
        return builtstring.str();
    }

    size_t size()
    {
        return sizeof(builtstring);
    }
};

// Copied from gridcoin
std::string ExtractXML(const std::string& xmldata, const std::string& key, const std::string& key_end)
{

    std::string extraction = "";
    std::string::size_type loc = xmldata.find(key, 0);

    if(loc != std::string::npos)
    {
        std::string::size_type loc_end = xmldata.find(key_end, loc + 3);

        if (loc_end != std::string::npos)
            extraction = xmldata.substr(loc + key.length(), loc_end - loc - key.length());
    }

    return extraction;
}

double Round(double d, int place)
{
    const double accuracy = std::pow(10, place);
    return std::round(d * accuracy) / accuracy;
}


fs::path NNPath()
{
    // TODO read from gridcoin config to allow custom location

    fs::path path = fs::current_path() / "NeuralNode";

    // Path don't exist then use default location
    if (!fs::exists(path))
        fs::create_directory(path);

    return path;
}

std::string NNPathStr()
{
    // TODO read from gridcoin config to allow custom location

    fs::path path = fs::current_path() / "NeuralNode";

    // Path don't exist then use default location
    if (!fs::exists(path))
        fs::create_directory(path);

        return path.string();
}

std::string ETagFile(const std::string& etag)
{
    return NNPathStr() + "/" + etag + ".gz";
}

double shave(double d, int place)
{
    std::string str = std::to_string(d);

    size_t pos = str.find(".");

    str = str.substr(0, pos + place + 1);

    return std::stod(str);

}

std::vector<std::string> split(const std::string& s, const std::string& delim)
{
    size_t pos = 0;
    size_t end = 0;
    std::vector<std::string> elems;

    while((end = s.find(delim, pos)) != std::string::npos)
    {
        elems.push_back(s.substr(pos, end - pos));
        pos = end + delim.size();
    }

    // Append final value
    elems.push_back(s.substr(pos, end - pos));
    return elems;
}

std::unordered_map<std::string, double> splittouomap(const std::string& s)
{
    std::unordered_map<std::string, double> out;

    std::vector<std::string> in = split(s, ";");

    for (const auto& data : in)
    {
        std::vector<std::string> filtered = split(data, ",");

        if (data.empty())
            continue;

        double d = 0;

        try
        {
            d = std::stod(filtered[1]);
            out.insert(std::make_pair(filtered[0], d));
        }

        catch (std::exception& ex)
        {
            // error?
            out.erase(out.begin(), out.end());

            return out;
        }
    }

    return out;
}

#endif // NNUTIL_H
