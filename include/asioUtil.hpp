#pragma once

#include <boost/asio.hpp>
#include<chrono>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>

namespace asioUtil {

	// timeout付きで非同期処理をする。
	// handlerは実行する非同期命令
	// timeout_handlerはタイムアウトorタイムアウトキャンセル時の処理
	void deadlineOperation(boost::asio::deadline_timer &timer,
		const int unsigned timeout_ms,
		std::function<void()> handler,
		std::function<void(const boost::system::error_code &)> timeout_handler);

	void deadlineOperation2(boost::asio::deadline_timer &timer,
		const int unsigned timeout_ms,
		std::function<void(const boost::system::error_code &)> timeout_handler);



	class deadlineOperation3 : public std::enable_shared_from_this<deadlineOperation3>{
	private:
		boost::asio::deadline_timer deadline_timer_;
	public:
		deadlineOperation3(boost::asio::io_service &io_service, unsigned int timeout_ms
			, std::function<void(const boost::system::error_code &)> handle_timeout)
			: deadline_timer_(io_service) {

			deadline_timer_.expires_from_now(
				boost::posix_time::milliseconds(timeout_ms));

			deadline_timer_.async_wait(
				[=](const boost::system::error_code &ec) {
				if (ec != boost::asio::error::operation_aborted) {
					handle_timeout(ec);
				}

			});
		};
		virtual ~deadlineOperation3() {
			// デストラクタでタイマーキャンセルする
			deadline_timer_.cancel();
		};
	};

}

