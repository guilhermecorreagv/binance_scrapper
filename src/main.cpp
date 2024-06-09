#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <string>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace beast = boost::beast;
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio;
namespace websocket = beast::websocket;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage should be: <main> <HOST> <PORT> <ENDPOINT> <SUB_STREAM> <OUT_FOLDER>\n";
        std::cerr << "Example: ./main data-stream.binance.vision 9443 /ws/btcbusd btcbusd@avgPrice ~/Downloads\n";
        return 1;
    }
    // connection params
    std::string host = argv[1];
    auto port = argv[2];
    std::string endpoint = "/";
    if (argc == 4)
        endpoint = argv[3];

    net::io_context ioc; // The io_context is required for all I/O
    ssl::context ctx{ssl::context::sslv23_client};

    // These objects perform our I/O
    websocket::stream<ssl::stream<tcp::socket>> ws{ioc, ctx};
    tcp::resolver resolver(ioc);

    // Look up the domain name
    auto const results = resolver.resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    auto ep = net::connect(beast::get_lowest_layer(ws), results);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
        throw beast::system_error(
            beast::error_code(
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category()),
            "Failed to set SNI Hostname");

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    host += ':' + std::to_string(ep.port());

    // Perform the SSL handshake
    ws.next_layer().handshake(ssl::stream_base::client);
    std::cout << "Did SSL handshake!\n";

    // Set a decorator to change the User-Agent of the handshake
    ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type &req)
        {
            req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-coro");
        }));

    // Perform the websocket handshake
    ws.handshake(host, endpoint.c_str());
    std::cout << "Did Websocket handshake!\n";

    // Send the message
    std::string request = "{\"method\":\"SUBSCRIBE\",\"params\":[\"btcusdt@avgPrice\"],\"id\":1}";
    ws.write(net::buffer(request));
    std::cout << "Sent request!\n";

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    for (int i = 0; i < 100; i++)
    {
        std::size_t read_bytes = ws.read(buffer);
        std::cout << "Read " << read_bytes << " bytes!\n";
        std::string s{beast::buffers_to_string(buffer.data())};
        json j{s};
        // for (auto &[key, value] : j.items())
        //     std::cout << "Key: " << key << "\nValue: " << value << std::endl;
        // if (i == 10)
        //     break;

        if (read_bytes > 0)
        {
            std::cout << beast::make_printable(buffer.data()) << std::endl;
        }
        usleep(3000000);

        buffer.consume(read_bytes);
    }
    // Read a message into our buffer

    // Close the WebSocket connection
    ws.close(websocket::close_code::normal);

    // If we get here then the connection is closed gracefully
}