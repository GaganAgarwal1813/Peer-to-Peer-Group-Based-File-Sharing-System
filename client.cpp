#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

using namespace std;

bool isloggedin;

class client_file_detail{
    public:
    long long int fsize;
    string clientipaddr;
    
    string clientfname;


};


class reqdChunkDetails{
    public:
    string clientchunip;
    string filename;
    long long int chunkNum; 
    string dest;

};



string logfile;
string t1ip;
string t2ip;
string clientip;
uint16_t t1port, t2port , clientport;
unordered_map<string, string> file_to_file_path;
unordered_map<string, vector<int>> fchunkinfomation;
unordered_map<string,unordered_map<string,bool>>upload_check;
int tval=0;
unordered_map<string, string> download_file_lis;
bool isfilecorrupt;
vector<string> cfile_piece_wise_hash;
vector<vector<string>> cdownloaded_file_chunk;


bool errorcheck(int sock,string inptline)
{
    return send(sock, &inptline[0], strlen(&inptline[0]), MSG_NOSIGNAL) == -1;
}



void synch(int sock)
{
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);
}


vector<string> string_to_vect(string inputline)
{
    string s;
    stringstream ss(inputline);
    vector<string> inpt;
    while(ss>>s)
    {
        inpt.push_back(s);  // Converting the input stringline to the vector of strings
    } 
    return inpt;
}

void addtodownload_file_lis(string i1,string i2)
{
    download_file_lis.insert({i1, i2});
}


vector<string> getTrackerInfo(char* path)
{
    vector<string> ans;
    // cout<<path<<endl;
    fstream tinfofile;
    tinfofile.open(path, ios::in);
    if(!tinfofile.is_open())
    {
        cout << "Tracker Info file not found.\n";
        exit(-1);

    }
    else
    {
        string line;
        while(getline(tinfofile, line))
        {
            ans.push_back(line);
        }
        tinfofile.close();
        return ans;
    }
    return ans;
}



vector<string>string_split(string add,string delimeter)
{
    size_t cposition = 0;
    vector<string>ans;
    while ((cposition = add.find(delimeter)) != string::npos) 
    {
        string t = add.substr(0, cposition);
        ans.push_back(t);
        int l=cposition + delimeter.length();
        add.erase(0, l);
    }
    ans.push_back(add);
    return ans;
}


void hash_str_conv(string& hashval,string segval)
{
   unsigned char md[20];
    if(SHA1(reinterpret_cast<const unsigned char *>(&segval[0]), segval.length(), md)){

        for(int i=0; i<20; i++){
            char buffer[3];
            string bufstr;
            sprintf(buffer, "%02x", md[i]&0xff);
            bufstr=string(buffer);
            hashval += bufstr;
        }

        hashval += "$$";   
    }
    else{

        printf("Error in hashing\n");
        hashval += "$$";
    }
    
    
}




int chunkwriterfunction(int csock, long long int chunkNum, char* fpath)
{
    char buffer[524288];
    string cont="";
    int n;
    string hashval = "";
    for(int total=0;total<524288;total=total+n)
    {
        n = read(csock, buffer, 524287);
        if (n <= 0)
        {
            break;
        }
        else
        {
            buffer[n] = 0;
            fstream outfile(fpath, std::fstream::in | std::fstream::out | std::fstream::binary);
            tval++;
            outfile.seekp(chunkNum*524288+total, ios::beg);
            outfile.write(buffer, n);
            outfile.close();
            string writtenlocst=to_string(total+chunkNum*524288 );
            string writtenloced=to_string(total+ chunkNum*524288 + n-1);
         
           
            cont =cont + buffer;
            
            bzero(buffer, 524288);
        }
    }
    
    hash_str_conv(cont, hashval);
   
    //hashval.pop_back();
   
    // if(hashval != cfile_piece_wise_hash[chunkNum])
    // {
    //     isfilecorrupt = true;
    // } 
    string fpathstr=string(fpath);
    vector<string>fpathvect=string_split(fpathstr, "/");
    string filename =fpathvect.back();
    string chnknumstr=to_string(chunkNum);
   
    fchunkinfomation[filename][chunkNum] = 1;
  


    return 0;
}



