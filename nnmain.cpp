#include "nnmain.h"
#include "nnlogger.h"
#include "nndb.h"
#include "nndownload.h"
#include "nnutil.h"

#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <zlib.h>
#include <unordered_set>

// Dummy testing
std::vector<std::pair<std::string, std::string>> vWhitelist;
std::unordered_set<std::string> mCPIDs;

std::vector<std::string> vCPIDs;
void setdummy();
bool IsNeuralParticipant();

void setdummy()
{
    // Dummy CPIDs -- In live network conditions the cpids and whitelist are in cache
    std::string file = "beacons.txt";
    std::string line;

    std::ifstream input_file(file, std::ios_base::in);

    while(std::getline(input_file, line))
    {
        if (line.empty())
            continue;

        mCPIDs.insert(line);
    }

    vWhitelist.push_back(std::make_pair("enigma@home", "http://www.enigmaathome.net/@"));
    vWhitelist.push_back(std::make_pair("odlk1", "https://boinc.multi-pool.info/latinsquares/@"));
    vWhitelist.push_back(std::make_pair("amicable numbers", "http://sech.me/boinc/Amicable/@"));
    vWhitelist.push_back(std::make_pair("citizen science grid", "http://csgrid.org/csg/@"));
    vWhitelist.push_back(std::make_pair("srbase", "http://srbase.my-firewall.org/sr5/@"));
    vWhitelist.push_back(std::make_pair("tn-grid", "http://gene.disi.unitn.it/test/@"));
    vWhitelist.push_back(std::make_pair("vgtu project@home", "https://boinc.vgtu.lt/@"));
    vWhitelist.push_back(std::make_pair("asteroids@home", "http://asteroidsathome.net/boinc/@"));
    vWhitelist.push_back(std::make_pair("collatz conjecture", "http://boinc.thesonntags.com/collatz/@"));
    vWhitelist.push_back(std::make_pair("cosmology@home", "http://www.cosmologyathome.org/@"));
    vWhitelist.push_back(std::make_pair("einstein@home", "http://einstein.phys.uwm.edu/@"));
    vWhitelist.push_back(std::make_pair("gpugrid", "http://www.gpugrid.net/@"));
    vWhitelist.push_back(std::make_pair("lhc@home classic", "https://lhcathome.cern.ch/lhcathome/@"));
    vWhitelist.push_back(std::make_pair("milkyway@home", "http://milkyway.cs.rpi.edu/milkyway/@"));
    vWhitelist.push_back(std::make_pair("nfs@home", "http://escatter11.fullerton.edu/nfs/@"));
    vWhitelist.push_back(std::make_pair("numberfields@home", "http://numberfields.asu.edu/NumberFields/@"));
    vWhitelist.push_back(std::make_pair("primegrid", "http://www.primegrid.com/@"));
    vWhitelist.push_back(std::make_pair("rosetta@home", "http://boinc.bakerlab.org/rosetta/@"));
    vWhitelist.push_back(std::make_pair("seti@home", "http://setiathome.berkeley.edu/@"));
    vWhitelist.push_back(std::make_pair("universe@home", "http://universeathome.pl/universe/@"));
    vWhitelist.push_back(std::make_pair("yafu", "http://yafu.myfirewall.org/yafu/@"));
    vWhitelist.push_back(std::make_pair("yoyo@home", "http://www.rechenkraft.net/yoyo/@"));

    return;
}

