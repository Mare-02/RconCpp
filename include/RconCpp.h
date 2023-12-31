#pragma once
#include <iostream>
#include "boost/asio.hpp"

#include <stdexcept>

#include <iomanip>
#include <intrin.h>

#ifndef RCONCPP_H
#define RCONCPP_H

// uncomment to get more insight into the buffers used in the rcon protocol
//#define OUTPUT_DEBUG

// uncomment this if you experience an error while authentication
//#define AUTH_ERROR

namespace RCONCPP {

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
	/// <param name="pTimeout">the time to wait for a response. Default 5</param>
	RconCpp(int32_t p_port, std::string p_ip, unsigned pTimeout=5);
	~RconCpp();

	/// <summary>
	/// Authenticate RCON, throws std::runtime_error on failure
	/// </summary>
	/// <param name="password">RCON password</param>
	void authenticate(std::string password);

	/// <summary>
	/// Close the connection
	/// </summary>
	void close();

	/// <summary>
	/// Establish a connection to the server, throws std::runtime_error on failure
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
	/// <returns>-1 on failure</returns>
	int sendCommand(std::string cmd, int32_t &id);

	/// <summary>
	/// This Method exposes an interface to thread this procedure of receiving
	/// </summary>
	/// <returns>returns empty string on failure or at a success the payload</returns>
	std::string recvAnswer(int32_t& id);

	/// <summary>
	/// return true if the Server is connected
	/// </summary>
	bool isConnected() { return connected; }

	/// <summary>
	/// return true if the connection is authenticated
	/// </summary>
	bool isAuthenticated() { return authenticated; }

	/// throws exceptions on timeout
	/// return 0 on success
	int send(int32_t id, int32_t type, const char* body);

	/// returns empty string on failure
	/// returns payload on success
	std::string recv(int32_t& id, int32_t& type, int32_t& packet_size);
private:
	int32_t port;
	int32_t id_inc;
	std::string ip;
	std::string pwd;

	bool authenticated;
	bool connected;

	bool big_endian = false;

	unsigned timeout;

	boost::asio::io_context service;
	boost::asio::ip::tcp::socket socket;

	// Timeout function
	void run(std::chrono::steady_clock::duration timeout);
};

}
#endif