#include "synclog.h"
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "util/util_option.h"
#include "util/util_log.h"

void usage(bool bExit = true)
{
    ostringstream ss;
    ss << "Usage: " << endl
       << "  mfwsynclog --sync-config=sync_test.xml --skel-config=skel_test.xml" << endl
       << "Options: " << endl
       << "  --sync-config=file     A xml file to describe where to load log file and which database sync to" << endl
       << "  --skel-config=file     A xml file to describe log data format and database table schema" << endl
       << "  --daemon               Enter daemon mode" << endl
       << "  --auto-create-db       Create database automatically" << endl
       << "  --prepare-table-only   Create or Alter table and exit" << endl
       << "  --var:key=value        Pass some variable" << endl
       << "  --print-skel[=file]    Print skel in readable text and exit" << endl
       ;
    cout << ss.str();
    if (bExit) {
        exit(-1);
    }
}

static void sighandler(int /*sig*/)
{
    g_bQuitApplicaiton = true;
}

map<string, string> getDefinedVariable(COption &option)
{
    map<string, string> m;
    const map<string, string> &mOption = option.getOption();
    for (map<string, string>::const_iterator first = mOption.begin(), last = mOption.end(); first != last; ++first) {
        const string &sKey = first->first;
        const string &sValue = first->second;
        if (UtilString::isBeginsWith(sKey, "var:")) {
            string sVariable = sKey.substr(4);
            m[sVariable] = sValue;
        }
    }
    return m;
}

int main(int argc, char *argv[])
{
    try {
        signal(SIGINT, sighandler);
        signal(SIGTERM, sighandler);

        CSyncLog synclog;
        COption option;
        option.parse(argc, argv);

        if (option.hasOption("print-skel")) {
            string sSkelConfig = option.getOption("skel-config");
            string sPrintFile = option.getOption("print-skel");
            if (sSkelConfig.empty()) {
                usage();
            }
            int32_t ret = synclog.printSkel(sSkelConfig, sPrintFile);
            if (ret != 0) {
                return ret;
            }
            return 0;
        }

        string sSyncConfig = option.getOption("sync-config");
        string sSkelConfig = option.getOption("skel-config");
        if (sSyncConfig.empty() || sSkelConfig.empty()) {
            usage();
        }

        if (option.hasOption("daemon")) {
            daemon(1, 1);
        }

        synclog.setVariable(getDefinedVariable(option));

        int32_t ret = 0;
        do {
            ret = synclog.loadConfig(sSyncConfig, sSkelConfig);
            if (ret != 0) break;

            ret = synclog.prepareDatabase(option.hasOption("auto-create-db"));
            if (ret != 0) {
                LOG_DEBUG("prepare database failed, ret:" << ret);
                break;
            }

            if (option.hasOption("prepare-table-only")) {
                break;
            }

            ret = synclog.prepareFileLoader();
            if (ret != 0) {
                break;
            }

            LOG_DEBUG("------- synclog application launched -------");
            try {
                ret = synclog.processLog();
                if (ret == -2) {
                    ret = 0;
                }
            } catch (std::exception &e) {
                LOG_DEBUG("------- synclog application terminated(exception) -------, " << e.what());
                ret = -1;
                break;
            }
            LOG_DEBUG("------- synclog application terminated -------, " << ret);
        } while(0);

        CLogManager::getInstance()->finiLog();

        if (ret != 0) {
            return ret;
        }
    } catch (std::exception &e) {
        cout << "Exception: " << e.what() << endl;
        return -1;
    }
    return 0;
}
