#ifndef LOGGER_H
#define LOGGER_H
#pragma once
#include "nnutil.h"

enum logattribute {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

enum logtype {
    NN,
    DB
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

class dblogger
{
private:

    std::ofstream logfile;

public:

    dblogger()
    {
        fs::path plogfile = fs::current_path();

        plogfile /= "nndb.log";

        logfile.open(plogfile.c_str(), std::ios_base::out | std::ios_base::app);

        if (!logfile.is_open())
            printf("DB : Logging : Failed to open logging file\n");
    }

    ~dblogger()
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

void _log(logtype eLoc, logattribute eType, const std::string& sCall, const std::string& sMessage)
{
    std::string sType;

    try
    {
        switch (eType)
        {
            case DEBUG:      sType = "DEBUG";      break;
            case INFO:       sType = "INFO";       break;
            case WARNING:    sType = "WARNING";    break;
            case ERROR:      sType = "ERROR";      break;
        }
    }

    catch (std::exception& ex)
    {
        printf("logger : exception occured in _log function (%s)\n", ex.what());

        return;
    }

    stringbuilder sOut;

    sOut.append(std::to_string(time(NULL)));
    sOut.ltype(sType);
    sOut.lcall(sCall);
    sOut.append(sMessage);

    if (eLoc == NN)
    {
        nnlogger nnlog;

        nnlog.output(sOut.value());
    }
    else
    {
        dblogger dblog;

        dblog.output(sOut.value());
    }

    return;
}

#endif // LOGGER_H
