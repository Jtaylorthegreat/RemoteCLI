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
#include <sys/stat.h>
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
#include <openssl/ssl.h>
#include <openssl/err.h>

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
#include "spdlog/sinks/rotating_file_sink.h"
#define CIPHER_LIST "AES256-GCM-SHA384:AES256-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-SHA256:AES128-GCM-SHA256:AES128-SHA256"
#define CIPHER_LIST13 "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_CCM_SHA256:TLS_AES_128_CCM_8_SHA256"


#include "funcsheet.h"


using namespace std;
using namespace sql;


  

string SPORT, DBserver, DBuser, DBuserPASS, DB, REGKEY, PSK, ENABLEREG, SERVLOG, SERVSSL, SERVSSLPRIVKEY, DEBUGVALUE, AFILEQUEUE, ALOGSWEB, SETTLS12, GLOBALAGENTTIMER;

SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
    perror("Unable to create SSL context");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
    }

    return ctx;
}

inline bool check_if_file_exists(const string& compfile) {
    struct stat compfilebuff;
    return (stat (compfile.c_str(), &compfilebuff) == 0);
}

void configure_context(SSL_CTX *ctx)
{
      SSL_CTX_set_ecdh_auto(ctx, 1);
    /*if (SSL_CTX_set_cipher_list(ctx, CIPHER_LIST) <=0){
        printf("Error setting the cipher list.\n");
        exit(0);
    }*/
    
    if (SETTLS12 == "1"){
        /*leaving SSL_CTX_set_cipher_list unset allows TLS 1.1 & 1.2 support with ANY cipher selected by the client. 
        By default ubuntu18 & centos 7 appear to select ECDHE-RSA-AES256-SHA currently, however anyone can specify a tls1.0 null cipher and it be not secure at all*/
        //SSL_CTX_set_cipher_list(ctx, CIPHER_LIST);
        if (SSL_CTX_set_ciphersuites(ctx, CIPHER_LIST13) <=0){
            spdlog::error("Error setting cipher list for legacy mode.");
        }
        spdlog::info("WARNING! Legacy TLS Mode Enabled, This mode may allow insecure connections as TLS ciphers are negotiated by the agent ");
    }
    else {
        SSL_CTX_set_cipher_list(ctx, CIPHER_LIST);
        SSL_CTX_set_ciphersuites(ctx, CIPHER_LIST13);
    }

    if (SSL_CTX_use_certificate_file(ctx, SERVSSL.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, SERVSSLPRIVKEY.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
    }
} 


void hblisten (SSL* ssl, char* clientconnectedfromip){
    char buf[1024] = {0};
    int receivedclientcomms, sd;
    if (SSL_accept(ssl) <= 0) {
        if (DEBUGVALUE == "1"){
            FILE *sslerrs;
            spdlog::error("{} Unsuccessful client TLS/SSL handshake, connection dropped", clientconnectedfromip);
            spdlog::debug("[Openssl error]");
            spdlog::debug("---------------------------------------------------------");
            sslerrs=fopen(SERVLOG.c_str(), "a+");
            ERR_print_errors_fp(sslerrs);
            fclose(sslerrs);
            spdlog::debug("---------------------------------------------------------");
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
        }
        else {
            spdlog::error("{} Unsuccessful client TLS/SSL handshake, connection dropped", clientconnectedfromip);
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
        }
    }
    spdlog::debug("{} Accepted client TLS/SSL handshake, processing connection", clientconnectedfromip);
    //send psk to client for verification:
    stringstream prefacepsksend;
    prefacepsksend << ":" << PSK << "*";
    const string sendpsk = prefacepsksend.str();
    const char *sendpskmess = sendpsk.c_str();
    SSL_write (ssl, sendpskmess, strlen(sendpskmess));


    if ((receivedclientcomms = SSL_read(ssl, buf, sizeof(buf))) <= 0 ){
        spdlog::error("{} Connection dropped by agent", clientconnectedfromip);
        sd = SSL_get_fd (ssl);
        SSL_free(ssl);
        close (sd);
        exit(1);
    }
    buf[receivedclientcomms] = '\0';

    //keep below comment for enabling debug client message output in future:
    //spdlog::debug("Recieved client communication: {}", buf);

    string receivedclientmess = buf;
    size_t q = receivedclientmess.find(":");
    size_t p = receivedclientmess.find("*");
    size_t r = receivedclientmess.find("!");
    size_t s = receivedclientmess.find("^");
    size_t t = receivedclientmess.find("+");
    string clientparsedregkey = receivedclientmess.substr(q + 1, p - q - 1);
    string clientparsedhostname = receivedclientmess.substr(p + 1, r - p - 1);
    string clientparsedaction = receivedclientmess.substr(r + 1, s - r - 1);
    string clientparsedhbkey = receivedclientmess.substr(s + 1, t - s - 1);
    string clientparsedcustomuploadfile = receivedclientmess.substr(t + 1);

    if (clientparsedaction == "heartbeatu"){

        if (mysql_library_init(0, NULL, NULL)) {
            spdlog::critical("Unable to connect to database for heart beat client check in");
            mysql_library_end();
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
        }
  
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::ResultSet *res;
        try {
            driver = get_driver_instance();
            con = driver->connect(DBserver.c_str(), DBuser.c_str(), DBuserPASS.c_str());
            con->setSchema(DB.c_str());
            con-> setAutoCommit(0);
            stmt = con->createStatement();

            //match host query
            stringstream MH;
            MH << "SELECT * FROM clients WHERE ClientEnabled=true AND Clientkey='" << clientparsedhbkey << "' AND Hostname='" << clientparsedhostname << "'";
            string matchhostfromkey = MH.str();
    
            //set clientok bool to true
            stringstream CLO, CLO2;
            CLO << "UPDATE clients set ClientOK=true, ClientHB=CURRENT_TIMESTAMP WHERE ClientEnabled=true AND Clientkey='" << clientparsedhbkey << "' AND Hostname='" << clientparsedhostname << "'";
            string setclientokbool = CLO.str();
            CLO2 << "update clientinfo INNER JOIN clients on clientinfo.Hostname = clients.Hostname SET  clientinfo.ClientIP=INET_ATON('" << clientconnectedfromip << "') WHERE clients.ClientEnabled=true AND clients.Clientkey='" << clientparsedhbkey << "' AND clients.Hostname='" << clientparsedhostname << "'";
            string updateclientip = CLO2.str();
            stmt->execute(setclientokbool.c_str());
            stmt->execute("COMMIT;");
            stmt->execute(updateclientip.c_str());
            stmt->execute("COMMIT;");
            //execute query to match host to key, if key matches log heartbeat
            res = stmt->executeQuery(matchhostfromkey.c_str());
            if(res->next() == false){
                const char *hbderror2 = ":hbderror2*--!";
                spdlog::error("{2} Hostname: {0} using key: {1} failed heartbeat attempt due to host/key combination being unregistered or disabled", clientparsedhostname.c_str(), clientparsedhbkey.c_str(), clientconnectedfromip);
                SSL_write(ssl, hbderror2, strlen(hbderror2));
                delete res;
                delete stmt;
                delete con;
                mysql_library_end();
                sd = SSL_get_fd (ssl);
                SSL_free(ssl);
                close (sd);
                exit(1);
            }
            delete res;
            delete stmt;
            delete con;
        }  catch (sql::SQLException &e) {
            spdlog::error("# ERR: {}", e.what());
            spdlog::error("Mysql error code: {}", e.what());
            spdlog::error("SQLState: {}", e.getSQLState());
            mysql_library_end();
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
            }
        mysql_library_end();
        spdlog::info("{} successful heartbeat", clientparsedhostname.c_str());
        const char *cuinitfail = ":CUSTINITFAILURE*";
        const char *cuinitsuccess = ":CUSTINITSUCCESS*";
        stringstream auploaddir;
        stringstream fulluploadfilepath;
        //add custom path (extern string) for remote.outputs later in conf file:
        auploaddir << "logs/.remote.uploads/" << clientparsedhostname.c_str();
        fulluploadfilepath << "logs/.remote.uploads/" << clientparsedhostname.c_str() << "/" << clientparsedcustomuploadfile.c_str();
        string hostupdir = auploaddir.str();
        string hostuploadedfile = fulluploadfilepath.str();
        if (mkdir(hostupdir.c_str(), S_IRWXU | S_IRGRP | S_IROTH) == -1){
            if( errno == EEXIST ) {
                goto runNormalCustomFileUpload;
            }
            else {
                spdlog::critical("Unable to create folder for host file uploads: {}", hostupdir.c_str());
                SSL_write (ssl, cuinitfail, strlen(cuinitfail));
                sd = SSL_get_fd (ssl);
                SSL_free(ssl);
                close (sd);
                exit(0);
            }    
        }
        spdlog::info("created directory for host file uploads: {}", hostupdir.c_str());


        runNormalCustomFileUpload:
        if (check_if_file_exists(hostuploadedfile.c_str()) ==1){
            spdlog::info("File: {1} already exists locally at path: {0}", hostuploadedfile.c_str(), clientparsedcustomuploadfile.c_str());
            spdlog::info("Replacing file {} with new transferred file", clientparsedcustomuploadfile.c_str());
            remove(hostuploadedfile.c_str());
        }
        ofstream cupfile;
            cupfile.open(hostuploadedfile.c_str(), ios::out | ios::binary);
                if(!cupfile){
                    spdlog::critical("Unable to create file location for agent upload: {}", hostuploadedfile.c_str());
                    SSL_write (ssl, cuinitfail, strlen(cuinitfail));
                    sd = SSL_get_fd (ssl);
                    SSL_free(ssl);
                    close (sd);
                    exit(0);
                }
                else {
                    SSL_write (ssl, cuinitsuccess, strlen(cuinitsuccess));
                    //PROCESS FILE MOVE HERE:
                    //-----------------------
                    size_t filebytes = stoi(clientparsedregkey);
                    float totaltransmission = 0;
                    size_t totalfilesize = filebytes;
                    
                    size_t slabsize = 4 * 1024;
                    size_t totalslabs = totalfilesize / slabsize;
                    size_t finalslabsize = totalfilesize % slabsize;
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
                        
                        vector<char> slabbindata(currentslabsize);
                                        
                        float reccustuploads = SSL_read(ssl, &slabbindata[0], currentslabsize);
                        if (reccustuploads < 0){
                            spdlog::error("connection error during agent file transfer: {}", hostuploadedfile.c_str());
                            cupfile.close();
                            sd = SSL_get_fd (ssl);
                            SSL_free(ssl);
                            close (sd);
                            exit(1);
                        }
                        cupfile.write(&slabbindata[0], currentslabsize);
                        totaltransmission += reccustuploads;
                        //for troubleshooting:
                        //printf("wrote file data: %lf\n", totaltransmission);
                    }
                    //for troubleshooting:    
                    //printf("total data written: %d\n", totaltransmission);
                    if (totaltransmission >= GB)
                        spdlog::info("{0} agent file upload successful, {1} GB transferred", clientparsedhostname.c_str(), totaltransmission/GB);
                    else if (totaltransmission >=MB && totaltransmission < GB)
                        spdlog::info("{0} agent file upload successful, {1} MB transferred", clientparsedhostname.c_str(), totaltransmission/MB);
                    else if (totaltransmission >=kB && totaltransmission < MB)
                        spdlog::info("{0} agent file upload successful, {1} kB transferred", clientparsedhostname.c_str(), totaltransmission/kB);
                    else
                        spdlog::info("{0} agent file upload successful, {1} bytes transferred", clientparsedhostname.c_str(), totaltransmission);
                    
                    cupfile.close();
                    sd = SSL_get_fd (ssl);
                    SSL_free(ssl);
                    close (sd);
                    stringstream sscopytodir;
                    sscopytodir << ALOGSWEB.c_str() << clientparsedhostname.c_str();
                    string copytodir = sscopytodir.str();

                    stringstream sscopyfullfilepath;
                    sscopyfullfilepath << ALOGSWEB.c_str() << clientparsedhostname.c_str() << "/" << clientparsedcustomuploadfile.c_str();
                    string cucopyfullfilepath = sscopyfullfilepath.str();

                    simple_copy_file_req(hostuploadedfile.c_str(), copytodir.c_str(), cucopyfullfilepath.c_str());

                    exit(0);
            
                }
    }




    //------------------------------------------------------------------------------------------------------------
    if (clientparsedaction == "heartbeatc"){

        if (mysql_library_init(0, NULL, NULL)) {
            spdlog::critical("Unable to connect to database for heart beat client check in");
            mysql_library_end();
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
        }
  
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::ResultSet *res;
        try {
            driver = get_driver_instance();
            con = driver->connect(DBserver.c_str(), DBuser.c_str(), DBuserPASS.c_str());
            con->setSchema(DB.c_str());
            con-> setAutoCommit(0);
            stmt = con->createStatement();

            //match host query
            stringstream MH;
            MH << "SELECT * FROM clients WHERE ClientEnabled=true AND Clientkey='" << clientparsedhbkey << "' AND Hostname='" << clientparsedhostname << "'";
            string matchhostfromkey = MH.str();
    
            //set clientok bool to true
            stringstream CLO, CLO2;
            CLO << "UPDATE clients set ClientOK=true, ClientHB=CURRENT_TIMESTAMP WHERE ClientEnabled=true AND Clientkey='" << clientparsedhbkey << "' AND Hostname='" << clientparsedhostname << "'";
            string setclientokbool = CLO.str();
            CLO2 << "update clientinfo INNER JOIN clients on clientinfo.Hostname = clients.Hostname SET  clientinfo.ClientIP=INET_ATON('" << clientconnectedfromip << "') WHERE clients.ClientEnabled=true AND clients.Clientkey='" << clientparsedhbkey << "' AND clients.Hostname='" << clientparsedhostname << "'";
            string updateclientip = CLO2.str();
            stmt->execute(setclientokbool.c_str());
            stmt->execute("COMMIT;");
            stmt->execute(updateclientip.c_str());
            stmt->execute("COMMIT;");
            //execute query to match host to key, if key matches log heartbeat
            res = stmt->executeQuery(matchhostfromkey.c_str());
            if(res->next() == false){
                const char *hbderror2 = ":hbderror2*--!";
                spdlog::error("{2} Hostname: {0} using key: {1} failed heartbeat attempt due to host/key combination being unregistered or disabled", clientparsedhostname.c_str(), clientparsedhbkey.c_str(), clientconnectedfromip);
                SSL_write(ssl, hbderror2, strlen(hbderror2));
                delete res;
                delete stmt;
                delete con;
                mysql_library_end();
                sd = SSL_get_fd (ssl);
                SSL_free(ssl);
                close (sd);
                exit(1);
            }
            delete res;
            delete stmt;
            delete con;
        }  catch (sql::SQLException &e) {
            spdlog::error("# ERR: {}", e.what());
            spdlog::error("Mysql error code: {}", e.what());
            spdlog::error("SQLState: {}", e.getSQLState());
            mysql_library_end();
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
            }
        mysql_library_end();
        spdlog::info("{} successful heartbeat", clientparsedhostname.c_str());
        const char *cinitfail = ":COMPINITFAILURE*";
        const char *cinitsuccess = ":COMPINITSUCCESS*";
        stringstream complogdir;
        stringstream completefilelog;
        //add custom path (extern string) for remote.outputs later in conf file:
        complogdir << "logs/.remote.outputs/" << clientparsedhostname.c_str();
        completefilelog << "logs/.remote.outputs/" << clientparsedhostname.c_str() << "/complete.output";
        string hostlogpath = complogdir.str();
        string hostcompletefile = completefilelog.str();
        if (mkdir(hostlogpath.c_str(), S_IRWXU | S_IRGRP | S_IROTH) == -1){
            if( errno == EEXIST ) {
                goto runNormalCompleteLogGrab;
            }
            else {
                spdlog::critical("Unable to create folder for host command output logs: {}", hostlogpath.c_str());
                SSL_write (ssl, cinitfail, strlen(cinitfail));
                sd = SSL_get_fd (ssl);
                SSL_free(ssl);
                close (sd);
                exit(0);
            }    
        }
        spdlog::info("created command output log directory: {}", hostlogpath.c_str());


        runNormalCompleteLogGrab:
        //rotate 1 file (add more later or even an option to configure rotated files):
        remove("logs/.remote.outputs/complete.output.1");
        rename(".outputs/complete.output", ".outputs/complete.output.1");
        ofstream complog;
            complog.open(hostcompletefile.c_str(), ios::out | ios::binary);
                if(!complog){
                    spdlog::critical("Unable to create file location for command output log: {}", hostcompletefile.c_str());
                    SSL_write (ssl, cinitfail, strlen(cinitfail));
                    sd = SSL_get_fd (ssl);
                    SSL_free(ssl);
                    close (sd);
                    exit(0);
                }
                else {
                    SSL_write (ssl, cinitsuccess, strlen(cinitsuccess));
                    //PROCESS FILE MOVE HERE:
                    //-----------------------
                    size_t filebytes = stoi(clientparsedregkey);
                    float totaltransmission = 0;
                    size_t totalfilesize = filebytes;
                    
                    size_t slabsize = 4 * 1024;
                    size_t totalslabs = totalfilesize / slabsize;
                    size_t finalslabsize = totalfilesize % slabsize;
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
                        
                        vector<char> slabbindata(currentslabsize);
                                        
                        float reccompoutputs = SSL_read(ssl, &slabbindata[0], currentslabsize);
                        if (reccompoutputs < 0){
                            spdlog::error("connection error during command log transfer: {}", hostcompletefile.c_str());
                            complog.close();
                            sd = SSL_get_fd (ssl);
                            SSL_free(ssl);
                            close (sd);
                            exit(1);
                        }
                        complog.write(&slabbindata[0], currentslabsize);
                        totaltransmission += reccompoutputs;
                        //for troubleshooting:
                        //printf("wrote file data: %lf\n", totaltransmission);
                    }
                    //for troubleshooting:    
                    //printf("total data written: %d\n", totaltransmission);
                    if (totaltransmission >= GB)
                        spdlog::info("{0} agent complete log successfully synced, {1} GB transferred", clientparsedhostname.c_str(), totaltransmission/GB);
                    else if (totaltransmission >=MB && totaltransmission < GB)
                        spdlog::info("{0} agent complete log successfully synced, {1} MB transferred", clientparsedhostname.c_str(), totaltransmission/MB);
                    else if (totaltransmission >=kB && totaltransmission < MB)
                        spdlog::info("{0} agent complete log successfully synced, {1} kB transferred", clientparsedhostname.c_str(), totaltransmission/kB);
                    else
                        spdlog::info("{0} agent complete log successfully synced, {1} bytes transferred", clientparsedhostname.c_str(), totaltransmission);
                    
                    complog.close();
                    sd = SSL_get_fd (ssl);
                    SSL_free(ssl);
                    close (sd);
                    //set up copy to web directory action log:
                    stringstream aweblogdir;
                    stringstream aweblogfile;
                    stringstream aweblogfile1;
                    aweblogdir << ALOGSWEB.c_str() << clientparsedhostname.c_str();
                    aweblogfile << ALOGSWEB.c_str() << clientparsedhostname.c_str() << "/complete.output";
                    aweblogfile1 << ALOGSWEB.c_str() << clientparsedhostname.c_str() << "/complete.output.1";

                    string actwebdir = aweblogdir.str();
                    string actwebfile = aweblogfile.str();
                    string actwebfile1 = aweblogfile1.str();
                    
                    copy_file_req(hostcompletefile.c_str(), actwebdir.c_str(), actwebfile.c_str(), actwebfile1.c_str());
                    exit(0);
            
                }
    }

    if (clientparsedaction == "heartbeat"){

        if (mysql_library_init(0, NULL, NULL)) {
            const char *hbdberror = ":hbdberror1*--!";
            SSL_write(ssl, hbdberror, strlen(hbdberror));
            spdlog::critical("Unable to connect to database for heart beat client check in");
            mysql_library_end();
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            delete(hbdberror);
            close (sd);
            exit(1);
        }
  
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::ResultSet *res;
        try {
            driver = get_driver_instance();
            con = driver->connect(DBserver.c_str(), DBuser.c_str(), DBuserPASS.c_str());
            con->setSchema(DB.c_str());
            con-> setAutoCommit(0);
            stmt = con->createStatement();

            //match host query
            stringstream MH;
            MH << "SELECT * FROM clients WHERE ClientEnabled=true AND Clientkey='" << clientparsedhbkey << "' AND Hostname='" << clientparsedhostname << "'";
            string matchhostfromkey = MH.str();
    
            //set clientok bool to true
            stringstream CLO, CLO2, call1check, call2check;
            CLO << "UPDATE clients set ClientOK=true, ClientHB=CURRENT_TIMESTAMP WHERE ClientEnabled=true AND Clientkey='" << clientparsedhbkey << "' AND Hostname='" << clientparsedhostname << "'";
            string setclientokbool = CLO.str();
            CLO2 << "update clientinfo INNER JOIN clients on clientinfo.Hostname = clients.Hostname SET  clientinfo.ClientIP=INET_ATON('" << clientconnectedfromip << "') WHERE clients.ClientEnabled=true AND clients.Clientkey='" << clientparsedhbkey << "' AND clients.Hostname='" << clientparsedhostname << "'";
            string updateclientip = CLO2.str();
            stmt->execute(setclientokbool.c_str());
            stmt->execute("COMMIT;");
            stmt->execute(updateclientip.c_str());
            stmt->execute("COMMIT;");
            call1check << "SELECT ClientAction FROM clientassignments WHERE Hostname='" << clientparsedhostname << "' AND ClientWaitingAction=1 AND ClientAction IS NOT NULL";
            string checkforcall1 = call1check.str();
            call2check << "SELECT ClientFile FROM clientassignments WHERE Hostname='" << clientparsedhostname << "' AND ClientWaiting=1 AND ClientFile IS NOT NULL";
            string checkforcall2 = call2check.str();
            //execute query to match host to key, if key matches log heartbeat
            res = stmt->executeQuery(matchhostfromkey.c_str());
            //added if statement for when client key is not found in db
            if(res->next() == false){
                const char *hbderror2 = ":hbderror2*--!";
                spdlog::error("{2} Hostname: {0} using key: {1} failed heartbeat attempt due to host/key combination being unregistered or disabled", clientparsedhostname.c_str(), clientparsedhbkey.c_str(), clientconnectedfromip);
                SSL_write(ssl, hbderror2, strlen(hbderror2));
                delete res;
                delete stmt;
                delete con;
                mysql_library_end();
                sd = SSL_get_fd (ssl);
                SSL_free(ssl);
                delete(hbderror2);
                close (sd);
                exit(1);
            }
            delete res;
            res = stmt->executeQuery(checkforcall2.c_str());
            if(res->next() == false){
                delete res;
                res = stmt->executeQuery(checkforcall1.c_str());
                if(res->next() == false){
                    spdlog::info("{} successful heartbeat", clientparsedhostname.c_str());
                    const char *hbsuccess = ":hbsuccess*--!";
                    SSL_write(ssl, hbsuccess, strlen(hbsuccess));
                }
                else{
                    //processcall1
                    spdlog::info("{} successful heartbeat", clientparsedhostname.c_str());
                    spdlog::debug("{} marked for action", clientparsedhostname.c_str());
                    string qclientaction = string(res->getString("ClientAction"));
                    stringstream ssclientactionreply;
                    ssclientactionreply << ":hbsuccesscall1*" << qclientaction;
                    string hbsuccesscall1 = ssclientactionreply.str();
                    SSL_write(ssl, hbsuccesscall1.c_str(), hbsuccesscall1.size());
                    char buf[1024] = {0};
                    receivedclientcomms = SSL_read(ssl, buf, sizeof(buf));
                    buf[receivedclientcomms] = '\0';
                    string receivedclientstat = buf;
                    size_t q = receivedclientstat.find(":");
                    size_t p = receivedclientstat.find("*");
                    string clientparsedinitstat = receivedclientstat.substr(q + 1, p - q - 1);
                    stringstream ssflipaction;
                    ssflipaction << "UPDATE clientassignments SET ClientWaitingAction=0 WHERE Hostname='" << clientparsedhostname << "'";

                    if (clientparsedinitstat == "AINITSUCCESS"){
                        string sflipaction = ssflipaction.str();
                        stringstream ssasuccess;
                        ssasuccess << "UPDATE clientinitstat SET LatestActionAttempt=1, LatestActionTime=NOW() WHERE Hostname='" << clientparsedhostname << "'";
                        string updateactionsuccess = ssasuccess.str();
                        stmt->execute(updateactionsuccess.c_str());
                        stmt->execute("COMMIT;");
                        stmt->execute(sflipaction.c_str());
                        stmt->execute("COMMIT;");
                        spdlog::info("{} action successfully initiated", clientparsedhostname.c_str());
                    }
                    if (clientparsedinitstat == "AINITFAIL"){
                        string fflipaction = ssflipaction.str();
                        stringstream ssafail;
                        ssafail << "UPDATE clientinitstat SET LatestActionAttempt=0, LatestActionTime=NOW() WHERE Hostname='" << clientparsedhostname << "'";
                        string updateactionfail = ssafail.str();
                        stmt->execute(updateactionfail.c_str());
                        stmt->execute("COMMIT;");
                        stmt->execute(fflipaction.c_str());
                        stmt->execute("COMMIT;");
                        spdlog::info("{} agent action failed to initiate", clientparsedhostname.c_str());
                    }
                }
            }
            else{
                //process call2
                spdlog::info("{} successful heartbeat", clientparsedhostname.c_str());
                spdlog::debug("{} marked for file move", clientparsedhostname.c_str());
                stringstream ssflipfileaction;
                ssflipfileaction << "UPDATE clientassignments SET ClientWaiting=0 WHERE Hostname='" << clientparsedhostname << "'";
                string qclientfilepath = string(res->getString("ClientFile"));
                ifstream canopencheck(qclientfilepath.c_str(), ios::binary);
                //if file can't be opened kill self:
                if(!canopencheck){
                    spdlog::error("Hostname: {0} Unable to open file path for transfer: {1}", clientparsedhostname.c_str(), qclientfilepath.c_str());
                    string fflipfile = ssflipfileaction.str();
                    stringstream ssffail;
                    ssffail << "UPDATE clientinitstat SET LatestFileAttempt=0, LatestFileTime=NOW() WHERE Hostname='" << clientparsedhostname << "'";
                    string updatefilefail = ssffail.str();
                    stmt->execute(updatefilefail.c_str());
                    stmt->execute("COMMIT;");
                    stmt->execute(fflipfile.c_str());
                    stmt->execute("COMMIT;");
                    const char *hbsuccnof = ":hbsuccess*--!";
                    SSL_write(ssl, hbsuccnof, strlen(hbsuccnof));
                    delete res;
                    delete stmt;
                    delete con;
                    mysql_library_end();
                    sd = SSL_get_fd (ssl);
                    SSL_free(ssl);
                    delete(hbsuccnof);
                    close (sd);
                    exit(0);
                }
                //if you want to add custom clientfiles path in conf later
                //add extern string and error checking below
                //string clientfilequeue = "/opt/server/sendqueue/";
                //size_t u = qclientfilepath.find(clientfilequeue.c_str());
                size_t u = qclientfilepath.find(AFILEQUEUE.c_str());
                string qclientfile = qclientfilepath.substr(u + AFILEQUEUE.length());
                stringstream ssclientfilereply;
                size_t totalfilesize = whats_my_size_vol_2(qclientfilepath.c_str()); 
                if (totalfilesize > INT_MAX){
                    spdlog::error("Hostname: {0} File too large for transfer: {1}", clientparsedhostname.c_str(), qclientfilepath.c_str());
                    string fflipfile = ssflipfileaction.str();
                    stringstream ssffail;
                    ssffail << "UPDATE clientinitstat SET LatestFileAttempt=0, LatestFileTime=NOW() WHERE Hostname='" << clientparsedhostname << "'";
                    string updatefilefail = ssffail.str();
                    stmt->execute(updatefilefail.c_str());
                    stmt->execute("COMMIT;");
                    stmt->execute(fflipfile.c_str());
                    stmt->execute("COMMIT;");
                    const char *hbsuccwrsize = ":hbsuccess*--!";
                    SSL_write(ssl, hbsuccwrsize, strlen(hbsuccwrsize));
                    delete res;
                    delete stmt;
                    delete con;
                    mysql_library_end();
                    sd = SSL_get_fd (ssl);
                    SSL_free(ssl);
                    delete(hbsuccwrsize);
                    close (sd);
                    exit(0);
                }
                int movefilesize = static_cast<int>(totalfilesize);
                ssclientfilereply << ":hbsuccesscall2*" << movefilesize << "!" << qclientfile;
                string hbsuccesscall2 = ssclientfilereply.str();
                SSL_write(ssl, hbsuccesscall2.c_str(), hbsuccesscall2.size());
                char buf[1024] = {0};
                receivedclientcomms = SSL_read(ssl, buf, sizeof(buf));
                buf[receivedclientcomms] = '\0';
                string receivedclientstat = buf;
                size_t q = receivedclientstat.find(":");
                size_t p = receivedclientstat.find("*");
                string clientparsedinitstat = receivedclientstat.substr(q + 1, p - q - 1);
                if (clientparsedinitstat == "FINITSUCCESS"){
                    string sflipfile = ssflipfileaction.str();
                    stringstream ssfsuccess;
                    ssfsuccess << "UPDATE clientinitstat SET LatestFileAttempt=1, LatestFileTime=NOW() WHERE Hostname='" << clientparsedhostname << "'";
                    string updatefilesuccess = ssfsuccess.str();
                    stmt->execute(updatefilesuccess.c_str());
                    stmt->execute("COMMIT;");
                    stmt->execute(sflipfile.c_str());
                    stmt->execute("COMMIT;");
                    spdlog::debug("{} file move successfully initiated", clientparsedhostname.c_str());
                    /*---------------------
                        starting file slabbing and move:
                    ---------------------*/
                        float totaltransmission = 0; 
                        size_t slabsize = 4 * 1024;
                        size_t totalslabs = totalfilesize / slabsize;
                        size_t finalslabsize = totalfilesize % slabsize;
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
                            canopencheck.read(&slabbindata[0], currentslabsize);
                            char sendingbuffer[currentslabmovesize];
                            copy(slabbindata.begin(), slabbindata.end(), sendingbuffer);
                            float partialpackets = SSL_write(ssl, sendingbuffer, sizeof(sendingbuffer));
                            if (partialpackets < 0){
                                spdlog::error("Hostname: {} disconnected during file transfer (partialpackets issue)", clientparsedhostname.c_str());
                                delete res;
                                delete stmt;
                                delete con;
                                mysql_library_end();
                                sd = SSL_get_fd (ssl);
                                SSL_free(ssl);
                                close (sd);
                                exit(1);
                            }
                            totaltransmission += partialpackets;
                            //printf("sent file data: %d\n", partialpackets);
                        }
                        if (totaltransmission >= GB)
                            spdlog::info("Hostname: {0} file transfer complete, {1} GB transferred", clientparsedhostname.c_str(), totaltransmission/GB);
                        else if (totaltransmission >=MB && totaltransmission < GB)
                            spdlog::info("Hostname: {0} file transfer complete, {1} MB transferred", clientparsedhostname.c_str(), totaltransmission/MB);
                        else if (totaltransmission >=kB && totaltransmission < MB)
                            spdlog::info("Hostname: {0} file transfer complete, {1} kB transferred", clientparsedhostname.c_str(), totaltransmission/kB);
                        else
                            spdlog::info("Hostname: {0} file transfer complete, {1} bytes transferred", clientparsedhostname.c_str(), totaltransmission);
                        

                        canopencheck.close();
                }
                if (clientparsedinitstat == "FINITFAILURE"){
                    string fflipfile = ssflipfileaction.str();
                    stringstream ssffail;
                    ssffail << "UPDATE clientinitstat SET LatestFileAttempt=0, LatestFileTime=NOW() WHERE Hostname='" << clientparsedhostname << "'";
                    string updatefilefail = ssffail.str();
                    stmt->execute(updatefilefail.c_str());
                    stmt->execute("COMMIT;");
                    stmt->execute(fflipfile.c_str());
                    stmt->execute("COMMIT;");
                    spdlog::debug("{} agent file move failed to initiate", clientparsedhostname.c_str());
                }
            }
        delete res;
        delete stmt;
        delete con;
        }  catch (sql::SQLException &e) {
            spdlog::error("# ERR: {}", e.what());
            spdlog::error("Mysql error code: {}", e.what());
            spdlog::error("SQLState: {}", e.getSQLState());
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
            }
        mysql_library_end();
        sd = SSL_get_fd (ssl);
        SSL_free(ssl);
        close (sd);
        exit(0);
    }


    if (clientparsedaction == "register"){
        if (ENABLEREG == "true" || ENABLEREG == "True" || ENABLEREG == "1"){
        if (clientparsedregkey != REGKEY){
            const char *incregkey = ":regerror1*--!";
            SSL_write(ssl, incregkey, strlen(incregkey));
            spdlog::error("{} registration request denied due to incorrect registration key", clientconnectedfromip);
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
        }

        spdlog::info("{} registration key accepted for client register request", clientconnectedfromip);

        if (mysql_library_init(0, NULL, NULL)) {
            const char *regdbfail = ":regerror2*--!";
            SSL_write(ssl, regdbfail, strlen(regdbfail));
            spdlog::critical("Unable to connect to database for client registration");
            mysql_library_end();
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
        }
        try {
            sql::Driver *driver;
            sql::Connection *con;
            sql::Statement *stmt;
            sql::ResultSet *res;
            driver = get_driver_instance();
            con = driver->connect(DBserver.c_str(), DBuser.c_str(), DBuserPASS.c_str());
            con->setSchema(DB.c_str());
            con-> setAutoCommit(0);
            stmt = con->createStatement();

            //prevent duplicate hostname registrations
            stringstream chkdup;
            chkdup << "SELECT * FROM clients WHERE Hostname='" << clientparsedhostname << "'";
            string checkduplicates = chkdup.str();

            res = stmt->executeQuery(checkduplicates.c_str());
                if(res->next() == false ){
                    //built registration query for host
                    stringstream regaddformat, regaddformat2, regaddformat3, regaddformat4;
                    regaddformat << "INSERT INTO clients(Hostname, Clientkey, ClientOK, ClientEnabled, ClientHB, ClientRegDate) VALUES ('" << clientparsedhostname << "', uuid(), false, false, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)";
                    string regclienttodb = regaddformat.str();
                    regaddformat2 << "INSERT INTO clientinfo(Hostname, ClientIP, OSVER) VALUES ('" << clientparsedhostname << "',(INET_ATON('" << clientconnectedfromip << "')), '" << clientparsedhbkey << "')";
                    string regclientinfotodb = regaddformat2.str();
                    regaddformat3 << "INSERT INTO clientinitstat(Hostname, LatestFileTime, LatestActionTime) VALUES ('" << clientparsedhostname << "', '1999-09-09 01:00:00', '1999-09-09 01:00:00')";
                    string regclientinitstattodb = regaddformat3.str();
                    regaddformat4 << "INSERT INTO clientassignments(Hostname, ClientWaiting, ClientFileDependsT, ClientFileTime, ClientWaitingAction, ClientActionDependsT, ClientActionTime) VALUES ('" << clientparsedhostname <<  "', false, false, '1999-09-09 01:00:00', false, false, '1999-09-09 01:00:00')";
                    string regclientassignmentstodb = regaddformat4.str();

                    stmt->execute("START TRANSACTION;");
                    stmt->execute(regclienttodb.c_str());
                    stmt->execute("COMMIT;");
                    stmt->execute(regclientinfotodb.c_str());
                    stmt->execute("COMMIT;");
                    stmt->execute(regclientinitstattodb.c_str());
                    stmt->execute("COMMIT;");
                    stmt->execute(regclientassignmentstodb.c_str());
                    stmt->execute("COMMIT;");
                    
                    spdlog::info("{} successfully registered", clientparsedhostname.c_str());
                }
                else{
                    const char *duplicatehostname = ":regerror3*--!";
                    SSL_write(ssl, duplicatehostname, strlen(duplicatehostname));
                    spdlog::error("{1} Recieved duplicate registration request for hostname: {0}", clientparsedhostname.c_str(), clientconnectedfromip);
                    mysql_library_end();
                    sd = SSL_get_fd (ssl);
                    SSL_free(ssl);
                    close (sd);
                    exit(1);
                }
    
            delete res;
            res = stmt->executeQuery(checkduplicates.c_str());
                while (res->next()) {
                    string clientuniquekey = string(res->getString("Clientkey"));
                    stringstream clientkeymess;
                    clientkeymess << ":regsuccess*" << clientuniquekey << "!";
                    string clientkeyreply = clientkeymess.str();
                    SSL_write(ssl, clientkeyreply.c_str(), clientkeyreply.size());
                }
            delete res;
            delete stmt;
            delete con;
            }  catch (sql::SQLException &e) {
                spdlog::critical("# ERR: {}", e.what());
                spdlog::critical("Mysql error code: {}", e.what());
                spdlog::critical("SQLState: {}", e.getSQLState());
                mysql_library_end();
                sd = SSL_get_fd (ssl);
                SSL_free(ssl);
                close (sd);
                exit(1);
                }
        mysql_library_end();
        sd = SSL_get_fd (ssl);
        SSL_free(ssl);
        close (sd);
        exit(0);
        }
        else {
            const char *regdisabled = ":regdisabled*--!";
            SSL_write(ssl, regdisabled, strlen(regdisabled));
            spdlog::error("{} registration request denied due to registration request being disabled", clientconnectedfromip);
            mysql_library_end();
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1); 
        }

    }

    if (clientparsedaction == "enable"){
        if (ENABLEREG == "true" || ENABLEREG == "True" || ENABLEREG == "1"){

        if (clientparsedregkey != REGKEY){
            const char *incregkeyforen = ":enerror1*--!";
            SSL_write(ssl, incregkeyforen, strlen(incregkeyforen));
            spdlog::error("{} Client enable request denied due to incorrect registration key", clientconnectedfromip);
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
        }
        spdlog::info("{} registration key accepted for client enable request", clientconnectedfromip);
        if (mysql_library_init(0, NULL, NULL)) {
            const char *endbfail = ":enerror2*--!";
            SSL_write(ssl, endbfail, strlen(endbfail));
            spdlog::critical("{} Unable to connect to database for client's enable request", clientparsedhostname.c_str());
            mysql_library_end();
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1);
        }

        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::ResultSet *res;
        try {
            driver = get_driver_instance();
            con = driver->connect(DBserver.c_str(), DBuser.c_str(), DBuserPASS.c_str());
            con->setSchema(DB.c_str());
            con-> setAutoCommit(0);
            stmt = con->createStatement();
    
            stringstream locateclientindb;
            locateclientindb << "SELECT * FROM clients WHERE Clientkey='" << clientparsedhbkey << "'";
            string locateclient = locateclientindb.str();
    
            res = stmt->executeQuery(locateclient.c_str());
                if(res->next() == false ){
                    const char *endbfail = ":enerror3*--!";
                    SSL_write(ssl, endbfail, strlen(endbfail));
                    spdlog::error("{} Client enable request denied due to client being unregistered", clientparsedhostname.c_str());
                    mysql_library_end();
                    sd = SSL_get_fd (ssl);
                    SSL_free(ssl);
                    close (sd);
                    exit(1);
                }
                else {
                    delete res;
                    stringstream clientenformat;
                    clientenformat << "UPDATE clients set ClientEnabled=true WHERE Clientkey='" << clientparsedhbkey << "'";
                    string enclientremotely = clientenformat.str();

                    stmt->execute("START TRANSACTION;");
                    stmt->execute(enclientremotely.c_str());
                    stmt->execute("COMMIT;");
    
                    const char *ensuccess = ":ensuccess*--!";
                    SSL_write(ssl, ensuccess, strlen(ensuccess));
                    spdlog::info("{} agent remotely enabled", clientparsedhostname.c_str());
                }
            delete stmt;
            delete con;
             }  catch (sql::SQLException &e) {
                   spdlog::error("# ERR: {}", e.what());
                   spdlog::error("Mysql error code: {}", e.what());
                   spdlog::error("SQLState: {}", e.getSQLState());
                   mysql_library_end();
                   sd = SSL_get_fd (ssl);
                   SSL_free(ssl);
                   close (sd);
                   exit(1);
                }
        mysql_library_end();
        sd = SSL_get_fd (ssl);
        SSL_free(ssl);
        close (sd);
        exit(0);
        }
        else{
            const char *endisabled = ":endisabled*--!";
            SSL_write(ssl, endisabled, strlen(endisabled));
            spdlog::error("{} Client enable request denied due to registration being disabled", clientconnectedfromip);
            mysql_library_end();
            sd = SSL_get_fd (ssl);
            SSL_free(ssl);
            close (sd);
            exit(1); 
        }
    }

   

}


 
int main()
{
    
check_conf(); 

ifstream openconf1;    
configfile outp;
grab_config(openconf1,outp);
openconf1.close();


 
//Assign conf values from config file
SPORT = outp.serverport;
DBserver = outp.serverip;
DBuser = outp.sqluser;
DBuserPASS = outp.sqlpass;
DB = outp.database;
REGKEY = outp.agregkey;
PSK = outp.preserverkey;
ENABLEREG = outp.enableregistration;
string debugging = outp.debugg;
SERVLOG = outp.serverlog;
SERVSSL = outp.sslcertfile;
SERVSSLPRIVKEY = outp.sslkeyfile;
AFILEQUEUE = outp.clientfilequeue;
ALOGSWEB = outp.actionwebdirectory;
string enabletls12 = outp.tlssetting;
string getglobalagenttimer = outp.offlinetimer;

if(debugging == "true" || debugging == "True" || debugging == "1"){
    DEBUGVALUE = "1";
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%d/%b/%Y:%H:%M:%S] [thread %t] [PID %P] --%l %v");
    auto my_logger = spdlog::rotating_logger_mt("file_logger", SERVLOG.c_str(), 1048576 * 70, 7);
    spdlog::set_default_logger(my_logger);
    my_logger->flush_on(spdlog::level::debug);
}
else {
    DEBUGVALUE = "0";
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%d/%b/%Y:%H:%M:%S] --%l %v");
    auto my_logger = spdlog::rotating_logger_mt("file_logger", SERVLOG.c_str(), 1048576 * 70, 7);
    spdlog::set_default_logger(my_logger);
    my_logger->flush_on(spdlog::level::info);
}
if(enabletls12 == "true" || enabletls12 == "True" || enabletls12 == "1"){
    SETTLS12 = "1";
}
else {
    SETTLS12 = "0";
}
if(getglobalagenttimer == "10m" || getglobalagenttimer == "10min"){
    GLOBALAGENTTIMER = "10m";
}
else if(getglobalagenttimer == "30m" || getglobalagenttimer == "30min"){
    GLOBALAGENTTIMER = "30m";
}
else {
    GLOBALAGENTTIMER = "5m";
}

