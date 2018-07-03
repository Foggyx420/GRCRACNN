#include "nnmain.h"
#include "nnlogger.h"
#include "nndb.h"
#include "nndownload.h"
#include "nnutil.h"

// Dummy testing
std::vector<std::pair<std::string, std::string>> vWhitelist;
std::vector<std::string> vCPIDs;
void setdummy();
bool IsNeuralParticipant();

void setdummy()
{
    // Dummy CPIDs -- In live network conditions the cpids and whitelist are in cache
    vCPIDs.push_back("f636448e64b48e26aaf610a48a48bb91");
    vCPIDs.push_back("0b5ef259411ec18e8dac2be0b732fd23");
    vCPIDs.push_back("a92e1cf0903633d62283ea1f101a1af3");
    vCPIDs.push_back("af73cd19f79a0ade8e6ef695faa2c776");
    vCPIDs.push_back("55cd02be28521073d367f7ca38615682");
    vWhitelist.push_back(std::make_pair("enigma@home", "http://www.enigmaathome.net/@"));
    vWhitelist.push_back(std::make_pair("odlk1", "https://boinc.multi-pool.info/latinsquares/@"));
    vWhitelist.push_back(std::make_pair("sztaki", "http://szdg.lpds.sztaki.hu/szdg/@"));
    return;
}

// Classes, etc
class nndata {

public:
    void deleteprojectfile(const std::string& etag)
    {
        try
        {
            // Check if the file exists
            fs::path check_file = NNPath();

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
            fs::path check_file = NNPath();

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

        if (prjheader.http_header(prjfileurl, etag))
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

    bool gatherprojectdata()
    {
        // Must have 90% success rate to make a contract
        int nRequired = std::floor((double)vWhitelist.size() * 0.9);
        int nSuccess = 0;

        for (auto const& vWL : vWhitelist)
        {
            if (vWL.first.empty() || vWL.second.empty())
            {
                _log(NN_WARNING, "gatherprojectdata", "Entry in Whitelist vector is empty!");

                continue;
            }

            std::size_t pos = vWL.second.find("@");
            std::string sUrl = vWL.second.substr(0, pos) + "stats/";
            std::string sPrjFile = sUrl + "user.gz";
            std::string sETag;

            if (gatherprojectheader(sPrjFile, sETag))
            {
                std::string sDBETag;

                if (SearchDatabase("ETAG", vWL.first, sDBETag))
                {
                    if (sDBETag == sETag)
                    {
                        // No change
                        _log(NN_INFO, "gatherprojectdata", "Project file on server for " + vWL.first + " is unchanged; no need to download");

                        nSuccess++;

                        continue;
                    }
                }

                else
                    _log(NN_WARNING, "gatherprojectdata", "Project file for " + vWL.first + " not found in database (new?)");

                // Here we download project files
                _log(NN_INFO, "gatherprojectdata", "Project file on server for " + vWL.first + " is new");

                if (!sDBETag.empty())
                    deleteprojectfile(sDBETag);

                if (downloadprojectfile(sPrjFile, sETag))
                {
                    _log(NN_INFO, "gatherprojectdata", "Project file for " + vWL.first + " downloaded and stored as " + sETag + ".gz");

                    if (InsertDatabase("ETAG", vWL.first, sETag))
                    {
                        nSuccess++;

                        continue;
                    }

                    else
                    {
                        _log(NN_ERROR, "gatherprojectdata", "Failed to insert ETag for project " + vWL.first);

                        continue;
                    }
                }

                else
                {
                    _log(NN_ERROR, "gatherprojectdata", "Failed to download project " + vWL.first + "'s file");

                    continue;
                }
            }

            else
            {
                _log(NN_ERROR, "gatherprojectdata", "Failed to receive project header for " + vWL.first);

                continue;
            }
        }

        if (nSuccess < nRequired)
        {
            _log(NN_ERROR, "gatherprojectdata", "Failed to retrieve required amount of projects <Successcount=" + std::to_string(nSuccess) + ", Requiredcount=" + std::to_string(nRequired) + ", whitelistcount=" + std::to_string(vWhitelist.size()) + ">");

            return false;
        }

        return true;
    }
};

// NN Class Functions

bool nn::isnnparticipant()
{
    return true;
}

bool nn::syncdata()
{
    if (!isnnparticipant())
        return false;

    setdummy();

    nndata data;

    if (!data.gatherprojectdata())
         return false;

}

bool nn::processprojectdata()
{

}
// Regular functions
