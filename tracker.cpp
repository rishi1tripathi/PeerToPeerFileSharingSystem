#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <sys/types.h>
#include <map>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
using namespace std;

class PeerDetails
{
public:
	string portPeer;
	string ipPeer;
	string userName;
	string password;
	PeerDetails(string usr, string pwd, string ip, string prt)
	{
		userName = usr;
		password = pwd;
		ipPeer = ip;
		portPeer = prt;
	}
};
class MiniPeerDetails
{
public:
	string ipPeer;
	string userName;
	string portPeer;
	MiniPeerDetails(string usr, string ip, string prt)
	{
		userName = usr;
		ipPeer = ip;
		portPeer = prt;
	}
	MiniPeerDetails(string ip, string prt)
	{
		ipPeer = ip;
		portPeer = prt;
	}
};
class GroupDetails
{
public:
	string groupId;
	int totalPeers;
	string ipOwner;
	string portOwner;
	GroupDetails(string gId, string ip, string prt)
	{
		groupId = gId;
		totalPeers = 1;
		ipOwner = ip;
		portOwner = prt;
	}
};
class FileInfo
{
public:
	string fileName;
	string fileSize;
	string hash;
	FileInfo(string fName, string fSize, string hsh)
	{
		fileName = fName;
		fileSize = fSize;
		hash = hsh;
	}
};

vector<PeerDetails> userlists;
vector<GroupDetails> allGroups;
map<string, string> ipPortToUserMap;
map<string, bool> ipPortToLoginMap;
map<string, string> userMaptoIpPort;
map<string, vector<MiniPeerDetails>> pendingMap;
map<string, vector<MiniPeerDetails>> acceptedMap;
map<string, vector<FileInfo>> groupFilesMap;
map<string, vector<MiniPeerDetails>> filesSeederMap;

vector<string> processInputCommand(string inputCommnad)
{
	vector<string> inputTokens;
	string temp = "";
	for (int i = 0; i < inputCommnad.length(); i++)
	{
		if (inputCommnad[i] == ' ')
		{
			inputTokens.push_back(temp);
			temp = "";
			continue;
		}
		else
			temp += inputCommnad[i];
	}
	inputTokens.push_back(temp);
	return inputTokens;
}

