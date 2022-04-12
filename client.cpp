#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <sys/types.h>
#include <map>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <pthread.h>
#define CHUNK_SIZE 512*1024
using namespace std;
string clientIp;
string clientPort;
string trackerIp;
string trackerPort;
map<string,bool> downloads;
struct SeederDetails
{
    string fileName;
    string peer;
    long int fileSize;
    long int noOfChunks;
    string destinaton;
    int seederNo;
    bool isLast;
    int noOfSeeders;
};
vector<string> splitByCustomDelimeter(string message, char delimiter)
{
    vector<string> res;
    string temp = "";
    for (int i = 0; i < message.length(); i++)
    {
        if (message[i] == delimiter)
        {
            res.push_back(temp);
            temp = "";
        }
        else
        {
            temp += message[i];
        }
    }
    if (temp != "")
        res.push_back(temp);
    return res;
}

string calHashofchunk(const char *schunk, int length)
{

    unsigned char hash[SHA_DIGEST_LENGTH] = {0};
    char buf[SHA_DIGEST_LENGTH] = {0};
    SHA1((unsigned char *)schunk, length, hash);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        sprintf((char *)&(buf[i]), "%02x", hash[i]);

    string ans;
    for (int i = 0; i < 20; i++)
        ans += buf[i];
    return ans;
}

