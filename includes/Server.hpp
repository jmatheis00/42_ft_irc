/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jmatheis <jmatheis@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/15 10:42:14 by jmatheis          #+#    #+#             */
/*   Updated: 2023/06/27 10:39:51 by jmatheis         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef Server_HPP
# define Server_HPP

# include <iostream>
# include <vector>

#define RESET       "\033[0m"               /* Reset */
#define RED         "\033[31m"              /* Red */
#define GREEN       "\033[32m"              /* Green */
#define YELLOW      "\033[33m"              /* Yellow */
#define PURPLE      "\033[35m"              /* Purple */

#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

#include "Client.hpp"
#include "Channel.hpp"

#define EVENTS (POLLIN | POLLOUT | POLLERR)
#define OPERPWD "LALALA"

class Client;
class Channel;
extern bool	server_shutdown;

class Server
{
    public:
		Server(uint16_t port, std::string password);

		bool ValidPort();
		void MainLoop();
		void server_setup();
		void acceptConnection();
		void CheckForDisconnections();


		bool IsUniqueNickname(std::string poss_nick);
		void AddChannel(std::string name);
		bool CheckPassword(std::string pass);
		Channel* GetChannel(std::string name);
		Channel* GetLastChannel();
		void DeleteChannel(std::string name);

		Client* GetClient(std::string name);


		// void handleClient();
		// void broadcastMessage();
		// // SETTER
		// void set_topic(std::string& topic);
		// void set_mode(std::string& mode);

		// // GETTER
		const std::string getPassword();
		// std::string get_mode();

		~Server(); //Destructor
		class SetupError: public std::exception {
			public:
				virtual const char* what() const throw();
		};
	private:
		int serverSocket_;
		uint16_t port_;
		std::string connection_pd_;
		struct sockaddr_in address_;
		std::string mode_;
		std::vector<pollfd>PollStructs_;

		std::vector<Client*>ConnectedClients_;
		std::vector<Channel*>channels_;

		Server(); //Default Constructor
		Server(const Server &copyclass); //Copy Constructor
		Server& operator= (const Server& copyop); //copy assignment operator

};

#endif