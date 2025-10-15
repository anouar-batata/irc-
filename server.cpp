#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <poll.h>
#include <sstream>






// ctrd handle...................









// class server_info
// {
//     public :
//         int has_register;
//         int PASS_flag;
//         int Nickname_flag;
//         int Username_flag;
// };


// struct server_info
// {
//     std::string nickname;
//     std::string username;
//     int fd;                  // Client's socket fd
//     int has_register = 0;
//     int PASS_flag = 0;
//     int Nickname_flag = 0;
//     int Username_flag = 0;
// };


//global variable;

std::string server_password;



struct server_info
{
    std::string nickname;
    std::string username;
    int fd; // client's socket fd; 
    int has_register;
    int PASS_flag;
    int Nickname_flag;
    int Username_flag;

    server_info() : fd(-1), has_register(0), PASS_flag(0), Nickname_flag(0), Username_flag(0) {}
};


void    accept_client(int server_fd, std::vector<pollfd> &fd, std::vector<server_info> &clients)
{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0)
    {
        throw std::runtime_error("accept failed");
    }

    pollfd tmp; bzero(&tmp, sizeof (pollfd));
    tmp.fd = client_fd;
    tmp.events = POLLIN;
    fd.push_back(tmp);

    server_info client_info;
    client_info.fd = client_fd;
    clients.push_back(client_info);

    std::cout << "New client connected to the server !" << std::endl;

}

server_info *find_the_client(int client_fd, std::vector<server_info> &clients)
{
    for(int i = 0; i < clients.size(); i++)
    {
        if (clients[i].fd == client_fd)
            return &clients[i];
    }
    return(nullptr);
}

std::string ft_trim(std::string &str)
{
    size_t first = str.find_first_not_of(" ");
    if (first == std::string::npos)
        return("");
    size_t last = str.find_last_not_of(" ");
    return(str.substr(first, last - first + 1));
}



void    detecte_the_command(std::string request, server_info *client, std::vector<server_info> &clients)
{
    try
    {
        if (!client->PASS_flag)
            {
                std::istringstream iss(request);
                std::string command;
                std::string value;
                iss >> command;
                std::getline(iss, value);
                value = ft_trim(value);
                // std::cout << "value : " << value << std::endl;
                if (command != "PASS")
                {
                    std::string error = "Error: command invalid of password!\n";
                    send(client->fd, error.c_str(), error.size(), 0);
                    throw std::runtime_error("command invalid of password!");
                }
                if (value != server_password)
                {
                    std::string error = "Error: invalid password!\n";
                    send(client->fd, error.c_str(), error.size(), 0);
                    throw std::runtime_error("invalid password!");
                }
                client->PASS_flag = 1;
            }
        else
        {
            std::istringstream iss(request);
            std::string command;
            std::string value;
            iss >> command;
            std::getline(iss, value);
            value = ft_trim(value);

            if (command == "NICK")
            {
                if (value.empty())
                {
                    std::string error = "Error: nickname cannot be empty!\n";
                    send(client->fd, error.c_str(), error.size(), 0);
                    throw std::runtime_error("nickname cannot be empty!");
                }
                // check if the nickname is already used
                for (size_t i = 0; i < clients.size(); i++)
                {
                    if (clients[i].nickname == value)
                    {
                        std::string error = "Error: nickname already in use!\n";
                        send(client->fd, error.c_str(), error.size(), 0);
                        throw std::runtime_error("nickname already in use!");
                    }
                }
                client->nickname = value;
                client->Nickname_flag = 1;
            }


            else if (command == "USER")
            {
                if (value.empty())
                {
                    std::string error = "Error: username cannot be empty!\n";
                    send(client->fd, error.c_str(), error.size(), 0);
                    throw std::runtime_error("username cannot be empty!");
                }
                client->username = value;
                client->Username_flag = 1;
            }
            else
            {
                std::string error = "Error: command invalid!\n";
                send(client->fd, error.c_str(), error.size(), 0);
                throw std::runtime_error("command invalid!");
            }

            if (client->PASS_flag && client->Nickname_flag && client->Username_flag)
            {
                client->has_register = 1;
                std::string msg = "You are now registered!\n";
                send(client->fd, msg.c_str(), msg.size(), 0);
                std::cout << "Client " << client->fd << " registered with nickname: " << client->nickname << " and username: " << client->username << std::endl;
            }

            // handle the two others command
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void    handle_the_req(server_info *client, std::vector<pollfd> &fds, std::vector<server_info> &clients)
{
    char buffer[1024];
    int byte_recived = recv(client->fd, buffer, sizeof(buffer) - 1, 0);
    if (byte_recived <= 0)
    {
        std::cout << "client disconnected !" << std::endl;
        close (client->fd);
        // remove from fds
        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].fd == client->fd)
            {
                fds.erase(fds.begin() + i);
                break;
            }
        }
        for (size_t i = 0; i < clients.size(); i++)
        {
            if (clients[i].fd == client->fd)
            {
                clients.erase(clients.begin() + i);
                break;
            }
        }
        return;
    }
    else
    {
        buffer[byte_recived] = '\0';
        std::string request = buffer;
        if (request.back() == '\n')
            request.pop_back();
        detecte_the_command(request, client, clients);
    }
    // split the req;
    // parsc the req
    // first is the passw;
    // second is the nickname or the username
    // initialise the data to the client
    // set the flag
}

