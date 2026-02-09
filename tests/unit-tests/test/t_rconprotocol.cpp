#include "gtest/gtest.h"

#include "odamex.h"

#include "rconprotocol.h"

#include <string>

namespace rcon::messages {

void PrintTo(const ClientMessage& message, std::ostream* os) {
	*os << message.serialize(true);
}

void PrintTo(const ServerMessage& message, std::ostream* os) {
	*os << message.serialize(true);
}

} // namespace rcon

using namespace rcon;

TEST(RCONProtocolClient, DeserializeCommand) {
	ClientMessage expect {
		23,
		messages::client::Command {
			"Hey server do somethin"
		}
	};
	std::string_view json = R"({
		"type": "command",
		"id": 23,
		"content": "Hey server do somethin"
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_TRUE(actual) << "Deserialize failed: " << actual.error().what();
	EXPECT_EQ(*actual, expect);
}

TEST(RCONProtocolClient, DeserializeLoginRequest) {
	ClientMessage expect {
		11,
		messages::client::LoginRequest {
			messages::ProtocolVersion {
				1, 0, 0
			}
		}
	};
	std::string_view json = R"({
		"type": "login_request",
		"id": 11,
		"content": "1.0.0"
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_TRUE(actual) << "Deserialize failed: " << actual.error().what();
	EXPECT_EQ(*actual, expect);
}

TEST(RCONProtocolClient, DeserializeLoginPassword) {
	ClientMessage expect {
		1234,
		messages::client::LoginPassword {
			"this is totally a hashed password"
		}
	};
	std::string_view json = R"({
		"type": "login_password",
		"id": 1234,
		"content": "this is totally a hashed password"
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_TRUE(actual) << "Deserialize failed: " << actual.error().what();
	EXPECT_EQ(*actual, expect);
}

TEST(RCONProtocolClient, DISABLED_DeserializeMaplist) {
	ClientMessage expect {
		500,
		messages::client::Maplist {}
	};
	std::string_view json = R"({
		"type": "maplist",
		"id": 500,
		"content": null
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_TRUE(actual) << "Deserialize failed: " << actual.error().what();
	EXPECT_EQ(*actual, expect);
}

TEST(RCONProtocolServer, DeserializeLoginResponse) {
	ServerMessage expect {
		2,
		messages::server::LoginResponse {
			123
		}
	};
	std::string_view json = R"({
		"type": "login_response",
		"id": 2,
		"content": 123
	})";
	auto actual = ServerMessage::deserialize(json);
	ASSERT_TRUE(actual) << "Deserialize failed: " << actual.error().what();
	EXPECT_EQ(*actual, expect);
}

TEST(RCONProtocolServer, DeserializeLoginFailure) {
	ServerMessage expect {
		2,
		messages::server::LoginFailure {
			"wrong password man"
		}
	};
	std::string_view json = R"({
		"type": "login_failure",
		"id": 2,
		"content": "wrong password man"
	})";
	auto actual = ServerMessage::deserialize(json);
	ASSERT_TRUE(actual) << "Deserialize failed: " << actual.error().what();
	EXPECT_EQ(*actual, expect);
}

TEST(RCONProtocolServer, DeserializeLoginSuccess) {
	ServerMessage expect {
		2,
		messages::server::LoginSuccess {
		}
	};
	std::string_view json = R"({
		"type": "login_success",
		"id": 2,
		"content": null
	})";
	auto actual = ServerMessage::deserialize(json);
	ASSERT_TRUE(actual) << "Deserialize failed: " << actual.error().what();
	EXPECT_EQ(*actual, expect);
}

TEST(RCONProtocolServer, DeserializePrint) {
	ServerMessage expect {
		2,
		messages::server::Print {
			PRINT_OBITUARY,
			"someone died dude"
		}
	};
	std::string_view json = R"({
		"type": "print",
		"id": 2,
		"content": {
			"printlevel": "obituary",
			"text": "someone died dude"
		}
	})";
	auto actual = ServerMessage::deserialize(json);
	ASSERT_TRUE(actual) << "Deserialize failed: " << actual.error().what();
	EXPECT_EQ(*actual, expect);
}

TEST(RCONProtocolServer, DISABLED_DeserializeMaplist) {
	ServerMessage expect {
		500,
		messages::server::Maplist {}
	};
	std::string_view json = R"({
		"type": "maplist",
		"id": 500,
		"content": null
	})";
	auto actual = ServerMessage::deserialize(json);
	ASSERT_TRUE(actual) << "Deserialize failed: " << actual.error().what();
	EXPECT_EQ(*actual, expect);
}

TEST(RCONProtocol, DeserializeNegativeID) {
	std::string_view json = R"({
		"type": "command",
		"id": -23,
		"content": "Hey server do somethin"
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_FALSE(actual) << "Deserialize succeeded: " << actual->serialize(true);
}

TEST(RCONProtocol, DeserializeNullID) {
	std::string_view json = R"({
		"type": "command",
		"id": null,
		"content": "Hey server do somethin"
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_FALSE(actual) << "Deserialize succeeded: " << actual->serialize(true);
}

TEST(RCONProtocol, DeserializeMissingNull) {
	std::string_view json = R"({
		"type": "login_success",
		"id": 0,
	})";
	auto actual = ServerMessage::deserialize(json);
	ASSERT_FALSE(actual) << "Deserialize succeeded: " << actual->serialize(true);
}

TEST(RCONProtocol, DeserializeNullType) {
	std::string_view json = R"({
		"type": null,
		"id": 0,
		"content": null
	})";
	auto actual = ServerMessage::deserialize(json);
	ASSERT_FALSE(actual) << "Deserialize succeeded: " << actual->serialize(true);
}

TEST(RCONProtocol, DeserializeMissingType) {
	std::string_view json = R"({
		"id": 0,
		"content": "hi"
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_FALSE(actual) << "Deserialize succeeded: " << actual->serialize(true);
}

TEST(RCONProtocol, DeserializeMissingID) {
	std::string_view json = R"({
		"type": "command",
		"content": "hi"
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_FALSE(actual) << "Deserialize succeeded: " << actual->serialize(true);
}

TEST(RCONProtocol, DeserializeExtraFieldRoot) {
	std::string_view json = R"({
		"type": "command",
		"id": 27,
		"content": "hi",
		"what-is-this": "something"
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_FALSE(actual) << "Deserialize succeeded: " << actual->serialize(true);
}

TEST(RCONProtocol, DeserializeExtraFieldContent) {
	std::string_view json = R"({
		"type": "print",
		"id": 27,
		"content": {
			"printlevel": "chat",
			"text": "hiiiiiii",
			"someweirdo": "whats up"
		}
	})";
	auto actual = ServerMessage::deserialize(json);
	ASSERT_FALSE(actual) << "Deserialize succeeded: " << actual->serialize(true);
}

TEST(RCONProtocol, DeserializeWrongContentType) {
	std::string_view json = R"({
		"type": "command",
		"id": 27,
		"content": {
			"text": "what"
		}
	})";
	auto actual = ClientMessage::deserialize(json);
	ASSERT_FALSE(actual) << "Deserialize succeeded: " << actual->serialize(true);
}

// TODO: add tests to make sure bad json fails to deserialize