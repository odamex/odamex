var dgram = require('dgram');
var util = require('util');

var options = {};
options.MAX_SERVER_AGE = 300000; // 5 minutes
options.PORT = 15000;

var SERVER_CHALLENGE = 5560020;
var LAUNCHER_CHALLENGE = 777123;

var servers = {};

var Server = function(address, port) {
	this.address = address;
	this.port = port;
	this.verified = true;
	this.timeoutId = setTimeout(this.timeout.bind(this), options.MAX_SERVER_AGE);
};
Server.prototype.heartbeat = function() {
	clearTimeout(this.timeoutId);
	this.timeoutId = setTimeout(this.timeout.bind(this), options.MAX_SERVER_AGE);
};
Server.prototype.timeout = function() {
	deleteServer(this.address, this.port);
};

function updateServer(address, port) {
	var key = address + ':' + port;
	if (key in servers) {
		servers[key].heartbeat();
		return;
	}
	servers[key] = new Server(address, port);
	util.log('Added new server: ' + key + ', ' + Object.keys(servers).length + " total");
}

function deleteServer(address, port) {
	var key = address + ':' + port;
	delete(servers[key]);
	util.log('Remote server timed out: ' + key + ', ' + Object.keys(servers).length + " total");
}

function writeServerData() {
	var serverBuffers = [];

	for (var server in servers) {
		if (!servers[server].verified) {
			continue;
		}

		var buffer = new Buffer(6);
		var octets = servers[server].address.split('.');
		for (var i = 0;i < 4;i++) {
			buffer.writeUInt8(parseInt(octets[0], 10), 0);
			buffer.writeUInt8(parseInt(octets[1], 10), 1);
			buffer.writeUInt8(parseInt(octets[2], 10), 2);
			buffer.writeUInt8(parseInt(octets[3], 10), 3);
		}
		buffer.writeInt16LE(servers[server].port, 4);
		serverBuffers.push(buffer);
	}

	header = new Buffer(6);
	header.writeInt32LE(LAUNCHER_CHALLENGE, 0);
	header.writeInt16LE(serverBuffers.length, 4);

	serverBuffers.unshift(header);
	return Buffer.concat(serverBuffers);
}

var master = dgram.createSocket('udp4');
master.on('message', function(msg, rinfo) {
	challenge = msg.readInt32LE(0);

	switch (challenge) {
	case SERVER_CHALLENGE:
		if (rinfo.size > 6) {
			util.log("Full response not implemented");
		} else if (rinfo.size == 6) {
			updateServer(rinfo.address, msg.readInt16LE(4));
		} else {
			updateServer(rinfo.address, rinfo.port);
		}
		break;
	case LAUNCHER_CHALLENGE:
		if (rinfo.size > 4) {
			util.log("Master syncing server list (ignored), IP = " + rinfo.address);
			return;
		}

		util.log("Client request IP = " + rinfo.address);
		var message = writeServerData();
		if (message.length > 576) {
			util.log("WARNING: Response is bigger than safe MTU");
		}
		master.send(message, 0, message.length, rinfo.port, rinfo.address);
		break;
	}
});
master.bind(options.port);
util.log("Odamex Master Started");