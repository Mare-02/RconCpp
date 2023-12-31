#include "RconCpp.h"

namespace RCONCPP 
{
    #ifdef OUTPUT_DEBUG
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
    #endif
    
    bool is_big_endian(void)
    {
        union {
            uint32_t i;
            char c[4];
        } bint = { 0x01020304 };

        return bint.c[0] == 1;
    }

    RconCpp::RconCpp(int32_t p_port, std::string p_ip, unsigned pTimeout) : port(p_port), ip(p_ip), socket(service), authenticated(false), id_inc(0), connected(false), timeout(pTimeout)
    {
        big_endian = is_big_endian();
    }

    RconCpp::~RconCpp()
    {
        close();
    }

    void RconCpp::run(std::chrono::steady_clock::duration timeout)
    {
        // Restart the io_context, as it may have been left in the "stopped" state
        // by a previous operation.
        service.restart();

        // Block until the asynchronous operation has completed, or timed out. If
        // the pending asynchronous operation is a composed operation, the deadline
        // applies to the entire operation, rather than individual operations on
        // the socket.
        service.run_for(timeout);

        // If the asynchronous operation completed successfully then the io_context
        // would have been stopped due to running out of work. If it was not
        // stopped, then the io_context::run_for call must have timed out.
        if (!service.stopped())
        {
            // Close the socket to cancel the outstanding asynchronous operation.
            socket.close();

            // Run the io_context again until the operation completes.
            service.run();
        }
    }

    // throws exception on failure
    void RconCpp::authenticate(std::string password)
    {
        authenticated = false;

        if (!connected)
        {
            throw std::runtime_error("There is no connection to the server");
        }

        pwd = password;

        send(0, SERVERDATA::SERVERDATA_AUTH, password.c_str());

        int32_t id = 0;
        int32_t type = 0;
        int32_t packet_size = 0;

#ifdef AUTH_ERROR
        recv(id, type, packet_size); // recv "shadow" packet
#endif

        packet_size = 0;

        std::string received = recv(id, type, packet_size);
        if (type != 2)
        {
            throw std::runtime_error("Password is incorrect");
        }

        authenticated = true;
    }

    int RconCpp::sendCommand(std::string cmd, int32_t& id)
    {
        id = id_inc;

        try
        {
            send(id_inc, SERVERDATA::SERVERDATA_EXECCOMMAND, cmd.c_str());
        }
        catch (const std::runtime_error e)
        {
            return -1;
        }

        return 0;
    }

    // returns empty String on failure
    std::string RconCpp::recvAnswer(int32_t& id)
    {
        int32_t type = 0;
        int32_t packet_size = 0;

        std::string rueckgabe = recv(id, type, packet_size);
        while (packet_size > 4000) 
        {
            rueckgabe += recv(id, type, packet_size);
        }
        return rueckgabe;
    }

    void RconCpp::close()
    {
        if (connected)
        {
            socket.close();
        }
        connected = false;
    }

    void RconCpp::connect()
    {
        auto endpoints = boost::asio::ip::tcp::resolver(service).resolve(ip, std::to_string(port));

        boost::system::error_code error;
        boost::asio::async_connect(socket, endpoints,
            [&](const boost::system::error_code& result_error,
                const boost::asio::ip::tcp::endpoint& /*result_endpoint*/)
            {
                error = result_error;
            });

        run(std::chrono::seconds(timeout));

        // Couldnt connect
        if (error)
        {
            throw std::runtime_error("Cant connect to Server");
        }

        connected = true;
    }

    // return "error on failure"
    std::string RconCpp::sendAndRecv(std::string cmd)
    {
        int32_t id = 0;

        if (sendCommand(cmd, id) != 0)
        {
            return std::string("error");
        }

        return recvAnswer(id);
    }

    int RconCpp::send(int32_t id, int32_t type, const char* body)
    {
        int32_t groesse = 8 + strlen(body) + 2 + 4;

        char* buffer = new char[groesse];
        memset(buffer, '\0', groesse);

        // Size of the Packet
        int32_t size_field = groesse - 4;
        if (big_endian)
        {
            size_field = _byteswap_ulong(size_field);
        }
        memcpy_s(buffer, groesse, &size_field, 4);

        // Id
        int32_t id_field = id;
        if (big_endian)
        {
            id_field = _byteswap_ulong(id_field);
        }
        memcpy_s(buffer + 4, groesse - 4, &id_field, 4);

        // Type
        int32_t type_field = type;
        if (big_endian)
        {
            type_field = _byteswap_ulong(type_field);
        }
        memcpy_s(buffer + (4 * 2), groesse - (4 * 2), &type_field, 4);

        // Body
        memcpy_s(buffer + (4 * 3), groesse - (4 * 3), body, strlen(body));

        // Ending Zeros
        memcpy_s(buffer + (4 * 3) + strlen(body), groesse, "\0\0", 2);

#ifdef OUTPUT_DEBUG
        print_bytes(std::cout, body, (unsigned char*)buffer, groesse);
#endif // OUTPUT_DEBUG

        boost::system::error_code error;
        boost::asio::async_write(socket, boost::asio::buffer(buffer, groesse),
            [&](const boost::system::error_code& result_error,
                std::size_t /*result_n*/)
            {
                error = result_error;
            });

        // Run the operation until it completes, or until the timeout.
        run(std::chrono::seconds(timeout));

        // Determine whether the read completed successfully.
        if (error)
        {
            delete[] buffer;
            throw std::runtime_error("Timeout");
        }

        delete[] buffer;

        id_inc = id_inc + 1;
        if (id_inc >= 1000)
        {
            id_inc = 0;
        }

        return 0;
    }

    std::string RconCpp::recv(int32_t& id, int32_t& type, int32_t& packet_size)
    {
        const size_t buf_size = 4096;

        char buf[buf_size];
        memset(buf, 0, buf_size);   // Das vllt anders machen, da es unn�tig performance zieht

        // read following Packet size
        boost::system::error_code error;
        boost::asio::async_read(socket, boost::asio::buffer(buf, buf_size), boost::asio::transfer_exactly(4),
            [&](const boost::system::error_code& result_error,
                std::size_t /*result_n*/)
            {
                error = result_error;
            });

        // Run the operation until it completes, or until the timeout.
        run(std::chrono::seconds(timeout));

        // Determine whether the read completed successfully.
        if (error)
        {
            return "";
        }

        int32_t size_len = *(reinterpret_cast<int32_t*>(buf));
        packet_size = size_len;
        if (big_endian)
        {
            size_len = _byteswap_ulong(*(reinterpret_cast<int32_t*>(buf)));
        }

        // read Packet
        boost::system::error_code error2;
        boost::asio::async_read(socket, boost::asio::buffer(buf + 4, buf_size - 4), boost::asio::transfer_exactly(size_len),
            [&](const boost::system::error_code& result_error,
                std::size_t /*result_n*/)
            {
                error2 = result_error;
            });

        // Run the operation until it completes, or until the timeout.
        run(std::chrono::seconds(timeout));

        // Determine whether the read completed successfully.
        if (error2)
        {
            return "";
        }

        memcpy_s(&id, 4, buf + 4, 4);
        memcpy_s(&type, 4, buf + 8, 4);
        if (big_endian)
        {
            id = _byteswap_ulong(id);
            type = _byteswap_ulong(type);
        }

#ifdef OUTPUT_DEBUG
        print_bytes(std::cout, "response", (unsigned char*)buf, size_len + 4);
        std::cout << buf + 12 << std::endl;
#endif // OUTPUT_DEBUG

        return std::string(buf + 12);
    }

}