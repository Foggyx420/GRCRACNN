#ifndef NNDB_H
#define NNDB_H
#pragma once

#include "nnlogger.h"
#include <sqlite3.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstring>
#include "../Gridcoin-Research/src/contract/superblock.h"
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

/************************
* Network Node Database *
************************/

class nndb
{
private:

    // Vars
    bool dbopen = false;
    sqlite3* db = nullptr;
    const char* z = NULL;
    int res = 0;
    sqlite3_stmt* stmt = nullptr;
    std::string nndbfile = "nn.db";
    bool dbbusy = false;
    char* err = NULL;
    char* tail = NULL;

    // Functions
/*    static int callback(void *count, int argc, char **argv, char **azColName) {
        int *c = count;

        *c = atoi(argv[0]);

        return 0;
    }
*/
    void open_db()
    {
        if (!dbopen && db != nullptr)
            db == nullptr;

        bool exists = false;

        if (fs::exists(nndbfile.c_str()))
            exists = true;

        res = sqlite3_open_v2(nndbfile.c_str(), &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, nullptr);

        if (res)
        {
            dbopen = false;

            _log(DB, ERROR, "open_db", "sqlite error occured while opening database (" + std::string(sqlite3_errstr(res)) + ")");
        }

        _log(DB, INFO, "open_db", "Database is being opened by request");

        dbopen = true;

        if (!exists)
            settings_db();

        startup_db();
    }

    void close_db()
    {
        vacuum_db();

        res = sqlite3_close_v2(db);

        _log(DB, INFO, "close_db", "Database is being closed by request");

        if (res)
            _log(DB, ERROR, "close_db", "sqlite error occured while closing database (" + std::string(sqlite3_errstr(res)) + ")");

        else
        {
            db = nullptr;

            dbopen = false;
        }
    }

    bool reopen_db()
    {
        // Refresh and reopen database
        // If db is closed for whatever reason lets reopen it
        _log(DB, WARNING, "reopen_db", "Database is being requested to be reopened");

        open_db();

        if (isopen_db())
        {
            _log(DB, INFO, "reopen_db", "Successfully reopened database");

            return true;
        }

        else
        {
            _log(DB, ERROR, "reopen_db", "Failed to reopen database");

            return false;
        }
    }