string client_as_server_connect(char* cserverip, char* cserverport_ip, string cmd)
{
    struct sockaddr_in caddress; 
    int csock = socket(AF_INET, SOCK_STREAM, 0);
    if(csock<0)
    {
        cout<<"Error in creating Socket"<<endl;
        return "error";
    }
   
    caddress.sin_family = AF_INET; 
    uint16_t cport = stoi(string(cserverport_ip));
    caddress.sin_port = htons(cport);
    if(inet_pton(AF_INET, cserverip, &caddress.sin_addr) < 0){ 
        perror("Client Connection Error(INET)");
    } 
    
    vector<string>cmdvect=string_split(cmd, "$$");
    string ccomand=cmdvect.front();
    if (connect(csock, (struct sockaddr *)&caddress, sizeof(caddress)) < 0) { 
        perror("Client Connection Error");
    } 
   
    if(ccomand=="get_chunk_vector")
    {
        char sreply[10240] = {0};
        int sstatus=send(csock , &cmd[0] , strlen(&cmd[0]) , MSG_NOSIGNAL );
        if(sstatus==-1)
        {
            cout<<"Error has occured while sending command"<<strerror(errno)<<endl;
            
            return "error";
        }
       
        int rstatus=read(csock, sreply, 10240);
        if(rstatus< 0){
            perror("err: ");
            return "error";
        }
        else
        {
            close(csock);
            return string(sreply);
        }
    }
    if(ccomand == "get_chunk")
    {
        int sstatus=send(csock , &cmd[0] , strlen(&cmd[0]) , MSG_NOSIGNAL );
        if(sstatus == -1)
        {
            printf("Error: %s\n",strerror(errno));
            return "error"; 
        }
        else
        {
            vector<string> cmdtokens = string_split(cmd, "$$");
            
            string despath = cmdtokens[3];
            long long int chunkNum = stoll(cmdtokens[2]);
            string cserverport_ipstr=string(cserverport_ip);
            string chunkNumstr=to_string(chunkNum);

            chunkwriterfunction(csock, chunkNum, &despath[0]);

            return "ss";
        }
    }
    if(ccomand == "get_file_path")
    {
        char sreply[10240] = {0};
        int sstatus=send(csock , &cmd[0] , strlen(&cmd[0]) , MSG_NOSIGNAL );
        if(sstatus == -1){
            printf("Error: %s\n",strerror(errno));
            return "error"; 
        }
        
        if(read(csock, sreply, 10240) < 0)
        {
            perror("err: ");
            return "error";
        }
        else
        {
            file_to_file_path[string_split(cmd, "$$").back()] = string(sreply);
        }
    }

    close(csock);
    string cportstr=to_string(cport);
    string cserveripstr=string(cserverip);
    return "aa";
}


long long int getfsize(char* fpath)
{
    long long int sz=-1;
    FILE *fpointer=fopen(fpath,"rb");
    if(!fpointer) 
    {
        printf("Enter valid path File not found.\n");
        return -1;
    }
    fseek (fpointer, 0, SEEK_END);
    sz = ftell(fpointer)+1;
    fclose(fpointer);
    return sz;
}

void getChunkInfo(client_file_detail* pf){

    vector<string> serverPeerAddress = string_split(string(pf->clientipaddr), ":");
    string pfnamestr=string(pf->clientfname);
    string command = "get_chunk_vector$$" + pfnamestr;
    string response = client_as_server_connect(&serverPeerAddress[0][0], &serverPeerAddress[1][0], command);
    int csize=cdownloaded_file_chunk.size();
    for(size_t i=0; i<csize; i++){
        if(response[i] == '1'){
            cdownloaded_file_chunk[i].push_back(string(pf->clientipaddr));
        }
    }

    delete pf;
}

string getfilename(string fpath)
{
    vector<string> ans=string_split(fpath,"/");
    string ansval=ans.back();
    return ansval;
}