namespace boostio = boost::iostreams;

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

    bool gatherprojectdata(nndb* db)
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

                if (SearchDatabase(db, "ETAG", vWL.first, sDBETag))
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

                    if (InsertDatabase(db, "ETAG", vWL.first, sETag))
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

    bool processprojectdata(nndb* db)
    {
        // Must have 90% success rate to make a contract
        int nRequired = std::floor((double)vWhitelist.size() * 0.9);
        int nSuccess = 0;

        for (const auto& wl : vWhitelist)
        {
            int64_t t = time(NULL);

            printf("#%s\n", wl.first.c_str());

            if (!importprojectdata(db, wl.first))
                _log(NN_ERROR, "processprojectdata", "Failed to process stats for project <project=" + wl.first + ">");

            else
                nSuccess++;

            int64_t u = time(NULL);
            printf("%s took %" PRId64 "\n", wl.first.c_str(), u-t);

        }

        if (nSuccess < nRequired)
        {
            _log(NN_ERROR, "processprojectdata", "Failed to process required amount of projects <Successcount=" + std::to_string(nSuccess) + ", Requiredcount=" + std::to_string(nRequired) + ", whitelistcount=" + std::to_string(vWhitelist.size()) + ">");

            return false;
        }

        else
            return true;
    }

    bool importprojectdata(nndb* db, const std::string& project)
    {
        printf("Importing project %s\n", project.c_str());

        std::string etag;

        if (!SearchDatabase(db, "ETAG", project, etag))
        {
            _log(NN_ERROR, "importprojectdata", "Failed to find etag for project <project=" + project + ">");

            return false;
        }

        // Drop previous table for project as we are fresh syncing this project from file

        db->drop_query(project);

        try
        {
            // GZIP input stream

            std::string file = NNPathStr() + "/" + etag + ".gz";

            printf("input file is %s\n", file.c_str());
            std::ifstream input_file(file, std::ios_base::in | std::ios_base::binary);

            if (!input_file)
            {
                _log(NN_ERROR, "importprojectdata", "Unable to open project file <file=" + etag + ".gz>");

                return false;
            }

            boostio::filtering_istream in;
            in.push(boostio::gzip_decompressor());
            in.push(input_file);

            bool bcomplete = false;
            bool berror = false;

            while (!bcomplete && !berror)
            {
                if (bcomplete)
                    break;

                std::string line;

                while (std::getline(in, line))
                {

                    if (line != "<users>")
                        continue;

                    else
                        break;
                }

                while (std::getline(in, line))
                {
                    if (bcomplete || berror)
                        break;

                    if (line == "</users>")
                    {
                        bcomplete = true;
                        printf("Complete\n");
                        break;
                    }

                    if (line == "<user>")
                    {
                        stringbuilder ud;

                        while (std::getline(in, line))
                        {
                            if (line == "</user>")
                                break;

                            ud.append(line);
                        }

                        std::string cpid = extractxml(ud.value(), "<cpid>", "</cpid>");
                        std::string rac = extractxml(ud.value(), "<expavg_credit>", "</expavg_credit>");

                        if (cpid.empty() || rac.empty())
                            continue;

                        if (std::stod(rac) < 10)
                            continue;

//                        printf("CPID is %s : RAC is %s\n", cpid.c_str(), rac.c_str());

                        if (mCPIDs.count(cpid) != 0)
                        {

                            if (!InsertDatabase(db, project, cpid, rac))
                            {
                                _log(NN_ERROR, "importprojectdata", "Failed to insert into project table with user data <project=" + project + ", cpid=" + cpid + ", rac=" + rac + ">");

                                continue;
                            }

                            if (!InsertDatabase(db, cpid, project, rac))
                            {
                                _log(NN_ERROR, "importprojectdata", "Failed to insert into cpid table with user data <project=" + project + ", cpid=" + cpid + ", rac=" + rac + ">");

                                // If this fails but last past we need to remove that data
                                db->delete_query(project, cpid);

                                continue;
                            }

                            // Success
                            continue;
                        }
                    }
                }
            }
        }

        catch (std::exception& ex)
        {
            return false;
        }

        catch (boost::exception& ex)
        {
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
    nndata data;

    if (!isnnparticipant())
        return false;

    setdummy();

    nndb* db = new nndb;
    int64_t a = time(NULL);
    if (!data.gatherprojectdata(db))
    {

        return false;
    }
    int64_t b = time(NULL);
    if (!data.processprojectdata(db))
    {
        return false;
    }

    int64_t c = time(NULL);
    printf("Tasks completed: downloads took %" PRId64 " seconds; Processing data for db took %" PRId64 " seconds; total time %" PRId64 "\n", b-a, c-b, c-a);
    return true;
}

// Regular functions
