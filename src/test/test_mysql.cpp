#include <iostream>
#include <unistd.h>

#include "util/util_log.h"
#include "util/util_option.h"
#include "util/util_mysql.h"

using namespace mfw;
using namespace std;

int main(int argc, char* argv[]) {

    CLogManager::getInstance()->initLog("./", "-");

    COption config;
    config.parse(argc, argv);

    if (!config.hasOption("h")
    || !config.hasOption("u")
    || !config.hasOption("p")) {
        cout << "Usage: " <<  argv[0] << endl;
        cout << " --h host" << endl;
        cout << " --P port default=3306" << endl;
        cout << " --u user" << endl;
        cout << " --p pwd" << endl;
        return 0;
    }

    CDBConf stConf;
    stConf.sHost = config.getOption("h");
    stConf.iPort = UtilString::strto<uint16_t>(config.getOption("P", "3306"));
    stConf.sDatabase = "tmp";
    stConf.sUser = config.getOption("u");
    stConf.sPassword = config.getOption("p");
    stConf.sCharset = "utf8";

    CMysql mysql;
    mysql.init(stConf);

    try {
        CMysqlTransaction transaction(mysql);

        mysql.execute("insert into a(id) values(3);");
        mysql.execute("insert into a(id) values(3);");
        
        transaction.commit();

        mysql.execute("insert into a(id) values(3);");
        mysql.execute("insert into tmp(id) values(1);");

    } catch (std::exception &e) {
        LOG_DEBUG(e.what());
    }

    CLogManager::getInstance()->finiLog();

    return 0;
}
