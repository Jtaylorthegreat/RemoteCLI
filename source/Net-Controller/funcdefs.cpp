/* Copyright 2019 Justin Taylor */

   /*This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Justin Taylor <taylor.justin199@gmail.com> */



#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <ctime>
#include <fstream>
#include <sstream>
#include <thread>
#include <sys/wait.h>
#include <algorithm>

#include "mysql_connection.h"
#include "mysql_driver.h"
#include "mysql_error.h"
#include <mysql.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"


#include "funcsheet.h"


using namespace std;
using namespace sql;



//functions

/*backend_processing:
--------------------------
runs every minute to check the DB for clients that are enabled and have not checked in within 5, 10, or 30 minutes depending on conf setting.
if clients have not checked in within the time limit set, it will set clientok boolean to false in DB
*/
void backend_processing(){

    spdlog::debug("Backend DB processing thread successfully initiated");
    
    while (true){
    
        if (mysql_library_init(0, NULL, NULL)) {
            spdlog::critical("Unable to connect to database for ClientOK check");
            mysql_library_end();
            exit(1);
        }
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        try {
            driver = get_driver_instance();
            con = driver->connect(DBserver.c_str(), DBuser.c_str(), DBuserPASS.c_str());
            con->setSchema(DB.c_str());
            con-> setAutoCommit(0);
            stmt = con->createStatement();
            stmt->execute("START TRANSACTION;");
            stmt->execute("UPDATE clients set ClientOK=false WHERE ClientEnabled=false");
            stmt->execute("COMMIT;");
            if(GLOBALAGENTTIMER == "30m"){
                stmt->execute("UPDATE clients set ClientOK=false WHERE ClientEnabled=true AND ClientHB < NOW() - INTERVAL 30 MINUTE");
                stmt->execute("COMMIT;");
            }
            else if(GLOBALAGENTTIMER == "10m"){
                stmt->execute("UPDATE clients set ClientOK=false WHERE ClientEnabled=true AND ClientHB < NOW() - INTERVAL 10 MINUTE");
                stmt->execute("COMMIT;");
            }
            else {
                stmt->execute("UPDATE clients set ClientOK=false WHERE ClientEnabled=true AND ClientHB < NOW() - INTERVAL 5 MINUTE");
                stmt->execute("COMMIT;");
            }
            stmt->execute("UPDATE clientassignments set ClientWaiting=1, ClientFileDependsT=0 where ClientFileDependsT=1 and ClientFileTime < NOW() and GroupName IS NULL");
            stmt->execute("COMMIT;");
            stmt->execute("UPDATE clientassignments set ClientWaitingAction=1, ClientActionDependsT=0 where ClientActionDependsT=1 and ClientActionTime < NOW() and GroupName IS NULL");
            stmt->execute("COMMIT;");
            stmt->execute("UPDATE groupassignments set GroupWaiting=1, GroupFileDependsT=0 where GroupFileDependsT=1 and GroupFileTime < NOW() and GroupEnabled=1");
            stmt->execute("COMMIT;");
            stmt->execute("UPDATE groupassignments set GroupWaitingAction=1, GroupActionDependsT=0 where GroupActionDependsT=1 and GroupActionTime < NOW() and GroupEnabled=1");
            stmt->execute("COMMIT;");
            //issues with group values always over writing agent values even if agents haven ran yet:
            //stmt->execute("UPDATE clientassignments INNER JOIN groupassignments on clientassignments.GroupName = groupassignments.GroupName SET clientassignments.ClientWaiting = groupassignments.GroupWaiting, clientassignments.ClientFile = groupassignments.GroupFile, clientassignments.ClientFileDependsT = groupassignments.GroupFileDependsT, clientassignments.ClientFileTime = groupassignments.GroupFileTime, clientassignments.ClientWaitingAction = groupassignments.GroupWaitingAction, clientassignments.ClientAction = groupassignments.GroupAction, clientassignments.ClientActionDependsT = groupassignments.GroupActionDependsT, clientassignments.ClientActionTime = groupassignments.GroupActionTime WHERE GroupEnabled=1");
            //update clients ONLY if groupwaiting and waitingaction are 1:
            stmt->execute("UPDATE clientassignments INNER JOIN groupassignments on clientassignments.GroupName = groupassignments.GroupName SET clientassignments.ClientWaiting = groupassignments.GroupWaiting, clientassignments.ClientFile = groupassignments.GroupFile WHERE GroupEnabled=1 AND GroupWaiting=1");
            stmt->execute("COMMIT;");
            stmt->execute("UPDATE clientassignments INNER JOIN groupassignments on clientassignments.GroupName = groupassignments.GroupName SET clientassignments.ClientWaitingAction = groupassignments.GroupWaitingAction, clientassignments.ClientAction = groupassignments.GroupAction WHERE GroupEnabled=1 AND GroupWaitingAction=1");
            stmt->execute("COMMIT;");
            //reset group values after previous client to group sync:
            stmt->execute("UPDATE groupassignments set GroupWaiting=0, GroupWaitingAction=0 WHERE GroupEnabled=1 AND GroupWaiting=1 AND GroupWaitingAction=1");
            stmt->execute("COMMIT;");
    
            delete stmt;
            delete con;
        }  catch (sql::SQLException &e) {
            spdlog::critical("# ERR: {}", e.what());
            spdlog::critical("Mysql error code: {}", e.what());
            spdlog::critical("SQLState: {}", e.getSQLState());
            exit(1);
            }
    
        mysql_library_end();
        spdlog::debug("Job for backend processing completed");
        sleep(60);
    }
}




