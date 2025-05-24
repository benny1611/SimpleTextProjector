#pragma once
#include "SharedVariables.h"
#include "Poco/Base64Decoder.h"
#include "Poco/JSON/Stringifier.h"
#include "Poco/Data/Session.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/PBKDF2Engine.h"
#include "Poco/Random.h"
#include "Poco/Base64Encoder.h"
#include "Poco/Crypto/DigestEngine.h"
#include "Poco/UUIDGenerator.h"
#include "Poco/DateTime.h"

using Poco::Base64Decoder;
using Poco::Base64Encoder;
using Poco::Data::Session;
using Poco::Data::Statement;
using namespace Poco::Data::Keywords;


Session* sqliteSession;

struct User {
	std::string username;
	std::string passwordHash;
	std::string passwordSalt;
	WebSocket* ws;
	bool isAdmin;
};

std::list<User> userCache;
std::map<std::string, std::pair<Poco::UUID, Poco::DateTime>> sessionTokens;
Mutex sessionTokenMutex;

std::string getErrorMessageJSONAsString(std::string message, std::string errorType) {
	Poco::JSON::Object::Ptr erorJSON = new Poco::JSON::Object;
	erorJSON->set("error", true);
	erorJSON->set("message", message);
	erorJSON->set("error_type", errorType);

	std::ostringstream oss;
	Poco::JSON::Stringifier::stringify(*erorJSON, oss);

	return oss.str();
}

bool getColor(Object::Ptr jsonObject, std::string key, WebSocket ws, Logger* consoleLogger, float& R, float& G, float& B, float& A) {
	std::string error = "";
	std::string errorMessage = getErrorMessageJSONAsString("Error: the color must be a JSON object in the form : " + key + " : R : <value>, G : <value>, B : <value>, A : <value>, all values are floats between 0.0 and 1.0", "color_error");
	Object::Ptr colorObj = jsonObject->get(key).extract<Object::Ptr>();
	if (!colorObj->has("R") || !colorObj->has("G") || !colorObj->has("B") || !colorObj->has("A")) {
		error = errorMessage;
	}
	if (!error.empty()) {
		ws.sendFrame(error.c_str(), error.length());
		return false;
	}
	try {
		R = colorObj->getValue<float>("R");
		G = colorObj->getValue<float>("G");
		B = colorObj->getValue<float>("B");
		A = colorObj->getValue<float>("A");
	}
	catch (Exception e) {
		error = getErrorMessageJSONAsString(e.message(), "color_error");
		ws.sendFrame(error.c_str(), error.length());
		return false;
	}

	if (R < 0 || R > 1.0 || G < 0 || G > 1.0 || B < 0 || B > 1.0 || A < 0 || A > 1.0) {
		error = getErrorMessageJSONAsString("Values R, G, B and A must be a float between 0.0 and 1.0", "color_error");
	}

	if (!error.empty()) {
		ws.sendFrame(error.c_str(), error.length());
		return false;
	}
	
	return true;
}

void handleText(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	std::string textValue = jsonObject->getValue<std::string>("text");
	consoleLogger->debug("Here's your encoded text: " + textValue);

	std::istringstream iss(textValue);
	std::ostringstream oss;

	try {
		Base64Decoder decoder(iss);
		oss << decoder.rdbuf();

		std::string decoded = oss.str();
		// TODO: when the implementation of multiple renderers is done, find the right renderer instead of taking the renderer with the ID = 1
		TextBoxRenderer* renderer = renderers[1];

		textMutex.lock();
		if (decoded != renderer->getText()) {
			renderer->setText(decoded);
			consoleLogger->debug("Here's your decoded text: {}", decoded);
		}
		textMutex.unlock();
	}
	catch (const Poco::InvalidArgumentException& e) {
		std::string error = getErrorMessageJSONAsString("Invalid Base64 string", "text_error");
		ws.sendFrame(error.c_str(), error.length());
	}
	catch (const std::exception& e) {
		std::string error = getErrorMessageJSONAsString("Something else happened: " + std::string(e.what()), "text_error");
		ws.sendFrame(error.c_str(), error.length());
	}
}

