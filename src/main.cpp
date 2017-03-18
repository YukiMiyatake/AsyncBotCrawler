
#include<iostream>
#include<boost/function.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "../include/http_client.hpp"


using namespace std;
using boost::asio::ip::tcp;


// callback
template<class SOC>
class fuga {
public:
	void hoge(httpx_client<SOC>& httpx_client) {
		cout << httpx_client.get_body().str() << endl;
	}
};


int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_service io_service;
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		fuga<http_socket> f;

		// work
		//auto work(std::make_shared<boost::asio::io_service::work>(io_service));


		// try www.google.co.jp
		auto s = make_shared<httpx_client<http_socket>>(io_service, string("www.google.co.jp"), string("/"));

		// set request_header
		std::string header = "GET " + std::string("/") + " HTTP/1.0\r\n" + "Host: " + "www.google.co.jp" + "\r\n" + "Accept: */*\r\n" + "Connection: close\r\n\r\n";
		s->set_header(header);
		
		s->go(boost::bind(&fuga<http_socket>::hoge, f, _1));


		io_service.run();

	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
