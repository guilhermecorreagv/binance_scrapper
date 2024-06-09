#include "scrapper.hpp"

namespace beast = boost::beast;
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio;
namespace websocket = beast::websocket;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;

std::filesystem::path getFileName(const std::filesystem::path &fileFolder, std::string_view streamName)
{
    // Get current date
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
#ifdef _WIN32
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif

    // Format date as YYYY-MM-DD
    std::ostringstream oss;
    oss << std::put_time(&buf, "%Y-%m-%d") << "-" << streamName << ".csv";
    std::string filename = oss.str();

    // Combine folder path with filename
    return fileFolder / filename;
}

BinanceScrapper::BinanceScrapper(std::string &host, std::string &port, std::string &endpoint,
                                 std::string &streamName, std::filesystem::path outfolder) : streamName(streamName), resolver(ioc), ctx(ssl::context::sslv23_client), ws(ioc, ctx)
{
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

    // Before subscribing we need to set up the file to be written
    std::filesystem::path filePath = getFileName(outfolder, streamName);

    std::cout << "Storing data in " << filePath << std::endl;

    outfile.open(filePath, std::ios::app);
    if (!outfile.is_open())
    {
        throw std::ios_base::failure("Failed to open file: " + filePath.string());
    }

    // Subscribe to the stream
    // this->subscribeStream(streamName);
}

void BinanceScrapper::subscribeStream(std::string streamName)
{
    std::string request = "{\"method\":\"SUBSCRIBE\",\"params\":[\"" + streamName + "\"],\"id\":1}";
    std::cout << "Request: " << request << std::endl;
    ws.write(net::buffer(request));
    std::cout << "Subscribed to " << streamName << "!\n";
}

void BinanceScrapper::unsubscribeStream(std::string streamName)
{
    std::string request = "{\"method\":\"UNSUBSCRIBE\",\"params\":[\"" + streamName + "\"],\"id\":312}";
    ws.write(net::buffer(request));
    std::cout << "Unsubscribed to" << streamName << "!\n";
}

void BinanceScrapper::run()
{
    // This buffer will hold the incoming message
    beast::flat_buffer buffer;
    beast::error_code ec;
    while (true)
    {
        std::size_t read_bytes = ws.read(buffer, ec);
        if (ec)
        {
            std::cerr << "read: " << ec.message() << "\n";
            return;
        }
        std::cout << "Read " << read_bytes << " bytes!\n";
        std::string s{beast::buffers_to_string(buffer.data())};
        json j{s};

        if (read_bytes > 0)
        {
            std::cout << beast::make_printable(buffer.data()) << std::endl;
        }

        buffer.consume(read_bytes);
    }
}