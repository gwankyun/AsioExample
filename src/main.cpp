#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif // _WIN32
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <ctime>

using namespace std;
namespace asio = boost::asio;

class Session : public std::enable_shared_from_this<Session>
{
public:
    explicit Session(asio::io_context& io_context, asio::ip::tcp::socket socket)
        : socket_(std::move(socket)),
        strand_(io_context.get_executor())
    {
    }

    void go()
    {
        auto self(shared_from_this());
        asio::spawn(strand_,
            [this, self](asio::yield_context yield)
        {
            try
            {
                auto re = socket_.remote_endpoint();
                cout << "address: " << re.address() << endl;
                cout << "port: " << re.port() << endl;

                time_t rawtime;
                time(&rawtime);
                struct tm* timeinfo = localtime(&rawtime);
                string now(asctime(timeinfo));

                socket_.async_write_some(boost::asio::buffer(now), yield);
            }
            catch (std::exception & e)
            {
                socket_.close();
            }
        });
    }

private:
    asio::ip::tcp::socket socket_;
    asio::strand<asio::io_context::executor_type> strand_;
};

int main(int argc, char* argv[])
{
    try
    {
        asio::io_context io_context;

        asio::spawn(io_context,
            [&](asio::yield_context yield)
        {
            asio::ip::tcp::acceptor acceptor(io_context,
                asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 8888));

            while (true)
            {
                boost::system::error_code ec;
                asio::ip::tcp::socket socket(io_context);
                acceptor.async_accept(socket, yield[ec]);
                if (!ec)
                {
                    std::make_shared<Session>(io_context, std::move(socket))->go();
                }
            }
        });

        io_context.run();
    }
    catch (exception & e)
    {
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;
}
