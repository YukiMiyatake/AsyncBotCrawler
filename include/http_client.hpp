#pragma once

#include <cstdlib>
#include <iostream>
#include<memory>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>

#include "asioUtil.hpp"


// alias
using http_socket = boost::asio::ip::tcp::socket;
using https_socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

namespace httpx {
	enum class STATE {
		INIT,
		RESOLVED,
		CONNECTED,
		HANDSHAKED,
		WRITTEN_REQUEST,
		READ_STATUS,
		READ_HEADER,
		READING_BODY,
		READ_BODY,
	};
};

// base is http
template<class SOC>
class httpx_client : public std::enable_shared_from_this<httpx_client<SOC>>
{
public:
	using HTTPX_CALLBACK = std::function< void(httpx_client<SOC>&)>;



private:
	std::string server_;
	std::string uri_;
	std::string port_;

	// complete callback
	HTTPX_CALLBACK callback_;


	boost::asio::deadline_timer deadline_timer_;

	boost::asio::streambuf request_;
	boost::asio::streambuf response_;
	boost::asio::ip::tcp::resolver resolver_;
	SOC socket_;

	httpx::STATE state_ = httpx::STATE::INIT;

	unsigned int status_code_ = 0;
	std::ostringstream message_header_;
	std::ostringstream message_body_;

	// shutdown function is different in HTTP and HTTPX
	std::function<void(void)>  shutdown_socket_;

	boost::asio::io_service &io_service_;
public:
	unsigned int timeout_ms_ = 1000;

public:
	// HTTP constructor
	httpx_client(boost::asio::io_service& io_service,
		const std::string& server, const std::string& uri, const std::string& port = "http")
		: io_service_(io_service), resolver_(io_service), socket_(io_service), server_(server), uri_(uri), port_(port), deadline_timer_(io_service) {

		shutdown_socket_ = [this]() { socket_.shutdown(boost::asio::socket_base::shutdown_type::shutdown_send); };
	}

	// HTTPX constructor
	httpx_client(boost::asio::io_service& io_service, boost::asio::ssl::context& context,
		const std::string& server, const std::string& uri, const std::string& port = "https")
		: io_service_(io_service), resolver_(io_service), socket_(io_service, context), server_(server), uri_(uri), port_(port), deadline_timer_(io_service) {

		shutdown_socket_ = [this]() { socket_.shutdown(); };
	}


	void go(HTTPX_CALLBACK callback) {

		auto  self(shared_from_this());

		callback_ = callback;
		state_ = httpx::STATE::INIT;

		tcp::resolver::query query(server_, port_);

		asioUtil::deadlineOperation2(deadline_timer_, timeout_ms_
		, [this, self](const boost::system::error_code &ec) {
			cerr << "timeout" << endl;
			shutdown_socket_();
		});

		resolver_.async_resolve(query
			, [this, self](const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator)
		{
			deadline_timer_.cancel();
			handle_resolve(err, endpoint_iterator);
		});

	}