spdlog::debug("------------------------------------------");
spdlog::debug("net-controller server daemon started");
    

spdlog::debug("Configuration loaded with the following values:");
spdlog::debug("ServerListeningPort         = {}", SPORT.c_str());
spdlog::debug("ServerListeningAddress      = {}", DBserver.c_str());
spdlog::debug("Database User               = {}", DBuser.c_str());
spdlog::debug("Database Password           = {}", DBuserPASS.c_str());
spdlog::debug("Database                    = {}", DB.c_str());
spdlog::debug("AgentRegistrationKey        = {}", REGKEY.c_str());
spdlog::debug("PreSharedKey                = {}", PSK.c_str());
spdlog::debug("AcceptClientRegistrations   = {}", ENABLEREG.c_str());
spdlog::debug("ServerLog                   = {}", SERVLOG.c_str());
spdlog::debug("AgentFileQueueFolder        = {}", AFILEQUEUE.c_str());
spdlog::debug("WebDirectory                = {}", ALOGSWEB.c_str());
spdlog::debug("AgentsMarkedOfflineAfter    = {}", GLOBALAGENTTIMER.c_str());


int PORT;
PORT = stoi(SPORT);

SSL_CTX *ctx;
init_openssl();
ctx = create_context();
configure_context(ctx);

