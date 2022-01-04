#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

using namespace std;

string logfile;
string t1ip;
string ctrackerip;

string t2ip;
int tcountval=0;
unordered_map<string, string> piece_wise_hash;
uint16_t t1port;
unordered_map<string, string> cname_to_port;
uint16_t t2port;
unordered_map<string, bool> isalreadyloggedin;
uint16_t ctrackerport;
unordered_map<string, unordered_map<string, set<string>>> seederList;
vector<string> grouplis;
unordered_map<string, string> fsize;
int tval=0;
unordered_map<string, string> grp_admin_lis;



unordered_map<string, string> loggedinpeople;

unordered_map<string, set<string>> all_grp_pending_req;


unordered_map<string, set<string>> allmember_of_grp;

void synch(int csock)
{
    char dummy[5];
    //cout<<"yo"<<endl;
    read(csock, dummy, 5);
    //cout<<"gg"<<endl;
}

void writelog(const string &data)
{
    ofstream log_file(logfile, ios_base::out | ios_base::app );
    log_file << data << endl;
}


void logcreation(char* argv[])
{
    string tnumberstr=string(argv[5]);
    ofstream lfile;
    logfile="tracker"+tnumberstr+"logfile.txt";
    lfile.open(logfile);
    lfile.clear();
    lfile.close();
}

vector<string> getTrackerInfo(char* path){
    fstream trackerInfoFile;
    trackerInfoFile.open(path, ios::in);

    string t;

    vector<string> res;

    

    if(!trackerInfoFile.is_open())
    {
        cout << "Tracker Info file not found.\n";
        exit(-1);      
    }
   
    while(getline(trackerInfoFile, t))
    {
        res.push_back(t);
    }
    trackerInfoFile.close();
    return res;
}

vector<string>string_split(string add,string delimeter)
{
    size_t cposition = 0;
    vector<string>ans;
    while ((cposition = add.find(delimeter)) != string::npos) {
        string t = add.substr(0, cposition);
        ans.push_back(t);
        int l=cposition + delimeter.length();
        add.erase(0, l);
    }
    ans.push_back(add);
    return ans;
}

void *inputchck(void *arg)
{
    while (true)
    {
        string ip;
        getline(cin, ip);
        if (ip == "quit")
        {
            exit(0);
        }
    }
}

