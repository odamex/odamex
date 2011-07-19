#ifndef __CL_DEMO_H__
#define __CL_DEMO_H__

#include "doomtype.h"
#include "d_net.h"
#include <string>
#include <vector>


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
	
	void writeMessages(buf_t *netbuffer, bool usercmd);
	void readMessages(buf_t* netbuffer);
	void capture(const buf_t* netbuffer);

	bool isRecording() const { return (state == NetDemo::recording); }
	bool isPlaying() const { return (state == NetDemo::playing); }
	bool isPaused() const { return (state == NetDemo::paused); }
private:
	void cleanUp();
	void copy(NetDemo &to, const NetDemo &from);
	void error(const std::string message);
	void reset();

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
		int		ticnum;
		int		offset;			// offset in the demo file
	} netdemo_index_entry_t;
	
	typedef struct
	{
		char    identifier[4];  // "ODAD"
		byte    version;
		byte    compression;    // type of compression used
		int		index_offset;	// offset from start of the file for the index
		int     index_size;     // gametic filepos index follows header
		short	index_spacing;	// number of gametics between indices
		byte    reserved[48];   // for future use
	} netdemo_header_t;
		
	netdemo_state_t		state;
	netdemo_state_t		oldstate;	// used when unpausing
	std::string			filename;
	FILE*				demofp;
	std::vector<byte>	cmdbuf;
	size_t				bufcursor;

	buf_t*				netbuf;

	netdemo_header_t	header;	
	std::vector<netdemo_index_entry_t> index;
};




#endif