void handleFontColor(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	float R, G, B, A;
	bool success = getColor(jsonObject, "font_color", ws, consoleLogger, R, G, B, A);

	if (success) {
		// TODO: when the implementation of multiple renderers is done, find the right renderer instead of taking the renderer with the ID = 1
		TextBoxRenderer* renderer = renderers[1];

		textMutex.lock();
		renderer->setColor(R, G, B, A);
		textMutex.unlock();
		consoleLogger->information("Here's your color: R: " + std::to_string(R) + ", G:" + std::to_string(G) + ", B: " + std::to_string(B) + ", A:" + std::to_string(A));
	}
}

void handleFontSize(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	float fontSizeValue = jsonObject->getValue<float>("font_size");
	if (fontSizeValue > 0) {
		// TODO: when the implementation of multiple renderers is done, find the right renderer instead of taking the renderer with the ID = 1
		TextBoxRenderer* renderer = renderers[1];

		textMutex.lock();
		renderer->setFontSize(fontSizeValue);
		textMutex.unlock();
	} else {
		std::string error = getErrorMessageJSONAsString("Error: could not set font size to: " + std::to_string(fontSizeValue), "font_size_error");
		ws.sendFrame(error.c_str(), error.length());
	}
}

void handleFont(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	std::string fontPathValue = jsonObject->getValue<std::string>("font");
	std::string last4Characters = fontPathValue.substr(fontPathValue.size() - 4);
	if (last4Characters != ".ttf") {
		fontPathValue += ".ttf";
	}
	consoleLogger->debug("Here is the font: {}", fontPathValue);

	std::string fontFullPath = "fonts/" + fontPathValue;

	// TODO: when the implementation of multiple renderers is done, find the right renderer instead of taking the renderer with the ID = 1
	TextBoxRenderer* renderer = renderers[1];

	textMutex.lock();
	if (fontFullPath != renderer->getFontPath()) {
		try {
			std::ifstream file(fontFullPath);
			if (file.is_open()) {
				file.close();
				
				renderer->setFont(fontFullPath);
			}
			else {
				std::string error = getErrorMessageJSONAsString("Error file " + fontFullPath + " not found", "font_error");
				ws.sendFrame(error.c_str(), error.length());
			}
		}
		catch (Exception e) {
			std::string error = getErrorMessageJSONAsString(e.message(), "font_error");
			ws.sendFrame(error.c_str(), error.length());
		}
	}
	textMutex.unlock();
}

void handleStream(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	bool shouldStream = jsonObject->getValue<bool>("stream");

	streamingServerMutex.lock();
	if (shouldStream && !isServerRunning) {
		// start the server

		screenStreamerTask = new ScreenStreamerTask(&streamingServerMutex, consoleLogger, 0, 0);
		taskManager->start(screenStreamerTask);

		isServerRunning = true;
		streamingServerMutex.unlock();
	}
	else if (!shouldStream && isServerRunning) {
		// stop the server & cleanup

		screenStreamerTask->cancel();
		streamingServerMutex.unlock();
		screenStreamerTask->getStopEvent()->wait();

		isServerRunning = false;
	}
	else {
		streamingServerMutex.unlock();

		std::string error;
		if (shouldStream) {
			error = getErrorMessageJSONAsString("Error: could not start the streaming server, probably it's already running", "stream_error");
		} else {
			error = getErrorMessageJSONAsString("Error: could not stop the streaming server, probably it's already stopped", "stream_error");
		}
		ws.sendFrame(error.c_str(), error.length());
	}
}

