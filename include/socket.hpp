#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>

#include<iostream>

#include "asioUtil.hpp"

namespace abc {

using http_socket = boost::asio::ip::tcp::socket;
using https_socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

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



	template<typename SOC>
	class crawler : public std::enable_shared_from_this<crawler<SOC>>
	{
	public:
		using Socket = SOC;

	public:
		using HTTPX_CALLBACK = std::function< void(crawler<SOC>&)>;



	private:
		boost::asio::io_service &io_service_;

		std::string server_;
		std::string uri_;
		std::string port_;

		// complete callback
		HTTPX_CALLBACK complete_handler_;


		boost::asio::deadline_timer deadline_timer_;

		boost::asio::streambuf request_;
		boost::asio::streambuf response_;
		boost::asio::ip::tcp::resolver resolver_;
		Socket socket_;

		STATE state_ = STATE::INIT;

		unsigned int status_code_ = 0;
		std::ostringstream message_header_;
		std::ostringstream message_body_;

		// shutdown function is different in HTTP and HTTPS
		std::function<void(void)>  shutdown_socket_;

	public:
		unsigned int timeout_ms = 1000;

	public:
		// HTTP constructor
		//template<typename SOC>
//		crawler(SOC soc, std::string server, std::string port/* = "http"*/);

	template< class S>
	crawler(boost::asio::io_service& io_service, 
		S&& server, S&& uri, S&& port)
		: io_service_(io_service), resolver_(io_service), socket_(io_service), 
		server_(std::forward<S>(server)), 
		uri_(std::forward<S>(uri)), 
		port_(std::forward<S>(port)), 
		deadline_timer_(io_service) {

		shutdown_socket_ = [this]() { socket_.shutdown(boost::asio::socket_base::shutdown_type::shutdown_send); };
	}

	template< class S, class T>
	crawler(boost::asio::io_service& io_service, T&& context,
		 S&& server, S&& uri, S&& port)
		: io_service_(io_service), resolver_(io_service), 
		socket_(io_service, std::forward<T>(context)), 
		server_(std::forward<S>(server)), 
		uri_(std::forward<S>(uri)), port_(std::forward<S>(port)), 
		deadline_timer_(io_service) {

		shutdown_socket_ = [this]() { socket_.shutdown(); };
	}



/*		
		crawler(SOC&& soc,
			std::string server, std::string port = "http")
			: io_service_((soc.get_io_service()))
			, resolver_(io_service_)
			, socket_(std::forward<SOC>(soc))
			, server_((server))
			, port_((port)), deadline_timer_(io_service_)
			, shutdown_socket_([this]() { socket_.shutdown(boost::asio::socket_base::shutdown_type::shutdown_send); })

		{
		}
*/

		std::shared_ptr<crawler> getSelf(){
			return this->shared_from_this();
		}

