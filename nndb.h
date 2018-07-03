#ifndef NNDB_H
#define NNDB_H

#include "nnlogger.h"
#include <sqlite3.h>

/* Conform to a basic layout for maximum camptiablility while keeping functions short and sweet
 * We will store by type with two values
 *
 * Type:
 *     REF              : TABLE         KEY         VALUE
 *-------------------------------------------------------
 *     project          : PROJECT       CPID        RAC
 *     cpid             : CPID          PROJECT     RAC
 *     etag             : ETAG          PROJECT     ETAG
 */

const std::string nndbfile = "nn.db";

/************************
* Network Node Database *
************************/

class nndb
{
private:

    sqlite3* db = nullptr;
    int res = 0;
    sqlite3_stmt* stmt = nullptr;

public:

    nndb()
    {
        res = sqlite3_open_v2(nndbfile.c_str(), &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, nullptr);

        if (res)
            _log(NN_ERROR, "nndb", "sqlite error occured while opening database (" + std::string(sqlite3_errstr(res)) + ")");
    }

    ~nndb()
    {
        res = sqlite3_close_v2(db);

        if (res)
            _log(NN_ERROR, "nndb", "sqlite error occured while closing database (" + std::string(sqlite3_errstr(res)) + ")");
    }

    bool insert_query(bool table, const std::string& querystring)
    {
        sqlite3_prepare_v2(db, querystring.c_str(), querystring.size(), &stmt, NULL);

        res = sqlite3_step(stmt);

        if (!table && res != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);

            return false;
        }

        sqlite3_finalize(stmt);

        return true;
    }

    bool search_query(const std::string& searchquery, std::string& value)
    {
        sqlite3_prepare_v2(db, searchquery.c_str(), searchquery.size(), &stmt, NULL);

        try
        {
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                value = (const char*)sqlite3_column_text(stmt, 0);

                sqlite3_finalize(stmt);

                return true;
            }
        }

        catch (std::bad_cast &ex)
        {
            _log(NN_DEBUG, "nndb_search_query", "Cast exception for <query=" + searchquery + "> (" + ex.what() + ")");
        }

        sqlite3_finalize(stmt);

        return false;
    }

    void drop_query(const std::string& table)
    {
        std::string dropquery = "DROP TABLE IF EXISTS '" + table + "';";

        sqlite3_prepare_v2(db, dropquery.c_str(), dropquery.size(), &stmt, NULL);

        sqlite3_step(stmt);
    }

    void delete_query(const std::string& table, const std::string& key)
    {
        std::string dropquery = "DELETE FROM TABLE IF EXISTS '" + table + "' WHERE key = '" + key + "';";

        sqlite3_prepare_v2(db, dropquery.c_str(), dropquery.size(), &stmt, NULL);

        sqlite3_step(stmt);
    }
};

bool SearchDatabase(const std::string& sTable, const std::string& sKey, std::string& sValue)
{
    if (sTable.empty() || sKey.empty())
    {
        _log(NN_ERROR, "SearchDatabase", "Arguements cannot be empty. Programming ERROR. <table=" + sTable + ", key=" + sKey + ">");

        return false;
    }

    std::string sSearchQuery;

    sSearchQuery = "SELECT value FROM '" + sTable + "' WHERE key = '" + sKey + "';";

    nndb db;

    if (db.search_query(sSearchQuery, sValue))
        return true;

    else
    {
        _log(NN_WARNING, "SearchDatabase", "No result found <table=" + sTable + ", key=" + sKey + ">");

        return false;
    }
}

bool InsertDatabase(const std::string& sTable, const std::string& sKey, const std::string& sValue)
{
    if (sValue.empty() || sTable.empty() || sKey.empty())
    {
        _log(NN_ERROR, "InsertDatabase", "Arguements cannot be empty. Programming ERROR. <table=" + sTable + ", key=" + sKey + ", valuea=" + sValue + ">");

        return false;
    }

    nndb db;

    std::string sInsertQuery;

    sInsertQuery = "CREATE TABLE IF NOT EXISTS '" + sTable + "' (key TEXT PRIMARY KEY NOT NULL, value TEXT NOT NULL);";

    if (!db.insert_query(true, sInsertQuery))
    {
        _log(NN_ERROR, "InsertDatabase", "Failed to insert into database <table=" + sTable + ", query=" + sInsertQuery + ">");

        return false;
    }

    sInsertQuery = "INSERT OR REPLACE INTO '" + sTable + "' VALUES('" + sKey + "', '" + sValue + "');";

    if (!db.insert_query(false, sInsertQuery))
    {
        _log(NN_ERROR, "InsertDatabase", "Failed to insert into database <table=" + sTable + ", query=" + sInsertQuery + ">");

        return false;
    }

    return true;
}

#endif // NNDB_H
