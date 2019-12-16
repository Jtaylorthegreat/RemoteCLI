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
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <mutex>    
#include <thread>
#include <sys/wait.h>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/x509v3.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "Clientd.h"


using namespace std;

string IP, PORT, KEY, AGENTLOG, PSK, DEBUGVALUE, RFILEQUEUE, CONTRVERIFY, AUPLOADQUEUE, ACHECKINTIME;


SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
        if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

}

void init_openssl()
{ 
    SSL_load_error_strings();  
    ERR_load_crypto_strings(); 
    OpenSSL_add_all_algorithms();
    SSL_library_init();
}

inline bool check_complete_files(const string& compfile) {
    struct stat compfilebuff;
    return (stat (compfile.c_str(), &compfilebuff) == 0);
}


void file_processing (SSL* ssl, string filetocreate, string filesize){
    size_t filebytes = stoi(filesize);
    stringstream ssputfilepath;
    //if you want to add custom path in conf later
    //add extern string and error checking below
    //ssputfilepath << "/opt/client/ReceivedFiles/" << filetocreate;
    ssputfilepath << RFILEQUEUE << filetocreate;
    string putfilepath = ssputfilepath.str();
    if (check_complete_files(putfilepath.c_str()) ==1){
        spdlog::info("File: {1} already exists locally at path: {0}", putfilepath.c_str(), filetocreate.c_str());
        spdlog::info("Replacing file {} with new transferred file", putfilepath.c_str());
        remove(putfilepath.c_str());
    }
    float totaltransmission = 0;
    ofstream mk1file;
    mk1file.open(putfilepath.c_str(), ios::out | ios::binary);
        if(!mk1file){
            spdlog::error("Unable to create file location: {}", putfilepath.c_str());
            const char *clientfilefailure = ":FINITFAILURE*";
            SSL_write (ssl, clientfilefailure, strlen(clientfilefailure));
            exit(0);
        }
    else {
        const char *clientfilesuccess = ":FINITSUCCESS*";
        SSL_write (ssl, clientfilesuccess, strlen(clientfilesuccess));
        
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
            float recfile = SSL_read(ssl, &slabbindata[0], currentslabsize);
            if (recfile < 0){
                spdlog::error("connection error during file transfer: {}", putfilepath.c_str());
                exit(0);
            }
            mk1file.write(&slabbindata[0], currentslabsize);
            totaltransmission += recfile;
            //for troubleshooting:
            //printf("wrote file data: %lf\n", totaltransmission);
        }
    //for troubleshooting:    
    //printf("total data written: %d\n", totaltransmission);
    if (totaltransmission >= GB)
        spdlog::info("{0} file transfer complete, {1} GB transferred", putfilepath.c_str(), totaltransmission/GB);
    else if (totaltransmission >=MB && totaltransmission < GB)
        spdlog::info("{0} file transfer complete, {1} MB transferred", putfilepath.c_str(), totaltransmission/MB);
    else if (totaltransmission >=kB && totaltransmission < MB)
        spdlog::info("{0} file transfer complete, {1} kB transferred", putfilepath.c_str(), totaltransmission/kB);
    else
        spdlog::info("{0} file transfer complete, {1} bytes transferred", putfilepath.c_str(), totaltransmission);

    mk1file.close();
    exit(0);
    }
}



