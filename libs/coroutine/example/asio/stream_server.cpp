#define NOMINMAX

#include <cstdlib>
#include <cstring>
#include <iterator>
#include <iostream>
#include <streambuf>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/coroutine/all.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/utility.hpp>

typedef boost::tuple< boost::system::error_code, std::size_t >                      tuple_t;
typedef boost::coroutines::coroutine< void( boost::system::error_code, std::size_t) >     coro_t;

class inbuf : public std::streambuf,
              private boost::noncopyable
{
private:
    static const std::streamsize        pb_size;

    enum
    { bf_size = 16 };

    int fetch_()
    {
        std::streamsize num = std::min(
            static_cast< std::streamsize >( gptr() - eback() ), pb_size);

        std::memmove(
            buffer_ + ( pb_size - num),
            gptr() - num, num);

        s_.async_read_some(
                boost::asio::buffer( buffer_ + pb_size, bf_size - pb_size),
                boost::bind( & coro_t::operator(), & coro_, _1, _2) );
        ca_();

        boost::system::error_code ec;
        std::size_t n = 0;

        boost::tie( ec, n) = ca_.get();
        if ( ec)
        {
            setg( 0, 0, 0);
            return -1;
        }

        setg( buffer_ + pb_size - num, buffer_ + pb_size, buffer_ + pb_size + n);
        return n;
    }

    boost::asio::ip::tcp::socket      &   s_;
    coro_t                            &   coro_;
    coro_t::caller_type                  &   ca_;
    char                                  buffer_[bf_size];

protected:
    virtual int underflow()
    {
        if ( gptr() < egptr() )
            return traits_type::to_int_type( * gptr() );

        if ( 0 > fetch_() )
            return traits_type::eof();
        else
            return traits_type::to_int_type( * gptr() );
    }

public:
    inbuf(
            boost::asio::ip::tcp::socket & s,
            coro_t & coro,
            coro_t::caller_type & ca) :
        s_( s), coro_( coro), ca_( ca), buffer_()
    { setg( buffer_ + 4, buffer_ + 4, buffer_ + 4); }
};
const std::streamsize inbuf::pb_size = 4;

class session : private boost::noncopyable
{
private:
    void handle_read_( coro_t::caller_type & ca)
    {
        inbuf buf( socket_, coro_, ca);
        std::istream s( & buf);

        std::string msg;
        do
        {
            std::getline( s, msg);
            std::cout << msg << std::endl; 
        } while ( "exit" != msg);
        io_service_.post(
            boost::bind(
                & session::destroy_, this) );
    }

    void destroy_()
    { delete this; }

    coro_t                          coro_;
    boost::asio::io_service     &   io_service_;
    boost::asio::ip::tcp::socket    socket_;

public:
    session( boost::asio::io_service & io_service) :
        coro_(),
        io_service_( io_service),
        socket_( io_service_)
    {}

    boost::asio::ip::tcp::socket & socket()
    { return socket_; }

    void start()
    { coro_ = coro_t( boost::bind( & session::handle_read_, this, _1) ); }
};

class server : public boost::enable_shared_from_this< server >
{
private:
    boost::asio::io_service         &   io_service_;
    boost::asio::ip::tcp::acceptor      acceptor_;

    void handle_accept_( session * new_session, boost::system::error_code const& error)
    {
        if ( ! error)
        {
            new_session->start();
            start();
        }
    }

    server( boost::asio::io_service & io_service, short port) :
        io_service_( io_service),
        acceptor_(
            io_service_,
            boost::asio::ip::tcp::endpoint( boost::asio::ip::tcp::v4(), port) )
    {}

public:
    typedef boost::shared_ptr< server > ptr_t;

    static ptr_t create( boost::asio::io_service & io_service, short port)
    { return ptr_t( new server( io_service, port) ); }

    void start()
    {
        session * new_session( new session( io_service_) );
        acceptor_.async_accept(
            new_session->socket(),
            boost::bind( & server::handle_accept_, this->shared_from_this(),
                new_session, boost::asio::placeholders::error) );
    }
};

int main( int argc, char * argv[])
{
    try
    {
        int port = 0;
        boost::program_options::options_description desc("allowed options");
        desc.add_options()
            ("help,h", "help message")
            ("port,p", boost::program_options::value< int >( & port), "port service is listening");

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
        {
            boost::asio::io_service io_service;
            io_service.post(
                boost::bind(
                    & server::start,
                    server::create(
                        io_service, port) ) );
            io_service.run();
        }
        std::cout << "Done" << std::endl;

        return  EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "Exception: " << e.what() << std::endl; }

    return EXIT_FAILURE;
}