/* grab_config:
----------------------
 pulls configs from conf file currently set to a conf file in the same directory as daemon named ".serverconfig"
moving this later to /etc/ 
*/
void grab_config(ifstream& in, configfile& out){
    in.open(".serverconfig");
    string line;
    
    while(!in.eof()){
        while(getline(in,line)){
            line.erase(remove_if(line.begin(), line.end(), ::isspace),
                                    line.end());
                if(line[0] == '#' || line.empty())
                    continue;
            auto delimiterPos = line.find("=");
            string name;
            name = line.substr(0, delimiterPos);
            if(name == "serverlisteningport")
                out.serverport = line.substr(delimiterPos +1); 
            if(name == "serverlisteningaddress")
                out.serverip = line.substr(delimiterPos +1);
            if(name == "dbuser")
                out.sqluser = line.substr(delimiterPos +1);
            if(name == "dbpass")
                out.sqlpass = line.substr(delimiterPos +1);
            if(name == "db")
                out.database = line.substr(delimiterPos +1);
            if(name == "agentregistrationkey")
                out.agregkey = line.substr(delimiterPos +1);
            if(name == "presharedkey")
                out.preserverkey = line.substr(delimiterPos +1);
            if(name == "acceptclientregistrationrequests")
                out.enableregistration = line.substr(delimiterPos +1);
            if(name == "debuglogging")
                out.debugg = line.substr(delimiterPos +1);
            if(name == "serverlog")
                out.serverlog = line.substr(delimiterPos +1);
            if(name == "certificatefile")
                out.sslcertfile = line.substr(delimiterPos +1);
            if(name == "certificatekeyfile")
                out.sslkeyfile = line.substr(delimiterPos +1);
            /*if(name == "certificatechainfile"){
                out.sslchainfile = line.substr(delimiterPos +1):
                else {
                    out.sslchainfile = "NOTSET";
                }
            }*/
            if(name == "agentfilequeuefolder")
                out.clientfilequeue = line.substr(delimiterPos +1);
            if(name == "agentactionweblogs")
                out.actionwebdirectory = line.substr(delimiterPos +1);
            if(name == "enablelegacytls")
                out.tlssetting = line.substr(delimiterPos +1);
            if(name == "markagentsofflineafter")
                out.offlinetimer = line.substr(delimiterPos +1);
        }
    
    }
    
}

