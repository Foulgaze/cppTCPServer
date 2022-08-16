#include "header.h"


/*
*/

string getMessageSize(string message)
{
    string retString = to_string(message.size());
    if(retString.size() > 4)
    {
        cerr << "Message is too large";
        return "CERR";
    }

    while(retString.size() != 4)
    {
        retString = "0"+retString;
    }
    return retString;

}

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
    retMessage = getMessageSize(retMessage) + retMessage;
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

void parseCommand(serverManager &serverManager, string msg, int clientSocket)
{
    auto clientList = serverManager.connectList;
     // Recieved Message Format is [UUID|Server Op Code|Client Op Code|Message] UUID is 32 bits, Both op codes are 2 bit, message is undefined length
        // Example Message is [2bc7f400-c637-462b-b28c-83ce20e74692|00|00|Foulgaze]
        int delimeter = msg.find_first_of("|");
        string uuid = msg.substr(0,delimeter);
        int serverOpCode = stoi(msg.substr(delimeter+1,2));
        string clientOpCode = msg.substr(delimeter+4,2);
        msg = msg.substr(delimeter+7);

        cout << "Recieved Message [UUID: " << uuid << "] [serverOpCode: " << to_string(serverOpCode) << "] [clientOpCode:" << clientOpCode << "] [msg: " << msg << "]" << endl;

        switch(serverOpCode)
        {
            default:
            {
                cerr << "Opcode [" << serverOpCode << "] was not recognized";
                break;
            }
            case 0: // Setting username
            {
                string newUser = msg;
                while(!serverManager.checkForUsername(newUser,clientSocket,uuid)) // Adds to username until it is unique
                {
                    newUser += " (1) ";
                }
                send_message(uuid ,clientOpCode, newUser,clientList); // 00 For new name

                break;
            }
            case 1: // Sending Message To All Users
            {
                send_message(uuid ,clientOpCode, msg,clientList); // 01 For new message in chatbox
                break;
            }
            case 2: // Ready Up Message
            {

            }
        }
}



void parseBuffer(serverManager&serverManager, string &buffer, int clientSocket)
{
    cout << buffer << endl;
    while(true)
    {
        string currentMessageBytes = "";
        int currentMessageLength = 0;
        int i = 0;
        while(currentMessageBytes.size() != 4)
        {
            if(i >= buffer.size())
            {
                return;
            }
            currentMessageBytes = currentMessageBytes + buffer[i++];

        }
        currentMessageLength = stoi(currentMessageBytes);
        if(currentMessageLength > buffer.size() - 4)
        {
            return;
        }

        string currentCommand = buffer.substr(i,currentMessageLength);
        parseCommand(serverManager,currentCommand, clientSocket);
        buffer = buffer.substr(i+currentMessageLength);

    }
}


int get_connection_data(int clientSocket,serverManager &serverManager, string &messageBuffer)
{
    // Create and clear the buffer
    char buff[BUFFSIZE];
    memset(buff,0,BUFFSIZE);

    // List of currently connnected players
    auto clientList = serverManager.connectList;

    string msg = "";

    // Get message (ssize_t)
    auto bytesRecv = recv(clientSocket,buff,BUFFSIZE,0);

    if((int) bytesRecv == -1) // Connection Error
    {
        cerr << "There was a connection issue" << endl;
    }

    if((int) bytesRecv == 0) // Removing a player
    {
        optional<string> uuid = serverManager.getUUID(clientSocket);
        optional<string> name = serverManager.getName(clientSocket);
        if(!uuid.has_value() || !name.has_value()) // Problem retriving UUID
        {
            cerr << "A problem has occured removing a player";
        }

        send_message(uuid.value(),"02",name.value(),clientList,clientSocket);
        serverManager.removeMember(clientSocket);
    
        return -1;
    }

    // TO DO
    // MAKE MSG ACCEPT LARGER THAN BUFFER STRINGS

    messageBuffer += string(buff);

    parseBuffer(serverManager,messageBuffer, clientSocket);
    
    return 0;
        

    
}

int main(int argc, char*argv[])
{
    string messageBuffer = "";
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
    serverManager serverManager;

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
                    serverManager.addMember("","",client);
                    FD_SET(client,&readMaster);
                }
                else // One of the clients is sending data, process and then clear the fd
                {
                    if(get_connection_data(i,serverManager,messageBuffer) == -1)
                    {
                        FD_CLR(i, &readMaster);
                    }
                    

                }
            }
        }
    }   

    return 0;
}