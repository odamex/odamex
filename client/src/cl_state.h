#pragma once

#include "i_net.h"

class ClientState
{
  protected:
	ClientState() = default;

  public:
	/**
	 * @brief Reset client state to init.
	 */
	virtual void reset() = 0;

	/**
	 * @brief Return address that we are currently connecting or connected
	 *        to.  Returns 0.0.0.0 if not connected.
	 */
	virtual const netadr_t& getAddress() = 0;

	/**
	 * @brief Get hash of password used to connect to server.  Empty if no
	 *        password was specified.
	 */
	virtual const std::string& getPasswordHash() = 0;

	/**
	 * @brief Start connecting to a given server.
	 *
	 * @param strAddress Address of server, possibly with port.
	 * @param strPassword Password used by server.
	 * @return True if connection process could be started.
	 */
	virtual bool startConnect(const std::string& strAddress,
	                          const std::string& strPassword = "") = 0;

	/**
	 * @brief Start reconnecting to the previously connected server.
	 *
	 * @return True if reconnection process could be done.
	 */
	virtual bool startReconnect() = 0;

	/**
	 * @brief Returns true if we can send a connection packet to the server.
	 */
	virtual bool canRetryConnect() = 0;

	/**
	 * @brief Returns true if we are currently connecting to a server.
	 */
	virtual bool isConnecting() = 0;

	/**
	 * @brief Returns true if we are currently connected to a server.
	 */
	virtual bool isConnected() = 0;

	/**
	 * @brief Returns true if the passed address is OK to be receiving packets
	 *        from.
	 */
	virtual bool isValidAddress(const netadr_t& cAddress) = 0;

	/**
	 * @brief Modify state after successfully getting server info packet.
	 */
	virtual void onGotServerInfo() = 0;

	/**
	 * @brief Modify state after sending connection packet.
	 */
	virtual void onSentConnect() = 0;

	/**
	 * @brief Modify state after connecting successfully.
	 */
	virtual void onConnected() = 0;

	/**
	 * @brief Modify state after disconnecting from a server.
	 */
	virtual void onDisconnect() = 0;

	static ClientState& get();
};
