#ifndef __CL_DEMO_H__
#define __CL_DEMO_H__

#include "doomtype.h"
#include "i_net.h"
#include "d_net.h"
#include <string>
#include <vector>
#include <list>

class NetDemo
{
public:
	NetDemo();
	~NetDemo();
	NetDemo(const NetDemo &rhs);
	NetDemo& operator=(const NetDemo &rhs);
	
	bool startPlaying(const std::string &filename);
	bool startRecording(const std::string &filename);
	bool stopPlaying();
	bool stopRecording();
	bool pause();
	bool resume();
	
	void writeMessages();
	void readMessages(buf_t* netbuffer);
	void capture(const buf_t* netbuffer);
	void writeMapChange();
	void writeIntermission();

	bool isRecording() const { return (state == NetDemo::st_recording); }
	bool isPlaying() const { return (state == NetDemo::st_playing); }
	bool isPaused() const { return (state == NetDemo::st_paused); }
	
	int getSpacing() const { return header.snapshot_spacing; }
	
	void nextSnapshot();
	void prevSnapshot();
	void nextMap();
	void prevMap();

	void ticker();
	int calculateTimeElapsed();
	int calculateTotalTime();
	const std::vector<int> getMapChangeTimes();
	const std::string &getFileName() { return filename; }
	
private:
	typedef enum
	{
		st_stopped,
		st_recording,
		st_playing,
		st_paused
	} netdemo_state_t;

	typedef enum
	{
		msg_packet		= 0xAA,
		msg_snapshot
	} netdemo_message_t;

	typedef struct
	{
		byte		type;
		uint32_t	length;
		uint32_t	gametic;
	} message_header_t;

	typedef struct
	{
		uint32_t	ticnum;
		uint32_t	offset;			// offset in the demo file
	} netdemo_index_entry_t;
	
	void cleanUp();
	void copy(NetDemo &to, const NetDemo &from);
	void error(const std::string &message);
	void reset();

	const netdemo_index_entry_t *snapshotLookup(int ticnum) const;
	void writeLauncherSequence(buf_t *netbuffer);
	void writeConnectionSequence(buf_t *netbuffer);
	
	void readSnapshotData(byte *buf, size_t length);
	void writeSnapshotData(byte *buf, size_t &length);
	
	void writeSnapshotIndexEntry();
	void writeMapIndexEntry();
	void readSnapshot(const netdemo_index_entry_t *snap);
	void writeChunk(const byte *data, size_t size, netdemo_message_t type);
	bool writeHeader();
	bool readHeader();
	
	bool atSnapshotInterval();
	
	bool writeSnapshotIndex();
	bool readSnapshotIndex();
	bool writeMapIndex();
	bool readMapIndex();
	int getCurrentSnapshotIndex() const;
	int getCurrentMapIndex() const;
	
	void writeLocalCmd(buf_t *netbuffer) const;
	bool readMessageHeader(netdemo_message_t &type, uint32_t &len, uint32_t &tic) const;
	void readMessageBody(buf_t *netbuffer, uint32_t len);
	void writeFullUpdate(int ticnum);

	typedef struct
	{
		char		identifier[4];  		// "ODAD"
		byte		version;
		byte    	compression;    		// type of compression used
		uint16_t	snapshot_index_size;	// number of snapshots in the index
		uint32_t	snapshot_index_offset;	// offset from start of the file for the index
		uint16_t	map_index_size;			// number of maps in the mapindex
		uint32_t	map_index_offset;		// offset from start of the file for the mapindex
		uint16_t	snapshot_spacing;		// number of gametics between indices
		uint32_t	starting_gametic;		// the gametic the demo starts at
		uint32_t	ending_gametic;			// the last gametic of the demo
		byte		reserved[36];   		// for future use
	} netdemo_header_t;
	
	static const size_t HEADER_SIZE = 64;
	static const size_t MESSAGE_HEADER_SIZE = 9;
	static const size_t INDEX_ENTRY_SIZE = 8;

	static const uint16_t SNAPSHOT_SPACING = 20 * TICRATE;

	static const size_t MAX_SNAPSHOT_SIZE = 131072;
	
	netdemo_state_t		state;
	netdemo_state_t		oldstate;	// used when unpausing
	std::string			filename;
	FILE*				demofp;

	std::list<buf_t>	captured;

	netdemo_header_t	header;	
	std::vector<netdemo_index_entry_t> snapshot_index;
	std::vector<netdemo_index_entry_t> map_index;
	
	byte				snapbuf[NetDemo::MAX_SNAPSHOT_SIZE];
	int					netdemotic;
};




#endif

