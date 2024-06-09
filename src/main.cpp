#include "scrapper.hpp"

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        std::cerr << "Usage should be: <main> <HOST> <PORT> <ENDPOINT> <SUB_STREAM> <OUT_FOLDER>\n";
        std::cerr << "Example: ./main data-stream.binance.vision 9443 /ws/btcbusd btcbusd@avgPrice ~/Downloads\n";
        return 1;
    }
    // connection params
    std::string host, port, endpoint, streamName;
    std::filesystem::path outfolder;

    host = argv[1];
    port = argv[2];
    endpoint = argv[3];
    streamName = argv[4];
    outfolder = argv[5];

    // Scrapper
    boost::asio::io_context ioc;
    BinanceScrapper scrapper(ioc, host, port, endpoint, streamName, outfolder);
    scrapper.subscribeStream(streamName);
    scrapper.run();
}