#ifndef NNUTIL_H
#define NNUTIL_H

#include <string>
#include <sstream>
#include <experimental/filesystem>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cmath>
#include <zlib.h>
#include <inttypes.h>

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

inline fs::path NNPath()
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

#endif // NNUTIL_H