void createUser(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 5)
	{
		string returnMessage = "Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string userName = peerToken[1], passWord = peerToken[2], ip = peerToken[3], port = peerToken[4];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) != ipPortToUserMap.end())
	{
		string returnMessage = "user alredy exist with given ip and port combination";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	if (userMaptoIpPort.find(userName) != userMaptoIpPort.end())
	{
		string returnMessage = "user_id already taken by another account";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	ipPortToUserMap[ipPortKey] = userName;
	ipPortToLoginMap[ipPortKey] = false;
	userMaptoIpPort[userName] = ipPortKey;
	PeerDetails peer(peerToken[1], peerToken[2], peerToken[3], peerToken[4]);
	userlists.push_back(peer);
	string returnMessage = "Successfully added user";
	send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
}
void login(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 5)
	{
		string returnMessage = "Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string userName = peerToken[1], passWord = peerToken[2], ip = peerToken[3], port = peerToken[4];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	for(PeerDetails item :userlists){
		if(item.userName==userName&&item.password==passWord){
		ipPortToLoginMap[ipPortKey] = true;
		string returnMessage = "Login successfull";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
		}
	}
	string returnMessage = "Wrong combination";
	send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
	return;

}
void createGroup(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 4)
	{
		string returnMessage = "Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string groupId = peerToken[1], ip = peerToken[2], port = peerToken[3];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	if (acceptedMap.find(groupId) != acceptedMap.end())
	{
		string returnMessage = "Group Already exist.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string userName = ipPortToUserMap[ipPortKey];
	GroupDetails group(groupId, ip, port);
	allGroups.push_back(group);
	vector<MiniPeerDetails> temp;
	MiniPeerDetails mini(userName, ip, port);
	temp.push_back(mini);
	acceptedMap[groupId] = temp;
	string returnMessage = "Group Creation Successfull.";
	send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
}

void joinGroup(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 4)
	{
		string returnMessage = "Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string groupId = peerToken[1], ip = peerToken[2], port = peerToken[3];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	if (acceptedMap.find(groupId) == acceptedMap.end())
	{
		string returnMessage = "Group Does not exist.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string userName = ipPortToUserMap[ipPortKey];

	vector<MiniPeerDetails> acceptedList = acceptedMap[groupId];
	for (MiniPeerDetails item : acceptedList)
	{
		if (item.ipPeer == ip && item.portPeer == port)
		{
			string returnMessage = "User already joined the group.";
			send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
			return;
		}
	}
	vector<MiniPeerDetails> pendingList = pendingMap[groupId];
	for (MiniPeerDetails item : pendingList)
	{
		if (item.ipPeer == ip && item.portPeer == port)
		{
			string returnMessage = "User already requested to join the group.";
			send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
			return;
		}
	}
	MiniPeerDetails mini(userName, ip, port);
	pendingList.push_back(mini);
	pendingMap[groupId] = pendingList;
	string returnMessage = "Successfully added request to join .";
	send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
}
void leaveGroup(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 4)
	{
		string returnMessage = "Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string groupId = peerToken[1], ip = peerToken[2], port = peerToken[3];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	if (acceptedMap.find(groupId) == acceptedMap.end())
	{
		string returnMessage = "Group Does not exist.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	vector<MiniPeerDetails> acceptedList = acceptedMap[groupId];
	auto it = begin(acceptedList);
	while (it != std::end(acceptedList))
	{
		if (ip == it->ipPeer && port == it->portPeer)
		{
			it = acceptedList.erase(it);
			acceptedMap[groupId] = acceptedList;
			string returnMessage = "Successfully removed.";
			send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
			return;
		}
		else
			++it;
	}
	string returnMessage = "User has not joined the group.";
	send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
}
void listPendingRequest(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 5)
	{
		string returnMessage = "0 Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string groupId = peerToken[2], ip = peerToken[3], port = peerToken[4];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "0 User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "0 Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	if (acceptedMap.find(groupId) == acceptedMap.end())
	{
		string returnMessage = "0 Group Does not exist.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string res = "";
	for (MiniPeerDetails item : pendingMap[groupId])
	{
		res += item.userName + " ";
	}
	res = "1 " + res;
	send(peerSocket, res.c_str(), strlen(res.c_str()), 0);
	return;
}
void acceptRequest(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 5)
	{
		string returnMessage = "Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string groupId = peerToken[1], userName = peerToken[2], ip = peerToken[3], port = peerToken[4];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	vector<MiniPeerDetails> pendingList = pendingMap[groupId];
	auto it = begin(pendingList);
	bool isAccepted = false;
	while (it != std::end(pendingList))
	{
		if (it->userName == userName)
		{
			isAccepted = true;
			MiniPeerDetails mini(it->userName, it->ipPeer, it->portPeer);
			vector<MiniPeerDetails> acceptedList = acceptedMap[groupId];
			acceptedList.push_back(mini);
			it = pendingList.erase(it);
			pendingMap[groupId] = pendingList;
			acceptedMap[groupId] = acceptedList;
			string returnMessage = "Successfully accepted.";
			send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
			return;
		}
		else
			++it;
	}
	if (!isAccepted)
	{
		string returnMessage = "Unable to find user.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
}
void listGroups(vector<string> peerToken, int peerSocket)
{
	string ip = peerToken[1], port = peerToken[2];
	if (peerToken.size() != 3)
	{
		string returnMessage = "0 Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "0 User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "0 Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string res = "";
	for (GroupDetails item : allGroups)
	{
		res += item.groupId + " ";
	}
	res = "1 " + res;
	send(peerSocket, res.c_str(), strlen(res.c_str()), 0);
	return;
}
void listFiles(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 4)
	{
		string returnMessage = "0 Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string groupId = peerToken[1], ip = peerToken[2], port = peerToken[3];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "0 User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "0 Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	if (acceptedMap.find(groupId) == acceptedMap.end())
	{
		string returnMessage = "0 Group Does not exist.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string res = "";
	for (FileInfo item : groupFilesMap[groupId])
	{
		res += item.fileName + " ";
	}
	res = "1 " + res;
	send(peerSocket, res.c_str(), strlen(res.c_str()), 0);
	return;
}
void uploadFile(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 7)
	{
		string returnMessage = "Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string fileName = peerToken[1], groupId = peerToken[2], fileSize = peerToken[3], fileHash = peerToken[4], ip = peerToken[5], port = peerToken[6];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	if (acceptedMap.find(groupId) == acceptedMap.end())
	{
		string returnMessage = "Group Does not exist.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool foundUser = false;
	for (MiniPeerDetails item : acceptedMap[groupId])
	{
		if (item.ipPeer == ip && item.portPeer == port)
			foundUser = true;
	}
	if (!foundUser)
	{
		string returnMessage = "User in not registered with the group.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool foundFile = false;
	vector<FileInfo> files = groupFilesMap[groupId];
	for (FileInfo item : files)
	{
		if (item.fileName == fileName)
		{
			foundFile = true;
			break;
		}
	}
	if (!foundFile)
	{
		FileInfo file(fileName, fileSize, fileHash);
		files.push_back(file);
		groupFilesMap[groupId] = files;
	}
	bool foundseeder = false;
	vector<MiniPeerDetails> seederList = filesSeederMap[groupId + ":" + fileName];
	for (MiniPeerDetails item : seederList)
	{
		if (item.ipPeer == ip && item.portPeer == port)
		{
			foundseeder = true;
			break;
		}
	}
	if (foundseeder)
	{
		string returnMessage = "User in already added as a seeder.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	MiniPeerDetails peer(ip, port);
	seederList.push_back(peer);
	filesSeederMap[groupId + ":" + fileName] = seederList;
	string returnMessage = "Added successfully.";
	send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
	return;
}

void downloadFile(vector<string> peerToken, int peerSocket)
{

	if (peerToken.size() != 6)
	{
		string returnMessage = "0 Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string groupId = peerToken[1], fileName = peerToken[2], ip = peerToken[4], port = peerToken[5];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	if (groupFilesMap.find(groupId) == groupFilesMap.end())
	{
		string returnMessage = "0 Group Does not exist.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	if (filesSeederMap.find(groupId + ":" + fileName) == filesSeederMap.end())
	{
		string returnMessage = "0 File not exist.";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	vector<FileInfo> files = groupFilesMap[groupId];
	string fileSize = "", fileHash = "";
	for (FileInfo item : files)
	{
		if (item.fileName == fileName)
		{
			fileSize = item.fileSize;
			fileHash = item.hash;
		}
	}

	string res = "";
	vector<MiniPeerDetails> seeders = filesSeederMap[groupId + ":" + fileName];
	for (MiniPeerDetails item : seeders)
	{
		if (item.ipPeer == ip && item.portPeer == port)
		{
			string returnMessage = "0 File already downloaded.";
			send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
			return;
		}
	}
	for (MiniPeerDetails item : filesSeederMap[groupId + ":" + fileName])
	{
		string ipPortKey = ip + ":" + port;
		bool login = ipPortToLoginMap[ipPortKey];
		if (login)
			res += item.ipPeer + ":" + item.portPeer + " ";
	}
	res = fileSize + " " + fileHash + "*" + res;
	res = "1 " + res;
	int q= send(peerSocket, res.c_str(), strlen(res.c_str()), 0);
	return;
}

void logout(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 3)
	{
		string returnMessage = "Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string ip = peerToken[1], port = peerToken[2];
	string ipPortKey = ip + ":" + port;
	ipPortToLoginMap[ipPortKey] = false;
	string returnMessage = "Successfully logged out";
	send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
	return;
}

void stopShare(vector<string> peerToken, int peerSocket)
{
	if (peerToken.size() != 5)
	{
		string returnMessage = "Invalid arguments";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	string groupId = peerToken[1], fileName = peerToken[2], ip = peerToken[3], port = peerToken[4];
	string ipPortKey = ip + ":" + port;
	if (ipPortToUserMap.find(ipPortKey) == ipPortToUserMap.end())
	{
		string returnMessage = "User not found";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	bool login = ipPortToLoginMap[ipPortKey];
	if (!login)
	{
		string returnMessage = "Please login first";
		send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
		return;
	}
	vector<MiniPeerDetails> files = filesSeederMap[groupId + ":" + fileName];
	auto it = begin(files);
	while (it != std::end(files))
	{
		if (ip == it->ipPeer && port == it->portPeer)
		{
			it = files.erase(it);
			filesSeederMap[groupId + ":" + fileName] = files;
			string returnMessage = "Removed seeder";
			send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
			return;
		}
		else
			++it;
	}
	string returnMessage = "Seeder not found";
  	send(peerSocket, returnMessage.c_str(), strlen(returnMessage.c_str()), 0);
	return;
}

void *serviceReq(void *arg)
{
	int peerSocket = *((int *)arg);
	vector<string> receivedCommand;

	while (true)
	{
		char buffer[1000] = {0};
		int rc = read(peerSocket, buffer, 1024);
		if (rc == 0)
		{
			close(peerSocket);
			return arg;
		}
		string peerMessage = string(buffer);
		vector<string> peerToken = processInputCommand(peerMessage);
		if (peerToken[0] == "create_user")
			createUser(peerToken, peerSocket);
		else if (peerToken[0] == "login")
			login(peerToken, peerSocket);
		else if (peerToken[0] == "create_group")
			createGroup(peerToken, peerSocket);
		else if (peerToken[0] == "join_group")
			joinGroup(peerToken, peerSocket);
		else if (peerToken[0] == "leave_group")
			leaveGroup(peerToken, peerSocket);
		else if (peerToken[0] == "requests")
			listPendingRequest(peerToken, peerSocket);
		else if (peerToken[0] == "accept_request")
			acceptRequest(peerToken, peerSocket);
		else if (peerToken[0] == "list_groups")
			listGroups(peerToken, peerSocket);
		else if (peerToken[0] == "list_files")
			listFiles(peerToken, peerSocket);
		else if (peerToken[0] == "upload_file")
			uploadFile(peerToken, peerSocket);
		else if (peerToken[0] == "download_file")
			downloadFile(peerToken, peerSocket);
		else if (peerToken[0] == "logout")
			logout(peerToken, peerSocket);
		else if (peerToken[0] == "stop_share")
			stopShare(peerToken, peerSocket);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	string logFileName = "tracker_log.txt";
	ofstream myfile(logFileName.c_str(), std::ios_base::out);
	if (argc != 2)
	{
		cout << "Invalid arguments" << endl;
		return 1;
	}
	fstream newfile;
	string ports[2];
	string ips[2];
	newfile.open(argv[1], ios::in);
	if (!newfile.is_open())
	{
		cout << "Unable to open file" << endl;
		return 1;
	}
	string ip, port;
	int pos = 0;
	while (getline(newfile, ip) && getline(newfile, port))
	{
		ips[pos] = ip;
		ports[pos] = port;
		pos++;
	}
	newfile.close(); //close the file object.
	int server_fd, new_socket;
	struct sockaddr_in address;
	int addrLen = sizeof(address);
	int opt = 1;
	int addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
				   &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ips[0].c_str());
	address.sin_port = htons(stoi(ports[0]));
	if (bind(server_fd, (struct sockaddr *)&address,
			 sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 10) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	pthread_t tid[10];
	int newSock;
	pos = 0;
	while (newSock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrLen))
	{
		if (pthread_create(&tid[pos++], NULL, serviceReq, &newSock) != 0)
		{
			cout << "Thread creation failed\n";
		}
	}
	close(server_fd);
	return 0;
}