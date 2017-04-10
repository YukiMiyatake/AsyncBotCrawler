#include"../include/httpx_client.hpp"



template <>
void httpx_client<http_socket>::handle_resolve(const boost::system::error_code& err,
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {

	auto  self(shared_from_this());
	if (!err)
	{
		state_ = httpx::STATE::RESOLVED;

		asioUtil::deadlineOperation2(deadline_timer_, timeout_ms
			, [this, self](const boost::system::error_code &ec) {
			std::cerr << "timeout" << std::endl;
			shutdown_socket_();
		});

		boost::asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
			[this, self](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
			deadline_timer_.cancel();
			handle_connect(ec);
		});
	}
	else
	{
		std::cerr << "Error: " << err.message() << "\n";
	}
}


template <>
void httpx_client<http_socket>::handle_connect(const boost::system::error_code& error)
{
	auto  self(shared_from_this());

	std::cerr << "handle_connect "  "\n";
	if (!error)
	{
		state_ = httpx::STATE::CONNECTED;

		asioUtil::deadlineOperation2(deadline_timer_, timeout_ms
			, [this, self](const boost::system::error_code &ec) {
			std::cerr << "timeout" << std::endl;
			shutdown_socket_();
		});

		boost::asio::async_write(socket_, request_,
			[this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
			deadline_timer_.cancel();
			handle_write_request(ec);
			}
		);
	}
	else
	{
		std::cerr << "Connect failed: " << error.message() << "\n";
	}
}