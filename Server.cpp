#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerConnectionFactory.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/NumberParser.h"
#include "Poco/Logger.h"
#include "Poco/AsyncChannel.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/AutoPtr.h"
#include "Poco/Process.h"
#include "Poco/NamedEvent.h"
#include "Poco/Mutex.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/JSONException.h"
#include "Poco/JSON/Object.h"
#include "Poco/Dynamic/Var.h"
#include "Poco/Base64Decoder.h"
#include "Server.h"
#include <iostream>

using Poco::Net::TCPServer;
using Poco::Net::TCPServerConnectionFilter;
using Poco::Net::TCPServerConnection;
using Poco::Net::TCPServerConnectionFactory;
using Poco::Net::TCPServerConnectionFactoryImpl;
using Poco::Net::StreamSocket;
using Poco::UInt16;
using Poco::NumberParser;
using Poco::Logger;
using Poco::AsyncChannel;
using Poco::ConsoleChannel;
using Poco::AutoPtr;
using Poco::Event;
using Poco::NamedEvent;
using Poco::Process;
using Poco::ProcessImpl;
using Poco::Exception;
using Poco::Mutex;
using Poco::Base64Decoder;
using Poco::JSON::Parser;
using Poco::JSON::JSONException;
using Poco::JSON::Object;
using Poco::Dynamic::Var;

bool getValue(Var& result, const std::string& property, std::string& value);

AutoPtr<ConsoleChannel> pCons(new ConsoleChannel);
AutoPtr<AsyncChannel> pAsync(new AsyncChannel(pCons));
Logger& logger = Logger::create("TCPServerLogger", pAsync);



Mutex* _textMutex;
char** _text;

namespace
{
	class ClientConnection : public TCPServerConnection
	{
	public:
		ClientConnection(const StreamSocket& s) : TCPServerConnection(s)
		{
		}

		void run()
		{
			StreamSocket& ss = socket();
			try {
				char buffer[1024];
				int n = ss.receiveBytes(buffer, sizeof(buffer));
				if (n > 0) {
					std::string input(buffer, n);
					Parser parser;
					Var result;
					bool isInputOK = false;
					try {
						result = parser.parse(input);
						isInputOK = true;
					}
					catch (JSONException e) {
						logger.warning("JSON input was not valid: " + input);
						isInputOK = false;
					}
					if (!isInputOK) {
						logger.warning("JSON input was not valid: " + input);
					}

					std::string text;
					if (getValue(result, "text", text)) {
						_textMutex->lock();
						std::stringstream sstream;
						sstream << text.c_str();
						Base64Decoder decoder(sstream);
						char* result = new char[text.size()]();
						decoder.get(result, text.size());
						*_text = result;
						logger.warning("Printing following text:");
						logger.information(text.c_str());
						_textMutex->unlock();
					}

				}
			}
			catch (Exception& exc) {
				logger.warning("ClientConnection: " + exc.displayText());
				ss.close();
			}
		}
	};

	typedef TCPServerConnectionFactoryImpl<ClientConnection> TCPFactory;
#if defined(POCO_OS_FAMILY_WINDOWS)
	NamedEvent terminator(ProcessImpl::terminationEventName(Process::id()));
#else
	Event terminator;
#endif
}


void Server::start() {
	try {

		TCPServer srv(new TCPFactory(), _port);
		srv.start();

		std::cout << "TCP server listening on port " << _port << std::endl;
		terminator.wait();
	}
	catch (Exception& exc) {
		std::cerr << exc.displayText() << std::endl;
	}
}

void Server::stop() {
	terminator.set();
}


bool getValue(Var& result, const std::string& property, std::string& value) {
	Object::Ptr object = result.extract<Object::Ptr>();
	Var tmp = object->get(property);
	if (tmp.isEmpty()) {
		value = nullptr;
	} else {
		value = tmp.toString();
	}
	return !tmp.isEmpty();
}

Server::Server(int port, Mutex* mutex, char** text) {
	_port = port;
	_textMutex = mutex;
	_text = text;
}