void handlePing(Object::Ptr jsonObject, WebSocket& ws, Logger* consoleLogger) {
	User usr;
	usr.username = "";
	usr.passwordHash = "";
	usr.passwordSalt = "";
	usr.ws = nullptr;

	WebSocket* tmp = &ws;

	std::list<User>::iterator it;
	for (it = userCache.begin(); it != userCache.end(); ++it) {
		if (it->ws == tmp) {
			usr = *it;
			break;
		}
	}

	if (!usr.username.empty() && !usr.passwordHash.empty() && !usr.passwordSalt.empty()) {
		sessionTokenMutex.lock();
		if (sessionTokens.count(usr.username)) {
			Poco::DateTime now;
			Poco::DateTime sessionTokenIssueTime;
			Poco::UUID sessionToken;

			sessionTokenIssueTime = sessionTokens[usr.username].second;
			sessionToken = sessionTokens[usr.username].first;
			sessionTokenMutex.unlock();

			int sessionTokenDurationInS = pConf->getInt("SessionTokenDurationS", 43200); // 12 hours default		

			Poco::Timespan timeSpan;
			timeSpan.assign(sessionTokenDurationInS, 0);

			Poco::DateTime expirityTime = sessionTokenIssueTime + timeSpan;

			if (now <= expirityTime) {
				Poco::JSON::Object::Ptr pingResponseJSON = new Poco::JSON::Object;
				pingResponseJSON->set("pong", sessionToken.toString());
				std::ostringstream oss;
				Poco::JSON::Stringifier::stringify(*pingResponseJSON, oss);
				std::string pingResponseString = oss.str();
				ws.sendFrame(pingResponseString.c_str(), pingResponseString.length());
				return;
			}
		}
		
		sessionTokenMutex.unlock();

		sessionTokenMutex.lock();
		sessionTokens.erase(usr.username);
		sessionTokenMutex.unlock();

		std::string errorMessage = getErrorMessageJSONAsString("Error: session expired", "session_error");
		ws.sendFrame(errorMessage.c_str(), errorMessage.length());

	}
	else {
		std::string errorMessage = getErrorMessageJSONAsString("Error: user not logged in", "session_error");
		ws.sendFrame(errorMessage.c_str(), errorMessage.length());
	}
}

void handleGet(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	std::string what = jsonObject->getValue<std::string>("get");
	consoleLogger->information("Getting: " + what);
	if (what == "stream") {
		streamingServerMutex.lock();
		std::string isStreamingJSON = "{\"isStreaming\":";
		if (isServerRunning) {
			streamingServerMutex.unlock();
			Event waitForOfferEvent;
			screenStreamerTask->registerReceiver(ws, &waitForOfferEvent);
			waitForOfferEvent.wait();
			isStreamingJSON += "true, \"offer\": ";
			isStreamingJSON += screenStreamerTask->getOffer(ws);
		} else {
			streamingServerMutex.unlock();
			isStreamingJSON += "false";
		}
		isStreamingJSON += "}";

		ws.sendFrame(isStreamingJSON.c_str(), isStreamingJSON.length());
	} else if (what == "ping") {
		handlePing(jsonObject, ws, consoleLogger);
	} else if (what == "monitors") {
		monitorInfo.monitorMutex.lock();
		ws.sendFrame(monitorInfo.monitorJSONAsString.c_str(), monitorInfo.monitorJSONAsString.length());
		monitorInfo.monitorMutex.unlock();
	} else {
		std::string error = getErrorMessageJSONAsString("get command not supported: " + what, "get_error");
		ws.sendFrame(error.c_str(), error.length());
	}
	consoleLogger->information("Done getting");
}

void handleSet(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	std::string what = jsonObject->getValue<std::string>("set");
	consoleLogger->information("Setting: " + what);
	if (what == "answer") {
		if (jsonObject->has("answer")) {
			Object::Ptr answer = jsonObject->get("answer").extract<Object::Ptr>();
			streamingServerMutex.lock();
			if (isServerRunning) {
				int result = screenStreamerTask->setAnswer(ws, answer);
				if (result < 0) {
					consoleLogger->debug("Could not connect to the screen streamer");
				}
			}
			streamingServerMutex.unlock();
		}
	} else {
		std::string error = getErrorMessageJSONAsString("set command not supported: " + what, "set_error");
	}
	consoleLogger->information("Done setting");
}

void handleBGColor(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	float R, G, B, A;
	bool success = getColor(jsonObject, "background_color", ws, consoleLogger, R, G, B, A);
	if (success) {
		// TODO: when the implementation of multiple renderers is done, find the right renderer instead of taking the renderer with the ID = 1
		TextBoxRenderer* renderer = renderers[1];

		textMutex.lock();
		backgroundColorR = R;
		backgroundColorG = G;
		backgroundColorB = B;
		backgroundColorA = A;
		textMutex.unlock();
		consoleLogger->information("Here's your color: R: " + std::to_string(R) + ", G:" + std::to_string(G) + ", B: " + std::to_string(B) + ", A:" + std::to_string(A));
	}
}

