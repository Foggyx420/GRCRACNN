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
#include <stdexcept>
#include <zlib.h>

// Move once porting to gridcoin
double ExtractValueFromVector(const std::vector<std::string>& data, const std::string& key);

// Dummy testing
std::vector<std::pair<std::string, std::string>> vWhitelist;
std::unordered_set<std::string> mCPIDs;
std::string dummycontract;

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
    vWhitelist.push_back(std::make_pair("worldcommunitygrid", "https://download.worldcommunitygrid.org/boinc/@"));

    std::string tccontractfile = "dummyprojects.txt";
    std::string tcline;
    std::string tcdata;

    std::ifstream tcinput_file(tccontractfile, std::ios_base::in);


    while(std::getline(tcinput_file, tcline))
    {
        if (tcline.empty())
            continue;

        tcdata = tcline;
    }

    dummycontract = tcdata;

    return;
}

// Return just the data we need and move to util once in gridcoin so it works there
double ExtractValueFromVector(const std::vector<std::string>& data, const std::string& key)
{
    for (const auto& search : data)
    {
        if (search.find(key) != std::string::npos)
        {
            // Found Data return the value of what we looking for ( Key, Value )
            std::vector<std::string> parse = split(search, ",");

            try
            {
                return std::stod(parse[1]);
            }

            catch (std::exception& ex)
            {
                return 0;
            }
        }
    }

    return 0;
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
            fs::path check_file = ETagFile(etag);

//            check_file /= etag + ".gz";

            if (fs::exists(check_file))
            {
                if (fs::remove(check_file))
                    return;

                else
                {
                    _log(NN, WARNING, "deleteprojectfile", "Project file was not found");

                    return;
                }
            }
        }

        catch (std::exception& ex)
        {
            _log(NN, ERROR, "deleteprojectfile", "Std exception occured (" + logstr(ex.what()) + ")");

            return;
        }
    }

    bool downloadprojectfile(nncurl* prjdownload, const std::string& prjfileurl)
    {
        try
        {
            fs::path check_file = ETagFile(prjdownload->gzfileetag());

            // File already exists
            if (fs::exists(check_file))
                return true;

            if (prjdownload->http_download(prjfileurl))
            {
                _log(NN, INFO, "downloadprojectfile", "Successfully downloaded project file <prjfile=" + prjfileurl + ", etag=" + prjdownload->gzfileetag() + ">");

                return true;
            }

            else
            {
                _log(NN, ERROR, "downloadrojectfile", "Failed to download project file <prjfile=" + prjfileurl + ", etag=" + prjdownload->gzfileetag() + ">");

                return false;
            }
        }

        catch (std::exception& ex)
        {
            _log(NN, ERROR, "downloadprojectfile", "Std exception occured (" + logstr(ex.what()) + ")");

            return false;
        }
    }

    bool gatherprojectheader(nncurl* prjheader, const std::string& prjfileurl)
    {
        if (prjheader->http_header(prjfileurl))
        {
            _log(NN, INFO, "gatherprojectheader", "Successfully received project file header <prjfile=" + prjfileurl + ", etag=" + prjheader->gzfileetag() + ">");

            return true;
        }

        else
        {
            _log(NN, WARNING, "gatherprojectheader", "Failed to receive project file header");

            return false;
        }
    }

    bool gatherprojectdata(nndb* db)
    {
        // Must have 90% success rate to make a contract
        int nRequired = std::floor((double)vWhitelist.size() * 0.9);
        int nSuccess = 0;

        db->create_table("ETAG");

        nncurl* curl = new nncurl;

        for (const auto& vWL : vWhitelist)
        {
            if (vWL.first.empty() || vWL.second.empty())
            {
                _log(NN, WARNING, "gatherprojectdata", "Entry in Whitelist vector is empty!");

                continue;
            }

            std::size_t pos = vWL.second.find("@");
            std::string sUrl = vWL.second.substr(0, pos) + "stats/";
            std::string sPrjFile;

            if (vWL.second.find("worldcommunitygrid") != std::string::npos)
                sPrjFile = sUrl + "user.xml.gz";

            else
                sPrjFile = sUrl + "user.gz";

            curl->reset();
            curl->clear();

            if (gatherprojectheader(curl, sPrjFile))
            {
                std::string sDBETag;

                if (db->search_table("ETAG", vWL.first, sDBETag))
                {
                    if (sDBETag == curl->gzfileetag())
                    {
                        // No change
                        if (fs::file_size(NNPathStr() + "/" + sDBETag + ".gz") == curl->gzfilesize())
                        {
                            _log(NN, INFO, "gatherprojectdata", "Project file on server for " + vWL.first + " is unchanged; no need to download");

                            nSuccess++;

                            continue;
                        }

                        else
                            _log(NN, WARNING, "gatherprojectdata", "Project file on server mismatches with local stores file; set to redownload");
                    }
                }

                else
                    _log(NN, WARNING, "gatherprojectdata", "Project file for " + vWL.first + " not found in database (new?)");

                // Here we download project files
                _log(NN, INFO, "gatherprojectdata", "Project file on server for " + vWL.first + " is new");

                if (!sDBETag.empty())
                    deleteprojectfile(sDBETag);

                curl->reset();

                if (downloadprojectfile(curl, sPrjFile))
                {
                    _log(NN, INFO, "gatherprojectdata", "Project file for " + vWL.first + " downloaded and stored as " + curl->gzfileetag() + ".gz");

                    if (db->insert_entry("ETAG", vWL.first, curl->gzfileetag()))
                    {
                        nSuccess++;

                        continue;
                    }

                    else
                    {
                        _log(NN, ERROR, "gatherprojectdata", "Failed to insert ETag for project " + vWL.first);

                        continue;
                    }
                }

                else
                {
                    _log(NN, ERROR, "gatherprojectdata", "Failed to download project " + vWL.first + "'s file");

                    continue;
                }
            }

            else
            {
                // Report the failure however if the project data does exist from a previous download we can use that as etag is in database already
                _log(NN, ERROR, "gatherprojectdata", "Failed to receive project header for " + vWL.first);

                continue;
            }
        }

        if (nSuccess < nRequired)
        {
            _log(NN, ERROR, "gatherprojectdata", "Failed to retrieve required amount of projects <Successcount=" + std::to_string(nSuccess) + ", Requiredcount=" + std::to_string(nRequired) + ", whitelistcount=" + std::to_string(vWhitelist.size()) + ">");

            return false;
        }

        return true;
    }

    bool processprojectdata(nndb* db)
    {
        // Must have 90% success rate to make a contract
        int nRequired = std::floor((double)vWhitelist.size() * 0.9);
        int nSuccess = 0;

        db->drop_table("DATA");
        db->vacuum_db();
        db->create_table("DATA");

        for (const auto& wl : vWhitelist)
        {
            int64_t t = time(NULL);

            printf("#%s\n", wl.first.c_str());

            bool bCritical;

            if (!importprojectdata(db, wl.first, bCritical))
                _log(NN, ERROR, "processprojectdata", "Failed to process stats for project <project=" + wl.first + ">");

            else
                nSuccess++;

            if (bCritical)
            {
                _log(NN, ERROR, "processprojectdata", "Critical error while processing stats for project; Aborting sync <project=" + wl.first + ">");

                return false;
            }

            int64_t u = time(NULL);
            printf("%s took %" PRId64 "\n", wl.first.c_str(), u-t);

        }

        if (nSuccess < nRequired)
        {
            _log(NN, ERROR, "processprojectdata", "Failed to process required amount of projects <Successcount=" + std::to_string(nSuccess) + ", Requiredcount=" + std::to_string(nRequired) + ", whitelistcount=" + std::to_string(vWhitelist.size()) + ">");

            return false;
        }

        else
        {
            if (!db->insert_entry("DATA", "SUCCESSCOUNT", std::to_string(nSuccess)))
            {
                _log(NN, ERROR, "processprojectdata", "Failed to add critical data to DATA table");

                return false;
            }

            return true;
        }
    }

    bool importprojectdata(nndb* db, const std::string& project, bool& critical)
    {
        critical = false;

        printf("Importing project %s\n", project.c_str());

        std::string etag;

        if (!db->search_table("ETAG", project, etag))
        {
            _log(NN, ERROR, "importprojectdata", "Failed to find etag for project <project=" + project + ">");

            return false;
        }

        // Drop previous table for project as we are fresh syncing this project from file

        db->drop_table(project);

        try
        {
            // GZIP input stream

            std::string file = NNPathStr() + "/" + etag + ".gz";

            printf("input file is %s\n", file.c_str());
            std::ifstream input_file(file, std::ios_base::in | std::ios_base::binary);

            if (!input_file)
            {
                _log(NN, ERROR, "importprojectdata", "Unable to open project file <file=" + etag + ".gz>");

                return false;
            }

            boostio::filtering_istream in;
            in.push(boostio::gzip_decompressor());
            in.push(input_file);

            bool bcomplete = false;
            bool berror = false;
            double prjtc = 0;

            db->create_table(project);

            while (!bcomplete && !berror)
            {
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
                        stringbuilder* ud = new stringbuilder;

                        while (std::getline(in, line))
                        {
                            if (line == "</user>")
                                break;

                            ud->append(line);
                        }

                        std::string cpid = ExtractXML(ud->value(), "<cpid>", "</cpid>");
                        std::string tc = ExtractXML(ud->value(), "<total_credit>", "</total_credit>");

                        if (cpid.empty() || tc.empty())
                            continue;

                        if (mCPIDs.count(cpid) != 0 && cpid.length() == 32)
                        {
                            if (!db->insert_entry(project, cpid, tc))
                            {
                                    _log(NN, ERROR, "importprojectdata", "Failed to insert user data into project table <table=" + project + ", cpid=" + cpid + ", tc=" + tc + ">");

                                    return false;

                            }

                            prjtc += std::stod(tc);

                            // Success
                            continue;
                        }
                    }
                }

                // Incomplete files or problems can put us here.
                if (!bcomplete)
                    return false;
            }

            // Now that we done store total rac in project table
            db->insert_entry(project, "TOTAL", std::to_string(prjtc));
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

    bool calcmagsbyproject(nndb* db)
    {
        // Iterate each project for said cpid and calculate the magnitude for each user. db contains the total credit and last sb contains previous total credit
        int successcount = 0;
        std::string scount;


        db->search_table("DATA", "SUCCESSCOUNT", scount);

        if (!scount.empty())
            successcount = std::stoi(scount);

        else
        {
            _log(NN, ERROR, "calcmagsbyproject", "Failed to pull the success count of successful projects synced");

            return false;
        }

        // Verify all project data exists in previous superblock
        // If not the success count needs to be adjusted so no mag is lost.

        int projectcount = 0;

        for (const auto& chkwl : vWhitelist)
        {
            if (dummycontract.find("<" + chkwl.first + ">") != std::string::npos)
                projectcount++;

            else
                continue;
        }

        for (const auto& wl : vWhitelist)
        {
            double totalmagforproject = 0;
            int64_t t = time(NULL);
            printf("*%s\n", wl.first.c_str());

            // Pull history data here from dummy contract for testing ADD later on non existence handling.
            std::unordered_map<std::string, double> tchistorydata = splittouomap(ExtractXML(dummycontract, "<" + wl.first + ">", "</" + wl.first + ">"));

            // If the results is empty then its a new project in neural network then we just bypass this part
            // TODO figure out a way to now have a magnitude loss for the period. surely this is not the best way !
            if (tchistorydata.empty())
            {
                _log(NN, INFO, "calcmagsbyproject", "Project is not in previous superblock contract; skipping and will record total credit only");

                continue;
            }

//            try
//            {
                db->drop_table("MAGNITUDES-" + wl.first);
                db->vacuum_db();
                db->create_table("MAGNITUDES-" + wl.first);

                double dtotalcredit = 0;

                int rowcount = db->get_row_count(wl.first);

                printf("project %s and rowcount %d\n", wl.first.c_str(), rowcount);
                if (rowcount == 0)
                {
                    _log(NN, INFO, "calcmagsbyproject", "No data for project; ignoring as it liekly failed to download <project=" + wl.first + ", rowcount=" + logstr(rowcount) + ">");

                    continue;
                }

                // CRITICAL SECTION
                // A New beacon will have a new total credit but the last superblock will not have that data to calculate a mag
                // Thus the total project credit needs to be changed to reflect that as to not throw off mag calculations considerably
                // This must be done first and not on the fly
                // DB work is fast thou gonna do this in a vector
                std::unordered_map<std::string, double> processedcpids;
                std::unordered_map<std::string, double> tabledata;

                int64_t ts = time(NULL);
                tabledata = db->table_to_uomap(wl.first);
                printf("took to uomap database table %" PRId64 "\n", (time(NULL) - ts));
                if (tabledata.empty())
                {
                    _log(NN, ERROR, "calcmagsbyproject", "DB error; table for project is empty <project=" + wl.first + ">");

                    return false;
                }

                for (const auto& map : tabledata)
                {
                    std::string pcpid = map.first;
                    double pstc = map.second;

                    if (pcpid.empty() || pstc == 0)
                        continue;

                    if (pcpid.length() != 32)
                        continue;

                    // Pull previous credit from last sb dummy contract
                    double psoldtc = 0;

                    auto tcsearch = tchistorydata.find(pcpid);

                    if (tcsearch != tchistorydata.end())
                        psoldtc = tcsearch->second;

                    else
                    {
                        // Cpid not in past sb contract
                        // Adjust the total project credit to not offset mag calculations
                        // Don't insert into vector to process
                        // They will join next superblock as the total credit will be stored for the project

                        continue;
                    }

                    if (pstc == psoldtc)
                    {
                        // No credit difference don't add to vector for processing of project mag
                        continue;
                    }

                    // Place into vector
                    else
                    {
                        double usercreditdiff = pstc - psoldtc;

                        if (usercreditdiff <= 0)
                        {
                            printf("noo im less then 00000\n");
                            continue;
                        }

                        dtotalcredit += usercreditdiff;
                        processedcpids.insert(std::make_pair(pcpid, usercreditdiff));

                    }
                }

                // Search each row and produce a magnitude for each user
                // We have NN multipler of 115000
                // Whitelist count is used
                // Total rac is used
                std::unordered_map<std::string, double> magdata;

                for (const auto& v : processedcpids)
                {
                    double credit = v.second;

                    if (credit < 0)
                    {
                        printf("ERROR LESS THEN 0 TC HOW?!?!\n");
                        return false;
                    }
                    double mag = ((credit / dtotalcredit) / (double)projectcount) * 115000;
                    //mag = Round(mag, 2);
                    if (mag == 0)
                        continue;

                    totalmagforproject += mag;
                    //                        mag = Round(mag, 2);
                    //                        mag = shave(mag, 2);

                    //       printf("DEBUG: ROW %d -> cpid %s -> rac %f\n", i, cpid.c_str(), rac);
                    magdata.insert(std::make_pair(v.first, mag));
                }

                if (!db->insert_bulk_uomap("MAGNITUDES-" + wl.first, magdata))

                //                    if (!db->insert_entry("MAGNITUDES-" + wl.first, v.first, std::to_string(mag)))
                {
                    _log(NN, ERROR, "calcmagsbyproject", "Failed to insert into database <project=" + wl.first + ">");

                    db->drop_table("MAGNITUDES-" + wl.first);
                    db->vacuum_db();

                    break;
                }

                _log(NN, INFO, "calcmagsbyproject", "Processed Rac for Mag calcuations <project=" + wl.first + ", count=" + logstr(rowcount - 1) + ">");
                //            }

                //            catch (std::exception& ex)
                //            {
                //                _log(NN, ERROR, "calcmagsbyproject", "Exception occured; assuming project failed <project=" + wl.first + "> (" + logstr(ex.what()) + ")");

                //                db->drop_table("MAGNITUDES-" + wl.first);
                //                db->vacuum_db();
                //            }

                printf("project %s has total magnitude of %f\n", wl.first.c_str(), totalmagforproject);
                int64_t u = time(NULL);
                printf("%s took %" PRId64 "\n", wl.first.c_str(), u-t);
        }
        if (projectcount < successcount)
            _log(NN, INFO, "calcmagsbyproject", "Finished processing mags for each project; new projects will be in the superblock afterwords; <successcount=" + logstr(successcount) + ", projectswithmag=" + logstr(projectcount) + ">");

        else
            _log(NN, INFO, "calcmagsbyproject", "Finished calculating mags for each project");

        return true;
    }


    bool calctotalmagbycpid(nndb* db)
    {
        // Iterate beacon list and search all projects for a result of MAG and add them to make a total mag per cpid table
        double totalmag;

        db->drop_table("MAGNITUDES");
        db->vacuum_db();
        db->create_table("MAGNITUDES");

        for (auto const& cpid : mCPIDs)
        {
            if (cpid.length() != 32)
                continue;

            double cpidmag = 0;

//            printf("Checking CPID %s\n", cpid.c_str());

            for (auto const& prj : vWhitelist)
            {
                try
                {
                    std::string value;

                    if (db->search_table("MAGNITUDES-" + prj.first, cpid, value))
                    {
                        // If CPID found in project do the math
                        if (value.empty())
                            continue;

//                        printf("mag for prj %s is %s\n", prj.first.c_str(), value.c_str());
//                        printf("stod value is %f\n", std::stod(value));
                        double mag = Round(std::stod(value), 2);

                        cpidmag += mag;
                    }
                }

                catch (std::exception& ex)
                {
                    printf("EXCEPTION %s\n", ex.what());
                    // handle
                }

            }

//            if (cpidmag < 0.25)
//            {
//                printf("User %s has a mag less then 0.25 -> %f\n", cpid.c_str(), cpidmag);
//                continue;
//            }
            //else if (cpidmag > 0.25 && cpidmag < 1)
            //    cpidmag = 1;

            if (cpidmag > 0)
            {
                cpidmag = Round(cpidmag, 2);
//                if (cpidmag < 1)
//                    _log(DB, INFO, "cpidmag", "MAg is less then 1 for cpid " + cpid + " mag " + logstr(cpidmag));

//                if (cpidmag < 0.25)
//                    continue;

                if (cpidmag > 32766)
                    cpidmag = 32766;

//                if (cpidmag < 1 and cpidmag > 0.25)
//                    cpidmag =1;

                if (!db->insert_entry("MAGNITUDES", cpid, std::to_string(cpidmag)))
                {
                    _log(NN, ERROR, "cacltotalmagbycpid", "Failed to insert into database; assuming database failed <cpid=" + cpid + ">");

                    return false;
                }

          //      printf("CPID %s has total mag of %f\n", cpid.c_str(), cpidmag);
                totalmag += cpidmag;
            }

            // If CPID has no mag; so lets save time by not placing this in database

            // Success this time
        }

        printf("total mag is %f\n", totalmag);
        db->vacuum_db();

        return true;
    }

    std::string contract(nndb* db)
    {
        // Verify we have enough projects synced to be able to make a contract
        // This currently stored in DB however i'd like to remove that manipulation chance
        std::string value;
        stringbuilder contract;

        db->search_table("DATA", "SUCCESSCOUNT", value);

        int maxzeros = (int)mCPIDs.size() * 0.06;
        int zeros = 0;

        try
        {
            double successcount = 0;

            if (!value.empty())
                successcount = std::stod(value);

            if (successcount < (vWhitelist.size() * 0.90))
            {
                // Fail
                _log(NN, ERROR, "contract", "Failed to meet minimum project count to make project <success=" + logstr(successcount) + ", required=" + logstr(vWhitelist.size()) + ">");

                return "";
            }
            // Build contract
            // <SBV2> superblock version 2
            // <M> Magnitudes
            // <A> Averages
            contract.oxml("MAGNITUDES");

            double totalnetworkmagnitude = 0;

            for (const auto& cpid : mCPIDs)
            {
                if (cpid.length() != 32 || cpid.empty())
                    continue;

                std::string smag = "";
                double cpidmag = 0;

                if (!db->search_table("MAGNITUDES", cpid, smag))
                {
                    _log(NN, ERROR, "contract", "Failed to search data in magnitudes table");

                    return "";
                }

                if (smag.empty() || smag == "0.000000")
                {
                    if (zeros < maxzeros)
                    {
                        zeros++;

                        continue;
                    }

                    else
                    {
                        contract.append("0,15;");
                    }

                    continue;
                }

                cpidmag = std::stod(smag);

                contract.append(cpid);
                contract.comma();
                contract.append(shave(cpidmag, 2));
                contract.semicolon();

                totalnetworkmagnitude += cpidmag;
            }

            contract.cxml("MAGNITUDES");
            contract.oxml("AVERAGES");
            contract.append("bullshit");
            contract.cxml("AVERAGES");
            // Gather Averages if we even need this anymore

        }

        catch (std::exception& ex)
        {
            return "";
        }

        return contract.value();
    }

    std::string builddummycontract(nndb* db)
    {
        // Iterate each project TC file for said cpid and store the TC for each user in a project contract. db contains the total credit
        int successcount = 0;
        std::string scount;

        db->search_table("DATA", "SUCCESSCOUNT", scount);

        if (!scount.empty())
            successcount = std::stoi(scount);

        else
        {
            _log(NN, ERROR, "builddummycontract", "Failed to pull the success count of successful projects synced");

            return "";
        }

        stringbuilder* dummycontract = new stringbuilder;

        dummycontract->oxml("PROJECTS");

        for (const auto& wl : vWhitelist)
        {
            int64_t t = time(NULL);
            printf("*%s\n", wl.first.c_str());

            // Conform in future to all one case for nn
            dummycontract->oxml(wl.first);

            try
            {
                int rowcount = db->get_row_count(wl.first);
                printf("rowcount %d\n", rowcount);
                if (rowcount == 0)
                    continue;

                for (int i = 1; i < (rowcount + 1); i++)
                {
                    std::string cpid;
                    std::string stc;

                    if (db->search_raw_table(wl.first, i, cpid, stc))
                    {
                        if (cpid.empty() || stc.empty())
                            continue;

                        if (cpid.length() != 32 && cpid != "TOTAL")
                            continue;

                        dummycontract->append(cpid);
                        dummycontract->comma();
                        dummycontract->append(stc);
                        dummycontract->semicolon();
                    }

                    else
                    {
                        printf("FAILED HERE\n");
                        // setup future failure here
                    }
                }
            }

            catch (std::exception& ex)
            {
                return "";
                // exception
            }

            dummycontract->cxml(wl.first);
        }

        dummycontract->cxml("PROJECTS");

        return dummycontract->value();
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
/*    if (!data.gatherprojectdata(db))
    {
        db->vacuum_db();

        return false;
    }
*/
    int64_t b = time(NULL);
/*    if (!data.processprojectdata(db))
    {
        db->vacuum_db();

        return false;
    }
*/

    int64_t c = time(NULL);

    if (!data.calcmagsbyproject(db))
    {
        db->vacuum_db();

        return false;
    }
    int64_t d = time(NULL);
    /*

    if (!data.calctotalmagbycpid(db))
    {
        db->vacuum_db();

        return false;
    }
*/
    db->checkpoint_db();
    printf("Tasks completed: downloads took %" PRId64 " seconds; Processing data for db took %" PRId64 " seconds; total time %" PRId64 "\n", b-a, c-b, c-a);
    printf("Processing of mags took %" PRId64 "\n", d-c);
    db->vacuum_db();

    int64_t g = time(NULL);
/*
    stringbuilder* sbcontract = new stringbuilder;

    std::string contract = data.contract(db);

    if (contract.empty())
    {
        return false;
    }

    std::string tccontract = data.builddummycontract(db);
    sbcontract->oxml("SBV2");
    sbcontract->append(contract);
    sbcontract->append(tccontract);
    sbcontract->cxml("SBV2");

    int64_t h = time(NULL);
    printf("Contract is %s\n", sbcontract->value().c_str());
    printf("contract creation took %" PRId64 "\n", h-g);
//    printf("Contract is sized %zu\n", tccontract.size());
//    printf("SB contract is %s\n", sbcontract->value().c_str());
//    printf("SB contract size is %zu\n", sbcontract->size());
*/
    return true;
}

// Regular functions