void check_conf(){
    string conffile = ".serverconfig";
    ifstream openconf(conffile.c_str());
        if(!openconf) {
            cout << "Server Failure - unable to read configuration file \n";
            exit(1);
        }
    openconf.close();   

//get count of conf lines   
int numoflines = 0;
string lines;
ifstream countfile;

string path;
path = ".serverconfig";
countfile.open(path);
while (getline(countfile, lines))
    ++numoflines;
countfile.close();

ifstream file(".serverconfig");
int line_counter = 0;

int serverlisteningportcount = 0;
int serverlisteningaddresscount = 0;
int dbusercount = 0;
int dbpasscount = 0;
int dbcount = 0;
int agentregkeycount = 0;
int presharedserverkeycount = 0;
int acceptclientregistrationrequestscount = 0;
int debugcount = 0;
int serverlogcount = 0;
int certificatefilecheckcount = 0;
int certificatekeyfilecheckcount = 0;
int agentfilequeuefoldercheckcount = 0;
int actionwebdirectorycheckcount = 0;
int tlssetcheckcount = 0;
int agentofflinetimercheckcount = 0;

string line;


string serverlisteningportoutput;
string serverlisteningaddressoutput;
string dbuseroutput;
string dbpassoutput;
string dboutput;
string presharedserverkeyoutput;
string agentregkeyoutput;
string acceptclientregistrationrequestsoutput;
string debugoutput;
string serverlogoutput;
string certificatefilecheckoutput;
string certificatekeyfilecheckoutput;
string agentfilequeuefoldercheckoutput;
string actionwebdirectorycheckoutput;
string tlssetcheckoutput;
string agentofflinetimercheckoutput;

bool serverlisteningportok;
bool serverlisteningaddressok;
bool dbuserok;
bool dbpassok;
bool dbok;
bool agentregkeyok;
bool presharedserverkeyok;
bool acceptclientregistrationrequestsok;
bool debugconfok;
bool serverlogok;
bool certificatefilecheckok;
bool certificatekeyfilecheckok;
bool agentfilequeuefoldercheckok;
bool actionwebdirectorycheckok;
bool tlssetcheckok;
bool agentofflinetimercheckok;

  while (getline (file,line)) {
    line_counter++;
     line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
        if(line[0] == '#' || line.empty())
            continue;
        if (line.find("serverlisteningport") != string::npos) {
            serverlisteningportcount++;
        }
        if (line.find("serverlisteningaddress") != string::npos) {
            serverlisteningaddresscount++;
        } 
        if (line.find("dbuser") != string::npos) {
            dbusercount++;
        }
        if (line.find("dbpass") != string::npos) {
            dbpasscount++;
        }
        if (line.find("db=") != string::npos) {
            dbcount++;
        }
        if (line.find("agentregistrationkey") != string::npos) {
            agentregkeycount++;
        }
        if (line.find("presharedkey") != string::npos) {
            presharedserverkeycount++;
        }
        if (line.find("acceptclientregistrationrequests") != string::npos) {
            acceptclientregistrationrequestscount++;
        }
        if (line.find("debuglogging") != string::npos) {
            debugcount++;
        }
        if (line.find("serverlog") != string::npos) {
            serverlogcount++;
        }
        if (line.find("certificatefile") != string::npos) {
            certificatefilecheckcount++;
        }
        if (line.find("certificatekeyfile") != string::npos) {
            certificatekeyfilecheckcount++;
        }
        if (line.find("agentfilequeuefolder") != string::npos) {
            agentfilequeuefoldercheckcount++;
        }
        if (line.find("agentactionweblogs") != string::npos) {
            actionwebdirectorycheckcount++;
        }
        if (line.find("enablelegacytls") != string::npos) {
            tlssetcheckcount++;
        }
        if (line.find("markagentsofflineafter") != string::npos) {
            agentofflinetimercheckcount++;
        }

    }
    file.close();
    
    if (serverlisteningportcount < 1){
        serverlisteningportoutput = "serverlisteningport not found in conf file"; 
        serverlisteningportok = 0;
    }
    if (serverlisteningportcount > 1){
        serverlisteningportoutput = "Too many instances of serverlisteningport found in conf file";
        serverlisteningportok = 0;
    }
    if (serverlisteningportcount == 1){
        serverlisteningportoutput = "serverlisteningport ok";
        serverlisteningportok = 1;
    }
    //-----------------------------------------------------
    if (serverlisteningaddresscount < 1){
        serverlisteningaddressoutput = "serverlisteningaddress not found in conf file"; 
        serverlisteningaddressok = 0;
    }
    if (serverlisteningaddresscount > 1){
        serverlisteningaddressoutput = "Too many instances of serverlisteningaddress found in conf file";
        serverlisteningaddressok = 0;
    }
    if (serverlisteningaddresscount == 1){
        serverlisteningaddressoutput = "serverlisteningaddress ok";
        serverlisteningaddressok = 1;
    }
    //-----------------------------------------------------
    if (dbusercount < 1){
        dbuseroutput = "dbuser not found in conf file"; 
        dbuserok = 0;
    }
    if (dbusercount > 1){
        dbuseroutput = "Too many instances of dbuser found in conf file";
        dbuserok = 0;
    }
    if (dbusercount == 1){
        dbuseroutput = "dbuser ok";
        dbuserok = 1;
         
    }
    //-----------------------------------------------------
    if (dbpasscount < 1){
        dbpassoutput = "dbpass not found in conf file"; 
        dbpassok = 0;
    }
    if (dbpasscount > 1){
        dbpassoutput = "Too many instances of dbpass found in conf file";
        dbpassok = 0;
    }
    if (dbpasscount == 1){
        dbpassoutput = "dbpass ok";
        dbpassok = 1;
    }
    //-----------------------------------------------------
    if (dbcount < 1){
        dboutput = "db not found in conf file"; 
        dbok = 0;
    }
    if (dbcount > 1){
        dboutput = "Too many instances of db found in conf file";
        dbok = 0;
    }
    if (dbcount == 1){
        dboutput = "db ok";
        dbok = 1;
    }
    //-----------------------------------------------------
    if (agentregkeycount < 1){
        agentregkeyoutput = "agentregistrationkey not found in conf file"; 
        agentregkeyok = 0;
    }
    if (agentregkeycount > 1){
        agentregkeyoutput = "Too many instances of agentregistrationkey found in conf file";
        agentregkeyok = 0;
    }
    if (agentregkeycount == 1){
        agentregkeyoutput = "agentregistrationkey ok";
        agentregkeyok = 1;
    }
    //-----------------------------------------------------
    if (presharedserverkeycount < 1){
        presharedserverkeyoutput = "presharedkey not found in conf file"; 
        presharedserverkeyok = 0;
    }
    if (presharedserverkeycount > 1){
        presharedserverkeyoutput = "Too many instances of presharedkey found in conf file";
        presharedserverkeyok = 0;
    }
    if (presharedserverkeycount == 1){
        presharedserverkeyoutput = "presharedkey ok";
        presharedserverkeyok = 1;
    }
    //-----------------------------------------------------
    if (acceptclientregistrationrequestscount < 1){
        acceptclientregistrationrequestsoutput = "acceptclientregistrationrequests not found in conf file"; 
        acceptclientregistrationrequestsok = 0;
    }
    if (acceptclientregistrationrequestscount > 1){
        acceptclientregistrationrequestsoutput = "Too many instances of acceptclientregistrationrequests found in conf file";
        acceptclientregistrationrequestsok = 0;
    }
    if (acceptclientregistrationrequestscount == 1){
        acceptclientregistrationrequestsoutput = "acceptclientregistrationrequests ok";
        acceptclientregistrationrequestsok = 1;
    }
    //-----------------------------------------------------
    if (debugcount < 1){
        debugoutput = "debug not found in conf file"; 
        debugconfok = 0;
    }
    if (debugcount > 1){
        debugoutput = "Too many instances of debug found in conf file";
        debugconfok = 0;
    }
    if (debugcount == 1){
        debugoutput = "debug ok";
        debugconfok = 1;
    }
    //-----------------------------------------------------
    if (serverlogcount < 1){
        serverlogoutput = "serverlog not found in conf file"; 
        serverlogok = 0;
    }
    if (serverlogcount > 1){
        serverlogoutput = "Too many instances of serverlog found in conf file";
        serverlogok = 0;
    }
    if (serverlogcount == 1){
        serverlogoutput = "serverlog ok";
        serverlogok = 1;
    }
    //-----------------------------------------------------
    if (certificatefilecheckcount < 1){
        certificatefilecheckoutput = "agentslog not found in conf file"; 
        certificatefilecheckok = 0;
    }
    if (certificatefilecheckcount > 1){
        certificatefilecheckoutput = "Too many instances of agentslog found in conf file";
        certificatefilecheckok = 0;
    }
    if (certificatefilecheckcount == 1){
        certificatefilecheckoutput = "agentslog ok";
        certificatefilecheckok = 1;
    }
    //-----------------------------------------------------
    if (certificatekeyfilecheckcount < 1){
        certificatekeyfilecheckoutput = "agentslog not found in conf file"; 
        certificatekeyfilecheckok = 0;
    }
    if (certificatekeyfilecheckcount > 1){
        certificatekeyfilecheckoutput = "Too many instances of agentslog found in conf file";
        certificatekeyfilecheckok = 0;
    }
    if (certificatekeyfilecheckcount == 1){
        certificatekeyfilecheckoutput = "agentslog ok";
        certificatekeyfilecheckok = 1;
    }
    //-----------------------------------------------------
    if (agentfilequeuefoldercheckcount < 1){
        agentfilequeuefoldercheckoutput = "agentfilequeuefolder not found in conf file"; 
        agentfilequeuefoldercheckok = 0;
    }
    if (agentfilequeuefoldercheckcount > 1){
        agentfilequeuefoldercheckoutput = "Too many instances of agentfilequeuefolder found in conf file";
        agentfilequeuefoldercheckok = 0;
    }
    if (agentfilequeuefoldercheckcount == 1){
        agentfilequeuefoldercheckoutput = "agentfilequeuefolder ok";
        agentfilequeuefoldercheckok = 1;
    }
    //-----------------------------------------------------
    if (actionwebdirectorycheckcount < 1){
        actionwebdirectorycheckoutput = "webdir not found in conf file"; 
        actionwebdirectorycheckok = 0;
    }
    if (actionwebdirectorycheckcount > 1){
        actionwebdirectorycheckoutput = "Too many instances of webdir found in conf file";
        actionwebdirectorycheckok = 0;
    }
    if (actionwebdirectorycheckcount == 1){
        actionwebdirectorycheckoutput = "webdir ok";
        actionwebdirectorycheckok = 1;
    }
    //-----------------------------------------------------
    if (tlssetcheckcount < 1){
        tlssetcheckoutput = "enablelegacytls not found in conf file"; 
        tlssetcheckok = 0;
    }
    if (tlssetcheckcount > 1){
        tlssetcheckoutput = "Too many instances of enablelegacytls found in conf file";
        tlssetcheckok = 0;
    }
    if (tlssetcheckcount == 1){
        tlssetcheckoutput = "enablelegacytls ok";
        tlssetcheckok = 1;
    }
    //-----------------------------------------------------
    if (agentofflinetimercheckcount < 1){
        agentofflinetimercheckoutput = "enablelegacytls not found in conf file"; 
        agentofflinetimercheckok = 0;
    }
    if (agentofflinetimercheckcount > 1){
        agentofflinetimercheckoutput = "Too many instances of enablelegacytls found in conf file";
        agentofflinetimercheckok = 0;
    }
    if (agentofflinetimercheckcount == 1){
        agentofflinetimercheckoutput = "enablelegacytls ok";
        agentofflinetimercheckok = 1;
    }
    //-----------------------------------------------------
    if (serverlisteningportok !=1 || serverlisteningaddressok !=1 || dbuserok !=1 || dbpassok !=1 || dbok !=1 || agentregkeyok !=1 || presharedserverkeyok !=1 || acceptclientregistrationrequestsok !=1 || debugconfok !=1 || serverlogok !=1 || certificatefilecheckok !=1 || certificatekeyfilecheckok !=1 || agentfilequeuefoldercheckok !=1 || actionwebdirectorycheckok !=1 || tlssetcheckok !=1 || agentofflinetimercheckok !=1){
        cout << "Configuration file error: \n";
        if(serverlisteningportok == 0){
            cout << serverlisteningportoutput << "\n";
        }
        if(serverlisteningaddressok == 0){
            cout << serverlisteningaddressoutput << "\n";
        }
        if(dbuserok == 0){
            cout << dbuseroutput << "\n";
        }
        if(dbpassok == 0){
            cout << dbpassoutput << "\n";
        }
        if(dbok == 0){
            cout << dboutput << "\n";
        }
        if(agentregkeyok == 0){
            cout << agentregkeyoutput << "\n";
        }
        if(presharedserverkeyok == 0){
            cout << presharedserverkeyoutput << "\n";
        }
        if(acceptclientregistrationrequestsok == 0){
            cout << acceptclientregistrationrequestsoutput << "\n";
        }
        if(debugconfok == 0){
            cout << debugoutput << "\n";
        }
        if(serverlogok == 0){
            cout << serverlogoutput << "\n";
        }
        if(certificatefilecheckok==0){
            cout << certificatefilecheckoutput << "\n";
        }
        if(certificatekeyfilecheckok==0){
            cout << certificatekeyfilecheckoutput << "\n";
        }
        if(agentfilequeuefoldercheckok==0){
            cout << agentfilequeuefoldercheckoutput << "\n";
        }
        if(actionwebdirectorycheckok==0){
            cout << actionwebdirectorycheckoutput << "\n";
        }
        if(tlssetcheckok==0){
            cout << tlssetcheckoutput << "\n";
        }
        if(agentofflinetimercheckok==0){
            cout << agentofflinetimercheckoutput << "\n";
        }
        exit(1);
     }

}