void    handle_req(int client_fd ,std::vector<pollfd> &fds, std::vector<server_info> &clients)
{
    server_info *client_connected = find_the_client(client_fd, clients);

    if (client_connected->has_register)
    {
            char buffer[1024];
    int byte_recived = recv(client_connected->fd, buffer, sizeof(buffer) - 1, 0);
    if (byte_recived <= 0)
    {
        std::cout << "client disconnected !" << std::endl;
        close (client_connected->fd);
        // remove from fds
        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].fd == client_connected->fd)
            {
                fds.erase(fds.begin() + i);
                break;
            }
        }
        for (size_t i = 0; i < clients.size(); i++)
        {
            if (clients[i].fd == client_connected->fd)
            {
                clients.erase(clients.begin() + i);
                break;
            }
        }
        return;
    }
        else
        {
        buffer[byte_recived] = '\0';
            std::string request = buffer;
            if (request.back() == '\n')
            request.pop_back();
        }
        std::cout << "command : " << buffer << std::endl;
        //handling_the_req(); // FOR ALAE AND SLAOUI
    }
    else
    {
        handle_the_req(client_connected, fds, clients);
        // must handle it 

    }
}

int   parse_the_input(char *av[])
{
    int i = 0;
    while(av[0][i] != '\0')
    {
        if (!isdigit(av[0][i]))
        {
            std::cout << "the port invalid !" << std::endl;
            return(0);
        }
        i++;
    }
    int port = std::atoi(av[0]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Invalid port!" << std::endl;
        return 0;
    }
    return(1);
}

int main(int ac, char *av[])
{
    if (ac != 3)
    {
        std::cout << "entre the right argumments!" << std::endl;
        exit(1);
    }

    if (!parse_the_input(&av[1]))
        exit(1);
    server_password = av[2];
    // std::cout << server_password << std::endl;

    std::vector<pollfd> fds;
    std::vector<server_info> clients;
    pollfd tmp; bzero(&tmp, sizeof (pollfd));

    int server_fd = -1;
    bool server_running = true;
    // char buffer[1024] = {0};
    struct sockaddr_in server_addr;

    try
    {

        server_fd = socket(AF_INET, SOCK_STREAM, 0);

        
        if (server_fd == -1)
            throw std::runtime_error("Socket failed!");
        
        tmp.fd = server_fd;
        tmp.events = POLLIN;
        fds.push_back(tmp);
        int opt = 1;
        
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw std::runtime_error("setsokopt failed!");
        
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(atoi(av[1]));
        
        if (bind(server_fd, (struct  sockaddr*)(&server_addr), sizeof(sockaddr_in)) < 0)
            throw std::runtime_error("bind failed!");

        if (listen(server_fd, SOMAXCONN) < 0)
            throw std::runtime_error("listen failed!");
    }
    catch(const std::exception& e)
        {
            if (server_fd == -1)
                close(server_fd);
            std::cerr << e.what() << '\n';
            exit(1);
        }
    
    std::cout << "the server start listening on port " << av[1] << ":)" << std::endl;
    while (server_running)
    {
        try
        {
            if (poll(&fds[0], fds.size(), -1)  == -1)
            throw std::runtime_error("poll failed!");
    
            for (size_t i = 0; i < fds.size(); i++)
            {
                if (fds[i].revents & POLLIN) {
                    if (fds[i].fd == server_fd) {
                        accept_client(server_fd, fds, clients);  // 90%
                    } else {
                        handle_req(fds[i].fd ,fds, clients);//
                    }
                }
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}
