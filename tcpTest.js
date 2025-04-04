const net = require('net');
const readline = require('readline');

const client = new net.Socket();
const HOST = '127.0.0.1'; // Change to server IP if needed
const PORT = 12301; // Change to the server's listening port

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

client.connect(PORT, HOST, () => {
    console.log(`Connected to server at ${HOST}:${PORT}`);
    promptUser();
});

client.on('data', (data) => {
    console.log(`\nReceived: ${data}`);
    promptUser();
});

client.on('close', () => {
    console.log('Connection closed');
    rl.close();
});

client.on('error', (err) => {
    console.error(`Connection error: ${err.message}`);
});

function promptUser() {
    rl.question('Enter message: ', (message) => {
        if (message.toLowerCase() === 'exit') {
            client.end();
            rl.close();
        } else {
            client.write(message);
            promptUser();
        }
    });
}