	void handle_resolve(const boost::system::error_code& err,
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {

		auto  self(shared_from_this());
		if (!err)
		{
			state_ = httpx::STATE::RESOLVED;

			asioUtil::deadlineOperation2(deadline_timer_, timeout_ms_
				, [this, self](const boost::system::error_code &ec) {
				cerr << "timeout" << endl;
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

	bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
	{
		std::cerr << "verify_certificate "  "\n";

		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		std::cerr << "Verifying " << subject_name << "  " << preverified << "\n";

		return preverified;
	}

	void handle_connect(const boost::system::error_code& error)
	{
		auto  self(shared_from_this());

		std::cerr << "handle_connect "  "\n";
		if (!error)
		{
			state_ = httpx::STATE::CONNECTED;

			asioUtil::deadlineOperation2(deadline_timer_, timeout_ms_
				, [this, self](const boost::system::error_code &ec) {
				cerr << "timeout" << endl;
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
			std::cerr << "Connect failed: " << error.message() << "\n";
		}
	}

	void handle_handshake(const boost::system::error_code& error)
	{
		// dammy
	}

	void handle_write_request(const boost::system::error_code& err)
	{
		auto  self(shared_from_this());

		std::cout << "handle_write_request "  "\n";
		if (!err)
		{
			state_ = httpx::STATE::WRITTEN_REQUEST;

			asioUtil::deadlineOperation2(deadline_timer_, timeout_ms_
				, [this, self](const boost::system::error_code &ec) {
				cerr << "timeout" << endl;
				shutdown_socket_();
			});

			boost::asio::async_read_until(socket_, response_, "\r\n",
				[this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
				deadline_timer_.cancel();
				handle_read_status_line(ec);
			});
		}
		else
		{
			std::cerr << "Error: " << err.message() << "\n";
		}
	}

	void handle_read_status_line(const boost::system::error_code& err)
	{
		auto  self(shared_from_this());

		std::cerr << "handle_read_status_line "  "\n";
		if (!err)
		{
			std::istream response_stream(&response_);
			std::string http_version;
			response_stream >> http_version;
			response_stream >> status_code_;
			std::string status_message;
			std::getline(response_stream, status_message);
			if (!response_stream || http_version.substr(0, 5) != "HTTP/")
			{
				std::cerr << "Invalid response\n";
				return;
			}
			if (status_code_ != 200)
			{
				std::cerr << "Response returned with status code ";
				std::cerr << status_code_ << "\n";
				return;
			}

			state_ = httpx::STATE::READ_STATUS;

			asioUtil::deadlineOperation2(deadline_timer_, timeout_ms_
				, [this, self](const boost::system::error_code &ec) {
				cerr << "timeout" << endl;
				shutdown_socket_();
			});

			boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
				[this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
				deadline_timer_.cancel();
				handle_read_headers(ec);
			});
		}
		else
		{
			std::cerr << "Error: " << err << "\n";
		}
	}


	void handle_read_headers(const boost::system::error_code& err)
	{
		auto  self(shared_from_this());

		std::cerr << "handle_read_headers "  "\n";
		if (!err)
		{
			// 
			std::istream response_stream(&response_);
			std::string header;
			while (std::getline(response_stream, header) && header != "\r")
			{
				message_header_ << header << "\n";
				//std::cerr << header << "\n";
			}
			//			std::cerr << "\n";

			//			cerr << message_header_.str();
			state_ = httpx::STATE::READ_HEADER;



			asioUtil::deadlineOperation(deadline_timer_, timeout_ms_
			
				, [this,self]() {
				boost::asio::async_read(socket_, response_,
					boost::asio::transfer_at_least(1),
					[this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
					deadline_timer_.cancel();
					handle_read_content(ec);
				});
				}
				, [this, self](const boost::system::error_code &ec) {
					cerr << "timeout" << endl;
					shutdown_socket_();
			});



		}
		else
		{
			std::cerr << "Error: " << err << "\n";
		}
	}

	void handle_read_content(const boost::system::error_code& err)
	{
		auto  self(shared_from_this());
		std::cerr << "handle_read_content "  "\n";
		//		if (err == boost::asio::error::eof)
		if (err)
		{

			//			cerr << &response_;
			message_body_ << &response_;


			state_ = httpx::STATE::READ_BODY;
			//	cerr << message_body_.str();
			[this, self]() {callback_(*this); }();
			//	callback_(*this);

		}
		else if (!err)
		{
			//			cerr << &response_;
			state_ = httpx::STATE::READING_BODY;
			message_body_ << &response_;

			response_.consume(response_.size());

			auto timer = std::make_shared<asioUtil::deadlineOperation3>(io_service_, 1000,
				[this,self](const boost::system::error_code &ec) {
					cerr << "timeout" << endl;
					[this, self]() { callback_(*this); }();
					shutdown_socket_();
				}
			);

			boost::asio::async_read(socket_, response_,
				boost::asio::transfer_at_least(1),
				[this, self, timer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
				handle_read_content(ec);
			});

		
		}
	}

	httpx::STATE get_status() { return(state_); };

	void set_header(std::string header) {
		request_.consume(request_.size());
		std::ostream request_stream(&request_);
		request_stream << header;
	}

	unsigned int get_status_code() { return(status_code_); };

	std::ostringstream& get_header() { return(message_header_); };
	std::ostringstream& get_body() { return(message_body_); };
	
};




//*

template <>
void httpx_client<https_socket>::handle_handshake(const boost::system::error_code& error)
{
	auto  self(shared_from_this());

	std::cerr << "handle_handshake "  "\n";
	state_ = httpx::STATE::HANDSHAKED;
	if (!error)
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
//*/
/*
		boost::asio::async_write(socket_, request_,
			boost::bind(&httpx_client::handle_write_request, shared_from_this(),
				boost::asio::placeholders::error));
//*/
	}
	else
	{
		std::cerr << "Handshake failed: " << error.message() << "\n";
	}

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
			handle_handshake(ec);
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

		socket_.set_verify_callback(
			boost::bind(&httpx_client::verify_certificate, this, _1, _2));


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

