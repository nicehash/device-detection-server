#include "listener_impl.hpp"
#include "trace.hpp"
#include "http_connection_impl.hpp"

#include <algorithm>
#include <numeric>

const uint32_t max_size =  1000000;

namespace http {

listener_impl::listener_impl(boost::asio::io_service &ios, uint32_t id, const std::string &address, uint16_t port, std::string && detected_devices_json, const close_cb& cb):
        m_io_service(ios),
        m_acceptor(ios),
        m_client_strand(ios),
        m_id{id},
        m_bind_address(address),
        m_bind_port(port),
        m_client_ids(max_size),
        m_detected_devices_json(std::move(detected_devices_json)),
        m_cb{cb}
{
    std::iota(m_client_ids.begin(), m_client_ids.end(), m_id * max_size);
}

boost::system::error_code listener_impl::start()
{
    m_acceptor.open(boost::asio::ip::tcp::v4());

    BOOL bOptVal = TRUE;
    int bOptLen = sizeof(BOOL);
    setsockopt(m_acceptor.native_handle(), SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, (char*)&bOptVal, bOptLen);

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::from_string(m_bind_address), m_bind_port);
    try {

        m_acceptor.bind(endpoint);

    } catch (boost::system::error_code &e) {
        TRACE("Cannot bind to http port: " << m_bind_port);
        return e;
    } catch (...) {
        TRACE("Cannot bind to http port: " << m_bind_port);
        return boost::system::errc::make_error_code(boost::system::errc::already_connected);
    }

    m_acceptor.listen();

    TRACE("Listening http: " << m_bind_address.data() << ":" << m_bind_port);

    accept_new_connections();

    return boost::system::error_code();
}

void listener_impl::stop()
{
    m_acceptor.close();
    m_client_strand.post([self = shared_from_this()]() {
        for (auto &c: self->m_client_list) {
            c->stop();
        }
        self->m_acceptor.cancel();
    });
}

void listener_impl::accept_new_connections()
{
    TRACE("Accept new connections: " << m_bind_address.data() << ":" << m_bind_port);

    auto tcp = std::make_shared<boost::asio::ip::tcp::socket>(m_io_service);
    std::weak_ptr<listener_impl> weak_self = shared_from_this();
    m_acceptor.async_accept(*tcp, [weak_self, tcp](const boost::system::error_code &ec) {
        if (auto self = weak_self.lock()) {
            self->handle_accept(ec, tcp);
        }
    });
}

void listener_impl::handle_accept(const boost::system::error_code &ec, const std::shared_ptr<boost::asio::ip::tcp::socket> &tcp)
{
    TRACE("Handle accept connections");
    if (ec) {
        TRACE("Error accepting, ec=" << ec.message().data());
        return;
    }

    auto self = shared_from_this();
    m_client_strand.post([self, tcp]() {
        uint32_t id = self->m_client_ids.front();
        auto cb = std::bind(&listener_impl::remove_connection, self, std::placeholders::_1);
        auto client = std::make_shared<http::connection_impl>(self->m_io_service, id, tcp, cb, self->m_detected_devices_json);

        self->m_client_ids.pop_front();
        self->m_client_list.insert(client);
        client->start();

        self->accept_new_connections();
    });

}

void listener_impl::remove_connection(const std::shared_ptr<connection> &connection)
{
    auto self = shared_from_this();
    m_client_strand.post([self, connection]() {
        self->m_client_ids.push_back(connection->index());
        auto it = self->m_client_list.find(connection);
        if (it != self->m_client_list.end()) {
            self->m_client_list.erase(it);
        }
        self->m_cb();
    });
}

}
