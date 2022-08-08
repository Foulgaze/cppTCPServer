#include <cstring>    // sizeof()
#include <iostream>
#include <string>   
#include <vector>

// headers for socket(), getaddrinfo() and friends
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <unistd.h>
#include <optional>

#define BUFFSIZE 4096

using namespace std;


class serverMember
{
    public:
    serverMember(string name = "",string uuid = "", int socketNum = -1)
    {
        this->name = name;
        this->uuid = uuid;
        this->socketNum = socketNum;
    }
    string name;
    string uuid;
    int socketNum;
};
    
class serverManager
{
    public:
    serverManager()
    {

    }
    
    optional<string> getName(int socket)
    {
        for(int i = 0; i < connectList.size(); ++i)
        {
            if(connectList[i]->socketNum == socket)
            {
                return connectList[i]->name;
            }
        }
        return {};
    }   

    optional<string> getUUID(int socket)
    {
        for(int i = 0; i < connectList.size(); ++i)
        {
            if(connectList[i]->socketNum == socket)
            {
                return connectList[i]->uuid;
            }
        }
        return {};
    }
    
    void addMember(string name,string uuid, int socketNum) // Adds a player to the list
    {
        serverMember* sm = new serverMember(name,uuid,socketNum);
        connectList.push_back(sm);
    }

    bool removeMember(int fd) // Removes a member from the list based on socket num
    {
        int deleteNum = -1;
        for(int i = 0; i < connectList.size(); ++i)
        {
            if(connectList[i]->socketNum == fd)
            {   
                deleteNum = i;
                break;
            }
        }
        if(deleteNum != -1)
        {
            serverMember* sm = connectList[deleteNum];
            connectList.erase(connectList.begin() + deleteNum);
            delete(sm);
            return true;
        }
        return false;
    }

    bool checkForUsername(string user, int socket,string id) // Checks to see if a username is already present in the player pool
    {
        for(int i = 0; i < connectList.size(); ++i)
        {
            if(user == connectList[i]->name && socket != connectList[i]->socketNum)
            {
                return false;
            }
        }
        for(int i = 0; i < connectList.size(); ++i)
        {
            if(socket == connectList[i]->socketNum)
            {
                connectList[i]->name = user;
                connectList[i]->uuid = id;
            }
        }
        return true;
    }

    vector<serverMember*> connectList;
};