var net = require('net');

var client = new net.Socket();
client.connect(3030, '127.0.0.1', function() {
	console.log('Connected');
	client.write('tick');
});

client.on('data', function(data) {
	console.log('Received: ' + data);
  setTimeout(() => {client.write('tick'); })
});

client.on('close', function() {
	console.log('Connection closed');
});