vector<string> stringtovect(string s)
{
    vector<string>vect;
    stringstream ss(s);
    while(ss>>s)
    {
        vect.push_back(s);
    }
    return vect;
}
void clientconv(int csock)
{
    writelog("Conversation with the client has started");
    string clientUid = "";
    string clientGid = "";
    while(true)
    {
        char input_line[1024]={0};
        int readstatus=read(csock, input_line, 1024);
        string input_linestr=string(input_line);
        if (readstatus <= 0)
        {
            isalreadyloggedin[clientUid] = false;
            close(csock);
            break;
        }
        else
        {
            writelog("client request:" + input_linestr);
        }
        vector<string>input_vect=stringtovect(input_linestr);
        int argcount=input_vect.size();
        if(input_vect[0]=="create_user")
        {
            if(argcount==3)
            {
                if(loggedinpeople.find(input_vect[1])!=loggedinpeople.end())
                {
                    write(csock,"User has already created account",32);
                }
                else
                {
                    loggedinpeople.insert({input_vect[1],input_vect[2]});
                    write(csock,"Account Created",15);
                }
            }
            else
            {
                write(csock, "Enter valid Command",19);
            }
            
        }
        if(input_vect[0]=="login")
        {
           // cout<<argcount<<" ";
            if(argcount==3)
            {
                int flg =0;
                if(loggedinpeople.find(input_vect[1])== loggedinpeople.end() && flg==0)
                {
                    write(csock, "Enter Correct UserName/Password",31);
                    flg=1;

                }
                if(loggedinpeople[input_vect[1]] != input_vect[2] && flg==0)
                {
                    write(csock, "Enter Correct UserName/Password",31);
                    flg=1;
                }
                if(isalreadyloggedin[input_vect[1]]==true && flg==0)
                {
                    write(csock, "You are already loggedin",24);
                    flg=1;
                }
                if(flg==0)
                {
                    isalreadyloggedin.insert({input_vect[1],true});
                    clientUid = input_vect[1];
                    char buffer[96];
                    //cout<<"Hello"<<endl;
                    write(csock, "Login Successful", 16);
                    read(csock, buffer, 96);
                    string peerAddress = string(buffer);
                    // cout<<"Address stroring while login"<<peerAddress<<endl;
                    cname_to_port[clientUid] = peerAddress;
                    flg=1;
                }


            }
            else
            {
                write(csock, "Enter correct arguments", 23);

            }
            
        }
        if(input_vect[0]=="logout")
        {
            isalreadyloggedin[clientUid]=false;
            write(csock, "Logout Successful", 17);
            writelog("logout sucess\n");
            
        }
        if(input_vect[0]=="create_group")
        {
            if(argcount==2)
            {
                int flg=0;
                for(int i=0;i<grouplis.size();i++)
                {
                    if(grouplis[i]==input_vect[1])
                    {
                        write(csock, "Group is already present", 24);
                        flg=1;
                    }
                }
                if(flg==0)
                {
                    grouplis.push_back(input_vect[1]);
                    grp_admin_lis.insert({input_vect[1], clientUid});
                    allmember_of_grp[input_vect[1]].insert(clientUid);
                    clientGid = input_vect[1];
                    write(csock, "Group created", 13);
                }
            }
            else
            {
                write(csock, "Enter valid Command",19);
            }
            
        }
        if(input_vect[0]=="list_groups")
        {
            //cout<<"Grps"<<endl;
            if(argcount==1)
            {
                write(csock, "All groups:", 11);
                synch(csock);


                // char dummy[5];
                // //cout<<"yo"<<endl;
                // read(csock, dummy, 5);
                // //cout<<"gg"<<endl;
               
                if(grouplis.size()==0)
                {
                    write(csock, "No groups found##", 18);
                }
                else
                {
                    
                    string reply = "";
                    for (size_t i = 0; i < grouplis.size(); i++)
                    {
                        reply =reply + grouplis[i] + "##";
                    }
                    write(csock, &reply[0], reply.length());

                }
 
            }
            else
            {
                write(csock, "Enter valid Command",19);
            }
        }
        if(input_vect[0]=="join_group")
        {
            if(argcount==2)
            {
                if(allmember_of_grp[input_vect[1]].find(clientUid) != allmember_of_grp[input_vect[1]].end())
                {

                    if(grp_admin_lis.find(input_vect[1]) != grp_admin_lis.end())
                    {
                        write(csock, "You are already in this group", 30);
                    }
                    if(grp_admin_lis.find(input_vect[1]) == grp_admin_lis.end())
                    {
                        write(csock, "Invalid group ID.", 19);
                    }

                }
                else
                {
                    all_grp_pending_req[input_vect[1]].insert(clientUid);
                    write(csock, "Group request sent", 18);
                    tval++;
                }     
            }   
            else
            {
                write(csock, "Enter valid Command",19);
            }
        }
        if(input_vect[0]=="leave_group")
        {
            if(argcount==2)
            {
                int flg=0;
                write(csock, "Leaving group...", 17);
                if(grp_admin_lis.find(input_vect[1]) == grp_admin_lis.end() && flg==0)
                {
                    write(csock, "Can't Leave it is an invalid group ID.", 38);
                    flg=1;
                }
                if(allmember_of_grp[input_vect[1]].find(clientUid) == allmember_of_grp[input_vect[1]].end() && flg==0)
                {
                    write(csock, "You are not the member of the group", 35);
                    flg=1;
                }
                if(flg==0)
                {
                    // char dummy[5];
                    // read(csock, dummy, 5);
                    if(grp_admin_lis[input_vect[1]] == clientUid){
                        write(csock, "You are the admin of this group, you cant leave!", 48);
                        flg=1;
                    }
                    if(grp_admin_lis[input_vect[1]] != clientUid){
                        allmember_of_grp[input_vect[1]].erase(clientUid);
                        write(csock, "Group left succesfully", 23);
                        flg=1;
                    }
                    // write(csock, "Admin of the group cannot leave", 31);
                    // flg=1;
                }
            }
            else
            {
                write(csock, "Enter valid Command",19);
            }
        }
        if(input_vect[0]=="list_files")
        {
            if(argcount==2)
            {
                write(csock, "All files are:", 15);
                char dummy[5];
                read(csock, dummy, 5);
               
                if(grp_admin_lis.find(input_vect[1])!=grp_admin_lis.end())
                {
                    if(seederList[input_vect[1]].size()!=0)
                    {
                        string op="";
                        for(auto it:seederList[input_vect[1]])
                        {
                            op=op+it.first+"$$";
                        }
                        int l=op.length();
                        op=op.substr(0,l-2);
                        l=op.length();
                        write(csock, &op[0], l);
                    }
                    else
                    {
                        write(csock, "No files found.", 15);
                    }
                }
                else
                {
                    write(csock, "Enter Valid Group Id",20);
                }
            }
            else
            {
                write(csock, "Enter valid Command",19);
            }
        }
        if(input_vect[0]=="list_requests")
        {
            if(argcount==2)
            {
                write(csock, "Fetching group requests...", 27);
                char dummy[5];
                read(csock, dummy, 5);
                string response="";
                // write(csock, "Group Requests:",15);
                
                
                if (grp_admin_lis.find(input_vect[1]) != grp_admin_lis.end()  ) //  May be Admin
                {
                    if(grp_admin_lis[input_vect[1]] == clientUid) // Is Admin
                    {
                        if(all_grp_pending_req[input_vect[1]].size()!=0)
                        {
                            auto i=all_grp_pending_req[input_vect[1]].begin();
                            while(i != all_grp_pending_req[input_vect[1]].end())
                            {
                                response =response + string(*i) + "$$";
                                i++;
                            }
                            
                            write(csock, &response[0], response.length());
                        }
                        else
                        {
                            write(csock, "Noreq", 5);
                        }
                    }
                    else // Not Admin
                    {
                        write(csock, "Notadmin", 8);
                    }
                   
                }
                else // Not Admin
                {
                    write(csock, "Notadmin", 8);
                }
            }
            else
            {
                write(csock, "Enter valid Command", 22);
            }

        }
        if(input_vect[0]=="accept_request")  // Showing some problem
        {
            if(argcount==3)
            {
                write(csock, "Accepting request...", 21);
                char dummy[5];
                read(csock, dummy, 5);
                
                if(grp_admin_lis.find(input_vect[1])!=grp_admin_lis.end())
                {
                    int flgg=0;
                    if(grp_admin_lis.find(input_vect[1])->second == clientUid)
                    {
                       
                        all_grp_pending_req[input_vect[1]].erase(input_vect[2]); 
                        flgg=1;
                        allmember_of_grp[input_vect[1]].insert(input_vect[2]);
                        
                        // cout<<"1"<<endl;
                        write(csock, "Request accepted.", 18);
                        
                    }
                    else
                    {
                        write(csock, "You are not the admin of this group", 35);
                    }
                }
                else
                {
                    write(csock, "Invalid group ID.", 19);
                }
            }
            else
            {
                write(csock, "Enter valid Command", 22);
            }
        }
        if(input_vect[0]=="stop_share")
        {
            if(argcount==3)
            {
                if(grp_admin_lis.find(input_vect[1])!=grp_admin_lis.end())
                {
                    if(seederList[input_vect[1]].find(input_vect[2]) != seederList[input_vect[1]].end())
                    {
                        seederList[input_vect[1]][input_vect[2]].erase(clientUid);
                        tcountval++;
                        if(seederList[input_vect[1]][input_vect[2]].size() != 0)
                        {
                            write(csock, "Stopped sharing the file", 25);
                        }
                        else
                        {
                            seederList[input_vect[1]].erase(input_vect[2]);
                            write(csock, "Stopped sharing the file", 25);
                        }
                        
                    }
                    else
                    {
                         write(csock, "File not yet shared in the group", 32);
                    }
                }
                else
                {
                    write(csock, "Invalid group ID.", 19);
                }
            }
            else
            {
                write(csock, "Enter valid Command",19);
            }
            
        }
        if(input_vect[0]=="show_downloads") // To see as well
        {
            write(csock, "Loading...", 10);
        }
        if(input_vect[0]=="upload_file")
        {
            if(argcount==3)
            {
                if (allmember_of_grp.find(input_vect[2]) != allmember_of_grp.end())
                {
                    if (allmember_of_grp[input_vect[2]].find(clientUid) != allmember_of_grp[input_vect[2]].end())
                    {
                        struct stat buffer;
                        const string &s=input_vect[1];
                        if((stat(s.c_str(), &buffer) == 0))
                        {
                            char fdetails[524288] = {0};
                            write(csock, "Uploading...", 12);
                            
                            if (read(csock, fdetails, 524288))
                            {
                                string fdetailsstr=string(fdetails) ;
                                if (fdetailsstr != "error")
                                {
                                    string hshval_of_pieces = "";
                                    vector<string> fdet = string_split(fdetailsstr, "$$");
                                    //fdet = [filepath, peer address, file size, file hash, piecewise hash]
                                    vector<string>tvect=string_split(string(fdet[0]), "/");
                                    string filename = tvect.back();

                                    size_t i = 4;
                                    while(i < fdet.size())
                                    {
                                        hshval_of_pieces =hshval_of_pieces + fdet[i];
                                        if(i==fdet.size() - 1)
                                        {
                                            i++;
                                            continue;
                                        }
                                        else
                                        {
                                            hshval_of_pieces =hshval_of_pieces + "$$";
                                            i++;
                                        }

                                    }
                                    piece_wise_hash[filename] = hshval_of_pieces;

                                    if (seederList[input_vect[2]].find(filename) == seederList[input_vect[2]].end())
                                    {
                                        tcountval++;
                                        //cout<<"Equal "<<clientUid<<endl;
                                        seederList[input_vect[2]].insert({filename, {clientUid}});
                                        fsize[filename] = fdet[2];
                                        
                                    }
                                    else
                                    {
                                        //cout<<"NOT Equal "<<clientUid<<endl;

                                        seederList[input_vect[2]][filename].insert(clientUid);
                                        fsize[filename] = fdet[2];
                                    }
                                    
                                    write(csock, "Uploaded", 8);
                                    
                                }
                                
                            }

                        }
                        else
                        {
                            write(csock, "DirectFileNotFound", 18);
                        }
                    }
                    else
                    {
                        write(csock, "NotAMember", 10);
                    }
                }
                else
                {
                    write(csock, "GroupNotFound", 13);
                }
            }
            else
            {
                write(csock, "Enter valid Command",19);
            }
        }
        if(input_vect[0]=="download_file")
        {
            if(argcount==4)
            {
                if (allmember_of_grp.find(input_vect[1]) != allmember_of_grp.end())
                {
                    if(allmember_of_grp[input_vect[1]].find(clientUid) != allmember_of_grp[input_vect[1]].end())
                    {
                        const string &s=input_vect[3];
                        struct stat buffer;
                        if (stat(s.c_str(), &buffer) == 0)
                        {
                            char fileDetails[524288] = {0};
                            // fileDetails = [filename, destination, group id]
                            write(csock, "Downloading...", 13);
                            if (read(csock, fileDetails, 524288))
                            {
                                string fdetailsstr=string(fileDetails);
                                string reply = "";
                                vector<string> fdet = string_split(string(fileDetails), "$$");
                                if (seederList[input_vect[1]].find(fdet[0]) == seederList[input_vect[1]].end())
                                {
                                    tcountval++;
                                    write(csock, "File not found", 14);
                                }
                                else
                                {
                                    //cout<<"Inside else"<<endl;
                                    for (auto i : seederList[input_vect[1]][fdet[0]])
                                    {
                                        //cout<<"fjsgrs"<<endl;
                                        // if(!isalreadyloggedin[i])
                                        // {
                                        //     continue;
                                        // }
                                        //cout<<"Value of i accessing for reply "<<i<<endl;
                                        reply += cname_to_port[i] + "$$";
                                        
                                    }
                                    //cout<<"Reply Sending while downloading"<<reply<<endl;
                                    reply += fsize[fdet[0]];
                                    // writeLog("seeder list: " + reply);
                                    write(csock, &reply[0], reply.length());
                                    synch(csock);
                                    // char dum[5];
                                    // read(csock, dum, 5);

                                    write(csock, &piece_wise_hash[fdet[0]][0], piece_wise_hash[fdet[0]].length());
                                    
                                    tval++;
                                    seederList[input_vect[1]][input_vect[2]].insert(clientUid);
                                }
                                tval++;
                            }
                        }
                        else
                        {
                            write(csock, "DirectNotPresent", 16);
                        }
                    }
                    else
                    {
                        tcountval++;
                        write(csock, "notGroupMember", 14);
                    }
                }
                else
                {
                    write(csock, "noSuchGroup", 11);
                }
            }
            else
            {
                write(csock, "Enter valid Command", 22);
            }
        }

    }
    string csockstr=to_string(csock);
    writelog("pthread ended successfully for socket number " +csockstr);
    close(csock);
}