		void request(HTTPX_CALLBACK callback) {

			auto self(getSelf());

			complete_handler_ = callback;
			state_ = STATE::INIT;

			boost::asio::ip::tcp::resolver::query query(server_, port_);

			asioUtil::deadlineOperation2(deadline_timer_, timeout_ms
				, [this, self](const boost::system::error_code &ec) {
				std::cerr << "timeout" << std::endl;
				shutdown_socket_();
			});

			resolver_.async_resolve(query
				, [this, self](const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
			{
				deadline_timer_.cancel();
				handle_resolve(err, endpoint_iterator);
			});

		}


		void handle_resolve(const boost::system::error_code& err,
			boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

		//	bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);

		void handle_connect(const boost::system::error_code& error);

		//	void handle_handshake(const boost::system::error_code& error);

		void handle_write_request(const boost::system::error_code& err)
		{
			auto self(getSelf());

			std::cout << "handle_write_request "  "\n";
			if (!err)
			{
				state_ = STATE::WRITTEN_REQUEST;

				asioUtil::deadlineOperation2(deadline_timer_, timeout_ms
					, [this, self](const boost::system::error_code &ec) {
					std::cerr << "timeout" << std::endl;
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
			auto self(getSelf());


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

				state_ = STATE::READ_STATUS;

				asioUtil::deadlineOperation2(deadline_timer_, timeout_ms
					, [this, self](const boost::system::error_code &ec) {
					std::cerr << "timeout" << std::endl;
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
			auto self(getSelf());

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

				//			std::cerr << message_header_.str();
				state_ = STATE::READ_HEADER;



				asioUtil::deadlineOperation(deadline_timer_, timeout_ms

					, [this, self]() {
					boost::asio::async_read(socket_, response_,
						boost::asio::transfer_at_least(1),
						[this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
						deadline_timer_.cancel();
						handle_read_content(ec);
					});
				}
					, [this, self](const boost::system::error_code &ec) {
					std::cerr << "timeout" << std::endl;
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
			auto self(getSelf());
			std::cerr << "handle_read_content "  "\n";
			//		if (err == boost::asio::error::eof)
			if (err)
			{

				//			std::cerr << &response_;
				message_body_ << &response_;


				state_ = STATE::READ_BODY;
				//	std::cerr << message_body_.str();
				[this, self]() {complete_handler_(*this); }();
				//	complete_handler_(*this);

			}
			else if (!err)
			{
				//			std::cerr << &response_;
				state_ = STATE::READING_BODY;
				message_body_ << &response_;

				response_.consume(response_.size());

				auto timer = std::make_shared<asioUtil::deadlineOperation3>(io_service_, 1000,
					[this, self](const boost::system::error_code &ec) {
					std::cerr << "timeout" << std::endl;
					[this, self]() { complete_handler_(*this); }();
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

		STATE get_status() { return(state_); };

		void set_request(std::string& header) {
			request_.consume(request_.size());
			std::ostream request_stream(&request_);
			request_stream << header;
		}

		unsigned int get_status_code() { return(status_code_); };

		std::ostringstream& get_header() { return(message_header_); };
		std::ostringstream& get_body() { return(message_body_); };



	};




/*
//	using namespace std;
	template <>
	crawler<http_socket>::crawler(http_socket soc,
		std::string server, std::string port )
		: io_service_((soc.get_io_service()))
		, resolver_(io_service_)
//		, socket_(std::forward<http_socket>(soc))
		, socket_(soc)
		, server_((server))
		, port_((port)), deadline_timer_(io_service_)
		, shutdown_socket_([this]() { socket_.shutdown(boost::asio::socket_base::shutdown_type::shutdown_send); })

	{
	}
*/

	template <>
	void crawler<http_socket>::handle_connect(const boost::system::error_code& error)
	{
		auto  self(getSelf());

		std::cerr << "handle_connect "  "\n";
		if (!error)
		{
			state_ = STATE::CONNECTED;

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

	template <>
	void crawler<http_socket>::handle_resolve(const boost::system::error_code& err,
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {

		auto  self(getSelf());
		if (!err)
		{
			state_ = STATE::RESOLVED;

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





/*
	template <>
	crawler<https_socket>::crawler(https_socket soc,
		std::string server, std::string port )
		: io_service_((soc.get_io_service()))
		, resolver_(io_service_)
//		, socket_(std::forward<https_socket>(soc))
		, socket_(soc)
		, server_((server))
		, port_((port)), deadline_timer_(io_service_)
		, shutdown_socket_([this]() { socket_.shutdown(); })

	{
		std::cerr << "crawler https"  "\n";
	}

*/

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
void crawler<https_socket>::handle_connect(const boost::system::error_code& error)
{
	auto  self(getSelf());

	std::cerr << "handle_connect https"  "\n";
	if (!error)
	{
		state_ = STATE::CONNECTED;

		asioUtil::deadlineOperation2(deadline_timer_, timeout_ms
			, [this, self](const boost::system::error_code &ec) {
			std::cerr << "timeout" << std::endl;
			shutdown_socket_();
		});

		std::cerr << "ahs https"  "\n";

		socket_.async_handshake(boost::asio::ssl::stream_base::client,
			[this, self](const boost::system::error_code& ec) {
				std::cerr << "handle_handshake 0 https"  "\n";
				deadline_timer_.cancel();

				// handle_handshake
				std::cerr << "handle_handshake https"  "\n";
				state_ = STATE::HANDSHAKED;
				if (!ec)
				{

					asioUtil::deadlineOperation2(deadline_timer_, timeout_ms
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
		std::cerr << "handle_connect FIN https"  "\n";

	}
	else
	{
		std::cerr << "Connect failed: " << error.message() << "\n";
	}
}

template <>
void crawler<https_socket>::handle_resolve(const boost::system::error_code& err,
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {

	std::cerr << "handle_resolve https"  "\n";
	auto  self(getSelf());
	if (!err)
	{
		state_ = STATE::RESOLVED;

		// no Verify
		socket_.set_verify_mode(boost::asio::ssl::verify_none);

		socket_.set_verify_callback(verify_certificate);
			//boost::bind(&httpx_client::verify_certificate, this, _1, _2));


		asioUtil::deadlineOperation2(deadline_timer_, timeout_ms
			, [this, self](const boost::system::error_code &ec) {
			std::cerr << "timeout" << std::endl;
			shutdown_socket_();
		});

	std::cerr << "asc"  "\n";
		boost::asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
			[this, self](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
			deadline_timer_.cancel();
			handle_connect(ec);
		});
	std::cerr << "asc fin"  "\n";

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



	// free function
//	socket<> create_http();

}


