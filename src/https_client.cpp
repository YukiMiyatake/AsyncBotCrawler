#include"../include/httpx_client.hpp"

bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
{
	std::cerr << "verify_certificate "  "\n";

	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	std::cerr << "Verifying " << subject_name << "  " << preverified << "\n";

	return preverified;
}




template <>
void httpx_client<https_socket>::handle_connect(const boost::system::error_code& error)
{
	auto  self(shared_from_this());

	std::cerr << "handle_connect "  "\n";
	if (!error)
	{
		state_ = httpx::STATE::CONNECTED;

		asioUtil::deadlineOperation2(deadline_timer_, timeout_ms_
			, [this, self](const boost::system::error_code &ec) {
			std::cerr << "timeout" << std::endl;
			shutdown_socket_();
		});

		socket_.async_handshake(boost::asio::ssl::stream_base::client,
			[this, self](const boost::system::error_code& ec) {
				deadline_timer_.cancel();

				// handle_handshake
				std::cerr << "handle_handshake "  "\n";
				state_ = httpx::STATE::HANDSHAKED;
				if (!ec)
				{
					//*
					asioUtil::deadlineOperation2(deadline_timer_, timeout_ms_
						, [this, self](const boost::system::error_code &ec) {
						//			cerr << "timeout" << endl;
						shutdown_socket_();
					});

					boost::asio::async_write(socket_, request_,
						[this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
						deadline_timer_.cancel();
						handle_write_request(ec);
					});
				}
				else
				{
					std::cerr << "Handshake failed: " << ec.message() << "\n";
				}

//			handle_handshake(this,ec);
		});
		/*
		socket_.async_handshake(boost::asio::ssl::stream_base::client,
		boost::bind(&httpx_client::handle_handshake, shared_from_this(),
		boost::asio::placeholders::error));
		*/
	}
	else
	{
		std::cerr << "Connect failed: " << error.message() << "\n";
	}
}

template <>
void httpx_client<https_socket>::handle_resolve(const boost::system::error_code& err,
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {

	auto  self(shared_from_this());
	if (!err)
	{
		state_ = httpx::STATE::RESOLVED;

		// no Verify
		socket_.set_verify_mode(boost::asio::ssl::verify_none);

		socket_.set_verify_callback(verify_certificate);
			//boost::bind(&httpx_client::verify_certificate, this, _1, _2));


		asioUtil::deadlineOperation2(deadline_timer_, timeout_ms_
			, [this, self](const boost::system::error_code &ec) {
			std::cerr << "timeout" << std::endl;
			shutdown_socket_();
		});

		boost::asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
			[this, self](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
			deadline_timer_.cancel();
			handle_connect(ec);
		});

		/*
		boost::asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
		boost::bind(&httpx_client::handle_connect, shared_from_this(),
		boost::asio::placeholders::error));
		*/
	}
	else
	{
		std::cerr << "Error: " << err.message() << "\n";
	}
}