void chunk_send_fun(char* fpath,int chunknumber,int csock)
{
    string sent = "";
    std::ifstream fdval(fpath, std::ios::in|std::ios::binary);
    tval++;
    fdval.seekg(chunknumber*524288, fdval.beg);

    
    char buffer[524288] = {0}; 
    fdval.read(buffer, sizeof(buffer));
    int count = fdval.gcount();
    int rc = send(csock, buffer, count, 0);
   
    if (rc == -1) {
        perror("[-]Error in sending file.");
        exit(1);
    }
    

    fdval.close();
    
}

void clientreqhandler(int csock){
    string clientid = "";
     char ipline[1024]={0};
    if(read(csock , ipline, 1024) <=0){
        close(csock);
        return;
    }
    
    vector<string> inputvect = string_split(string(ipline), "$$");
   
    if(inputvect[0]=="get_chunk_vector")
    {
        string temp="";
        vector<int> chunkvec = fchunkinfomation[inputvect[1]];
        for(int i=0;i<chunkvec.size();i++)
        {
            temp += to_string(chunkvec[i]);
        }
        char* reply=&temp[0];
        write(csock, reply, strlen(reply));
        close(csock);
        return;
    }
    else if(inputvect[0] == "get_file_path")
    {
        string fpath = file_to_file_path[inputvect[1]];
        write(csock, &fpath[0], strlen(fpath.c_str()));
        close(csock);
        return;
    }
    else if(inputvect[0]=="get_chunk")
    {
        long long int chunknumber=stoll(inputvect[2]);
        string fpath=file_to_file_path[inputvect[1]];
        

        chunk_send_fun(&fpath[0], chunknumber, csock); 
        close(csock);
        return;
    }
    
    close(csock);
    return;
}

