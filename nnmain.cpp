#include "nnmain.h"
#include "nnlogger.h"
#include "nndb.h"
#include "nndownload.h"
#include "nnutil.h"

bool dropdatabase(const std::string& table)
{
    // Add conditions for bool
    nndb db;

    std::string sDumpTable = "DROP TABLE IF EXISTS '" + table + "';";

    db.drop_query(sDumpTable);

    _log(NN_INFO, "DropTable", "Dropped table <table=" + table + ">");

    return true;
}


class nndata {

public:
    void deleteprojectfile(const std::string& etag)
    {
        try
        {
            // Check if the file exists
            fs::path check_file = "NetworkNode";

            check_file /= etag + ".gz";

            if (fs::exists(check_file))
            {
                if (fs::remove(check_file))
                    return;

                else
                {
                    _log(NN_WARNING, "deleteprojectfile", "Project file was not found");

                    return;
                }
            }
        }

        catch (std::exception& ex)
        {
            _log(NN_ERROR, "deleteprojectfile", "Std exception occured (" + std::string(ex.what()) + ")");

            return;
        }
    }

    bool downloadprojectfile(const std::string& prjfileurl, const std::string& etag)
    {
        try
        {
            fs::path check_file = "NeuralNode";

            if (!fs::exists(check_file))
                fs::create_directory(check_file);

            check_file /= etag + ".gz";

            // File already exists
            if (fs::exists(check_file))
                return true;

            nncurl prjdownload;

            if (prjdownload.http_download(prjfileurl, std::string(check_file)))
            {
                _log(NN_INFO, "downloadprojectfile", "Successfully downloaded project file <prjfile=" + prjfileurl + ", etags=" + etag + ">");

                return true;
            }

            else
            {
                _log(NN_ERROR, "downloadrojectfile", "Failed to download project file <prjfile=" + prjfileurl + ", etags=" + etag + ">");

                return false;
            }
        }

        catch (std::exception& ex)
        {
            _log(NN_ERROR, "downloadprojectfile", "Std exception occured (" + std::string(ex.what()) + ")");

            return false;
        }
    }

    bool gatherprojectheader(const std::string& prjfileurl, std::string& etag)
    {
        nncurl prjheader;

        if (prjheader.http_header(prjfileurl,etag))
        {
            _log(NN_INFO, "gatherprojectheader", "Successfully received project file header <prjfile=" + prjfileurl + ", etag=" + etag + ">");

            return true;
        }

        else
        {
            _log(NN_WARNING, "gatherprojectheader", "Failed to receive project file header");

            return false;
        }
    }
};
