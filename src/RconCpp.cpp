#include "RconCpp.h"

void print_bytes(std::ostream& out, const char* title, const unsigned char* data, size_t dataLen, bool format = true) {
    out << title << std::endl;
    out << std::setfill('0');
    for (size_t i = 0; i < dataLen; ++i) {
        out << std::hex << std::setw(2) << (int)data[i];
        if (format) {
            out << (((i + 1) % 16 == 0) ? "\n" : " ");
        }
    }
    out << std::endl;
}


namespace RconCpp {

    RconCpp::RconCpp(int32_t p_port, std::string p_ip) : port(p_port), ip(p_ip), socket(service), authenticated(false)
    {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(p_ip), p_port);

        socket.connect(endpoint);
    }

    RconCpp::~RconCpp()
    {
    }

    void RconCpp::authenticate(std::string password)
    {
        pwd = password;

        send(socket, 0, SERVERDATA::SERVERDATA_AUTH, password.c_str());
        
        int32_t id = 0;
        int32_t type = 0;
        recv(socket, id, type);

        std::string received = recv(socket, id, type);
        if (type != 2) 
        {
            throw std::exception("password was not accepted");
        }

        authenticated = true;
    }

    int RconCpp::send(boost::asio::ip::tcp::socket& Socket, int32_t id, int32_t type, const char* body)
    {
        int32_t groesse = 8 + strlen(body) + 2 + 4;

        char* buffer = new char[groesse];
        memset(buffer, '\0', groesse);

        // TODO: wenn mehr als 1024 bytes gesendet werden, wird das Protokoll anders

        // Size of the Packet
        int32_t size_field = groesse-4;
        memcpy(buffer, &size_field, 4);

        // Id
        int32_t id_field = id;
        memcpy(buffer + 4, &id_field, 4);

        // Type
        int32_t type_field = type;
        memcpy(buffer + 8, &type_field, 4);

        // Body
        memcpy(buffer + 12, body, strlen(body));

        // Ending Zeros
        memcpy_s(buffer+12+strlen(body), groesse, "\0\0", 2);

#ifdef _DEBUG
        print_bytes(std::cout, body, (unsigned char*)buffer, groesse);
#endif // _DEBUG

        boost::asio::write(Socket, boost::asio::buffer(buffer, groesse));

        return 0;
    }

    std::string RconCpp::recv(boost::asio::ip::tcp::socket& Socket, int32_t& id, int32_t& type)
    {
        char buf[1024];
        memset(buf, 0, 1024);

        // read following Packet size
        boost::asio::read(Socket, boost::asio::buffer(buf, 1024), boost::asio::transfer_exactly(4));
        int32_t size_len = *(reinterpret_cast<int32_t*>(buf));

        // read Packet
        boost::asio::read(Socket, boost::asio::buffer(buf + 4, 1024 - 4), boost::asio::transfer_exactly(size_len));
        
        std::memcpy(&id, buf + 4, 4);
        std::memcpy(&type, buf + 8, 4);

#ifdef _DEBUG
        print_bytes(std::cout, "response", (unsigned char*)buf, size_len + 4);
        std::cout << buf + 12 << std::endl;
#endif // _DEBUG

        return std::string(buf + 12);
    }

}