int main(int argc,char* argv[])
{
    if(argc!=3)
    {
        cout<<"Pass the correct number of arguments"<<endl;
        return -1;
    }
    
    logcreation(argv);
    
    string arg2str=string(argv[2]);

    vector<string> trackeraddress = getTrackerInfo(argv[1]);

    if(arg2str=="1")
    {
        t1ip=trackeraddress[0];
        t1port=stoi(trackeraddress[1]);
        ctrackerip=t1ip;
        ctrackerport=t1port;
        
    }
    else if(string(argv[2])=="2")
    {
        t2ip=trackeraddress[2];
        t2port=stoi(trackeraddress[3]);
        ctrackerip=t2ip;
        ctrackerport=t2port;
    }
    else
    {
        cout<<"More than 2 trackers are not supported"<<endl;
        return -1;
    }
    
    writelog("Tracker 1 Address : " + string(t1ip) + ":" + to_string(t1port));
    writelog("Tracker 2 Address : " + string(t2ip) + ":" + to_string(t2port));
    writelog("Log file name : " + logfile + "\n");

    int tsock=socket(AF_INET, SOCK_STREAM, 0);
    
    
    pthread_t exitDetectionThreadId;

    if(tsock==0)
    {
        cout<<"Socket Failed"<<endl;
        exit(1);
    }
    else
    {
        writelog("Socket of tracker is created.");
    }
    int opt = 1;
    if (setsockopt(tsock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(1);
    }
    else
    {
        writelog("Set sock opt is done atleast");
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(ctrackerport);

    int inetstatus=inet_pton(AF_INET, &ctrackerip[0], &address.sin_addr);
    if(inetstatus<=0)
    {
        cout<<"Adress is wrong INET error"<<endl;
        return -1;
    }
    else
    {
        writelog("inet_pton is done");
    }
    int bindstatus=bind(tsock, (struct sockaddr *)&address, sizeof(address));
    if(bindstatus<0)
    {
        cout<<"Binding Failed"<<endl;
        return -1;
    }
    else
    {
        writelog("Binding Completed");
    }
    int lstatus=listen(tsock, 3);
    if(lstatus<0)
    {
        cout<<"Listening failed"<<endl;
        return -1;
    }
    else
    {
        writelog("Listening");
    }
    vector<thread> threadsvect;

    if (pthread_create(&exitDetectionThreadId, NULL, inputchck, NULL) == -1)
    {
        perror("pthread");
        exit(EXIT_FAILURE);
    }
    else
    {
        writelog("P thread working successfully");

    }
    int addresslen = sizeof(address);
    while(true)
    {
        int csock=accept(tsock, (struct sockaddr *)&address, (socklen_t *)&addresslen);
        if(csock==0)
        {
            cout<<"Connection not accepted"<<endl;
           
        }
        else
        {
            writelog("Connection accepted");

        }
        threadsvect.push_back(thread(clientconv, csock));
    }
    auto it=threadsvect.begin();
    while(it!=threadsvect.end())
    {
        if(!(it->joinable()))
        {
            it++;
            continue;
        }   
        it->join();
        it++;
    }
    writelog("EXITING.");
    return 0;
}