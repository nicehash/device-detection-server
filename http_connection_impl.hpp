#pragma once

#include "http_connection.hpp"

#include <boost/asio.hpp>

#include <boost/beast.hpp>
#include <memory>
#include <deque>

namespace http {

using http_request_ptr = std::shared_ptr<boost::beast::http::request<boost::beast::http::string_body>>;
using http_response_ptr = std::shared_ptr<boost::beast::http::response<boost::beast::http::string_body>>;

class connection_impl : public connection, public std::enable_shared_from_this<connection_impl> {
public:
    ~connection_impl() override = default;
    connection_impl(
            boost::asio::io_service& ioc,
            const uint32_t index,
            const std::shared_ptr<boost::asio::ip::tcp::socket> &tcp,
            const close_cb &on_socket_closed,
            const std::string& detected_devices_json);

    void start() override;

    void stop() override;

    uint32_t index() const override { return m_index; }

    bool got_quit() const override { return m_got_quit; }

private:

    void write(boost::beast::http::response<boost::beast::http::string_body> const &response);

    void handle_write(const boost::system::error_code &ec, std::size_t bytes_transferred, http_response_ptr const &rp);

    void read();

    void handle_read(
            const boost::system::error_code &ec,
            std::size_t bytes_transferred,
            std::shared_ptr<boost::beast::flat_buffer> const &flat_buffer,
            http_request_ptr const &request);

    void close_socket();

    void handle_request(const http_request_ptr& request);

    const uint32_t m_index;
    boost::asio::io_service& m_io_service;
    boost::asio::io_service::strand m_strand;
    boost::asio::deadline_timer m_disconnect_timer;
    close_cb m_on_socket_closed;

    std::shared_ptr<boost::asio::ip::tcp::socket> m_tcp;
    bool m_closed = false;

    std::string m_detected_devices_json;
    bool m_got_quit = false;
 };

}

