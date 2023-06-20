#include <iostream>
#include "RconCpp.h"

int main()
{
	int32_t port = 21115;	// Enter the RCON port
	std::string ip = "127.0.0.1";	// Enter the ip address of the server
	std::string rcon_pwd = "YourPassword";

	RconCpp::RconCpp rcon(port, ip);

	try
	{
		rcon.connect();
	}
	catch (std::runtime_error e)
	{
		std::cout << e.what() << std::endl;
		return -1;
	}

	try
	{
		rcon.authenticate(rcon_pwd);
	}
	catch (std::runtime_error e)
	{
		std::cout << e.what() << std::endl;
		return -1;
	}

	std::cout << "success\n";

	std::string players = rcon.sendAndRecv("listplayers");
	std::cout << players << std::endl;

	return 0;
}