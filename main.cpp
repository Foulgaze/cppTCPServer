#include "header.h"


/*
*/


int accept_new_connection(int server_socket)
{
    int client_socket;
    sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    client_socket = accept(server_socket, (sockaddr*)&client,&clientSize);
    return client_socket;
}

void send_message(string uuid, string opCode, string message,auto& clientList, int socket = -1) // This function sends the message to all users connected except for the specified socket
{
    string retMessage = uuid + "|" + opCode + "|" + message;
    cout << "Sending [" << retMessage << "]" << endl;
    byte retMsg[retMessage.length()];
    memcpy(retMsg, retMessage.data(), retMessage.length());
    for(int i = 0; i < clientList.size(); ++i)
    {
        if(clientList[i]->socketNum != socket)
        {
            send(clientList[i]->socketNum,retMsg,retMessage.length(),0);
        }
    }
}

int get_connection_data(int clientSocket,serverManager &sm)
{
    // Create and clear the buffer
    char buff[BUFFSIZE];
    memset(buff,0,BUFFSIZE);

    // List of currently connnected players
    auto clientList = sm.connectList;

    string msg = "";

    // Get message (ssize_t)
    auto bytesRecv = recv(clientSocket,buff,BUFFSIZE,0);

    if((int) bytesRecv == -1) // Connection Error
    {
        cerr << "There was a connection issue" << endl;
    }

    if((int) bytesRecv == 0) // Removing a player
    {
        optional<string> uuid = sm.getUUID(clientSocket);
        optional<string> name = sm.getName(clientSocket);
        if(!uuid.has_value() || !name.has_value()) // Problem retriving UUID
        {
            cerr << "A problem has occured removing a player";
        }

        send_message(uuid.value(),"02",name.value(),clientList,clientSocket);
        sm.removeMember(clientSocket);
    
        return -1;
    }

    // TO DO
    // MAKE MSG ACCEPT LARGER THAN BUFFER STRINGS

    msg += string(buff);

    if(msg.size() > 2) // Standard command to pass onto clients
    {
        int delimeter = msg.find_first_of("|");
        string uuid = msg.substr(0,delimeter);
        int opCode = stoi(msg.substr(delimeter+1,2));
        msg = msg.substr(delimeter+4);

        cout << "Recieved Message [UUID: " << uuid << " opCode: " << to_string(opCode) << " msg: " << msg << "]" << endl;

        switch(opCode)
        {
            default:
            {
                cerr << "Opcode [" << opCode << "] was not recognized";
                break;
            }
            case 0: // Setting username
            {
                string newUser = msg;
                while(!sm.checkForUsername(newUser,clientSocket,uuid)) // Adds to username until it is unique
                {
                    newUser += " (1) ";
                }
                send_message(uuid ,"00", newUser,clientList); // 00 For new name
                break;
            }
            case 1: // Sending Chat Message
            {
                send_message(uuid ,"01", msg,clientList); // 01 For new message in chatbox
                break;
            }
        }
        
    }
    else
    {
        cerr << "The Message Recieved Did not Fit the Critera [" << msg << "]" << endl;
    }
    
    return 0;
        

    
}

int main(int argc, char*argv[])
{
    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0); //, Creates a TCP socket for IPV_4
    if(listening == -1) // A socket couldn't be created
    {
        cerr << "Can't create a socket!";
        return -1;
    }

    // Bind the socket to a IP / port
    sockaddr_in hint;
    hint.sin_family = AF_INET; // Set socketaddr_in to IPV_4
    hint.sin_port = htons(54000); //translation for big/little endians. Set socketport to 54000
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

    if( bind(listening, (sockaddr*)&hint, sizeof(hint)) == -1)
    {
        cerr << "Can't bind to IP/port";
        return -2;
    }

    // Mark the socket for listening in
    if(listen(listening, SOMAXCONN) == -1)
    {
        cerr << "Can't listen";
        return -3;
    }

    fd_set readMaster,readfds,writefds;
    FD_ZERO(&readMaster);
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    
    FD_SET(listening,&readMaster); // Adds listening port to list of ports that are being checked for read

    vector<int> connectedFDPorts;
    serverManager sm;

    while(true)
    {
        readfds = readMaster;
        if(select(FD_SETSIZE,&readfds,NULL,NULL,NULL) < 0)
        {
            cerr << "A problem has occured reading the file descriptors";
        }

        for(int i = 0; i < FD_SETSIZE; ++i) // Iterate through all file descriptors, checking for ones that have data available
        {
            if(FD_ISSET(i,&readfds))
            {
                if(i == listening) // New Connection, add to serverManager and masterFDList
                {
                    int client = accept_new_connection(listening);
                    sm.addMember("","",client);
                    FD_SET(client,&readMaster);
                }
                else // One of the clients is sending data, process and then clear the fd
                {
                    if(get_connection_data(i,sm) == -1)
                    {
                        FD_CLR(i, &readMaster);
                    }
                    

                }
            }
        }
    }   

    return 0;
}