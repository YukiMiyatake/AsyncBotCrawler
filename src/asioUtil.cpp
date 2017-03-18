#include "../include/asioUtil.hpp"

//#include <boost/bind.hpp>
//#include <functional>
#include<iostream>

namespace asioUtil {
	void deadlineOperation(boost::asio::deadline_timer &timer,
		const unsigned int timeout_ms,
		std::function<void()> handler,
		std::function<void(const boost::system::error_code &)> timeout_handler) {


		timer.expires_from_now(
			boost::posix_time::milliseconds(timeout_ms));//duration<integer_type, ratio<1, 1000>>seconds(10)

		timer.async_wait(
			[=](const boost::system::error_code &ec) {
//			timer.cancel();
			if (ec != boost::asio::error::operation_aborted) {
				timeout_handler(ec);
			}
		});

		handler();
	}

	void deadlineOperation2(boost::asio::deadline_timer &timer,
		const unsigned int timeout_ms,
		std::function<void(const boost::system::error_code &)> timeout_handler) {


		timer.expires_from_now(
			boost::posix_time::milliseconds(timeout_ms));//duration<integer_type, ratio<1, 1000>>seconds(10)

		timer.async_wait(
			[=](const boost::system::error_code &ec) {
			//			timer.cancel();
			if (ec != boost::asio::error::operation_aborted) {
				timeout_handler(ec);
			}
		});

	}

}