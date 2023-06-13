#include "RconCpp.h"


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

namespace RconCpp {

    RconCpp::RconCpp(int32_t p_port, std::string p_ip) : port(p_port), ip(p_ip), socket(service), authenticated(false), id_inc(0), connected(false)
    {

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

    void RconCpp::authenticate(std::string password)
    {
        if (!connected) 
        {
            throw std::runtime_error("There is no connection to the server");
        }

        pwd = password;

        send(0, SERVERDATA::SERVERDATA_AUTH, password.c_str());
        
        int32_t id = 0;
        int32_t type = 0;
        recv(id, type); // recv "shadow" packet

        std::string received = recv(id, type);
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

    std::string RconCpp::recvAnswer(int32_t& id)
    {
        int32_t type;

        try
        {
            return recv(id, type);
        }
        catch (const std::runtime_error e)
        {
            return std::string("error");
        }
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

        run(std::chrono::seconds(5));

        // Couldnt connect
        if (error)
        {
            throw std::runtime_error("Cant connect to Server");
        }

        connected = true;
    }

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
        run(std::chrono::seconds(5));

        // Determine whether the read completed successfully.
        if (error)
        {
            delete[] buffer;
            throw std::runtime_error("Timeout");
        }

        delete[] buffer;

        id_inc++;
        if (id_inc >= 15)
        {
            id_inc = 0;
        }

        return 0;
    }

    std::string RconCpp::recv(int32_t& id, int32_t& type)
    {
        const size_t buf_size = 4096;

        char buf[buf_size];
        memset(buf, 0, buf_size);   // Das vllt anders machen, da es unnötig performance zieht

        // read following Packet size
        boost::system::error_code error;
        boost::asio::async_read(socket, boost::asio::buffer(buf, buf_size), boost::asio::transfer_exactly(4),
            [&](const boost::system::error_code& result_error,
                std::size_t /*result_n*/)
            {
                error = result_error;
            });

        // Run the operation until it completes, or until the timeout.
        run(std::chrono::seconds(5));

        // Determine whether the read completed successfully.
        if (error)
        {
            throw std::runtime_error("Timeout");
        }


        int32_t size_len = *(reinterpret_cast<int32_t*>(buf));

        // read Packet
        boost::system::error_code error2;
        boost::asio::async_read(socket, boost::asio::buffer(buf + 4, buf_size - 4), boost::asio::transfer_exactly(size_len),
            [&](const boost::system::error_code& result_error,
                std::size_t /*result_n*/)
            {
                error2 = result_error;
            });

        // Run the operation until it completes, or until the timeout.
        run(std::chrono::seconds(5));

        // Determine whether the read completed successfully.
        if (error2)
        {
            throw std::runtime_error("Timeout");
        }

        memcpy_s(&id, 4, buf + 4, 4);
        memcpy_s(&type, 4, buf + 8, 4);

#ifdef OUTPUT_DEBUG
        print_bytes(std::cout, "response", (unsigned char*)buf, size_len + 4);
        std::cout << buf + 12 << std::endl;
#endif // OUTPUT_DEBUG

        return std::string(buf + 12);
    }

}