    void settings_db()
    {
        std::unordered_set<std::string> properties;
        properties.insert("PRAGMA auto_vacuum = 2;");
        properties.insert("PRAGMA synchronous = OFF;");
        properties.insert("PRAGMA temp_store = 2;");
        properties.insert("PRAGMA locking_mode = EXCLUSIVE;");
        properties.insert("PRAGMA automatic_index = FALSE;");
        properties.insert("PRAGMA threads = 255;");
        properties.insert("PRAGMA journal_mode = WAL;");
        properties.insert("PRAGMA wal_autocheckpoint = -1;");
        properties.insert("PRAGMA wal_checkpoint = PASSIVE;");

        for (const auto& p : properties)
        {
            std::string query = p;

            sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    void startup_db()
    {
        // This must be set each time the db is loaded
        sqlite3_prepare_v2(db, "PRAGMA cache_size = -10000;", -1, &stmt, NULL);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

public:

    nndb()
    {
        open_db();
    }

    ~nndb()
    {
        close_db();
    }

    // Functions
    bool isopen_db()
    {
        return dbopen;
    }

    void vacuum_db()
    {
        if (!dbopen)
            if (!reopen_db())
                return;

        _log(DB, INFO, "vacuum_db", "Vacuum of database has been requested");

        std::string query = "VACUUM;";
        int64_t u = time(NULL);

        sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        _log(DB, INFO, "vacuum_db", "Vacuum of data completed in " + logstr(time(NULL) - u) + " seconds");

        return;
    }

    void checkpoint_db()
    {
        if (!dbopen)
            if (!reopen_db())
                return;

        int walframeesize;
        int walframeschk;

        _log(DB, INFO, "checkpoint_db", "Checkpoint has been requested");

        int64_t u = time(NULL);

        res = sqlite3_wal_checkpoint_v2(db, z, SQLITE_CHECKPOINT_TRUNCATE, &walframeesize, &walframeschk);

        if (res)
        {
            _log(DB, WARNING, "checkpoint_db", "Sqlite returned a result code during checkpoint (" + std::string(sqlite3_errstr(res)) + ")");

            return;
        }

        _log(DB, INFO, "checkpoint_db", "Successfully checkpointed in " + logstr(time(NULL) - u) + " seconds");
    }

    void create_table(const std::string& table)
    {
        if (!dbopen)
            if (!reopen_db())
                return;

        std::string query = "CREATE TABLE IF NOT EXISTS '" + table  + "' (key TEXT PRIMARY KEY NOT NULL, value TEXT NOT NULL);";

        sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return;
    }

    bool search_table(const std::string& table, const std::string& key, std::string& value, bool emptyok = false)
    {
        if (!dbopen)
            if (!reopen_db())
                return false;

        if (table.empty() || key.empty())
        {
            _log(DB, ERROR, "search_table", "Parameters cannot be empty; <table=" + table + ", key=" + key + ">");

            return false;
        }

        std::string query = "SELECT value FROM '" + table + "' WHERE key = '" + key + "';";

        sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

        try
        {
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                value = (const char*)sqlite3_column_text(stmt, 0);

                sqlite3_finalize(stmt);
            }

            return true;
        }

        catch (std::bad_cast &ex)
        {
            _log(DB, DEBUG, "search_table", "Cast exception for <table=" + table + ", query=" + query + "> (" + ex.what() + ")");
        }

        sqlite3_finalize(stmt);

        return false;
    }

    std::unordered_map<std::string, double> table_to_uomap(const std::string& table)
    {
        std::unordered_map<std::string, double> uomap;

        if (!dbopen)
            if (!reopen_db())
                return uomap;

        if (table.empty())
        {
            _log(DB, ERROR, "search_table", "Parameters cannot be empty; <table=" + table + ">");

            return uomap;
        }

        // Return empty uomap if table don't exist
        if (this->get_row_count(table) == 0)
            return uomap;

        std::string query = "SELECT key, value FROM '" + table + "';";

        sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

        try
        {
            while(true)
            {
                int stat = sqlite3_step(stmt);

                if (stat == SQLITE_ROW)
                {
                    std::string key = (const char*)sqlite3_column_text(stmt, 0);
                    std::string value = std::stod((const char*)sqlite3_column_text(stmt, 1));

                    uomap.insert(std::make_pair(key, value));

                    continue;
                }

                if (stat == SQLITE_DONE)
                {
                    sqlite3_finalize(stmt);

                    return uomap;
                }

                if (stat != SQLITE_DONE && stat != SQLITE_ROW)
                {
                    // Some problem occured, clear map and send back empty map
                    sqlite3_finalize(stmt);

                    uomap.erase(uomap.begin(), uomap.end());

                    return uomap;
                }
            }
        }

        catch (std::bad_cast &ex)
        {
            _log(DB, DEBUG, "search_table", "Cast exception for <table=" + table + ", query=" + query + "> (" + ex.what() + ")");
        }

        sqlite3_finalize(stmt);

        return uomap;

    }

    int get_row_count(const std::string& table)
    {
        if (!dbopen)
            if (!reopen_db())
                return false;

        if (table.empty())
        {
            _log(DB, ERROR, "get_row_count", "Parameters cannot be empty; <table=" + table + ">");

            return false;
        }

        int count = 0;

        std::string query = "SELECT COUNT(*) FROM '" + table + "';";

        //res = sqlite3_exec(db, query.c_str(), callback, &count, NULL);
        sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

        try
        {
            if (sqlite3_step(stmt) != SQLITE_OK)
            {
                count = sqlite3_column_int(stmt, 0);

                sqlite3_finalize(stmt);

                return count;
            }
        }

        catch (std::bad_cast &ex)
        {
            _log(DB, DEBUG, "get_row_count", "Cast exception for <table=" + table + "> (" + logstr(ex.what()) + ")");
        }

        sqlite3_finalize(stmt);

        return 0;
    }

    bool search_raw_table(const std::string& table, int row, std::string& key, std::string& value)
    {
        if (!dbopen)
            if (!reopen_db())
                return false;

        if (table.empty() || row == 0)
        {
            _log(DB, ERROR, "search_raw_table", "Parameters cannot be empty; <table=" + table + ", row=" + logstr(row) + ">");

            return false;
        }

        std::string query = "SELECT key,value FROM '" + table + "' WHERE ROWID='" + std::to_string(row) + "';";

        sqlite3_prepare_v2(db, query.c_str(), query.size(), &stmt, NULL);

        try
        {
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                key = (const char*)sqlite3_column_text(stmt, 0);
                value = (const char*)sqlite3_column_text(stmt, 1);

                sqlite3_finalize(stmt);

                return true;
            }
        }

        catch (std::bad_cast &ex)
        {
            _log(DB, DEBUG, "search_raw_table", "Cast exception for <table=" + table + ", row=" + std::to_string(row) + "> (" + logstr(ex.what()) + ")");
        }

        sqlite3_finalize(stmt);

        return false;

    }

    void drop_table(const std::string& table)
    {
        if (!dbopen)
            if (!reopen_db())
                return;

        if (table.empty())
        {
            _log(DB, ERROR, "drop_table", "Parameters cannot be empty; <table=" + table + ">");

            return;
        }

        std::string dropquery = "DROP TABLE IF EXISTS '" + table + "';";

        sqlite3_prepare_v2(db, dropquery.c_str(), dropquery.size(), &stmt, NULL);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        vacuum_db();
    }

    bool insert_entry(const std::string& table, const std::string& key, const std::string& value)
    {
        if (!dbopen)
            if (!reopen_db())
                return false;

        if (table.empty() || key.empty() || value.empty())
        {
            _log(DB, ERROR, "insert_entry", "Parameters cannot be empty; <table=" + table + ", key=" + key + ", value=" + value + ">");

            return false;
        }

        std::string entry = "INSERT OR REPLACE INTO '" + table + "' VALUES('" + key + "', '" + value + "');";

        sqlite3_prepare_v2(db, entry.c_str(), entry.size(), &stmt, NULL);

        res = sqlite3_step(stmt);

        if (res != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);

            return false;
        }

        sqlite3_finalize(stmt);

        return true;
    }