void handleMonitor(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	int monitorIndex = jsonObject->getValue<int>("monitor");

	monitorInfo.monitorMutex.lock();

	if (monitorIndex != monitorInfo.monitorIndex && monitorIndex >= 0 && monitorIndex < monitorInfo.monitorCount) {
		monitorInfo.monitorIndex = monitorIndex;
		monitorInfo.hasChanged = true;
	} else if(monitorIndex < 0 || monitorIndex >= monitorInfo.monitorCount) {
		int monitorMaxIndex = monitorInfo.monitorCount - 1;
		std::string error = getErrorMessageJSONAsString("Monitor index out of range. Values must be between 0 and " + std::to_string(monitorMaxIndex), "monitor_error");
		ws.sendFrame(error.c_str(), error.length());
	}

	monitorInfo.monitorMutex.unlock();
}

Session* getSession() {
	static bool connectorRegistered = false;
	if (!connectorRegistered) {

		Poco::Data::SQLite::Connector::registerConnector();
		connectorRegistered = true;

		sqliteSession = new Poco::Data::Session("SQLite", "users.db");

		*sqliteSession << "CREATE TABLE IF NOT EXISTS User ("
			"Username TEXT PRIMARY KEY, "
			"PasswordHash TEXT NOT NULL, "
			"PasswordSalt TEXT NOT NULL, "
			"IsAdmin BOOLEAN NOT NULL CHECK (IsAdmin IN (0, 1)))",
			now;
		Statement insert(*sqliteSession);
		insert << "INSERT OR IGNORE INTO User(Username, PasswordHash, PasswordSalt, IsAdmin) VALUES('admin' , '674c34644a66506e342f5545675962574f69427167513d3d61646d696e' , 'gL4dJfPn4/UEgYbWOiBqgQ==' , 1 )";
		insert.execute();
	}

	return sqliteSession;
}

User getUser(std::string username, std::string& error, WebSocket& ws) {

	std::list<User>::iterator it;
	for (it = userCache.begin(); it != userCache.end(); ++it) {
		if (it->username == username) {
			return *it;
		}
	}

	User usr;
	usr.username = "";
	usr.passwordHash = "";
	usr.passwordSalt = "";
	usr.ws = nullptr;
	usr.isAdmin = false;

	try {
		Session session = *getSession();

		Statement select(session);

		select << "SELECT Username, PasswordHash, PasswordSalt, IsAdmin FROM User WHERE Username = ? LIMIT 1", use(username), into(usr.username), into(usr.passwordHash), into(usr.passwordSalt), into(usr.isAdmin);
		select.execute();

		if (!usr.username.empty() && !usr.passwordHash.empty() && !usr.passwordSalt.empty()) {
			usr.ws = &ws;
			userCache.push_back(usr);
		}
	}
	catch (const Poco::Exception& ex) {
		error = "SQL Error: " + ex.displayText();
	}

	return usr;
}

std::string generateSalt(int length = 16) {
	Poco::Random rng;
	rng.seed(); // Seed the random generator

	std::string salt;
	for (int i = 0; i < length; ++i) {
		salt += static_cast<char>(rng.next(256)); // Random bytes
	}

	// Encode in Base64 for storage
	std::ostringstream encoded;
	Base64Encoder encoder(encoded);
	encoder.write(salt.data(), salt.size());
	encoder.close();
	return encoded.str();
}


std::string hashPassword(const std::string& password, const std::string& salt) {
	Poco::Crypto::DigestEngine sha256Engine("SHA256");
	std::string saltedPassword = salt + password;
	std::vector<unsigned char> digestBytes = std::vector<unsigned char>(saltedPassword.begin(), saltedPassword.end());
	return sha256Engine.digestToHex(digestBytes, digestBytes.size());
}

