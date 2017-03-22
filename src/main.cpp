
#include<iostream>
#include<boost/function.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "../include/httpx_client.hpp"

#include<vector>
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


#define METHOD http_socket
//#define METHOD https_socket

void foo1(int& i) {
}
void foo2(int&& i) {
}


template<typename Hoge>
void sss(Hoge& s) {

}

int main(int argc, char* argv[])
{
	int i;
	int &i1(i);						// lvalue -> lvalue ref			well-formed
	int &i2(i1);					// lvalue ref -> lvalue ref		well-formed
//	int &i3(1);						// rvalue -> lvalue ref			ill-formed

	int &&ii(1);					// rvalue -> rvalue ref			well-formed
//	int &&ii2(ii);					// lvalue ref -> rvalue ref		ill-formed 
	int &&ii3(std::move(ii));		// rvalue ref -> rvalue ref		well-formed


	foo1(i);						// lvalue -> lvalue ref			well-formed
	foo1(i1);						// lvalue ref -> lvalue ref		well-formed
//	foo1(1);						// rvalue -> lvalue ref			ill-formed

	foo2(1);						// rvalue -> rvalue ref			well-formed
//	foo2(ii);						// lvalue ref -> rvalue ref		ill-formed 
	foo2(std::move(ii));			// rvalue ref -> rvalue ref		well-formed


	sss<vector<int>>(vector<int>());

	try
	{
		boost::asio::io_service io_service;
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		fuga<METHOD> f;

		// work
		//auto work(std::make_shared<boost::asio::io_service::work>(io_service));


		// TODO: RAII
		// try www.google.co.jp
		string url( "www.google.co.jp" );
//		auto ss = new httpx_client<METHOD>(io_service, url, string("/"), string("http"));
//		auto s = make_shared<httpx_client<METHOD>>(io_service, url, string("/"), string("http"));
//		auto s = make_shared<httpx_client<METHOD>>(io_service, ctx, string("www.google.co.jp"), string("/"), string("https"));

		// set request_header
		std::string header = "GET " + std::string("/") + " HTTP/1.0\r\n" + "Host: " + "www.google.co.jp" + "\r\n" + "Accept: */*\r\n" + "Connection: close\r\n\r\n";
//		s->set_request(header);
//		s->request(boost::bind(&fuga<METHOD>::hoge, f, _1));


		io_service.run();

	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
