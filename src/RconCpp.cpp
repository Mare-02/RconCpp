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

    RconCpp::RconCpp(int32_t p_port, std::string p_ip) : port(p_port), ip(p_ip), socket(service), authenticated(false), id_inc(0)
    {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(p_ip), p_port);

        socket.connect(endpoint);
    }

    RconCpp::~RconCpp()
    {
    }

    // TODO: timeout throws
    void RconCpp::authenticate(std::string password)
    {
        pwd = password;

        send(0, SERVERDATA::SERVERDATA_AUTH, password.c_str());
        
        int32_t id = 0;
        int32_t type = 0;
        recv(id, type); // recv "shadow" packet

        std::string received = recv(id, type);
        if (type != 2) 
        {
            throw std::exception("password was not accepted");
        }

        authenticated = true;
    }

    void RconCpp::sendCommand(std::string cmd, int32_t& id)
    {
        id = id_inc;

        try
        {
            send(id_inc, SERVERDATA::SERVERDATA_EXECCOMMAND, cmd.c_str());
        }
        catch (std::exception e)
        {
            throw e;
        }

    }

    std::string RconCpp::recvAnswer(int32_t& id)
    {
        int32_t type;

        return recv(id, type);
    }

    void RconCpp::close()
    {
        socket.close();
    }

    // TODO: timeout throws
    int RconCpp::send(int32_t id, int32_t type, const char* body)
    {
        int32_t groesse = 8 + strlen(body) + 2 + 4;

        char* buffer = new char[groesse];
        memset(buffer, '\0', groesse);

        // TODO: wenn mehr als 4096 bytes gesendet werden, wird das Protokoll anders

        // Size of the Packet
        int32_t size_field = groesse-4;
        memcpy_s(buffer, groesse, &size_field, 4);

        // Id
        int32_t id_field = id;
        memcpy_s(buffer + 4, groesse-4, &id_field, 4);

        // Type
        int32_t type_field = type;
        memcpy_s(buffer + (4*2), groesse-(4*2), &type_field, 4);

        // Body
        memcpy_s(buffer + (4*3), groesse-(4*3), body, strlen(body));

        // Ending Zeros
        memcpy_s(buffer+(4*3)+strlen(body), groesse, "\0\0", 2);

#ifdef _DEBUG
        print_bytes(std::cout, body, (unsigned char*)buffer, groesse);
#endif // _DEBUG

        boost::asio::write(socket, boost::asio::buffer(buffer, groesse));

        delete[] buffer;

        id_inc++;
        if (id_inc >= 15)
        {
            id_inc = 0;
        }
        std::cout << id_inc << std::endl;

        return 0;
    }

    // TODO: timeout throws
    std::string RconCpp::recv(int32_t& id, int32_t& type)
    {
        const size_t buf_size = 4096;

        char buf[buf_size];
        memset(buf, 0, buf_size);   // Das vllt anders machen, da es unnötig performance zieht

        // read following Packet size
        boost::asio::read(socket, boost::asio::buffer(buf, buf_size), boost::asio::transfer_exactly(4));
        int32_t size_len = *(reinterpret_cast<int32_t*>(buf));

        // read Packet
        boost::asio::read(socket, boost::asio::buffer(buf + 4, buf_size - 4), boost::asio::transfer_exactly(size_len));
        
        memcpy_s(&id, 4, buf + 4, 4);
        memcpy_s(&type, 4, buf + 8, 4);

#ifdef _DEBUG
        print_bytes(std::cout, "response", (unsigned char*)buf, size_len + 4);
        std::cout << buf + 12 << std::endl;
#endif // _DEBUG

        return std::string(buf + 12);
    }

}