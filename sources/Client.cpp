/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jmatheis <jmatheis@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/26 11:23:14 by jmatheis          #+#    #+#             */
/*   Updated: 2023/08/03 16:12:36 by jmatheis         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Client.hpp"

Client::Client()
{
    std::cout << "Default Constructor" << std::endl;
}

Client::Client(int fd, Server* server) : ClientFd_(fd), ClientState_(-1), server_(server)
{
    std::cout << "Constructor" << std::endl;
}

Client::Client(const Client &copyclass) : ClientFd_(copyclass.ClientFd_)
        , ClientState_(copyclass.ClientState_), server_(copyclass.server_)
{
    std::cout << "Copy Constructor" << std::endl;
    *this = copyclass;
}

Client& Client::operator= (const Client& copyop)
{
    std::cout << "Copy Assignment Operator" << std::endl;
    if(this != &copyop)
    {
        // nickname_ = copyop.nickname_;
        // username_ = copyop.username_;
    }
    return(*this);
}

Client::~Client()
{
    std::cout << "Destructor" << std::endl;
}


// SETTER

void Client::set_nickname(std::string& nickname)
{
    nickname_ = nickname;
}
void Client::set_username(std::string& username)
{
    username_ = username;
}

// GETTER

std::string Client::get_nickname()
{
    return(nickname_);
}

std::string Client::get_username()
{
    return(username_);
}

int Client::get_state()
{
    return(ClientState_);
}

int Client::get_fd()
{
    return(ClientFd_);
}

// OTHER

void Client::ConnectionClosing()
{

}

// END OF MESSAGE ALWAYS \r\n ????
// Check if command is wrong, example "PASSpwd" (no space between command and param)
void Client::ReceiveCommand()
{
    // std::cout << "Receive Command" << std::endl;
    char buffer[512];
    ssize_t received = recv(ClientFd_, buffer, sizeof(buffer), 0);
    if (received <= 0)
        return ;
    buffer[received] = '\0';
    buffer_ = std::string(buffer);
    
    // FOR WEECHAT
    size_t rc = buffer_.find("\r\n");
    if(rc != std::string::npos)
        buffer_ = buffer_.substr(0, rc);

    // FOR NC
    size_t nl = buffer_.find("\n");
    if(nl != std::string::npos)
        buffer_ = buffer_.substr(0, nl);

    CheckCommand(buffer_);
}

void Client::SendData()
{
    while(params_.empty() != true)
        params_.pop_back();
    params_.clear();
    cmd_ = "";
    trailing_ = "";
    buffer_ = "";

    if(output_.empty())
        return ;

    send(ClientFd_, output_.data(), output_.size(), 0);
    output_ = "";
}

void Client::SetCmdParamsTrailing(std::string buf)
{
    std::string tmp;
    if (buf.find(' ') == std::string::npos)
    {
        cmd_ = buf;
        return ;
    }
    else
        cmd_ = buf.substr(0, buf.find(' '));

    if(buf.find(':') == std::string::npos)
        tmp = buf.substr(buf.find(' ') + 1, buf.size());
    else
    {
        tmp = buf.substr(buf.find(' ') + 1, buf.find(':') - (buf.find(' ')+1));
        trailing_ = buf.substr(buf.find(':'), buf.size()-(buf.find(':')));
    }

    std::istringstream stream(tmp);
    std::string token;
    while(stream >> token)
        params_.push_back(token);

    // PRINTING EVERYTHING CMD, PARAMS & TRAILING
    std::cout << "Command: " << cmd_ << std::endl;
    for(unsigned int i = 0; i < params_.size(); i++)
        std::cout << "Param[" << i << "]: " << params_[i] << std::endl;
    std::cout << "Trailing: " << trailing_ << std::endl;
}

void Client::CheckCommand(std::string buf)
{
    SetCmdParamsTrailing(buf);

    std::string cmds[16] = { "PASS", "CAP", "NICK", "USER", "JOIN", "PING", "MODE",
        "NAMES", "PART", "PRIVMSG", "INVITE", "TOPIC", "KICK", "OPER", "NOTICE", "QUIT"};
	void (Client::*fp[16])(void) = {&Client::PassCmd, &Client::CapCmd, &Client::NickCmd,
        &Client::UserCmd, &Client::JoinCmd, &Client::PingCmd, &Client::ModeCmd, &Client::NamesCmd,
        &Client::PartCmd, &Client::PrivmsgCmd, &Client::InviteCmd, &Client::TopicCmd,
        &Client::KickCmd, &Client::OperCmd, &Client::NoticeCmd, &Client::QuitCmd};

    for(int i = 0; i < 16; i++)
    {
        if(cmd_ == cmds[i])
        {
            (this->*fp[i])();
            return ;
        }
    }
}

// COMMANDS

void Client::PassCmd()
{
    if ( ClientState_ >= REGISTERED) // What if password already validated but not yet registered?
        output_ = Messages::ERR_ALREADYREGISTRED();
    else if (params_.empty())
    {
        output_ = Messages::ERR_NEEDMOREPARAMS(cmd_);
        return ;
    }
    else if (server_->CheckPassword(params_[0]) == false)
        output_ = Messages::ERR_PASSWDMISMATCH();
    // Check for multiple params, ...
    else
        ClientState_ = PASS;
}

void Client::CapCmd()
{
    // output_ = Messages::RPL_CAP();
}

void Client::NickCmd()
{
    if (ClientState_ < PASS)
        output_ = Messages::ERR_NOTREGISTERED(cmd_);
    else if(params_.empty() == true)
        output_ = Messages::ERR_NONICKNAMEGIVEN();
    // ONLY 8 CHARACTERS ????
    // CHECK FOR SOME SPECIAL SIGNS, ...
    // if(params_.find(' ') != std::string::npos
    //     || params_.find(',') != std::string::npos || params_.find('?') != std::string::npos
    //     || params_.find('!') != std::string::npos || params_.find('@') != std::string::npos
    //     || params_.find('.') != std::string::npos || params_[0] == '$' || params_[0] == ':'
    //     || params_[0] == '&' || params_[0] == '#')
    // {
    //     output_ = Messages::ERR_ERRONEUSNICKNAME(nickname_, params_);
    //     return ;
    // }

    // PROBLEM WHEN CONNECTING WITH SAME NICK, SERVER CLOSES!!!!
    // CHECK FOR UNIQUE NICKNAME
    else if(server_->IsUniqueNickname(params_[0]) == false)
        output_ = Messages::ERR_NICKNAMEINUSE(params_[0]);
    else
    {
        if (nickname_.empty() == false)
            output_ = Messages::RPL_NICKCHANGE(nickname_, params_[0], username_);
        else if (ClientState_ == PASS && !username_.empty())
        {
            ClientState_ = REGISTERED;
            output_ = Messages::RPL_WELCOME(nickname_, username_);
        }
        nickname_ = params_[0];
    }
}

void Client::UserCmd() // How to change your username afterwards ? Bc USER is just at the begining
{
    if (params_.size() != 3 || trailing_.empty()) // Does the trailing need to start with ":" ?
        output_ = Messages::ERR_NEEDMOREPARAMS(cmd_);
    else if ((params_[1] != "0" && params_[1] != "*")
        || (params_[2] != "0" && params_[2] != "*"))
        output_ = Messages::ERR_UMODEUNKNOWNFLAG(nickname_); // Space
    else if (ClientState_ < PASS) // Is PASS = 0 ?
        output_ = Messages::ERR_NOTREGISTERED(cmd_);
    else if (!nickname_.empty())
    {
        ClientState_ = REGISTERED;
        output_ = Messages::RPL_WELCOME(nickname_, username_);
    }
    else if ( ClientState_ >= REGISTERED)
        output_ = Messages::ERR_ALREADYREGISTRED();
    username_ = trailing_;
}

// CHANNEL = FULL?, TOO MANY CHANNELS?, INVITEONLY CHANNEL?
void Client::JoinCmd()
{
    if(params_.size() < 1 || params_.size() > 2)
    {
        output_ = Messages::ERR_NEEDMOREPARAMS(cmd_);
        return ;
    }
    std::vector<std::string>::iterator it;
    std::vector<std::string>keys;
    if(params_.size() == 2)
    {
        std::stringstream key(params_[1]);
        std::string ke;
        while(getline(key, ke, ','))
            keys.push_back(ke);
        it = keys.begin();
    }
    std::stringstream name(params_[0]);
    std::string token;
    while(getline(name, token, ','))
    {
        if(token[0] != '&' && token[0] != '#')
        {
            output_ = Messages::ERR_NOSUCHCHANNEL(nickname_, params_[0]);
            return ;
        }            
    }
    std::stringstream name2(params_[0]);
    while(getline(name2, token, ','))
    {
        std::cout << "HERE" << std::endl;
        server_->AddChannel(params_[0]);
        server_->GetLastChannel()->AddClientToChannel(this);
        channels_.push_back((server_->GetLastChannel()));
        if (keys.empty()== false && it != keys.end())
        {
            server_->GetLastChannel()->set_key(*it);
            output_ = output_.append(Messages::RPL_JOIN_WITHKEY(nickname_, username_, token, *it)); //MULTIPLE MESSAGES!!!!
            it++;
        }
        else
            output_ = output_.append(Messages::RPL_JOIN(nickname_, username_, token)); //MULTIPLE MESSAGES!!!!
    } 
}

void Client::PingCmd()
{

}

void Client::ModeCmd()
{

}

void Client::NamesCmd()
{

}

void Client::PartCmd()
{

}

void Client::PrivmsgCmd()
{

}

void Client::InviteCmd()
{

}

void Client::TopicCmd()
{
    if(params_.size() < 1 || params_.size() > 1
        || (params_.size() != 1 && trailing_ != ""))
    {
        output_ = Messages::ERR_NEEDMOREPARAMS(cmd_);
        return ;
    }
    Channel* c = server_->GetChannel(params_[0]);
    if (c == nullptr)
    {
        output_ = Messages::ERR_NOSUCHCHANNEL(nickname_, params_[0]);
        return ;
    }
    if(params_.size() == 1)
    {
        if(trailing_ == "")
        {
            if(c->get_topic() == "")
                output_ = Messages::RPL_NOTOPIC(nickname_, params_[0]);
            else
                output_ = Messages::RPL_TOPIC(nickname_, params_[0], c->get_topic());
        }
        else if(trailing_ == ":")
        {
            std::string clear = "";
            c->set_topic(clear);
            output_ = Messages::RPL_TOPICCHANGE(nickname_, username_, params_[0], clear);
        }
        else if(trailing_.size() > 1)
        {
            c->set_topic(&trailing_[1]);
            output_ = Messages::RPL_TOPICCHANGE(nickname_, username_, params_[0], &trailing_[1]);
        }
    }
}

void Client::KickCmd()
{

}

void Client::OperCmd()
{

}

void Client::NoticeCmd()
{

}

void Client::QuitCmd()
{
    if(params_.size() > 0)
    {
        output_ = Messages::ERR_NEEDMOREPARAMS(cmd_);
        return ;
    }

    if(channels_.empty() == false)
    {
        std::vector<Channel*>::iterator it = channels_.begin();
        while(it != channels_.end())
        {
            (*it)->RemoveClientFromChannel(this);
            it++;
        }
    }

    if(params_.size() == 0)
    {
        if (trailing_ == "")
            output_ = Messages::RPL_QUIT(nickname_, username_);
        else
            output_ = Messages::RPL_QUIT_MESSAGE(nickname_, username_, trailing_);
        ClientState_ = DISCONNECTED;
    }
}
