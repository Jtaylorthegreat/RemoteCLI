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
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

using namespace std;

struct configfile {
    string serverip;
    string serverport;
    string clientkey;
    string debugg;
    string agentlog;
    string presharedkey;
    string recievethosefiles;
    string verifymycontroller;
    string upqueue;
    string clienttimer;
  };

extern string IP, PORT, KEY, AGENTLOG, PSK, DEBUGVALUE, RFILEQUEUE, CONTRVERIFY, AUPLOADQUEUE, ACHECKINTIME;
extern string * const CONSTUPLOADQUEUE;

const float kB = 1024;
const float MB = 1048576;
const float GB = 1073741824;

void check_conf();
void grab_config(ifstream& in, configfile& out);
void outputrotate();
string pipe_command_output_to_file(string cmd);
void command_processing(string passed_cmd);
size_t whats_my_size_vol_2(string filepass);
typedef vector<string> stringvec;
void read_directory(const string& dirname, stringvec& vec);
