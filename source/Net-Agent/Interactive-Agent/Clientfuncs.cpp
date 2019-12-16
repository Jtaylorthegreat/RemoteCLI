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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
#include <ctime>

#include "Client.h"

using namespace std;

mutex mtx;

void print_help(){
        printf("\n");
        printf("                RemoteCLI Net-Agent version " VERSION              "\n");
        printf("                  by Justin Taylor 2019                     \n");
        printf("\n");
        printf("client [options]\n");
        printf("\n");
        printf(" --help           -h                     show this help\n");
        printf(" --register       -r                     register agent with monitoring server (must provide server registration key)\n");
        printf(" --enable         -e                     enable agent (must provide server registration key)\n");
        printf(" --heartbeat      -hb                    trigger manual agent heartbeat\n");
        printf(" --version        -v                     show version number\n");
        printf("\n");

}   


void check_conf(){
    string agentconfig = ".agentconfig";
    ifstream fin(agentconfig.c_str());
        if(!fin) {
            cout << "Agent Failure - unable to read agent configuration file \n";
            exit(1);
        }
    fin.close();
    int numoflines = 0;
    string lines, path;
    ifstream countfile;
    path =".agentconfig";
    countfile.open(path);
    while (getline(countfile, lines))
        ++numoflines;
    countfile.close();
    ifstream file(".agentconfig");
    int line_counter = 0;

    int serveraddresscount = 0;
    int serverlisteningportcount = 0;
    int clientkeycount = 0;
    int agentlogcount = 0;
    int presharedkeycount = 0;
    int rsetfilequeuecount = 0;

    string line, serveraddressoutput, serverlisteningportoutput, clientkeyoutput, agentlogoutput, presharedkeyoutput, rsetfilequeueoutput;
    bool serveraddressok;
    bool serverlisteningportok;
    bool clientkeyok;
    bool agentlogok;
    bool presharedkeyok;
    bool rsetfilequeueok;

    while (getline (file,line)) {
        line_counter++;
        line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
            if(line[0] == '#' || line.empty())
                continue;
            if (line.find("serveraddress") != string::npos){
                serveraddresscount++;
            }
            if (line.find("serverlisteningport") != string::npos){
                serverlisteningportcount++;
            }
            if (line.find("clientkey") != string::npos){
                clientkeycount++;
            }
            if (line.find("agentlog") != string::npos){
                agentlogcount++;
            }
            if (line.find("presharedkey") != string::npos){
                presharedkeycount++;
            }
            if (line.find("receivedfilequeue") != string::npos){
                rsetfilequeuecount++;
            }
    }
    file.close();
    if (serveraddresscount < 1){
        serveraddressoutput = "serveraddress not found in conf file"; 
        serveraddressok = 0;
    }
    if (serveraddresscount > 1){
        serveraddressoutput = "Too many instances of serveraddress found in conf file";
        serveraddressok = 0;
     }
    if (serveraddresscount == 1){
        serveraddressoutput = "serveraddress ok";
        serveraddressok = 1;
    }
    //--------------------------------------------------------------------------------
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
    //--------------------------------------------------------------------------------
    if (clientkeycount < 1){
       clientkeyoutput = "clientkey not found in conf file"; 
       clientkeyok = 0;
    }
    if (clientkeycount > 1){
        clientkeyoutput = "Too many instances of clientkey found in conf file";
        clientkeyok = 0;
    }
    if (clientkeycount == 1){
       clientkeyoutput = "clientkey ok";
        clientkeyok = 1;
    }
    //--------------------------------------------------------------------------------
    if (agentlogcount < 1){
       agentlogoutput = "agentlog not found in conf file"; 
       agentlogok = 0;
    }
    if (agentlogcount > 1){
        agentlogoutput = "Too many instances of agentlog found in conf file";
        agentlogok = 0;
    }
    if (agentlogcount == 1){
       agentlogoutput = "agentlog ok";
        agentlogok = 1;
    }
    //--------------------------------------------------------------------------------
    if (presharedkeycount < 1){
       presharedkeyoutput = "presharedkey not found in conf file"; 
       presharedkeyok = 0;
    }
    if (presharedkeycount > 1){
        presharedkeyoutput = "Too many instances of presharedkey found in conf file";
        presharedkeyok = 0;
    }
    if (presharedkeycount == 1){
       presharedkeyoutput = "presharedkey ok";
        presharedkeyok = 1;
    }
    //--------------------------------------------------------------------------------
    if (rsetfilequeuecount < 1){
       rsetfilequeueoutput = "receivedfilequeue not found in conf file"; 
       rsetfilequeueok = 0;
    }
    if (rsetfilequeuecount > 1){
        rsetfilequeueoutput = "Too many instances of receivedfilequeue found in conf file";
        rsetfilequeueok = 0;
    }
    if (rsetfilequeuecount == 1){
       rsetfilequeueoutput = "receivedfilequeue ok";
        rsetfilequeueok = 1;
    }
    //--------------------------------------------------------------------------------
    if (serveraddressok !=1 || serverlisteningportok !=1 || clientkeyok !=1 || agentlogok !=1 || presharedkeyok !=1 || rsetfilequeueok !=1){
       cout << "Configuration file error: \n";
       if(serveraddressok == 0){
           cout << serveraddressoutput << "\n";
       }
       if(serverlisteningportok == 0){
           cout << serverlisteningportoutput << "\n";
       }
       if(clientkeyok == 0){
           cout << clientkeyoutput << "\n";
       }
       if(agentlogok == 0){
           cout << agentlogoutput << "\n";
       }
       if(presharedkeyok == 0){
           cout << presharedkeyoutput << "\n";
       }
       if(rsetfilequeueok == 0){
           cout << rsetfilequeueoutput << "\n";
       }
       exit(1);
   }
}



