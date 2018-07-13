#ifndef NNDOWNLOAD_H
#define NNDOWNLOAD_H
#pragma once

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
    uintmax_t filesize;
    std::string etag;

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

    void reset()
    {
        curl_easy_reset(curl);
    }

    void clear()
    {
        buffer = "";
        header = "";
        http_code = 0;
        filesize = 0;
        etag = "";
    }

    uintmax_t gzfilesize()
    {
        return filesize;
    }

    std::string gzfileetag()
    {
        return etag;
    }

    bool http_download(const std::string& url)
    {
        try {
            FILE* fp;

            fp = fopen(ETagFile(etag).c_str(), "wb");

            if(!fp)
            {
                _log(NN, ERROR, "nncurl_http_download", "Failed to open project file to download project data into <destination=" + etag + ">");

                return false;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writetofile);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_ACCEPTTIMEOUT_MS, 10000);
            curl_easy_setopt(curl, CURLOPT_PROXY, "");
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            res = curl_easy_perform(curl);
            fclose(fp);

            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            // TODO: consider if its a 401 and need centralized access we call scraper for this  alone.
            // Above code 399 is error for client/server
            if (http_code >= 400)
            {
                _log(NN, ERROR, "nncurl_http_download", "HTTP response code for project file <urlfile=" + url + "> (" + logstr(http_code) + ")");

                try
                {
                    fs::remove(ETagFile(etag).c_str());
                }

                catch (std::exception& ex)
                {
                    _log(NN, ERROR, "nncurl_http_download", "Failed to remove file <file=" + etag + "> (" + logstr(ex.what()) + ")");
                }

                return false;
            }

            if (res)
            {
                _log(NN, ERROR, "nncurl_http_download", "Failed to download project file <urlfile=" + url + "> (" + curl_easy_strerror(res) + ")");

                try
                {
                    fs::remove(ETagFile(etag).c_str());
                }

                catch (std::exception& ex)
                {
                    _log(NN, ERROR, "nncurl_http_download", "Failed to remove file <file=" + etag + "> (" + logstr(ex.what()) + ")");
                }

                return false;
            }

            if (fs::file_size(ETagFile(etag).c_str()) != filesize)
            {
                _log(NN, ERROR, "nncurl_http_download", "Failed to download project file; File size mismatch: server header reported %" + logstr(filesize) + " but download size is " + logstr(fs::file_size(ETagFile(etag).c_str())) + " <urlfile=" + url + "> (" + curl_easy_strerror(res) + ")");

                try
                {
                    fs::remove(ETagFile(etag));
                }

                catch (std::exception& ex)
                {
                    _log(NN, ERROR, "nncurl_http_download", "Failed to remove file <file=" + ETagFile(etag) + "> (" + logstr(ex.what()) + ")");
                }

                return false;
            }

            return true;

        }

        catch (std::exception& ex)
        {
            _log(NN, ERROR, "nncurl_http_download", "Std exception occured (" + logstr(ex.what()) + ")");

            return false;
        }
    }

    bool http_header(const std::string& url)
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
        curl_easy_setopt(curl, CURLOPT_ACCEPTTIMEOUT_MS, 10000);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        res = curl_easy_perform(curl);

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (res)
        {
            _log(NN, ERROR, "nncurl_http_header", "Failed to capture header of project file <urlfile=" + url + ">");

            return false;
        }

        // Above code 399 is error for client/server
        if (http_code >= 400)
        {
            _log(NN, ERROR, "nncurl_http_download", "HTTP response code for project file <urlfile=" + url + "> (" + logstr(http_code) + ")");

            return false;
        }

        std::istringstream ssinput(header);

        for (std::string line; std::getline(ssinput, line);)
        {
            if (line.find("ETag: ") != std::string::npos)
            {
                size_t posa = line.find('"');

                std::string modstring = line.substr(posa + 1);

                size_t posb = modstring.find('"');

                etag = modstring.substr(0, posb);
            }

            if (line.find("Content-Length: ") != std::string::npos)
            {
                std::string modstring = line.substr(16, line.size() - 16);

                if (!modstring.empty())
                    filesize = std::stoi(line.substr(16, line.size() - 16));
            }
        }

        if (etag.empty())
        {
            _log(NN, ERROR, "nncurl_http_header", "Failed to capture ETag for project url <urlfile=" + url + ">");

            return false;
        }

        if (filesize == 0)
        {
            _log(NN, ERROR, "nncurl_http_header", "Failed to capture filesize for project url <urlfile=" + url + ">");

            return false;
        }

        _log(NN, INFO, "nncurl_http_header", "Captured ETag for project url <urlfile=" + url + ", ETag=" + etag + ", size=" + logstr(filesize) + ">");

        return true;
    }
};

#endif // NNDOWNLOAD_H