void* client_as_server(void* arg)
{
   
   
    struct sockaddr_in sock_address;  // socket address
    int server_sock;  // server socket created
    int addrlen = sizeof(sock_address); 
    
     
   
    if((server_sock=socket(AF_INET, SOCK_STREAM, 0))==0)
    {
        perror("Socket Creation failed\n");
        exit(1);
    }
    int opt = 1; 
     if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
     { 
        perror("setsockopt"); 
        exit(1); 
    } 
  
    sock_address.sin_family = AF_INET; 
    sock_address.sin_port=htons(clientport);
    
    int ipton;
    if(ipton=inet_pton(AF_INET, &clientip[0], &sock_address.sin_addr)<=0)
    {
        cout<<"Address is invalid"<<endl;
        return NULL;
    }

    int bindstatus =bind(server_sock, (struct sockaddr *)&sock_address,  sizeof(sock_address));

    if(bindstatus<0)
    {
        perror("bind failed");
        exit(1);
    }
   

    int listenstatus =listen(server_sock,3);
    
    if(listenstatus<0)
    {
         
        perror("listen");
        exit(EXIT_FAILURE);
    }
  
    
    vector<thread> threadsvect;
    while(true)
    {
        int csock=accept(server_sock, (struct sockaddr *)&sock_address, (socklen_t *)&(sock_address));
        if(csock<0)
        {
            perror("Acceptance error");
          
        }
       

        threadsvect.push_back(thread(clientreqhandler, csock));
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

   
    close(server_sock);
    

}




int is_alreadyUploaded(int sock,string fname,vector<string>input)
{
    if(upload_check[input[2]].find(fname)!=upload_check[input[2]].end())
    {
        cout<<"File is Already uploaded previously"<<endl;
        int sstatus=send(sock , "error" , 5 , MSG_NOSIGNAL );
        if(sstatus != -1)
        {
            return 0;
        }
        else
        {
            printf("Error: %s\n",strerror(errno));
            return -1;
        }
        
    }
    return 1;
}




string hashfun(char* fpath)
{
    FILE* fpointer;
   // int i;
    long long int fsize=getfsize(fpath);
    if(fsize==-1)
        return "$";
    string hashval="";
    char line[32769];
    int segments = fsize/ 524288 +1; // 512 KB into BYTES
    fpointer=fopen(fpath,"r");
    if(!fpointer)
    {
        hashval=hashval+"$$";
        printf("Enter valid file name File not found.\n");
        hashval.pop_back();
        hashval.pop_back();
        return hashval;
    }
    for(int i=0;i<=segments-1;i++)
    {
        string segval;
        int accum=0;
        int rc;
        while(accum < 524288 && (rc = fread(line, 1, min(32767, 524288-accum), fpointer)))
        {
            line[rc] = '\0';
            segval =segval+ line;
            accum =accum+ strlen(line);  
            memset(line, 0, sizeof(line));
        }
        hash_str_conv(hashval,segval);
    }
    fclose(fpointer);
    hashval.pop_back();
    hashval.pop_back();
    return hashval;

}



void getChunk(reqdChunkDetails* reqdChunk){

   

    string filename = reqdChunk->filename;
    vector<string> serverPeerIP = string_split(reqdChunk->clientchunip, ":");
    long long int chunkNum = reqdChunk->chunkNum;
    string destination = reqdChunk->dest;

    string command = "get_chunk$$" + filename;
    string cnumstr=to_string(chunkNum);
    command=command + "$$" + cnumstr;
    command=command + "$$" + destination;
    client_as_server_connect(&serverPeerIP[0][0], &serverPeerIP[1][0], command);
    
    delete reqdChunk;
    return;
}

string filehashhelper(char* path)
{
    string hashval;
    unsigned char md[32];
    ifstream input(path);
    ostringstream buffer; 
    buffer << input.rdbuf(); 
    string data =  buffer.str();
    if(SHA256(reinterpret_cast<const unsigned char *>(&data[0]), data.length(), md))
    {
        for(int i=0; i<32; i++){
            char buffer[3];
            string buffstr;
            sprintf(buffer, "%02x", md[i]&0xff);
            buffstr=string(buffer);
            hashval += buffstr;
        }
        return hashval;
    }
    cout<<"Error in Hashing"<<endl;
    return hashval;
}

string fsizehelper(char* fpath)
{
    long long int sz=-1;
    FILE *fpointer=fopen(fpath,"rb");
    if(!fpointer) 
    {
        printf("Enter valid path File not found.\n");
        return to_string(-1);
    }
    fseek (fpointer, 0, SEEK_END);
    sz = ftell(fpointer)+1;
    fclose(fpointer);
    return to_string(sz);
}

void appendip_port(string&fileDetails,string fpathstr,string filehash,string piece_wise_hash,string fsize)
{
    fileDetails += fpathstr + "$$" + string(clientip) + ":" + to_string(clientport) + "$$";
    fileDetails += fsize + "$$"+filehash + "$$"+piece_wise_hash;
    
}

void piece_wise_algo(vector<string>clients , vector<string> inpt)
{
    long long filesize = stoll(clients.back());
    cdownloaded_file_chunk.clear();
    clients.pop_back();
    long long segments = filesize/524288+1;
    
    cdownloaded_file_chunk.resize(segments);


    vector<thread> thread1; 
    vector<thread> thread2;
    int n=clients.size();
    size_t i=0;

    while(i<n) {
        client_file_detail* clientpointer = new client_file_detail();
        clientpointer->fsize = segments;
        clientpointer->clientfname = inpt[2];
        clientpointer->clientipaddr = clients[i];
        thread1.push_back(thread(getChunkInfo, clientpointer));
        i++;
    }

    auto it=thread1.begin();
    while(it!=thread1.end())
    {
        if(!(it->joinable()))
        {
            it++;
            continue;
        }   
        it->join();
        it++;
    }
  
    int n1=cdownloaded_file_chunk.size();
    for(size_t i=0; i<n1; i++){
        int sz=cdownloaded_file_chunk[i].size();
        if( sz== 0){
            cout << "All parts of the file are not available." << endl;
            return;
        }
    }
    thread1.clear();
    srand((unsigned) time(0));
    long long int segmentsReceived = 0;
    


    string despath1val=inpt[3] + "/" ;
    string finaldestpath = despath1val+ inpt[2];
    FILE* fp = fopen(&finaldestpath[0], "r+");
	if(fp == 0){
        string ss(filesize, '\0');
        fstream in(&finaldestpath[0],ios::out|ios::binary);
        tval++;
        in.write(ss.c_str(),strlen(ss.c_str()));  
        in.close();
        tval--;
        fchunkinfomation[inpt[2]].resize(segments,0);
        

        vector<int> tmp(segments, 0);
        isfilecorrupt = false;
        fchunkinfomation[inpt[2]] = tmp;
        
        string clientfpath;

        while(segmentsReceived < segments)
        {

            
            long long int randompiece;
            while(true){
                randompiece = rand()%segments;
                if(fchunkinfomation[inpt[2]][randompiece] != 0)
                { 
                   continue;
                }
                else
                {
                    break;
                }
            }
            long long int peersWithThisPiece = cdownloaded_file_chunk[randompiece].size();
            string randompeer = cdownloaded_file_chunk[randompiece][rand()%peersWithThisPiece];
            tval++;

            reqdChunkDetails* req = new reqdChunkDetails();
            
            req->clientchunip = randompeer;
            string dest1=inpt[3] + "/";
            req->dest = dest1 + inpt[2];
            req->chunkNum = randompiece;
            string reqchunknumstr=to_string(req->chunkNum);
            req->filename = inpt[2];

            
            fchunkinfomation[inpt[2]][randompiece] = 1;

            thread2.push_back(thread(getChunk, req));
            segmentsReceived++;
            clientfpath = randompeer;
        }    

        auto it=thread2.begin();
        while(it!=thread2.end())
        {
            if(!(it->joinable()))
            {
                it++;
                continue;
            }   
            it->join();
            it++;
        }

        addtodownload_file_lis(inpt[2],inpt[1]);
        
        if(isfilecorrupt==false){

            cout << "Download completed. No corruption detected." << endl;    
        }
        if(isfilecorrupt==true){
            cout << "Downloaded completed. The file may be corrupted." << endl;
        }
        
       
        string comd_to_pass;
        vector<string> serverAddress = string_split(clientfpath, ":");
        comd_to_pass="get_file_path$$" ;
        comd_to_pass=comd_to_pass+ inpt[2];
        client_as_server_connect(&serverAddress[0][0], &serverAddress[1][0], comd_to_pass);
        return;



	}
    else
    {
		printf("The file already exists.\n") ;
        fclose(fp);
        return;
    }
    


}

int processcommand(int sock,vector<string> inputvect)
{
    char sreply[10240];
    bzero(sreply,1024);
    read(sock,sreply,10240);
    cout<<sreply<<endl;
    string sreply1=string(sreply);

    if(sreply1=="Enter valid Command")
        return 0;
    if(inputvect[0]=="login")
    {
        if(sreply1!="Login Successful")
        {
            return 0;
        }
        else
        {
            string cportval=to_string(clientport);
            string caddress=clientip+":"+cportval;
            isloggedin=true;
            int l=caddress.length();
            write(sock,&caddress[0],l);
            return 0;
        }
    }
    if(inputvect[0]=="logout")
    {
        isloggedin=false;
        return 0;
    }
    if(inputvect[0]=="list_groups")
    {
        write(sock,"test",5);
        char response[98304];
        memset(response,0,98304); // Making response 0
        read(sock,response,98304); // Reading response from tracker
        string resstr=string(response);
        vector<string>groups=string_split(resstr,"##");
        for(int i=0;i<groups.size()-1;i++)
        {
            cout<<groups[i]<<endl;
        }
        return 0;
    }
    if(inputvect[0]=="leave_group")
    {
        
        char buf[96];
        read(sock, buf, 96);
        cout << buf << endl;
     
        return 0;
    }
    if(inputvect[0]=="list_requests")
    {
        
        synch(sock);
        char response[98304];
        memset(response,0,98304); // Making response 0
        read(sock,response,98304); // Reading response from tracker
        string resstr=string(response);
        if(resstr=="Notadmin")
        {
            cout<<"You are not admin of this group"<<endl;
            return 0;
        }
        if(resstr=="Noreq")
        {
            cout<<"No Requests are pending "<<endl;
            return 0;
        }
        vector<string> reqlist = string_split(resstr, "$$");
        for(int i=0; i<reqlist.size()-1; i++){
            cout << reqlist[i] << endl;
        }
        return 0;
        
    }
    if(inputvect[0]=="list_files")
    {
       
        synch(sock);

        char buf[1024];
        bzero(buf, 1024);
        read(sock, buf, 1024);
        string bufstr=string(buf);
        vector<string> listOfFiles = string_split(bufstr, "$$");
        tval++;

        for(auto i: listOfFiles)
            cout << i << endl;
        return 0;
    }
    if(inputvect[0]=="download_file")
    {
        if(sreply1=="noSuchGroup")
        {
            cout << "Group doesn't exist" << endl;
            return 0;
        }
        if(sreply1=="notGroupMember")
        {
            cout << "You are not a member of this group" << endl;
            return 0;
        }
        if(sreply1=="DirectNotPresent")
        {
            cout << "Directory not found" << endl;
            return 0;
        }
        string fdetail = "";
        if(inputvect.size()==4)
        {
                
            fdetail = fdetail + inputvect[2] + "$$";
            fdetail = fdetail + inputvect[3] + "$$";
            char sreply[524288] = {0}; 
            fdetail = fdetail + inputvect[1];
            // fileDetails = [filename, destination, group id]

            int sockstatval1=send(sock , &fdetail[0] , strlen(&fdetail[0]) , MSG_NOSIGNAL );
            if(sockstatval1== -1)
            {
                printf("Error: %s\n",strerror(errno));
                return -1;
            }
                
            read(sock , sreply, 524288); 

            if(string(sreply) != "File not found"){

                
                vector<string> peersWithFile = string_split(sreply, "$$");
                
                
               
                synch(sock);

                bzero(sreply, 524288);
                read(sock , sreply, 524288);
                
                tval=tval+1;      

                vector<string> tmp = string_split(string(sreply), "$$");
                 
                tval=tval-1;
                
                cfile_piece_wise_hash = tmp;

                piece_wise_algo(peersWithFile,inputvect);
                return 0;

            }
            else
            {
                cout << sreply << endl;
                return 0;
            }
            return 0;
        }
        return 0;
        
    }
    if(inputvect[0]=="accept_request")
    {
      
        synch(sock);

        char buf[96];
        read(sock, buf, 96);
        cout << buf << endl;
    }
    if(inputvect[0]=="upload_file")
    {
        if(sreply1=="NotAMember")
        {
            cout<<"You are not the member of the group"<<endl;
            return 0;
        }
        if(sreply1=="GroupNotFound")
        {
            cout<<"Group is not created yet"<<endl;
            return 0;
        }
        if(sreply1=="DirectFileNotFound")
        {
            cout<<"File is not found"<<endl;
            return 0;
        }
        if(inputvect.size()==3)
        {
            
            char* fpath = &inputvect[1][0];
            string fpathstr=string(fpath);
            string fname=getfilename(fpathstr);
            string fileDetails = "";
            int check_already_uploaded=is_alreadyUploaded(sock,fname,inputvect);
            if(check_already_uploaded !=1)
            {
                return check_already_uploaded;
            }
            upload_check[inputvect[2]][fname]=true;
            file_to_file_path[fname]=fpathstr;
            string piece_wise_hash=hashfun(fpath);
            if(piece_wise_hash!="$")
            {
                string filehash=filehashhelper(fpath);
                string fsize=fsizehelper(fpath);

                appendip_port(fileDetails,fpathstr,filehash,piece_wise_hash,fsize);



               
                int sockcheckval=send(sock , &fileDetails[0] , strlen(&fileDetails[0]) , MSG_NOSIGNAL );
                if(sockcheckval!=-1)
                {
                     char server_reply[10240] = {0}; 
                    read(sock , server_reply, 10240); 
                    cout << server_reply << endl;
                
                    long long int r=stoll(fsize)/524288 +1;
                    vector<int> tmp(r+1,1);
                    fchunkinfomation[fname] = tmp;
                    return 0;
                }
                else
                {
                    cout<<"Error "<<strerror(errno)<<endl;
                    return -1;
                }

            }
            else
            {
                return 0;
            }
        }
        else
            return 0;
        
    }
    if(inputvect[0]=="list_files")
    {
        upload_check[inputvect[1]].erase(inputvect[2]);
        return 0;
    }
    if(inputvect[0]=="show_downloads")
    {
        for(auto it: download_file_lis)
        {
            cout << "[C] " << it.second << " " << it.first << endl;
        }
        return 0;  
    }
}

int connectToTracker(int trackerNum, struct sockaddr_in &serv_addr, int sock)
{
    char* ctracker_ip;
    uint16_t ctracker_port;
    if(trackerNum!=1)
    {
        ctracker_ip =&t2ip[0];
        ctracker_port =t2port;
    }
    else
    {
        ctracker_ip =&t1ip[0];
        ctracker_port =t1port;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port=htons(ctracker_port);
    bool err=0;
    if(inet_pton(AF_INET, ctracker_ip, &serv_addr.sin_addr)<=0)
    {
        err=1;
    }
    int cstatus=connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if( cstatus< 0)
    {
        err=1;
    }
    if(err)
    {
        if(trackerNum!=1)
        {
            return -1; 
        }
        else
            return connectToTracker(2, serv_addr, sock);
            
    }
    return 0;

}

int main(int argc,char* argv[])
{
    if(argc!=3)
    {
        cout<<"Pass the correct number of arguments"<<endl;
        return -1;
    }

    char curDir[128];
    getcwd(curDir, 128); // storing current directory path
    string tinfofname=argv[2];

    // string pathtinfo=string(curDir)+"/"+tinfofname;

    string pathtinfo=tinfofname;

    vector<string>tinfovect=getTrackerInfo(&pathtinfo[0]);

    string clientinfo=argv[1]; 

    vector<string>addrinfo= string_split(clientinfo,":");

    clientip = addrinfo[0]; // IP Address
    clientport = stoi(addrinfo[1]); // Port Number


   

    t1ip=tinfovect[0];  // Tracker 1 ip
    t1port=stoi(tinfovect[1]); // Tracker 1 port
    t2ip=tinfovect[2];  // Tracker 2 IP
    t2port=stoi(tinfovect[3]); // Tracker 2 Port
    
    

    int socketpointer=socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM (Type of Transfer)Provides sequenced, two-way byte streams with a transmission mechanism for stream data.
    struct sockaddr_in server_address;  // a transport address and port for the AF_INET address family.
    

    if(socketpointer<0)
    {
        cout<<"Error in Creating Socket"<<endl;
        return -1;
    }
    

    pthread_t server_thread; // server thread
    int threadval=pthread_create(&server_thread, NULL, client_as_server, NULL);
    if(threadval == -1)  // Creating a new thread and checking if it is created successfully or not
    {
        perror("pthread"); 
        exit(EXIT_FAILURE); 
    }
    
    //int connect_status;

    if(connectToTracker(1,server_address,socketpointer)<0)
    {
    
        exit(-1);
    }

    while(true)
    {
        string s;
        string input_line;  
        
        cout<<">> ";
        getline(cin, input_line);
        if(input_line.length()<0)
            continue;
        vector<string>input=string_to_vect(input_line); 

        if(isloggedin==true && input[0]=="login" )
        {
            cout<<"You have already logged in into the account"<<endl;
            continue;
        }


        if( isloggedin==false && input[0]!="create_user" &&  input[0]!="login")
        {
            cout<<"Login or create account before please"<<endl;
            continue;
        }
        
        bool errorstatus=errorcheck(socketpointer,input_line);
       
        if(errorstatus)
        {
            printf("Error: %s\n",strerror(errno));
            return -1;
        }
     

        processcommand(socketpointer,input);
        
        
    }
    close(socketpointer);
    return 0;

}