void grab_config(ifstream& in, configfile& out){
    in.open(".agentconfig");
    string line;
    while(!in.eof()){
        while(getline(in,line)){
            line.erase(remove_if(line.begin(), line.end(), ::isspace),
                                    line.end());
                if(line[0] == '#' || line.empty())
                    continue;
            auto delimiterPos = line.find("=");
            string name;
            string debugg;
            name = line.substr(0, delimiterPos);
            if(name == "serverlisteningport")
                out.serverport = line.substr(delimiterPos +1);
            if(name == "serveraddress")
                out.serverip = line.substr(delimiterPos +1);
            if(name == "clientkey")
                out.clientkey = line.substr(delimiterPos +1);
            if(name == "debuglogging")
                out.debugg = line.substr(delimiterPos +1);
            if(name == "agentlog")
                out.agentlog = line.substr(delimiterPos +1);
            if(name == "presharedkey")
                out.presharedkey = line.substr(delimiterPos +1);     
            if(name == "receivedfilequeue")
                out.recievethosefiles = line.substr(delimiterPos +1); 
        }
    }
}


void delete_line(const char *file_name, int n) 
{ 
    ifstream is(file_name); 
    ofstream ofs; 
    ofs.open("temp.txt", ofstream::out); 
    char c; 
    int line_no = 1; 
    while (is.get(c)) 
    { 
        if (c == '\n') 
        line_no++; 
        if (line_no != n) 
            ofs << c; 
    } 
    ofs.close(); 
    is.close(); 
    remove(file_name); 
    rename("temp.txt", file_name); 
} 

void outputrotate(){
    remove(".outputs/complete.output.3");
    rename(".outputs/complete.output.2", ".outputs/complete.output.3");
    rename(".outputs/complete.output.1", ".outputs/complete.output.2");
    rename(".outputs/complete.output", ".outputs/complete.output.1");
}

string pipe_command_output_to_file(string cmd) {
    string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];
    //cmd.append(" 2>&1");

    stream = popen(cmd.c_str(), "r");
    if (stream) {
        while (!feof(stream))
        if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
    }
    return data;
}

void command_processing(string passed_cmd){

    outputrotate();
    mtx.lock();
    string userhome = "cd ~; ";
    stringstream sslaunchcmd;
    sslaunchcmd << userhome << passed_cmd;
    string launchcmd = sslaunchcmd.str();

    string stringoldata = pipe_command_output_to_file(launchcmd.c_str());
    ofstream tmpoutputlog;
    tmpoutputlog.open(".outputs/tmp.output", ios::out | ios::app);
    tmpoutputlog << stringoldata;
    time_t now = time(0); 
    char* clienttime = ctime(&now);
    tmpoutputlog << "=======================================\n";
    tmpoutputlog << "Command: " << passed_cmd << endl;
    tmpoutputlog << "Complete: " << clienttime << endl;
    tmpoutputlog << "=======================================\n";
    tmpoutputlog.close();
    rename(".outputs/tmp.output", ".outputs/complete.output");
    mtx.unlock();
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
