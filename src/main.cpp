
#include<iostream>
#include<boost/function.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>



using namespace std;
using boost::asio::ip::tcp;




int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_service io_service;

		// work
		//auto work(std::make_shared<boost::asio::io_service::work>(io_service));


		// HTTPS
		if (argc != 3) {
			cerr << "abc server url \n";
		}
		else {
	


		}

		io_service.run();

	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
