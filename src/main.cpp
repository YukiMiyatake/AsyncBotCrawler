
#include<iostream>
#include<boost/function.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

//#include "../include/socket.hpp"
#include "../include/httpx_client.hpp"

#include "../include/socket.hpp"

#include<vector>
using namespace std;
using boost::asio::ip::tcp;


// callback
/*
template<class SOC>
class callback {
public:
	void print(httpx_client<SOC>& httpx_client) {
		cout << httpx_client.get_body().str() << endl;
	}
};
*/

using METHOD = boost::asio::ip::tcp::socket;

template<class SOC>
class callback {
public:
	void print(abc::crawler<SOC>& crawl) {
		cout << crawl.get_body().str() << endl;
	}
};


//#define METHOD http_socket
//#define METHOD https_socket



template<typename Hoge>
void sss(Hoge& s) {

}

int main(int argc, char* argv[])
{
	
	try
	{
		boost::asio::io_service io_service;
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		callback<METHOD> f;

		// work
		//auto work(std::make_shared<boost::asio::io_service::work>(io_service));

		// TODO: RAII
		// try www.google.co.jp
//		auto s = make_shared<httpx_client<METHOD>>(io_service, string("www.google.co.jp"), string("/"), string("http"));
//		auto s = make_shared<httpx_client<METHOD>>(io_service, string("www.google.co.jp"), string("/"), string("http"));

		auto s = make_shared<abc::crawler<METHOD>>( std::move(METHOD(io_service)) , string("www.google.co.jp"), string("http"));

		// set request_header
		std::string header = "GET " + std::string("/") + " HTTP/1.0\r\n" + "Host: " + "www.google.co.jp" + "\r\n" + "Accept: */*\r\n" + "Connection: close\r\n\r\n";
		s->set_request(header);
		s->request(boost::bind(&callback<METHOD>::print, f, _1));


		io_service.run();

	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
