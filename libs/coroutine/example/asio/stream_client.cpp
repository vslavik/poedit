#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>

int main( int argc, char* argv[])
{
    try
    {
        std::string host, msg, port;
        int timeout = 1;
        boost::program_options::options_description desc("allowed options");
        desc.add_options()
            ("help,h", "help message")
            ("host,a", boost::program_options::value< std::string >( & host), "host running the service")
            ("port,p", boost::program_options::value< std::string >( & port), "port service is listening")
            ("message,m", boost::program_options::value< std::string >( & msg), "message to send")
            ("timeout,t", boost::program_options::value< int >( & timeout), "timeout between message 'exit' message in seconds");

        boost::program_options::variables_map vm;
        boost::program_options::store(
            boost::program_options::parse_command_line(
                argc,
                argv,
                desc),
            vm);
        boost::program_options::notify( vm);
 
        if ( vm.count("help") ) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }

        boost::asio::io_service io_service;

        boost::asio::ip::tcp::resolver resolver( io_service);
        boost::asio::ip::tcp::resolver::query query( boost::asio::ip::tcp::v4(), host, port);
        boost::asio::ip::tcp::resolver::iterator iterator( resolver.resolve( query) );

        boost::asio::ip::tcp::socket s( io_service);
        s.connect( * iterator);

        msg.append("\n"); // each message is terminated by newline
        boost::asio::write( s, boost::asio::buffer( msg, msg.size() ) );
        std::cout << msg << " sendet" << std::endl;
        boost::this_thread::sleep( boost::posix_time::seconds( timeout) );
        std::string exit("exit\n"); // newline
        boost::asio::write( s, boost::asio::buffer( exit, exit.size() ) );
        std::cout << exit << " sendet" << std::endl;

        std::cout << "Done" << std::endl;
        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return EXIT_FAILURE;
}
