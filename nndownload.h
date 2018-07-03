#ifndef NNDOWNLOAD_H
#define NNDOWNLOAD_H

#include "nnutil.h"
#include "nnlogger.h"
#include "curl/curl.h"

class nncurl
{
public:
    friend class nnmain;
    friend class nndb;
private:

    CURL* curl;
    std::string buffer;
    std::string header;
    long http_code;
    CURLcode res;

    static size_t writeheader(void* ptr, size_t size, size_t nmemb, void* userp)
    {
        ((std::string*)userp)->append((char*)ptr, size * nmemb);
        return size * nmemb;
    }

    static size_t writetofile(void* ptr, size_t size, size_t nmemb, FILE* fp)
    {
        return fwrite(ptr, size, nmemb, fp);
    }

public:

    nncurl()
        : curl(curl_easy_init())
    {
    }

    ~nncurl()
    {
        if (curl)
            curl_easy_cleanup(curl);
    }

    bool http_download(const std::string& url, const std::string& destination)
    {
        try {
            FILE* fp;

            fp = fopen(destination.c_str(), "wb");

            if(!fp)
            {
                _log(NN_ERROR, "nncurl_http_download", "Failed to open project file to download project data into <destination=" + destination + ">");

                return false;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writetofile);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_PROXY, "");
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            res = curl_easy_perform(curl);
            fclose(fp);

            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            // Above code 399 is error for client/server
            if (http_code >= 400)
            {
                _log(NN_ERROR, "nncurl_http_download", "HTTP response code for project file <urlfile=" + url + "> (" + std::to_string(http_code) + ")");

                try
                {
                    fs::remove(fp);
                }

                catch (std::exception& ex)
                {
                    _log(NN_ERROR, "nncurl_http_download", "Failed to remove file <file=" + destination + "> (" + std::string(ex.what()) + ")");
                }

                return false;
            }

            if (res > 0)
            {
                if (fp)
                    fclose(fp);

                _log(NN_ERROR, "nncurl_http_download", "Failed to download project file <urlfile=" + url + "> (" + curl_easy_strerror(res) + ")");

                try
                {
                    fs::remove(fp);
                }

                catch (std::exception& ex)
                {
                    _log(NN_ERROR, "nncurl_http_download", "Failed to remove file <file=" + destination + "> (" + std::string(ex.what()) + ")");
                }

                return false;
            }

            return true;

        }

        catch (std::exception& ex)
        {
            _log(NN_ERROR, "nncurl_http_download", "Std exception occured (" + std::string(ex.what()) + ")");

            return false;
        }
    }

    bool http_header(const std::string& url, std::string& etag)
    {
        struct curl_slist* headers = NULL;

        headers = curl_slist_append(headers, "Accept: */*");
        headers = curl_slist_append(headers, "User-Agent: Header-Reader");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeheader);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (res > 0)
        {
            _log(NN_ERROR, "nncurl_http_header", "Failed to capture header of project file <urlfile=" + url + ">");

            return false;
        }

        // Above code 399 is error for client/server
        if (http_code >= 400)
        {
            _log(NN_ERROR, "nncurl_http_download", "HTTP response code for project file <urlfile=" + url + "> (" + std::to_string(http_code) + ")");

            return false;
        }

        std::istringstream ssinput(header);

        for (std::string line; std::getline(ssinput, line);)
        {
            if (line.find("ETag: ") != std::string::npos)
            {
                size_t pos;

                std::string modstring = line;

                pos = modstring.find("ETag: ");

                etag = modstring.substr(pos + 6, modstring.size());

                etag = etag.substr(1, etag.size() - 3);
            }

        }

        if (etag.empty())
        {
            _log(NN_ERROR, "nncurl_http_header", "Failed to capture ETag for project url <urlfile=" + url + ">");

            return false;
        }

        _log(NN_INFO, "nncurl_http_header", "Captured ETag for project url <urlfile=" + url + ", ETag=" + etag + ">");

        return true;
    }
};

#endif // NNDOWNLOAD_H