string getFileHash(string filePath, long int fileSize)
{
    string res;
    int c;
    ifstream file(filePath, ifstream::binary);
    long int chunkSize = CHUNK_SIZE;
    int noOfChunks = fileSize / chunkSize;
    int lastChunkSize = fileSize % chunkSize;

    if (lastChunkSize == 0)
        lastChunkSize = chunkSize;
    else
        ++noOfChunks;
    for (int i = 0; i < noOfChunks; ++i)
    {
        int currentChunkSize;
        if (i == noOfChunks - 1)
            currentChunkSize = lastChunkSize;
        else
            currentChunkSize = chunkSize;

        char *chunk = new char[currentChunkSize];
        file.read(chunk,
                  currentChunkSize);

        string sh1out = calHashofchunk(chunk, currentChunkSize);
        memset(chunk,0,currentChunkSize);
        delete(chunk);
        res = res + sh1out;
    }

    return res;
}
void *leecherService(void *arg)
{
    struct SeederDetails *item = (struct SeederDetails *)arg;
    string fileName = item->fileName;
    int noOfSeeders = item->noOfSeeders;
    int noOfChunks = item->noOfChunks;
    bool isLast = item->isLast;
    string destination = item->destinaton;
    long int fileSize = item->fileSize;
    int seederNo = item->seederNo;
    string peer = item->peer;
    vector<string> ipPort = splitByCustomDelimeter(peer, ':');
    int seederSock;
    struct sockaddr_in seederAddress;
    if ((seederSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed in seeder");
        return NULL;
    }
    seederAddress.sin_addr.s_addr = inet_addr(ipPort[0].c_str());
    seederAddress.sin_family = AF_INET;
    seederAddress.sin_port = htons(stoi(ipPort[1]));
    if (connect(seederSock, (struct sockaddr *)&seederAddress, sizeof(seederAddress)) < 0)
    {
        perror("\nConnection Failed \n");
        return NULL;
    }
    int toDownloadChunks = noOfChunks / noOfSeeders;
    if (seederNo < noOfChunks % noOfSeeders)
        toDownloadChunks++;
    string outputFilePath = destination;
    FILE *fpt = fopen(outputFilePath.c_str(), "wb");
    if(!fpt){
        return NULL;
    }
    long int chunkSize = CHUNK_SIZE;
    long int totalDownloadBytes = (toDownloadChunks - 1) * chunkSize;
    if (!isLast)
        totalDownloadBytes += chunkSize;
    else{

        totalDownloadBytes += (fileSize % chunkSize)==0?chunkSize :(fileSize % chunkSize);
    }
    string sendSize = fileName + " "+ to_string(totalDownloadBytes);
    send(seederSock, sendSize.c_str(), strlen(sendSize.c_str()), 0);
    char tmp[2]={0};
    read(seederSock, tmp, 2);
    for (int i = 1; i <= toDownloadChunks; i++)
    {
        long int chunkSize = CHUNK_SIZE;
        int currentChunkSize;
        if (i == toDownloadChunks && isLast)
            currentChunkSize = (fileSize % chunkSize)==0?chunkSize :(fileSize % chunkSize);
        else
            currentChunkSize = chunkSize;
        long int baseSeekPos = ((i - 1) * noOfSeeders * chunkSize) + (seederNo - 1) * chunkSize;
        int bufSize = 0;
        while (bufSize < currentChunkSize)
        {
            char *buffer = new char[4096];
            memset(buffer,0,4096);
            long int seekPos = baseSeekPos + bufSize;
            string sendCommand = to_string(seekPos);
            send(seederSock, sendCommand.c_str(), strlen(sendCommand.c_str()), 0);
            int rc = read(seederSock, buffer, 4096);
            string demo(buffer);
            fseek(fpt, seekPos, 0);
            int x= fwrite(buffer, sizeof(char), rc, fpt);
            delete (buffer);
            bufSize += 2;
        }
    }
    fclose(fpt);
    close(seederSock);
    return NULL;
}
void createUser(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
     memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    cout << string(buffer) << endl;
   
    delete(buffer);
}
void login(vector<string> inputToken, int trackerSock, string inputCommand)
{
       char *buffer = new char[1024];
       memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    cout << string(buffer) << endl;
    delete(buffer);   
}
void createGroup(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    cout << string(buffer) << endl;
    delete(buffer);   
}
void joinGroup(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    cout << string(buffer) << endl;
    delete(buffer);  
}
void logout(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    cout << sendCommoand << endl;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    delete(buffer);  
}
void stopShare(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    cout << string(buffer) << endl;
    delete(buffer);  
}
void showDownloads(vector<string> inputToken, int trackerSock, string inputCommand)
{
   map<string, bool>::iterator it;
       for (it = downloads.begin(); it != downloads.end(); it++)
       {
            vector<string> arr = splitByCustomDelimeter(it->first,':');
            bool stat = it->second;
            cout<< it->second<< " "<< arr[0]<<" "<< arr[1]<<endl;
       }
   
}
void leaveGroup(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    cout << string(buffer) << endl;
    delete(buffer); 
}
void listPendingRequests(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    string ret= string(buffer);
    string status =ret.substr(0,1);
    if(status=="0"){
        cout<<ret.substr(2,ret.length());
    }
    else{
        ret= ret.substr(2,ret.length());
        for(string item :splitByCustomDelimeter( string(ret),' ')){
             cout<<item<<endl;
        }
    }
    delete(buffer); 
}
void acceptRequests(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    cout << string(buffer) << endl;
    delete(buffer); 
}

void listGroups(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    string ret= string(buffer);
    string status =ret.substr(0,1);
    if(status=="0"){
        cout<<ret.substr(2,ret.length());
    }
    else{
        ret= ret.substr(2,ret.length());
        for(string item :splitByCustomDelimeter( string(ret),' ')){
            cout<<item<<endl;
        }
    }
    delete(buffer); 
}
void listFiles(vector<string> inputToken, int trackerSock, string inputCommand)
{
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    string ret= string(buffer);
    string status =ret.substr(0,1);
    if(status=="0"){
        cout<<ret.substr(2,ret.length());
    }
    else{
        ret= ret.substr(2,ret.length());
        for(string item :splitByCustomDelimeter( string(ret),' ')){
            cout<<item<<endl;
        }
    }
    delete(buffer); 
}
void uploadFile(vector<string> inputToken, int trackerSock)
{
    string filePath = inputToken[1], groupId = inputToken[2];
    ifstream file(filePath, ifstream::binary);
    struct stat fileStatus;
    stat(filePath.c_str(), &fileStatus);
    long int fileSize = fileStatus.st_size;
    string fileHashCombined = getFileHash(filePath, fileSize);
    string shortFileHash = calHashofchunk(fileHashCombined.c_str(), fileHashCombined.length());
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputToken[0] + " " + filePath + " " + groupId + " " + to_string(fileSize) + " " + shortFileHash + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    cout << string(buffer) << endl;
    delete(buffer);
}

void downloadFile(vector<string> inputToken, int trackerSock, string inputCommand)
{
    string groupId = inputToken[1], filePath = inputToken[2], destinationlocation = inputToken[3];
    char *buffer = new char[1024];
    memset(buffer,0,1024);
    string sendCommoand = inputCommand + " " + clientIp + " " + clientPort;
    send(trackerSock, sendCommoand.c_str(), strlen(sendCommoand.c_str()), 0);
    int rc = read(trackerSock, buffer, 1024);
    string receivedMessage = string(buffer);
    string status = receivedMessage.substr(0, 1);
    receivedMessage = receivedMessage.substr(2, receivedMessage.length());
    if (status == "0")
    {
        cout << string(receivedMessage) << endl;
        return;
    }
    vector<string> temp = splitByCustomDelimeter(receivedMessage, '*');
    vector<string> peers = splitByCustomDelimeter(temp[1], ' ');
    int noOfSeeders = peers.size();
    vector<string> temp2 = splitByCustomDelimeter(temp[0], ' ');
    long int fileSize = stoi(temp2[0]);
    string fileHash = temp2[1];
    long int chunkSize = CHUNK_SIZE;
    int noOfChunks = fileSize / chunkSize;
    int lastChunkSize = fileSize % chunkSize;
    if (lastChunkSize == 0)
        lastChunkSize = chunkSize;
    else
        ++noOfChunks;
    pthread_t seeders_id[10];
    int limit = min(noOfChunks, noOfSeeders);
    for (int i = 0; i < limit; i++)
    {
        struct SeederDetails *item =new SeederDetails();
        item->fileName = filePath;
        item->noOfSeeders = limit;
        item->seederNo = i + 1;
        item->fileSize = fileSize;
        item->peer = peers[i];
        item->destinaton = destinationlocation;
        item->noOfChunks = noOfChunks;
        if (i == limit - 1)
            item->isLast = true;
        else
            item->isLast = false;

         int r=  pthread_create(&seeders_id[i], NULL, leecherService, (void *)item);
         pthread_join(seeders_id[i], NULL);
         delete(item);
    }
   
    string fileHashCombined = getFileHash(destinationlocation, fileSize);
    string shortFileHash = calHashofchunk(fileHashCombined.c_str(), fileHashCombined.length());
    if(fileHash==shortFileHash){
        string uploadFileC="upload_file";
        uploadFileC+= " " + filePath + " " + groupId + " " + to_string(fileSize) + " " + shortFileHash + " " + clientIp + " " + clientPort;
        send(trackerSock, uploadFileC.c_str(), strlen(uploadFileC.c_str()), 0);
        read(trackerSock, buffer, 1024);
        return;
    }
    else{
    cout<<"Failed"<<endl;
    }

    delete(buffer);
}
void *servicePeerClient(void *)
{
    int trackerSock;
    struct sockaddr_in trackerAddress;
    if ((trackerSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed in peer");
        return NULL;
    }
    trackerAddress.sin_addr.s_addr = inet_addr(trackerIp.c_str());
    trackerAddress.sin_family = AF_INET;
    trackerAddress.sin_port = htons(stoi(trackerPort));
    if (connect(trackerSock, (struct sockaddr *)&trackerAddress, sizeof(trackerAddress)) < 0)
    {
        perror("\nConnection Failed \n");
        return NULL;
    }
    string inputCommand;
    cout << "Enter commands seperated by space" << endl;
    while (true)
    {
        string inputCommand;
        getline(cin, inputCommand);
        vector<string> inputTokens = splitByCustomDelimeter(inputCommand, ' ');
        if (inputTokens[0] == "create_user")
            createUser(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "login")
            login(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "create_group")
            createGroup(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "join_group")
            joinGroup(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "leave_group")
            leaveGroup(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "requests")
            listPendingRequests(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "accept_request")
            acceptRequests(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "list_groups")
            listGroups(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "list_files")
            listFiles(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "upload_file")
            uploadFile(inputTokens, trackerSock);
        else if (inputTokens[0] == "download_file")
            downloadFile(inputTokens, trackerSock, inputCommand);
        else if (inputTokens[0] == "logout")
            logout(inputTokens, trackerSock,inputCommand);
        else if (inputTokens[0] == "show_downloads")
            showDownloads(inputTokens, trackerSock,inputCommand);
        else if (inputTokens[0] == "stop_share")
            stopShare(inputTokens, trackerSock,inputCommand);
        else
        {
            cout << "Invalid Command." << endl;
        }
    }
    return NULL;
}

void *seederService(void *arg)
{
    int peerSocket = *((int *)arg);
    vector<string> receivedCommand;
    char buffer[100] = {0};
    int rc = read(peerSocket, buffer, 1000);
    string receivedMessage = string(buffer);
    vector<string> tokens = splitByCustomDelimeter(receivedMessage, ' ');
    string fileName = tokens[0];
    long int toUpload= stoi(tokens[1]);
    string ack="1";
    send(peerSocket, ack.c_str(), strlen(ack.c_str()), 0);
    FILE *fp = fopen(fileName.c_str(), "rb");
    int bufSize=0;  
    while(bufSize<toUpload){
        char *receiveBuffer = new char[1000];
        char *sendBuffer = new char[4096];
        int rc = read(peerSocket, receiveBuffer, 1000);
        string seekPos(receiveBuffer);
        fseek(fp, stoi(seekPos), 0);
        long int sizeChunk = fread(sendBuffer, sizeof(char), 4096, fp);
        send(peerSocket, sendBuffer, sizeChunk, 0);
        bufSize+=2;
        memset(receiveBuffer,0,1000);
        memset(sendBuffer,0,4096);
        delete(receiveBuffer);
        delete(sendBuffer);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Invalid arguments" << endl;
        return 1;
    }
    string logFileName = "client_log.txt";
    ofstream myfile(logFileName.c_str(), std::ios_base::out);
    fstream newfile;
    string trackerPorts[2];
    string trackerIps[2];
    newfile.open(argv[2], ios::in);
    if (!newfile.is_open())
    {
        cout << "Unable to open file" << endl;
        return 1;
    }
    string ip, port;
    int pos = 0;
    while (getline(newfile, ip) && getline(newfile, port))
    {
        trackerIps[pos] = ip;
        trackerPorts[pos] = port;
        pos++;
    }
    newfile.close();
    string temp = string(argv[1]);
    clientIp = temp.substr(0, temp.find_first_of(':'));
    clientPort = temp.substr(temp.find_first_of(':') + 1);
    trackerIp = trackerIps[0];
    trackerPort = trackerPorts[0];
    pthread_t clientPeer, serverPeer;
    if (pthread_create(&clientPeer, NULL, servicePeerClient, NULL) != 0)
    {
        cout << "Failed to create client thread\n";
        return -1;
    }
    struct sockaddr_in address;
    int addrlen = sizeof(address), opt = 1, server_fd, new_socket;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed in seeder");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    memset(&address, '\0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(clientIp.c_str());
    address.sin_port = htons(stoi(clientPort));

    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed in seeder");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    int newSock;
    while (1)
    {
        if (newSock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen))
        {
            if (pthread_create(&serverPeer, NULL, seederService, &newSock) != 0)
            {
                cout << "Failed to create server request service thread\n";
            }
        }
    }
    return 0;
}