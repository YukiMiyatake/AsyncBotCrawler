
#include<iostream>
#include<boost/function.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

//#include "../include/socket.hpp"
//#include "../include/httpx_client.hpp"
#include <boost/bind.hpp>

#include "../include/socket.hpp"

#include<vector>
using namespace std;
using boost::asio::ip::tcp;


//using METHOD = http_socket;
//using METHOD = abc::https_socket;


template<class SOC>
class callback {
public:
	void print(abc::crawler<SOC>& crawl) {
		cout << crawl.get_body().str() << endl;
	}
};



template<typename Hoge>
void sss(Hoge& s) {

}

int main(int argc, char* argv[])
{
	
	try
	{
		boost::asio::io_service io_service;
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		callback<abc::http_socket> f;
		callback<abc::https_socket> f2;

		// work
		//auto work(std::make_shared<boost::asio::io_service::work>(io_service));


//		auto s = make_shared<httpx_client<METHOD> >( std::move(METHOD(io_service) ) , string("www.google.co.jp"), string("https"));
//		auto s = make_shared<abc::crawler<METHOD> >( std::move(METHOD(io_service) ) , string("www.google.co.jp"), string("http"));
//		auto s = make_shared<abc::crawler<METHOD> >( std::move(METHOD(io_service,ctx) ), string("www.google.co.jp"), string("https"));


		auto s = make_shared<abc::crawler<abc::http_socket> >( std::move(abc::http_socket(io_service) ) , string("www.google.co.jp"), string("/"), string("http"));
		auto s2 = make_shared<abc::crawler<abc::https_socket> >( std::move(abc::https_socket(io_service,ctx) ), string("www.google.co.jp"), string("/"), string("https"));
//		auto s = make_shared<abc::crawler<abc::http_socket> >( io_service, string("www.google.co.jp"),string("www.google.co.jp"), string("http"));
//		auto s2 = make_shared<abc::crawler<abc::https_socket> >( io_service, ctx, string("www.google.co.jp"),string("www.google.co.jp"), string("https"));

		// set request_header
		std::string header = "GET " + std::string("/") + " HTTP/1.0\r\n" + "Host: " + "www.google.co.jp" + "\r\n" + "Accept: */*\r\n" + "Connection: close\r\n\r\n";
		s->set_request(header);
		s->request(boost::bind(&callback<abc::http_socket>::print, f, _1));

		s2->set_request(header);
		s2->request(boost::bind(&callback<abc::https_socket>::print, f2, _1));


		io_service.run();

	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
