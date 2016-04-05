#pragma once
#include <asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

using asio::ip::tcp;
using asio::ip::udp;

constexpr size_t bufsize = 1 << 16;

class tcp_con
  : public boost::enable_shared_from_this<tcp_con>
{
public:
  typedef boost::shared_ptr<tcp_con> pointer;

  static pointer create(asio::io_service& io_service){
    return pointer(new tcp_con(io_service));
  }

  tcp::socket& socket(){
    return socket_;
  }

  void start(){
    message_ = make_daytime_string();

    asio::async_write(socket_, asio::buffer(message_),
        boost::bind(&tcp_con::handle_write, shared_from_this()));
  }

private:
  tcp_con(asio::io_service& io_service)
    : socket_(io_service)
  {
  }

  void handle_write()
  {
  }

  tcp::socket socket_;
  std::string message_;
};

class tcp_server
{
public:
  tcp_server(asio::io_service& io_service)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), 13))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    tcp_con::pointer new_connection =
      tcp_con::create(acceptor_.get_io_service());

    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&tcp_server::handle_accept, this, new_connection,
          asio::placeholders::error));
  }

  void handle_accept(tcp_con::pointer new_connection,
      const asio::error_code& error)
  {
    if (!error)
    {
      new_connection->start();
    }

    start_accept();
  }

  tcp::acceptor acceptor_;
};

class udp_server
{
public:
  udp_server(asio::io_service& io_service)
    : socket_(io_service, udp::endpoint(udp::v4(), 13))
  {
    start_receive();
  }

private:
  void start_receive()
  {
    socket_.async_receive_from(
        asio::buffer(recv_buffer_), remote_endpoint_,
        boost::bind(&udp_server::handle_receive, this,
          asio::placeholders::error));
  }

  void handle_receive(const asio::error_code& error)
  {
    if (!error){
      boost::shared_ptr<std::string> message(
          new std::string(make_daytime_string()));

      socket_.async_send_to(asio::buffer(*message), remote_endpoint_,
          boost::bind(&udp_server::handle_send, this, message));

      start_receive();
    }
  }

  void handle_send(boost::shared_ptr<std::string> /*message*/)
  {
  }
  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  boost::array<char, bufsize> recv_buffer_;
};


int main()
{
  try
  {
    asio::io_service io_service;
    tcp_server server1(io_service);
    udp_server server2(io_service);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}