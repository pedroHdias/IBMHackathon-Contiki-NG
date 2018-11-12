//comunicação com a rede WSN
var dgram = require('dgram');
var ManageServer = dgram.createSocket('udp6');
var PORT = 8000;
var HOST = 'fd00::1';
var mote = ' ';

//comunicação com a aplicação na plataforma IBM Watson IoT
var Client = require('ibmiotf');
var config = {
    "org" : " ",
    "id" : " ",
    "domain": "internetofthings.ibmcloud.com",
    "type" : " ",
    "auth-method" : "token",
    "auth-token" : " "
};
var deviceClient = new Client.IotfDevice(config);
deviceClient.connect();

ManageServer.on('listening', () => {
    const address = ManageServer.address();
    console.log(`server listening ${address.address}:${address.port}`);
});

ManageServer.on('message', (msg, rinfo) => {
        console.log(`server got: ${msg} from ${rinfo.address}:${rinfo.port}`);
        // temp:value,humi:value,light:value
        var data = msg.toString().split(',').map(x => x.split(':')).reduce((obj, [k, v]) => {
            obj[k] = v;
            return obj;
        }, {});
        // //POST to IBM Cloud
        deviceClient.publish("status","json",JSON.stringify(data));  
	console.log(JSON.stringify(data));  
    });

deviceClient.on('connect', function () {
//Add your code here
    console.log("Connected")
});

deviceClient.on("command", function (commandName,format,payload,topic) {
    console.log(commandName,format,payload.toString('utf8'),topic) ;
    if(commandName === "update") {
        console.log('RECEIVED SMTG\n');
        var request = JSON.parse(payload.toString('utf8'));
        var actuator = request.actuator;
        var value = request.value;
        var message = actuator + ":" + value;
        ManageServer.send(message, 0, message.length, 10000, mote, function (err, bytes) {
            if (err) throw err;
            console.log('Message sent to mote ---- ' + message);
            console.log("\n");
        });
    } else {
        console.log("Command not supported.. " + commandName);
    }
});

ManageServer.bind(PORT, HOST);