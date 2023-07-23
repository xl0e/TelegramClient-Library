#ifndef TELEGRAM_CLIENT_H
#define TELEGRAM_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <Client.h>
#include <MultipartRequest.h>

#if not defined(PRINT)
#define PRINT(...) Serial.print(__VA_ARGS__);
#define PRINTLN(...) Serial.println(__VA_ARGS__);
#define PRINTF(f, ...) Serial.println(f, __VA_ARGS__);
#endif

struct TMessage
{
	uint32_t updateId;
	uint32_t chatId = 0;
	uint32_t date;
	String text;
	String sender;
};

class TelegramClient
{
private:
	Client *_client;
	String _token;
	String _host;
	uint16_t _port;

public:
	TelegramClient(Client &client) : _host(F("api.telegram.org")), _port(443)
	{
		this->_client = &client;
	}

	~TelegramClient()
	{
		this->_client = nullptr;
	}

	void setToken(const String &token)
	{
		_token = token;
	}

	void init()
	{
		if (_client->connected())
		{
			while (_client->available())
			{
				_client->read();
			}
			_client->stop();
		}
		_client->connect(_host.c_str(), _port);
	}

	TMessage getUpdates(const uint32_t offset)
	{
		TMessage message;
		init();
		String requestStr = "GET /bot" + _token + "/getUpdates?limit=1&offset=" + String(offset) + " HTTP/1.1";
		PRINTLN(requestStr);
		_client->println(requestStr);
		_client->print(F("Host: "));
		_client->println(_host);
		_client->println(F("User-Agent: TG_Client"));
		_client->println();
		_client->flush();

		uint32_t size = skipHeaders();
		DynamicJsonDocument doc(2 * size);
		DeserializationError error = deserializeJson(doc, *_client);
		JsonObject json = doc.as<JsonObject>();

		if (!error)
		{
			message.updateId = json["result"][0]["update_id"];
			message.sender = json["result"][0]["message"]["from"]["username"].as<String>();
			message.text = json["result"][0]["message"]["text"].as<String>();
			message.chatId = json["result"][0]["message"]["chat"]["id"].as<uint32_t>();
			message.date = json["result"][0]["message"]["date"];
			PRINTF("Got message with update_id: %d\n", message.updateId);
		}
		else
		{
			PRINTLN("Error JSON");
			PRINTLN(error.c_str());
		}
		return message;
	}

	String sendMessage(uint32_t chatId, const String &text)
	{
		PRINTF("Send message to chat: %d\n", chatId);
		if (chatId > 0)
		{
			const size_t size = text.length() + 100;
			DynamicJsonDocument doc(size);

			JsonObject json = doc.to<JsonObject>();
			json["chat_id"] = chatId;
			json["text"] = text;
			String msg;
			serializeJson(doc, msg);

			return postMessage(msg);
		}

		PRINTLN("chatId not defined");
		return "";
	}

	String postMessage(const String &msg)
	{
		init();
		PRINTLN("Sending message");
		PRINTLN(msg);
		_client->println("POST /bot" + _token + "/sendMessage HTTP/1.1");
		_client->print(F("Host: "));
		_client->println(_host);
		_client->println(F("User-Agent: TG_Client"));
		_client->println(F("Content-Type: application/json"));
		_client->print(F("Content-Length: "));
		_client->println(msg.length());
		_client->println();
		_client->println(msg);
		_client->flush();

		return readResponse();
	}

	String sendFile(const uint32_t chatId, File &file, const String &caption = "")
	{
		PRINT("Send file to chat: ");
		PRINTLN(chatId);
		if (chatId > 0)
		{
			init();
			String postRequest = "POST /bot" + _token + "/sendDocument HTTP/1.1";
			PRINTLN(postRequest);
			_client->println(postRequest);
			_client->print(F("Host: "));
			_client->println(_host);
			_client->println(F("User-Agent: TG_Client"));
			MultipartRequest multipart;
			multipart.addFile("document", file);
			multipart.addText("chat_id", String(chatId));
			if (caption)
			{
				multipart.addText("caption", caption);
			}
			multipart.printTo(_client);
			return readResponse();
		}

		PRINTLN("Chat_id not defined");
		return "";
	}

	String readResponse()
	{
		skipHeaders();
		String payload;
		while (_client->connected() || (_client->available() > 0))
		{
			payload += (char)_client->read();
		}

		return payload;
	}

	uint32_t skipHeaders()
	{
		uint32_t tmp = millis();
		while (!_client->available() && millis() - tmp < 1000)
			;
		tmp = 0;
		String header;
		while (_client->connected() || (_client->available() > 0))
		{
			header = _client->readStringUntil('\n');
			PRINTLN(header);
			header.toLowerCase();
			if (header.startsWith(F("content-length:")))
			{
				String len = header.substring(15);
				len.trim();
				tmp = len.toInt();
			}
			else if (header.length() <= 1 && (header == "" || header[0] == '\r'))
			{
				break;
			}
		}
		return tmp;
	}
};

#endif
