var EventEmitter = require('events').EventEmitter;
var util = require('util');
var dgram = require('dgram');
var PNG = require('png').Png;
var fs = require('fs');
var engine = require('engine.io');
var static = require('node-static');

var fileServer = new static.Server('./app');

var http = require('http').createServer(function(request, response) {
    request.addListener('end', function () {
        //console.log("Serve file");
        fileServer.serve(request, response);
    }).resume();
}).listen(8080);

var server = engine.attach(http);
var sockets = [];
function removeSocket(s) { sockets.splice(sockets.indexOf(s), 1); }
function broadcast(msg) {
    var data = JSON.stringify(msg);
    sockets.forEach(function(s) {
        s.send(data);
    });
}

server.on('connection', function(socket) {
    sockets.push(socket);
    socket.on('message', function(msg) {
        console.log(">"+msg);
        client0.send(msg);
    });
    socket.on('close', function() {
        removeSocket(socket);
    });
});


var CVRemoteClient = function(_port, _ip) {
    this.port = _port || 9988;
    this.ip = _ip || "127.0.0.1";
    this.src = dgram.createSocket('udp4');
    this.receivedParams = false;
    this.params = {};
    this.img_width = 0;
    this.img_height = 0;
    var self = this;
    this.src.on("message", function(data, rinfo) {
        self.handleMessage(data, rinfo);
    });
}
util.inherits(CVRemoteClient, EventEmitter);

CVRemoteClient.prototype.handleMessage = function(buf, rinfo) {
    var prefix = buf.slice(0,1).toString('ascii');
    var content = buf.slice(1);
    if (prefix == '0') {
        var params = content.toString('ascii').split("\n");
        this.params = {};
        for (var i = 0; i < params.length; i++) {
            var param = params[i].split(':');
            if (param.length == 2) {
                this.params[param[0]] = param[1];
            }
        }
        console.log(util.inspect(this.params));
        this.receivedParams = true;
    } else if (this.receivedParams) {
        var param = this.params[prefix];
        //console.log("param: "+param);
        if (param == "IMG") {
            if (content.length != this.img_width * this.img_height) {
                //console.log("Wrong pix count, requesting size");
                //console.log(content.length + " vs " + this.img_width + "x" + this.img_height + "=" + this.img_width * this.img_height);
                this.send("GET SIZE");
            } else {
                var png = new PNG(content, this.img_width, this.img_height, 'gray', 8);
                png.encode(function(data, err) {
                    if (err) console.log('Error: ' + error.toString());
                    else broadcast({
                        param: param,
                        value: data.toString('base64')
                    });
                });
            }
        } else if (param == "SIZE") {
            var vals = content.toString('ascii').split(' ');
            this.img_width = parseInt(vals[0]);
            this.img_height = parseInt(vals[1]);
            //console.log('New img size: '+this.img_width+'x'+this.img_height);
            /*
            broadcast({
                param: param,
                value: content.toString('ascii')
            });
            */
        } else {
            broadcast({
                param: param,
                value: content.toString()
            });
        }
    } else {
        this.send("GET PARAMS");
    }

    console.log("Recieved data: "+prefix+" ("+content.length+")");
}

CVRemoteClient.prototype.setPort = function(port) {
    this.port = port;
}

CVRemoteClient.prototype.setIP = function(ip) {
    this.ip = ip;
}


CVRemoteClient.prototype.send = function(m) {
    var msg = new Buffer(m);
    this.src.send(msg, 0, msg.length, this.port, this.ip);
}

CVRemoteClient.prototype.init = function() {
    this.receivedParams = false;
    this.params = {};
    this.send("GET PARAMS");
}

var client0 = new CVRemoteClient();
//var client0 = new CVRemoteClient(9988, "192.168.6.112");

client0.init();
