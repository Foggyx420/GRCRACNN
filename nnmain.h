#ifndef NNMAIN_H
#define NNMAIN_H

#include <string>

// Global Vars

extern int64_t nNNLastSynced = 0;
extern bool bNNStillSyncing = false;
extern bool bNNHasValidContract = false;

// gridcoin client class
class nn
{
public:
    bool syncdata();

private:
    bool isnnparticipant();
};

#endif // NNMAIN_H
