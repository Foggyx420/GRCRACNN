#ifndef NNUTIL_H
#define NNUTIL_H

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

fs::path NNPath()
{
    // TODO read from gridcoin config to allow custom location

    fs::path path = fs::current_path() / "NeuralNode";

    // Path don't exist then use default location
    if (!fs::exists(path))
    {
        path = fs::current_path() / "NeuralNode";

        return path;
    }

    else
        return path;
}

std::string NNPathStr()
{
    // TODO read from gridcoin config to allow custom location

    fs::path path = fs::current_path() / "NeuralNode";

    // Path don't exist then use default location
    if (!fs::exists(path))
    {
        path = fs::current_path() / "NeuralNode";

        return path.string();
    }

    else
        return path.string();
}

// Copied from gridcoin
std::string extractxml(const std::string& xmldata, const std::string& key, const std::string& key_end)
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

/*{
std::unordered_map<std::string, std::string> storeinmap(const std::string& s, const std)

}
std::unordered_map<std::pair<std::string, std::string>> split(const std::string& s, const std::string& delim)
{
    size_t pos = 0;
    size_t end = 0;
    std::unordered_map<std::pair<std::string, std::string>> elems;

    while ((end = s.find(delim, pos)) != std::string::npos)
    {
        elems.insert(s.substr(pos, end - pos));
        pos = end + delim.size();
    }

    // Append final value
    elems.insert(s.substr(pos, end - pos));

    return elems;
}
*/
#endif // NNUTIL_H