void handleAuthentication(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	Object::Ptr authObj = jsonObject->get("authenticate").extract<Object::Ptr>();
	if (authObj->has("user") && authObj->has("password")) {
		std::string user = authObj->getValue<std::string>("user");
		std::string password = authObj->getValue<std::string>("password");

		if (!user.empty() && user.size() <= 255 && !password.empty() && password.size() <= 255) {

			std::string error = "";

			User usr = getUser(user, error, ws);

			if (!usr.username.empty() && !usr.passwordHash.empty() && !usr.passwordSalt.empty()) {

				std::string computedPasswordHash = hashPassword(password, usr.passwordSalt);
				if (computedPasswordHash == usr.passwordHash) {

					Poco::UUIDGenerator generator;
					Poco::UUID sessionToken = generator.create();
					Poco::DateTime now;

					std::pair<Poco::UUID, Poco::DateTime> sessionTokenTimePair;
					sessionTokenTimePair.first = sessionToken;
					sessionTokenTimePair.second = now;

					sessionTokenMutex.lock();
					sessionTokens.insert({ usr.username, sessionTokenTimePair });
					sessionTokenMutex.unlock();

					std::string successMessage = "{\"message\": \"Succesfully logged in\", \"session_token\": \"" + sessionToken.toString() + "\"}";
					ws.sendFrame(successMessage.c_str(), successMessage.length());
				}
				else {
					std::string errorMessage = getErrorMessageJSONAsString("ERROR: wrong password", "auth_error");
					ws.sendFrame(errorMessage.c_str(), errorMessage.length());
				}
			} else {
				std::string message = "ERROR: Could not find user";
				if (!error.empty()) {
					message += "; " + error;
				}
				std::string errorMessage = getErrorMessageJSONAsString(message, "auth_error");
				ws.sendFrame(errorMessage.c_str(), errorMessage.length());
			}
		}
		else {
			std::string errorMessage = getErrorMessageJSONAsString("ERROR: user or password empty or too long", "auth_error");
			ws.sendFrame(errorMessage.c_str(), errorMessage.length());
		}
	}
	else {
		std::string errorMessage = getErrorMessageJSONAsString("ERROR: missing either user or password", "auth_error");
		ws.sendFrame(errorMessage.c_str(), errorMessage.length());
	}
}

void handleRegistration(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {

	bool isRegistrationOpen = pConf->getBool("ServerRegistrationsOpen", false);
	if (isRegistrationOpen) {
		Object::Ptr registerObj = jsonObject->get("register").extract<Object::Ptr>();
		if (registerObj->has("user") && registerObj->has("password")) {
			std::string user = registerObj->getValue<std::string>("user");
			std::string password = registerObj->getValue<std::string>("password");

			std::string salt = generateSalt();
			std::string hashedPassword = hashPassword(password, salt);

			try {
				Session* session = getSession();

				std::string errorUser = "";

				User usr = getUser(user, errorUser, ws);

				if (usr.username.empty() && usr.passwordHash.empty() && usr.passwordSalt.empty()) {
					Statement insert(*session);
					insert << "INSERT INTO User(Username, PasswordHash, PasswordSalt, IsAdmin) VALUES(? , ? , ? , 0 )", use(user), use(hashedPassword), use(salt);
					insert.execute();
					std::string successMessage = "{\"message\":\"Successfully added user: " + user + "\"}";
					ws.sendFrame(successMessage.c_str(), successMessage.length());
				}
				else {
					std::string error = getErrorMessageJSONAsString("ERROR: User already exists", "registration_error");
					ws.sendFrame(error.c_str(), error.length());
				}

			}
			catch (const Poco::Exception& ex) {
				std::string error = getErrorMessageJSONAsString("SQL Error: " + ex.displayText(), "registration_error");
				ws.sendFrame(error.c_str(), error.length());
			}

		}
		else {
			std::string errorMessage = getErrorMessageJSONAsString("ERROR: missing either user or password", "registration_error");
			ws.sendFrame(errorMessage.c_str(), errorMessage.length());
		}
	}
	else {
		std::string errorMessage = getErrorMessageJSONAsString("ERROR: Registrations are closed", "registration_error");
		ws.sendFrame(errorMessage.c_str(), errorMessage.length());
	}
}


