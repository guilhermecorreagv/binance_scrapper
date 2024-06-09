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
#include <string>
#include <filesystem>

#include <nlohmann/json.hpp>

class BinanceScrapper
{
public:
    BinanceScrapper(std::string host, std::string port, std::string endpoint, std::string streamName, std::filesystem::path outfolder);
    void subscribeStream(std::string streamName);
    void unsubscribeStream(std::string streamName);
    void run();
    nlohmann::json handleUpdate(std::string raw_response);
    ~BinanceScrapper()
    {
        ws.close(boost::beast::websocket::close_code::normal);
    }

private:
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::resolver resolver{ioc};
    boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23_client};
    boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> ws{ioc, ctx};
};
