#pragma once

#include <deque>
#include <fstream>

#include "i_net.h"
#include "MessageQueue.h"

struct PacketHeaderType;

class NetDemo
{
public:
	NetDemo() = default;
	~NetDemo();
	NetDemo(const NetDemo &rhs)             = delete;
	NetDemo& operator=(const NetDemo &rhs)  = delete;

	NetDemo(NetDemo&&) = default;
	NetDemo& operator=(NetDemo&&) = default;


	bool startPlaying(const std::string &filename);
	bool startRecording(const std::string &filename);
	bool stopPlaying();
	bool stopRecording();
	bool pause();
	bool resume();
	bool seekNetdemotic(int requestedNetdemotic);
	bool seekGametic(int requestedGametic);

	void writeMessages();
	void readMessages(buf_t* netbuffer);
	void capture(const buf_t* netbuffer);
	void capture(const std::basic_string<byte>& buffer);
	void capturePacketHeader(const PacketHeaderType& header);
	void writeMapChange();
	void writeIntermission();


	[[nodiscard]] bool isRecording() const  { return (state == NetDemo::st_recording); }
	[[nodiscard]] bool isPlaying() const    { return (state == NetDemo::st_playing); }
	[[nodiscard]] bool isPaused() const     { return (state == NetDemo::st_paused); }
	[[nodiscard]] bool isInPlayback() const { return isPlaying() or isPaused(); }

	[[nodiscard]] int getSpacing() const { return header.snapshot_spacing; }

	[[nodiscard]] int getNetdemotic() const { return netdemotic; }
	[[nodiscard]] int getGametic() const    { return netdemotic + header.starting_gametic; }

	void nextTic();
	void prevTic();
	void nextSnapshot();
	void prevSnapshot();
	void nextMap();
	void prevMap();

	bool ticker();
	[[nodiscard]] int calculateTimeElapsed() const;
	[[nodiscard]] int calculateTotalTime() const;
	[[nodiscard]] const std::vector<int> getMapChangeTimes() const;
	[[nodiscard]] const std::string &getFileName() const { return filename; }

private:
	enum netdemo_state_t
	{
		st_stopped,
		st_recording,
		st_playing,
		st_paused
	};

	enum netdemo_message_t
	{
		msg_packet      = 0xAA,
		msg_snapshot,
		msg_map_change,
		msg_eof
	};

	struct message_header_t
	{
		byte        type    { 0 };
		uint32_t    length  { 0 };
		uint32_t    gametic { 0 };
	};

	struct netdemo_index_entry_t
	{
		uint32_t        ticnum  { 0 };
		std::streampos  offset  { 0 };  // offset in the demo file

		auto operator<=>(const netdemo_index_entry_t& other) const
		{
			return ticnum <=> other.ticnum;
		}
		auto operator<=>(uint32_t otherTic) const
		{
			return ticnum <=> otherTic;
		}
	};

	void cleanUp();
	void error(const std::string &message);
	void fatalError(const std::string &message);
	void reset();

	void writeLauncherSequence(buf_t *netbuffer);
	void writeConnectionSequence();

	void readSnapshotData(std::vector<byte>& buf);
	void writeSnapshotData(std::vector<byte>& buf);

	void writeChunk(const byte *data, size_t size, netdemo_message_t type);
	bool writeHeader();
	bool readHeader();

	bool atSnapshotInterval();

	void populateMessageIndexes();

	using SnapshotVector = std::vector<netdemo_index_entry_t>;

	[[nodiscard]] SnapshotVector::const_iterator getCurrentSnapshotIter      () const;
	[[nodiscard]] SnapshotVector::const_iterator getCurrentMapIter           () const;
	[[nodiscard]] SnapshotVector::const_iterator getSnapshotForGametic       (uint32_t gameticnum) const;
	[[nodiscard]] SnapshotVector::const_iterator getSnapshotForNetdemotic    (uint32_t netdemoticnum) const;
	[[nodiscard]] SnapshotVector::const_iterator getMapLoadSnapshotForGametic(uint32_t gameticnum) const;

	[[nodiscard]] SnapshotVector::const_iterator lookupSnapshot(const SnapshotVector& i_vector, uint32_t gameticnum) const;

	bool readSnapshot(SnapshotVector::const_iterator snap);

	bool readMessageHeader(netdemo_message_t &type, uint32_t &len, uint32_t &tic);
	void readMessageBody(buf_t *netbuffer, uint32_t len);

	static constexpr size_t         HEADER_SIZE = 64;
	static constexpr std::streamoff MESSAGE_HEADER_SIZE = 9;
	static constexpr size_t         INDEX_ENTRY_SIZE = 8;

	static constexpr uint16_t SNAPSHOT_SPACING = 20 * TICRATE;

	struct netdemo_header_id_t
	{
		char        identifier[4]   { 0, 0, 0, 0};  // "ODAD"
		byte        version         { 0 };          // 4, 3, etc...

		bool Read(std::fstream& io_stream);
	};

	// The following exists only for a remote chance of compatibility with old netdemos.
	// At the very least, we want to be able to make sense of the headers, but there's
	// zero guarantee of it working.  In fact, it's likely to not work because the message
	// content is almost certainly different enough to be non-functional with current
	// message body formats.
	struct netdemo_header3_t
	{
		netdemo_header_id_t id              {};     // version 3
		byte        compression             { 0 };  // type of compression used
		uint16_t    snapshot_index_size     { 0 };  // number of snapshots in the index
		uint32_t    snapshot_index_offset   { 0 };  // offset from start of the file for the index
		uint16_t    map_index_size          { 0 };  // number of maps in the mapindex
		uint32_t    map_index_offset        { 0 };  // offset from start of the file for the mapindex
		uint16_t    snapshot_spacing        { 0 };  // number of gametics between indices
		uint32_t    starting_gametic        { 0 };  // the gametic the demo starts at
		uint32_t    ending_gametic          { 0 };  // the last gametic of the demo
		byte        reserved[36]            { 0 };  // for future use

		bool Read(std::fstream& io_stream);
	};

    // Now for the current netdemo version.
	struct netdemo_header4_t
	{
		netdemo_header_id_t id      {};             // version 4
		byte        compression     { 0 };          // type of compression used
		uint16_t    snapshot_spacing{ 0 };          // number of gametics between indices
		uint32_t    starting_gametic{ 0 };          // the gametic the demo starts at
		uint32_t    ending_gametic  { 0 };          // the last gametic of the demo
		byte        reserved[48]    { 0 };          // for future use

		bool Read(std::fstream& io_stream);
		void Import(const netdemo_header3_t& oldHeader)
		{
			// we deliberately skip 'id' and 'reserved'.
			compression      = oldHeader.compression;
			snapshot_spacing = oldHeader.snapshot_spacing;
			starting_gametic = oldHeader.starting_gametic;
			ending_gametic   = oldHeader.ending_gametic;
		}
	};

	netdemo_state_t state   { st_stopped };
	netdemo_state_t oldstate{ st_stopped };   // used when unpausing
	std::string     filename{ };
	std::fstream    demofp  { };

	MessageQueue      captured {};
	buf_t             workingBuffer {MAX_UDP_PACKET};

	netdemo_header4_t   header        {};
	SnapshotVector      snapshot_index{};
	SnapshotVector      map_index     {};

	buf_t               outputBuffer    { NETDEMO_STARTUP_PACKET_SIZE };
	std::vector<byte>   snapbuf         { };
	int                 netdemotic      { 0 };
	int                 pause_netdemotic{ 0 };
	int                 last_map_tic    { 0 };
};
