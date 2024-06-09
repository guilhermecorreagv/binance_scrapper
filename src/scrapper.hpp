#pragma once

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
#include <fstream>
#include <string_view>
#include <chrono>
#include <iomanip>
#include <sstream>

#include <nlohmann/json.hpp>

std::filesystem::path getFileName(const std::filesystem::path &fileFolder, std::string_view streamName);

class BinanceScrapper
{
public:
    BinanceScrapper(std::string &host, std::string &port, std::string &endpoint,
                    std::string &streamName, std::filesystem::path outfolder);
    void subscribeStream(std::string streamName);
    void unsubscribeStream(std::string streamName);
    void run();
    nlohmann::json handleUpdate(std::string_view raw_response);
    ~BinanceScrapper()
    {
        unsubscribeStream(streamName);
        ws.close(boost::beast::websocket::close_code::normal);
    }

private:
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ssl::context ctx;
    boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> ws;
    std::string outfileName;
    std::string streamName;
    std::ofstream outfile;
};
