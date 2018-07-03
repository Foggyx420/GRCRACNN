#ifndef LOGGER_H
#define LOGGER_H

#include "nnutil.h"

enum logattribute {
    NN_DEBUG,
    NN_INFO,
    NN_WARNING,
    NN_ERROR
};

class nnlogger
{
private:

    std::ofstream logfile;

public:

    nnlogger()
    {
        fs::path plogfile = fs::current_path();

        plogfile /= "nn.log";

        logfile.open(plogfile.c_str(), std::ios_base::out | std::ios_base::app);

        if (!logfile.is_open())
            printf("NN : Logging : Failed to open logging file\n");
    }

    ~nnlogger()
    {
        if (logfile.is_open())
        {
            logfile.flush();
            logfile.close();
        }
    }

    void output(const std::string& tofile)
    {
        if (logfile.is_open())
            logfile << tofile << std::endl;

        return;
    }
};

void _log(logattribute eType, const std::string& sCall, const std::string& sMessage)
{
    std::string sType;

    try
    {
        switch (eType)
        {
            case NN_DEBUG:     sType = "DEBUG";    break;
            case NN_INFO:      sType = "INFO";     break;
            case NN_WARNING:   sType = "WARNING";  break;
            case NN_ERROR:     sType = "ERROR";    break;
        }
    }

    catch (std::exception& ex)
    {
        printf("NN : logger : exception occured in _log function (%s)\n", ex.what());

        return;
    }

    stringbuilder sOut;

    sOut.append(std::to_string(time(NULL)));
    sOut.ltype(sType);
    sOut.lcall(sCall);
    sOut.append(sMessage);
    sOut.nl();

    nnlogger log;

    log.output(sOut.value());

    return;
}

#endif // LOGGER_H
