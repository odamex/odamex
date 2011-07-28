#ifndef __CL_DEMO_H__
#define __CL_DEMO_H__

#include "doomtype.h"
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
	
	bool startPlaying(const std::string filename);
	bool startRecording(const std::string filename);
	bool stopPlaying();
	bool stopRecording();
	bool pause();
	bool resume();
	
	void writeMessages();
	void readMessages(buf_t* netbuffer);
	void capture(const buf_t* netbuffer);

	bool isRecording() const { return (state == NetDemo::recording); }
	bool isPlaying() const { return (state == NetDemo::playing); }
	bool isPaused() const { return (state == NetDemo::paused); }
	
	int getSpacing() const { return header.snapshot_spacing; }
	
	void skipTo(buf_t *netbuffer, int ticnum);
private:
	void cleanUp();
	void copy(NetDemo &to, const NetDemo &from);
	void error(const std::string message);
	void reset();

	int snapshotLookup(int ticnum);
	void writeSnapshot(buf_t *netbuf);
	void readSnapshot(buf_t *netbuf, int ticnum);
	bool writeHeader();
	bool readHeader();
	bool writeIndex();
	bool readIndex();
	void writeLocalCmd(buf_t *netbuffer) const;
	bool readMessageHeader(uint32_t &len, uint32_t &tic) const;
	void readMessageBody(buf_t *netbuffer, uint32_t len);
	void writeFullUpdate(int ticnum);

	typedef enum
	{
		stopped,
		recording,
		playing,
		paused
	} netdemo_state_t;

	typedef struct
	{
		uint32_t	ticnum;
		uint32_t	offset;			// offset in the demo file
	} netdemo_snapshot_entry_t;
	
	typedef struct
	{
		char		identifier[4];  		// "ODAD"
		byte		version;
		byte    	compression;    		// type of compression used
		uint32_t	snapshot_index_offset;	// offset from start of the file for the index
		uint32_t	snapshot_index_size;	// gametic filepos index follows header
		uint16_t	snapshot_spacing;		// number of gametics between indices
		uint32_t	first_gametic;
		byte		reserved[44];   		// for future use
	} netdemo_header_t;
	
	static const size_t HEADER_SIZE = 64;
	static const size_t INDEX_ENTRY_SIZE = 8;

	static const uint16_t SNAPSHOT_SPACING = 5 * TICRATE;

	static const size_t MAX_SNAPSHOT_SIZE = MAX_UDP_PACKET;
	
	netdemo_state_t		state;
	netdemo_state_t		oldstate;	// used when unpausing
	std::string			filename;
	FILE*				demofp;
	std::vector<byte>	cmdbuf;
	size_t				bufcursor;

	std::list<buf_t>	captured;

	netdemo_header_t	header;	
	std::vector<netdemo_snapshot_entry_t> snapshot_index;
};




#endif

