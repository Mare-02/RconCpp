#pragma once
#include <iostream>
#include "boost/asio.hpp"

#include <stdexcept>

#include <bit>
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
	/// <summary>
	/// Constructer of the RCON class
	/// </summary>
	/// <param name="p_port">port of the server</param>
	/// <param name="p_ip">ip address of the server</param>
	RconCpp(int32_t p_port, std::string p_ip);
	~RconCpp();

	/// <summary>
	/// Authenticate RCON
	/// </summary>
	/// <param name="password">RCON password</param>
	void authenticate(std::string password);
	
	/// <summary>
	/// Close the connection
	/// </summary>
	void close();

	/// <summary>
	/// Establish a connection to the server
	/// </summary>
	void connect();

	/// <summary>
	/// Use this Method to send and receive normal messages
	/// </summary>
	/// <param name="cmd">The Command you want to send</param>
	/// <returns>The response or "error" when an error occured</returns>
	std::string sendAndRecv(std::string cmd);

	/// <summary>
	/// This Method exposes an interface to thread this procedure of sending
	/// </summary>
	int sendCommand(std::string cmd, int32_t &id);

	/// <summary>
	/// This Method exposes an interface to thread this procedure of receiving
	/// </summary>
	std::string recvAnswer(int32_t& id);

private:
	int32_t port;
	int32_t id_inc;
	std::string ip;
	std::string pwd;

	bool authenticated;
	bool connected;

	boost::asio::io_context service;
	boost::asio::ip::tcp::socket socket;

	int send(int32_t id, int32_t type, const char* body);
	std::string recv(int32_t& id, int32_t& type);

	// Timeout function
	void run(std::chrono::steady_clock::duration timeout);
};

}