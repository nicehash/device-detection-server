#pragma once

#include "listener.hpp"
#include "http_connection.hpp"

#include <boost/asio.hpp>

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_set>
#include <deque>

namespace http {

class listener_impl : public listener, public std::enable_shared_from_this<listener_impl> {
public:
    ~listener_impl() override = default;

    listener_impl(boost::asio::io_service&, uint32_t id, const std::string &address, uint16_t port);

    boost::system::error_code start() override;

    void stop() override;

private:
    void accept_new_connections();

    void handle_accept(const boost::system::error_code &ec, const std::shared_ptr<boost::asio::ip::tcp::socket> &socket);

    void remove_connection(const std::shared_ptr<connection> &connection);

    boost::asio::io_service &m_io_service;
    boost::asio::ip::tcp::acceptor m_acceptor;
    boost::asio::io_service::strand m_client_strand;

    uint32_t m_id;
    std::string m_bind_address;
    uint16_t m_bind_port;

    std::unordered_set<std::shared_ptr<connection>> m_client_list;
    std::deque<uint32_t> m_client_ids;
};
}


