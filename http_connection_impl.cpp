#include "http_connection_impl.hpp"

#include "trace.hpp"

using namespace std::placeholders;

std::string error_body(int error_code, std::string reason) {
    std::string out;
    out = R"---({"error":{"code":)---";
    out += std::to_string(error_code);
    out += R"---(,"message":")---";
    out += reason;
    out += R"---("}})---";
    return out;
}

auto create_decline_request_response(const std::shared_ptr<boost::beast::http::request<boost::beast::http::string_body>> &request, const boost::beast::http::status status, std::string &&body)
{
    boost::beast::http::response<boost::beast::http::string_body> response{status, request->version()};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::content_type, "application/json");
    response.body() = std::move(body);
    response.prepare_payload();
    return response;
}

auto create_success_response(const std::shared_ptr<boost::beast::http::request<boost::beast::http::string_body>> &request, std::string &&body)
{
    boost::beast::http::response<boost::beast::http::string_body> response{boost::beast::http::status::ok, request->version()};
    response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    response.set(boost::beast::http::field::access_control_allow_origin, "*");
    response.set(boost::beast::http::field::content_type, "application/json");
    response.body() = std::move(body);
    response.prepare_payload();
    return response;
}

namespace http {

connection_impl::connection_impl(
        boost::asio::io_service& ioc,
        const uint32_t index,
        const std::shared_ptr<boost::asio::ip::tcp::socket> &tcp,
        const close_cb &on_socket_closed,
        const std::string& detected_devices_json):
        m_index{index},
        m_io_service{ioc},
        m_strand{ioc},
        m_on_socket_closed{on_socket_closed},
        m_tcp{tcp},
        m_disconnect_timer{ioc},
        m_detected_devices_json(detected_devices_json)
{
}

void connection_impl::start()
{
    m_strand.post(std::bind(&connection_impl::read, shared_from_this()));
}

void connection_impl::stop()
{
    m_strand.post(std::bind(&connection_impl::close_socket, shared_from_this()));
}

void connection_impl::read()
{
    if (m_closed) {
        return;
    }

    m_disconnect_timer.expires_from_now(boost::posix_time::seconds(10));
    m_disconnect_timer.async_wait([self = shared_from_this()](const auto &ec){
        if (ec) return;
        self->close_socket();
    });

    auto buffer = std::make_shared<boost::beast::flat_buffer>();
    auto request = std::make_shared<boost::beast::http::request<boost::beast::http::string_body>>();

    boost::beast::http::async_read(*m_tcp, *buffer, *request,
            m_strand.wrap(std::bind(&connection_impl::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2, buffer, request)));
}

void connection_impl::handle_read(
        const boost::system::error_code &ec,
        std::size_t bytes_transferred,
        std::shared_ptr<boost::beast::flat_buffer> const &flat_buffer,
        http_request_ptr const &request)
{
    m_disconnect_timer.cancel();

    if (ec || bytes_transferred == 0) {
        if (ec != boost::beast::websocket::error::closed && ec != boost::asio::error::eof && ec != boost::asio::error::connection_reset &&
        ec != boost::system::errc::operation_canceled && ec != boost::beast::http::error::end_of_stream) {
            TRACE("[" << m_index << "]: " << "Failed to read: " << ec.message().data());
        } else {
            TRACE("[" << m_index << "]: " << "Failed to read: " << ec.message().data());
        }
        close_socket();
        return;
    }

    handle_request(request);
}

void connection_impl::write(boost::beast::http::response<boost::beast::http::string_body> const &response)
{
    if (m_closed) {
        return;
    }

    auto rp = std::make_shared<boost::beast::http::response<boost::beast::http::string_body>>(std::move(response));
    boost::beast::http::async_write(*m_tcp, *rp,
            m_strand.wrap(std::bind(&connection_impl::handle_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2, rp)));
}

void connection_impl::handle_write(const boost::system::error_code &ec, std::size_t bytes_transferred, http_response_ptr const &rp)
{
    if (ec || bytes_transferred == 0) {
        if (ec != boost::system::errc::operation_canceled && ec != boost::asio::error::eof &&
        ec != boost::asio::error::connection_reset && ec != boost::beast::http::error::end_of_stream) {
            TRACE("[" << m_index << "]: " << "Failed to write: " << ec.message().data());
        } else {
            TRACE("[" << m_index << "]: " << "Failed to write: " << ec.message().data());
        }
        close_socket();
        return;
    }

    read();
}

void connection_impl::close_socket()
{
    if (m_closed)
        return;

    m_closed = true;

    TRACE("[" << m_index << "]: Closing socket");

    m_disconnect_timer.cancel();

    if (m_tcp) {
        boost::system::error_code ec;
        m_tcp->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        TRACE("[" << m_index << "]: TCP connection_impl closed");

        m_on_socket_closed(shared_from_this());
    }
}


void connection_impl::handle_request(const http_request_ptr &request)
{
    if (request->target().empty() ||
        request->target()[0] != '/' ||
        request->target().find("..") != boost::beast::string_view::npos) {
        TRACE("Malformed target: " << request->target().to_string().data());
        write(create_decline_request_response(request, boost::beast::http::status::bad_request, error_body(1, "Invalid URL")));
        return;
    }

    TRACE("Received: " << request->target().to_string().data());

    std::string out = m_detected_devices_json; // copy
    //out = R"---({"devices":[]})---";
    write(create_success_response(request, std::move(out)));
}

}