int whats_my_size(string filepass){
    
    streampos begin,end;
    ifstream findfilesize (filepass.c_str(), ios::binary);
        /*if(!findfilesize){
            spdlog::error("-- Unable to open file for transfer: {}", clientparsedhostname.c_str());
            spdlog::error("-- Unable to open file for transfer: {}", filepass.c_str());
            
            exit(1);
        }*/
    begin = findfilesize.tellg();
    findfilesize.seekg (0, ios::end);
    end = findfilesize.tellg();
    findfilesize.close();
    int data = (end-begin);
    return data;

}


size_t whats_my_size_vol_2(string filepass) {
    ifstream fcheck(filepass.c_str(), ios::binary);
        //check added to main below is redundant:
        /*if(!fcheck){
            spdlog::error("-- Unable to open file for transfer: {}", filepass.c_str());
            return 0;
        }*/
    struct stat passedfile;
    if(stat(filepass.c_str(), &passedfile) != 0) {
        return 0;
    }
    return passedfile.st_size;   
}

void copy_file_req(string sourcefile, string destdirectory, string copyfilefullpath, string rotatefile){
    if (mkdir(destdirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1){
        if( errno == EEXIST ) {
            goto runNormalCopy;
        }
        else {
            spdlog::error("Unable to create directory for command output logs: {}", destdirectory.c_str());
            exit(0);
        }
    }
    else {
        spdlog::info("Directory created for web output logs: {}", destdirectory.c_str());
    }
    runNormalCopy:
    //rotation:
    remove(rotatefile.c_str());
    rename(copyfilefullpath.c_str(), rotatefile.c_str());
    //end of rotation, to use only file copy remove above to lines and 'rotatefile' in function declare
    ifstream canopensourcef(sourcefile.c_str(), ios::binary);
        if(!canopensourcef){
            spdlog::error(" Unable to open sourcefile {0} for copy to webdirectory: {1}", sourcefile.c_str(), copyfilefullpath.c_str());
            exit(0);
        }
    ofstream canopendestf;
    canopendestf.open(copyfilefullpath.c_str(), ios::out | ios::binary);
        if(!canopendestf){
            spdlog::error(" Unable to write to webdirectory {1} for copy of sourcefile: {0}", sourcefile.c_str(), copyfilefullpath.c_str());
            exit(0);
        }
    size_t webfilesize = whats_my_size_vol_2(sourcefile.c_str());
        if (webfilesize > INT_MAX){
            spdlog::error(" Source file {0} too large for copy to webdirectory: {1}", sourcefile.c_str(), copyfilefullpath.c_str());
            exit(0);
        }
    size_t slabsize = 4 * 1024;
    size_t totalslabs = webfilesize / slabsize;
    size_t finalslabsize = webfilesize % slabsize;
    if (finalslabsize !=0){
        ++totalslabs;
    }
    else {
        finalslabsize = slabsize;
    }
    for (size_t slab = 0; slab < totalslabs; slab++){
        size_t currentslabsize =
        slab == totalslabs - 1
        ? finalslabsize
        : slabsize;

        int currentslabmovesize = static_cast<int>(currentslabsize);

        vector<char> slabbindata(currentslabsize);
        canopensourcef.read(&slabbindata[0], currentslabsize);
        char readingbuffer[currentslabmovesize];
        copy(slabbindata.begin(), slabbindata.end(), readingbuffer);
        
        canopendestf.write(&slabbindata[0], currentslabsize);
        


        }
        canopensourcef.close();
        canopendestf.close();

}

void copy_file_req_part_2(string sourcefile, string destdirectory, string copyfilefullpath, string rotatefile1, string rotatefile2, string rotatefile3){
    if (mkdir(destdirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1){
        if( errno == EEXIST ) {
            goto runNormalCopy;
        }
        else {
            spdlog::error("Unable to create directory for agent file upload: {}", destdirectory.c_str());
            exit(0);
        }
    }
    else {
        spdlog::info("Directory created for agent file upload: {}", destdirectory.c_str());
    }
    runNormalCopy:
    //rotation:
    remove(rotatefile3.c_str());
    rename(rotatefile2.c_str(), rotatefile3.c_str());
    rename(rotatefile1.c_str(), rotatefile2.c_str());
    rename(copyfilefullpath.c_str(), rotatefile1.c_str());

    //rename(copyfilefullpath.c_str(), rotatefile.c_str());
    //end of rotation, to use only file copy remove above to lines and 'rotatefile' in function declare
    ifstream canopensourcef(sourcefile.c_str(), ios::binary);
        if(!canopensourcef){
            spdlog::error(" Unable to open sourcefile {0} for copy to webdirectory: {1}", sourcefile.c_str(), copyfilefullpath.c_str());
            exit(0);
        }
    ofstream canopendestf;
    canopendestf.open(copyfilefullpath.c_str(), ios::out | ios::binary);
        if(!canopendestf){
            spdlog::error(" Unable to write to webdirectory {1} for copy of sourcefile: {0}", sourcefile.c_str(), copyfilefullpath.c_str());
            exit(0);
        }
    size_t webfilesize = whats_my_size_vol_2(sourcefile.c_str());
        if (webfilesize > INT_MAX){
            spdlog::error(" Source file {0} too large for copy to webdirectory: {1}", sourcefile.c_str(), copyfilefullpath.c_str());
            exit(0);
        }
    size_t slabsize = 4 * 1024;
    size_t totalslabs = webfilesize / slabsize;
    size_t finalslabsize = webfilesize % slabsize;
    if (finalslabsize !=0){
        ++totalslabs;
    }
    else {
        finalslabsize = slabsize;
    }
    for (size_t slab = 0; slab < totalslabs; slab++){
        size_t currentslabsize =
        slab == totalslabs - 1
        ? finalslabsize
        : slabsize;

        int currentslabmovesize = static_cast<int>(currentslabsize);

        vector<char> slabbindata(currentslabsize);
        canopensourcef.read(&slabbindata[0], currentslabsize);
        char readingbuffer[currentslabmovesize];
        copy(slabbindata.begin(), slabbindata.end(), readingbuffer);
        
        canopendestf.write(&slabbindata[0], currentslabsize);
        
        }
        canopensourcef.close();
        canopendestf.close();

}


void simple_copy_file_req(string sourcefile, string destdirectory, string copyfilefullpath){
    if (mkdir(destdirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1){
        if( errno == EEXIST ) {
            goto runNormalCopy;
        }
        else {
            spdlog::error("Unable to create directory: {}", destdirectory.c_str());
            exit(0);
        }
    }
    else {
        spdlog::info("Directory created: {}", destdirectory.c_str());
    }
    runNormalCopy:
    ifstream canopensourcef(sourcefile.c_str(), ios::binary);
        if(!canopensourcef){
            spdlog::error(" Unable to open sourcefile {0} for copy to directory: {1}", sourcefile.c_str(), copyfilefullpath.c_str());
            exit(0);
        }
    ofstream canopendestf;
    canopendestf.open(copyfilefullpath.c_str(), ios::out | ios::binary);
        if(!canopendestf){
            spdlog::error(" Unable to write to directory {1} for copy of sourcefile: {0}", sourcefile.c_str(), copyfilefullpath.c_str());
            exit(0);
        }
    size_t webfilesize = whats_my_size_vol_2(sourcefile.c_str());
        if (webfilesize > INT_MAX){
            spdlog::error(" Source file {0} too large for copy to directory: {1}", sourcefile.c_str(), copyfilefullpath.c_str());
            exit(0);
        }
    size_t slabsize = 4 * 1024;
    size_t totalslabs = webfilesize / slabsize;
    size_t finalslabsize = webfilesize % slabsize;
    if (finalslabsize !=0){
        ++totalslabs;
    }
    else {
        finalslabsize = slabsize;
    }
    for (size_t slab = 0; slab < totalslabs; slab++){
        size_t currentslabsize =
        slab == totalslabs - 1
        ? finalslabsize
        : slabsize;

        int currentslabmovesize = static_cast<int>(currentslabsize);

        vector<char> slabbindata(currentslabsize);
        canopensourcef.read(&slabbindata[0], currentslabsize);
        char readingbuffer[currentslabmovesize];
        copy(slabbindata.begin(), slabbindata.end(), readingbuffer);
        
        canopendestf.write(&slabbindata[0], currentslabsize);
        


        }
        canopensourcef.close();
        canopendestf.close();

}