int mysocket = socket(AF_INET, SOCK_STREAM, 0);
if (mysocket == -1) {
    spdlog::error("Server failed to create initial socket");
    return -1;
}

spdlog::debug("Initial socket creation successful");    
 
sockaddr_in tech;
tech.sin_family = AF_INET;
tech.sin_port = htons(PORT);
tech.sin_addr.s_addr = INADDR_ANY;
bind(mysocket, (sockaddr*)&tech, sizeof(tech));

spdlog::debug("Port {} successfully bound to socket", SPORT.c_str());    

int optval = 1;
socklen_t optlen = sizeof(optval);
setsockopt(mysocket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen); 
    
listen(mysocket, SOMAXCONN);
 
sockaddr_in client;
socklen_t clientSize = sizeof(client);
int clientsocket, pid;

spdlog::debug("Server waiting for connections on port {}", SPORT.c_str());

thread t1(backend_processing);
t1.detach();

while (true) {


    SSL *ssl;
    clientsocket = accept(mysocket, (sockaddr*)&client, &clientSize);
    if (clientsocket < 0) {
        spdlog::error("Client error on socket accept");
        cerr << "Error on accept" << endl;
        return -1;
    }
    //added below to get client ip:
    char *clientconnectedfromip = inet_ntoa(client.sin_addr);
    
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, clientsocket);
    pid = fork ();
    if (pid < 0) {
        cerr << "error forking" << endl;
        spdlog::critical("Server failed to fork client connection");
        return -1;
    }
        
    if (pid == 0) {
        close(mysocket);
        hblisten(ssl, clientconnectedfromip);
        exit (0);
    }
    else {
        //added wait for defunct proccess cleanup
        wait(NULL);
        close(clientsocket);
        SSL_free(ssl);
    }

 
 }
 SSL_CTX_free(ctx);
 return 0;
}
