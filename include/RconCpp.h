#pragma once
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include "boost/asio.hpp"

#include <exception>

#include <bit>
#include <concepts>
#include <ostream>
#include <type_traits>
#include <iomanip>

namespace RconCpp {

enum SERVERDATA : int32_t {
	SERVERDATA_AUTH = 3,
	SERVERDATA_AUTH_RESPONSE = 2,
	SERVERDATA_EXECCOMMAND = 2,
	SERVERDATA_RESPONSE_VALUE = 0
};


class RconCpp
{
public:
	RconCpp(int32_t p_port, std::string p_ip);
	~RconCpp();

	void authenticate(std::string password);

	void sendCommand(std::string cmd, int32_t &id);
	std::string recvAnswer(int32_t& id);

	void close();

private:
	int32_t port;
	int32_t id_inc;
	std::string ip;
	std::string pwd;

	bool authenticated;

	boost::asio::io_service service;
	boost::asio::ip::tcp::socket socket;

	int send(int32_t id, int32_t type, const char* body);
	std::string recv(int32_t& id, int32_t& type);
};

}