    bool insert_bulk_entry(const std::string project, const std::vector<std::pair<std::string, std::string>>& data)
    {
        if (!dbopen)
            if (!reopen_db())
                return false;

        if (project.empty() || data.empty())
        {
            _log(DB, ERROR, "insert_bulk_entry", "Parameters cannot be empty; <table=" + project + ", data.size=" + std::to_string(data.size()) + ">");

            return false;
        }

        std::string entry;

        entry = "INSERT OR REPLACE INTO '" + project + "' VALUES('@key', '@value');";

        sqlite3_prepare_v2(db, entry.c_str(), -1, &stmt, NULL);
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &err);

        for (const auto& d : data)
        {
            std::string cpid = d.first + "\t";
            std::string rac = d.second + "\t";

            sqlite3_bind_text(stmt, 0, cpid.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 1, rac.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
        }

        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &err);
        //sqlite3_finalize(stmt);
        if (err != NULL)
        {
            _log(DB, ERROR, "insert_bulk_entry", "Error while inserting bulk project data (" + std::string(err) + ")");

            return false;
        }

        else
            return true;
    }

    void delete_entry(const std::string& table, const std::string& key)
    {
        if (!dbopen)
            if (!reopen_db())
                return;

        if (table.empty() || key.empty())
        {
            _log(DB, ERROR, "delete_entry", "Parameters cannot be empty; <table=" + table + ", key=" + key + ">");

            return;
        }

        std::string dropquery = "DELETE FROM TABLE IF EXISTS '" + table + "' WHERE key = '" + key + "';";

        sqlite3_prepare_v2(db, dropquery.c_str(), dropquery.size(), &stmt, NULL);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
};

#endif // NNDB_H
