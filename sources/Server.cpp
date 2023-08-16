/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: amechain <amechain@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/26 11:23:14 by jmatheis          #+#    #+#             */
/*   Updated: 2023/08/16 11:58:25 by amechain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

Server::Server()
{
    serverSocket_ = 0;
    std::cout << "Default Constructor" << std::endl;
}

Server::Server(const Server &copyclass)
{
    std::cout << "Copy Constructor" << std::endl;
    *this = copyclass;
}

Server& Server::operator= (const Server& copyop)
{
    std::cout << "Copy Assignment Operator" << std::endl;
    if(this != &copyop)
    { ;
    }
    return(*this);
}

Server::~Server()
{
    std::cout << "Destructor" << std::endl;
    close(serverSocket_);
    for (std::vector<Client*>::iterator it = ConnectedClients_.begin(); it != ConnectedClients_.end(); ++it)
        delete *it;
    for (std::vector<Channel*>::iterator it = channels_.begin(); it != channels_.end(); ++it)
        delete *it;
}

Server::Server(uint16_t port, std::string password) : port_(port)
{
    connection_pd_ = password;

    serverSocket_ = 0;
    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = port_;
    // INPUT HANDLING
}

void Server::server_setup()
{
    if ((serverSocket_ = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cout << "Opening Socket ";
        throw SetupError();
    }
    int opted = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opted, sizeof(opted)) == - 1)
    {
        std::cout << "Socket ";
        throw SetupError();
    }
    if (bind(serverSocket_, (struct sockaddr*)&address_, sizeof(address_)) < 0)
    {
        std::cout << "Binding Socket ";
        throw SetupError();
    }
	fcntl(serverSocket_, F_SETFL, O_NONBLOCK);
    if (listen(serverSocket_, SOMAXCONN) == -1)
    {
        std::cout << "Listen from Server ";
        throw SetupError();
    }

    // pollfd tmp = {.revents = 0, .events = EVENTS, .fd = serverSocket_};
    pollfd tmp;
    memset(&tmp, 0, sizeof(tmp)); // Initialize the struct to zero
    tmp.events = EVENTS;
    tmp.fd = serverSocket_;
    PollStructs_.push_back(tmp);

}

void Server::MainLoop()
{
    while(server_shutdown == false)
    {
        if(poll(PollStructs_.data(), PollStructs_.size(), 10) == -1)
            return ;
        if(PollStructs_[0].revents & POLLIN)
            acceptConnection();
        for(unsigned int i = 1; i < PollStructs_.size(); i++)
        {
            if(PollStructs_[i].revents & POLLERR)
                ConnectedClients_[i-1]->ConnectionClosing();
            else if(PollStructs_[i].revents & POLLIN)
                ConnectedClients_[i-1]->ReceiveCommand();
            if(PollStructs_[i].revents & POLLOUT)
                ConnectedClients_[i-1]->SendData();
        }
        CheckForDisconnections();
    }
}

void Server::acceptConnection()
{
    // ACCEPT MUST BE IN WHILE LOOP while(true)
    // When does while end ????
    // accept function blocks until a client connects to the server
    int address_length = sizeof(address_);
    int new_client_fd = accept(serverSocket_, (struct sockaddr*)&address_, (socklen_t*)&address_length);
    if (new_client_fd == -1)
    {
        std::cout << "Accept ";
        throw SetupError();
    }
    // pollfd tmp = {.revents = 0, .events = EVENTS, .fd = new_client_fd};
    pollfd tmp;
    memset(&tmp, 0, sizeof(tmp)); // Initialize the struct to zero
    tmp.events = EVENTS;
    tmp.fd = new_client_fd;

    PollStructs_.push_back(tmp);
    ConnectedClients_.push_back(new Client(new_client_fd, this));
    std::cout << "Accept Connection" << std::endl;
}

void Server::CheckForDisconnections()
{
    // CHECK DISCONNECTED CLIENTS
    if(ConnectedClients_.empty() == false)
    {
        for(unsigned int i = 1; i < ConnectedClients_.size(); i++)
        {
            if(ConnectedClients_[i]->get_state() == 2)
            {
                PollStructs_.erase(PollStructs_.begin()+(i+1));
                delete ConnectedClients_[i];
                ConnectedClients_.erase(ConnectedClients_.begin()+i);
            }
            else
                i++;
        }
    }
    // CHECK CHANNELS THAT ARE 0 (-> no users in channel)
    if(channels_.empty() == false)
    {
        for(unsigned int i = 0; i < channels_.size(); i++)
        {
            if(channels_[i] == 0)
            {
                // delete channels_[i]; already deleted
                channels_.erase(channels_.begin()+i);
            }
        }
    }

    // CHECK AS WELL FOR CHANNELS
}

bool Server::IsUniqueNickname(std::string poss_nick)
{
    std::vector<Client*>::iterator it = ConnectedClients_.begin();
    while(it != ConnectedClients_.end())
    {

        if(poss_nick == (*it)->get_nickname())
            return(false);
        it++;
    }
    return(true);
}

void Server::AddChannel(std::string name)
{
    channels_.push_back(new Channel(name));
}

Channel* Server::GetLastChannel()
{
    return(channels_.back());
}

bool Server::CheckPassword(std::string pass)
{
    if(connection_pd_ != pass)
        return(false);
    return(true);
}


// // SETTER

// void Server::set_topic(std::string& topic)
// {
//     topic_ = topic;
// }
// void Server::set_mode(std::string& mode)
// {
//     mode_ = mode;
// }

// // GETTER

Channel* Server::GetChannel(std::string name)
{
	if (channels_.size() == 0)
		return(0);
    for(unsigned int i = 0; i < channels_.size(); i++)
    {
        if(channels_[i]->get_name() == name)
            return(channels_[i]);
    }
    return(0);
}

void Server::DeleteChannel(std::string name)
{
    for(unsigned int i = 0; i < channels_.size(); i++)
    {
        if(channels_[i]->get_name() == name)
        {
            delete (channels_[i]);
            channels_[i] = 0;
        }
    }
}

Client* Server::GetClient(std::string name)
{
    for(unsigned int i = 0; i < ConnectedClients_.size(); i++)
    {
        if(ConnectedClients_[i]->get_nickname() == name)
            return(ConnectedClients_[i]);
    }
    return(0);
}

const std::string Server::getPassword()
{
    return(connection_pd_);
}

// std::string Server::get_topic()
// {
//     return(topic_);
// }

// std::string Server::get_mode()
// {
//     return(mode_);
// }

// EXCEPTIONS

const char* Server::SetupError::what() const throw()
{
	return("Error: Check Server Settings, set up not possible!");
}
