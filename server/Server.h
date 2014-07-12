#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "IO.h"
#include "Game.h"
#include "Session.h"

namespace holdem {

using boost::asio::ip::tcp;

class Server : public IO {
public:
    Server(boost::asio::io_service &io_service, const int port, const int num_players, const int initial_chips)
        : io_service_(io_service),
          acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
          num_players_(num_players),
          initial_chips_(initial_chips)
    {
		std::cerr << "Starting server with port " << port << std::endl;
        start_accept();
    }

    void broadcast(const std::string &message) override
    {
        for (int i = 0; i < num_players_; i++)
            send(i, message);
    }

    void send(int i, const std::string &message) override
    {
        sessions_[i]->send(message + "\n");
    }

    void receive(int i, std::string &message) override
    {
        sessions_[i]->receive(message);
    }

private:
    void start_accept()
    {
        Session *new_session = new Session(io_service_,
            [this](Session *new_session) -> bool {
                if (static_cast<int>(sessions_.size()) >= num_players_)
                    return false;

                sessions_.emplace_back(new_session);

                if (static_cast<int>(sessions_.size()) == num_players_)
                    async_start_game();

                return true;
            });

        acceptor_.async_accept(new_session->socket(),
            boost::bind(&Server::handle_accept, this, new_session, boost::asio::placeholders::error));
    }

    void handle_accept(Session *new_session, const boost::system::error_code &error)
    {
        if (!error)
        {
            new_session->start();
        }
        else
        {
            std::cerr << "Server handle_accept error: " << error.message() << "\n";
            delete new_session;
        }

        start_accept();
    }

    void async_start_game()
    {
        io_service_.post(boost::bind(&Server::start_game, this));
    }

    void start_game()
    {
        std::vector<std::string> names;
        for (auto &session : sessions_) {
			names.emplace_back(session->login_name());
			
			std::cout << "[broadcast] " << names.back() << std::endl;
			broadcast(names.back());
		}

		std::cout << "[broadcast] player list end" << std::endl;
		broadcast("player list end");

		std::cout << "[broadcast] initial chips = " << initial_chips_ << std::endl;
		{
			std::ostringstream oss;
			oss << "initial chips = " << initial_chips_;
			broadcast(oss.str().c_str());
		}
		
		Game game(*this, names, initial_chips_);

        std::vector<int> blinds { 1, 2, 5, 10, 20, 50, 100, 200, 500 };
		bool end_game = false;
        for (int blind : blinds) {
        for (int t = 1; t <= 3; t++)
			{
				if (game.run(blind) <= 1) {
					std::cerr << "fewer than 2 players left, game over" << std::endl;
					broadcast("game over");
					end_game = true;
					break;
				}
			}
			if (end_game) break;
		}

		game.final_stat();
    }

    boost::asio::io_service &io_service_;
    tcp::acceptor acceptor_;
    const int num_players_;
    const int initial_chips_;
    std::vector<std::unique_ptr<Session>> sessions_;
};

}