int main()
{
    
    check_conf();
    ifstream openconf1;    
    configfile outp;
    grab_config(openconf1, outp);
    openconf1.close();



    IP = outp.serverip;
    PORT = outp.serverport;
    KEY = outp.clientkey;
    AGENTLOG = outp.agentlog;
    PSK = outp.presharedkey;
    RFILEQUEUE = outp.recievethosefiles;
    AUPLOADQUEUE = outp.upqueue;
    const string CONSTUPLOADQUEUE = AUPLOADQUEUE;
    string debugging = outp.debugg;
    string verifyingmyc = outp.verifymycontroller;
    string getclienttimer = outp.clienttimer;
    


    if(debugging == "true" || debugging == "True" || debugging == "1"){
        DEBUGVALUE = "1";
        spdlog::set_level(spdlog::level::debug);
        spdlog::set_pattern("[%d/%b/%Y:%H:%M:%S] [thread %t] [PID %P] --%l %v");
        auto my_logger = spdlog::rotating_logger_mt("file_logger", AGENTLOG.c_str(), 1048576 * 70, 7);
        spdlog::set_default_logger(my_logger);
        my_logger->flush_on(spdlog::level::debug);
    }
    else {
        DEBUGVALUE = "0";
        spdlog::set_level(spdlog::level::info);
        spdlog::set_pattern("[%d/%b/%Y:%H:%M:%S] --%l %v");
        auto my_logger = spdlog::rotating_logger_mt("file_logger", AGENTLOG.c_str(), 1048576 * 70, 7);
        spdlog::set_default_logger(my_logger);
        my_logger->flush_on(spdlog::level::info);
    }

    if(verifyingmyc == "true" || verifyingmyc == "True" || verifyingmyc == "1"){
        CONTRVERIFY = "1";
    }
    else {
        CONTRVERIFY = "0";
    }
    if(getclienttimer == "5m" || getclienttimer == "5min"){
        ACHECKINTIME = "5m";
    }
    else if(getclienttimer == "15m" || getclienttimer == "15min"){
        ACHECKINTIME = "15m";
    }
    else {
        ACHECKINTIME = "30s";
    }

    spdlog::debug("------------------------------------------");
    spdlog::debug("net-agent daemon started");

    spdlog::debug("Configuration loaded with the following values:");
    spdlog::debug("MasterServerIP           = {}", IP.c_str());
    spdlog::debug("MasterServerPort         = {}", PORT.c_str());
    spdlog::debug("Agent key loaded         = {}", KEY.c_str());
    spdlog::debug("Agent log                = {}", AGENTLOG.c_str());
    spdlog::debug("PreSharedKey             = {}", PSK.c_str());
    spdlog::debug("Receivefilequeue         = {}", RFILEQUEUE.c_str());
    spdlog::debug("VerifyServerCertificate  = {}", CONTRVERIFY.c_str());
    spdlog::debug("AgentUploadQueue         = {}", AUPLOADQUEUE.c_str());
    spdlog::debug("AgentChecking            = {}", ACHECKINTIME.c_str());



    char hostname[1024];
    gethostname(hostname, 1024);

    init_openssl();
    SSL_CTX *ctx;
    ctx = create_context();
    configure_context(ctx);

    while (true) { 
        SSL *ssl;
        int csocket = socket(AF_INET, SOCK_STREAM, 0);
        if (csocket == -1) {
            spdlog::critical(" Agent unable to obtain socket");
            return -1;
        }
        int servPORT = stoi(PORT);
        sockaddr_in tech;
        tech.sin_family = AF_INET;
        tech.sin_port = htons(servPORT);
        inet_pton(AF_INET, IP.c_str(), &tech.sin_addr);
        
        int conreq = connect(csocket, (sockaddr*)&tech, sizeof(tech));
        if (conreq < 0 ){
            spdlog::error("Unable to connect to server: {0} on port {1}", IP.c_str(), PORT.c_str());
        }
        else {
            X509 *givencert;
            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, csocket);
            if (CONTRVERIFY == "1"){
                SSL_set_hostflags(ssl, X509_CHECK_FLAG_MULTI_LABEL_WILDCARDS);
                SSL_set_verify(ssl, SSL_VERIFY_PEER, NULL);
                if (!SSL_set1_host(ssl, IP.c_str())) {
                    /* handle and log error*/
                    spdlog::debug("Error setting server's hostname in verify ssl function");
                }
            }
            else {
                //this may be redundant since by default it doesn't verify the ssl now:
                SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);
            }
            if (SSL_connect (ssl) <= 0){
                if (DEBUGVALUE == "1"){
                    FILE *sslerrs;
                    spdlog::error("Unsuccessful TLS/SSL handshake with server {}", IP.c_str());
                    spdlog::debug("[Openssl error]");
                    spdlog::debug("---------------------------------------------------------");
                    sslerrs=fopen(AGENTLOG.c_str(), "a+");
                    ERR_print_errors_fp (sslerrs);
                    fclose(sslerrs);
                    spdlog::debug("---------------------------------------------------------");
                }
                else {
                    spdlog::error("Unsuccessful TLS/SSL handshake with server {}", IP.c_str());
                }
            }
            else {
                //X509 *givencert = NULL;
                givencert = SSL_get_peer_certificate(ssl);
                if (CONTRVERIFY == "1"){
                    if (givencert == NULL){
                        spdlog::error("Controller Failed Certificate Verification - No Certificate presented");
                        SSL_free (ssl);
                        close(csocket);
                        X509_free(givencert);
                    }
                    else {
                        if(SSL_get_verify_result(ssl)!=X509_V_OK){
                            spdlog::error("Controller Failed Certificate Verification");
                            SSL_free (ssl);
                            close(csocket);
                            X509_free(givencert);
                        }
                        else {
                            spdlog::debug("Controller Certificate Verified");
                        }
                    }

                }
                
                //Verification of server PSK:
                char pskbuf[1024] = {0};
                int recservpskcomms = SSL_read(ssl, pskbuf, sizeof(pskbuf));
                pskbuf[recservpskcomms] = '\0';
                string rawservpskcomms = pskbuf;
                size_t header = rawservpskcomms.find(":");
                size_t footer = rawservpskcomms.find("*");
                string serverparsedpsk = rawservpskcomms.substr(header + 1, footer - header - 1);
                if (serverparsedpsk != PSK){
                    spdlog::error("Server {} failed preshared key check, connection dropped", IP.c_str());
                    spdlog::debug("Server {0} using psk: {1} failed preshared key check, connection dropped", IP.c_str(), serverparsedpsk.c_str());
                    SSL_free (ssl);
                    close(csocket);
                    X509_free(givencert);
                }
                else {
                    //normal heartbeat call:
                    stringstream prefacekey;
                    prefacekey << ":--*" << hostname << "!heartbeat^" << KEY << "+";
                    const string sendkey = prefacekey.str();
                    const char *sendmessage = sendkey.c_str();
                    
                    vector<string> vect;
                    int numoffilesinupload = 0;
                    read_directory(AUPLOADQUEUE.c_str(), vect);
                    numoffilesinupload = vect.size();
                    if (numoffilesinupload > 0 ){
                        sort(vect.begin(), vect.end());
                        string custupfile = vect[0];
                    
                        stringstream ssuploadfile;
                        ssuploadfile << AUPLOADQUEUE << custupfile;
                        string fullfileuploadpath = ssuploadfile.str();
                        size_t completeuploadsize = whats_my_size_vol_2(fullfileuploadpath.c_str()); 
                        if(completeuploadsize > INT_MAX){
                            spdlog::error("Custom upload file: {} too large for transfer to server", fullfileuploadpath.c_str());
                            goto runaftercustomuploadcheck;
                        }
                        else {
                            //process custom file upload here:
                            ifstream canopencheck(fullfileuploadpath.c_str(), ios::binary);
                            int movecompleteuploadsize = static_cast<int>(completeuploadsize);
                            stringstream prefacekeyu;
                            prefacekeyu << ":" << movecompleteuploadsize << "*" << hostname << "!heartbeatu^" << KEY << "+" << custupfile;
                            const string sendkeyu = prefacekeyu.str();
                            const char *sendmessageu = sendkeyu.c_str();
                            SSL_write (ssl, sendmessageu, strlen(sendmessageu));
                            char buf[1024] = {0};
                            //start processing custom file upload
                            //-------------------------
                            int receivedservercomms;
                            receivedservercomms = SSL_read(ssl, buf, sizeof(buf));
                            buf[receivedservercomms] = '\0';
                            string receivedserverstat = buf;
                            size_t q = receivedserverstat.find(":");
                            size_t p = receivedserverstat.find("*");
                            string serverparsedinitstat = receivedserverstat.substr(q + 1, p - q - 1);
                            if (serverparsedinitstat == "CUSTINITSUCCESS"){
                                spdlog::info("Server initialized transfer of custom file upload");
                                /*---------------------
                                starting file slabbing and move:
                                ---------------------*/
                                float totaltransmission = 0; 
                                size_t slabsize = 4 * 1024;
                                size_t totalslabs = completeuploadsize / slabsize;
                                size_t finalslabsize = completeuploadsize % slabsize;
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
                                        spdlog::error("Connection to server failed during custom file upload");
                                        SSL_free (ssl);
                                        close(csocket);
                                        X509_free(givencert);
                                    }
                                    totaltransmission += partialpackets;
                                }
                                if (totaltransmission >= GB)
                                    spdlog::info("Custom file {1} successfully sent to server, {0} GB transferred", totaltransmission/GB, fullfileuploadpath.c_str());
                                else if (totaltransmission >=MB && totaltransmission < GB)
                                    spdlog::info("Custom file {1} successfully sent to server, {0} MB transferred", totaltransmission/MB, fullfileuploadpath.c_str());
                                else if (totaltransmission >=kB && totaltransmission < MB)
                                    spdlog::info("Custom file {1} successfully sent to server, {0} kB transferred", totaltransmission/kB, fullfileuploadpath.c_str());
                                else
                                    spdlog::info("Custom file {1} successfully sent to server, {0} bytes transferred", totaltransmission, fullfileuploadpath.c_str());
                                canopencheck.close();
                                remove(fullfileuploadpath.c_str());
                                SSL_free (ssl);
                                close(csocket);
                                X509_free(givencert);
                            }
                            if (serverparsedinitstat == "CUSTINITFAILURE"){
                                spdlog::error("Server failed to initialize upload of custom file");
                                SSL_free (ssl);
                                close(csocket);
                                X509_free(givencert);
                            }
                        }
                    }
                    else {
                        runaftercustomuploadcheck:
                        if(check_complete_files(".outputs/complete.output") ==1){
                            size_t completeoutputsize = whats_my_size_vol_2(".outputs/complete.output"); 
                            if(completeoutputsize > INT_MAX){
                                spdlog::error("command output log: .outputs/complete.output too large for transfer to server");
                                goto runNormalHeartBeat;
                            }
                            else{
                                //setupcommandoutput sizecall:
                                ifstream canopencheck(".outputs/complete.output", ios::binary);
                                int movecompleteoutputsize = static_cast<int>(completeoutputsize);
                                stringstream prefacekeyc;
                                prefacekeyc << ":" << movecompleteoutputsize << "*" << hostname << "!heartbeatc^" << KEY << "+";
                                const string sendkeyc = prefacekeyc.str();
                                const char *sendmessagec = sendkeyc.c_str();
                                SSL_write (ssl, sendmessagec, strlen(sendmessagec));
                                char buf[1024] = {0};
                                //process output log send
    			                //-------------------------
    			                int receivedservercomms;
                                receivedservercomms = SSL_read(ssl, buf, sizeof(buf));
                                buf[receivedservercomms] = '\0';
                                string receivedserverstat = buf;
                                size_t q = receivedserverstat.find(":");
                                size_t p = receivedserverstat.find("*");
                                string serverparsedinitstat = receivedserverstat.substr(q + 1, p - q - 1);
                                if (serverparsedinitstat == "COMPINITSUCCESS"){
                                    spdlog::info("Server initialized transfer of command output log");
                                    /*---------------------
                                    starting file slabbing and move:
                                    ---------------------*/
                                    float totaltransmission = 0; 
                                    size_t slabsize = 4 * 1024;
                                    size_t totalslabs = completeoutputsize / slabsize;
                                    size_t finalslabsize = completeoutputsize % slabsize;
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
                                            spdlog::error("Connection to server failed during command outputlog file transfer");
                                            SSL_free (ssl);
                                            close(csocket);
                                            X509_free(givencert);
                                        }
                                        totaltransmission += partialpackets;
                                    }
                                    if (totaltransmission >= GB)
                                        spdlog::info("Command outputlog successfully sent to server, {} GB transferred", totaltransmission/GB);
                                    else if (totaltransmission >=MB && totaltransmission < GB)
                                        spdlog::info("Command outputlog successfully sent to server, {} MB transferred", totaltransmission/MB);
                                    else if (totaltransmission >=kB && totaltransmission < MB)
                                        spdlog::info("Command outputlog successfully sent to server, {} kB transferred", totaltransmission/kB);
                                    else
                                        spdlog::info("Command outputlog successfully sent to server, {} bytes transferred", totaltransmission);
                                    canopencheck.close();
                                    remove(".outputs/complete.output");
                                    SSL_free (ssl);
                                    close(csocket);
                                    X509_free(givencert);
                                }
                                if (serverparsedinitstat == "COMPINITFAILURE"){
                                    spdlog::error("Server failed to initialize transfer of command output log");
                                    SSL_free (ssl);
                                    close(csocket);
                                    X509_free(givencert);
                                }
                            }
                        }
                        else {
                            runNormalHeartBeat:
                            SSL_write (ssl, sendmessage, strlen(sendmessage));
                            char buf[1024] = {0};
                            int receivedservercomms, pid;
                            receivedservercomms = SSL_read(ssl, buf, sizeof(buf));
                            buf[receivedservercomms] = '\0';
                            string serverunparsedresponse = buf;
                            size_t q = serverunparsedresponse.find(":");
                            size_t p = serverunparsedresponse.find("*");
                            size_t r = serverunparsedresponse.find("!");
                            string serverparsedcode = serverunparsedresponse.substr(q + 1, p - q - 1);
                            string serverparsedfilesize = serverunparsedresponse.substr(p + 1, r - p - 1);
                            string serverparsedf = serverunparsedresponse.substr(r + 1);
                            string serverparsedact = serverunparsedresponse.substr(p + 1);
                            if (serverparsedcode == "hbdberror1"){
                                spdlog::error("#301 - Server unable to connect to database for client's heartbeat");
                                SSL_free (ssl);
                                close(csocket);
                                X509_free(givencert);
                            }
                            if (serverparsedcode == "hbderror2"){
                                spdlog::error("#302 - Server unable to process heartbeat request due to client hostname/key combination not being registered or enabled in database");
                                SSL_free (ssl);
                                close(csocket);
                                X509_free(givencert);
                            }
                            if (serverparsedcode == "hbsuccess"){
                                spdlog::info("Client heartbeat request successful");
                                SSL_free (ssl);
                                close(csocket);
                                X509_free(givencert);
                            }
                            if (serverparsedcode == "hbsuccesscall1"){
                                spdlog::info(" Server requests action: {}", serverparsedact.c_str());
                                const char *clientactionsuccess = ":AINITSUCCESS*";
                                SSL_write (ssl, clientactionsuccess, strlen(clientactionsuccess));
                                SSL_free (ssl);
                                close(csocket);
                                X509_free(givencert);
                                thread t1(command_processing, serverparsedact);
                                t1.join();
                            }
                            if (serverparsedcode == "hbsuccesscall2"){
                                size_t logparsedfilesize = stoi(serverparsedfilesize);
                                if (logparsedfilesize >= GB)
                                    spdlog::info("Server requests to transfer {1} GB for file: {0}", serverparsedf.c_str(), logparsedfilesize/GB);
                                else if (logparsedfilesize >=MB && logparsedfilesize < GB)
                                    spdlog::info("Server requests to transfer {1} MB for file: {0}", serverparsedf.c_str(), logparsedfilesize/MB);
                                else if (logparsedfilesize >=kB && logparsedfilesize < MB)
                                    spdlog::info("Server requests to transfer {1} kB for file: {0}", serverparsedf.c_str(), logparsedfilesize/kB);
                                else
                                    spdlog::info("Server requests to transfer {1} bytes for file: {0}", serverparsedf.c_str(), logparsedfilesize);
                                pid = fork ();
                                if (pid < 0) {
                                    const char *clientfilefailure = ":FINITFAILURE*";
                                    SSL_write (ssl, clientfilefailure, strlen(clientfilefailure));
                                    SSL_free (ssl);
                                    close(csocket);
                                    X509_free(givencert);
                                }
                                if (pid == 0){
                                    file_processing(ssl, serverparsedf, serverparsedfilesize);
                                    exit(0);
                                }
                                else{
                                    wait(NULL);
                                    SSL_free (ssl);
                                    close(csocket);
                                    X509_free(givencert);
                                }
                            }
                        } 
                    }     
                }    
            }
        }
        if (ACHECKINTIME == "5m"){
            sleep(300);
        }
        else if (ACHECKINTIME == "15m"){
            sleep(900);
        }
        else {
            sleep(30);
        }
    }
    SSL_CTX_free (ctx);
    spdlog::info("agent shutdown gracefully");
    return 0;
}
