var app = angular.module('remoteControlApp', []);
window._stream = false;

app.factory('socket', function($rootScope) {
    var socket = new eio.Socket();
    return {
        on: function (eventName, callback) {
          socket.on(eventName, function () {  
            var args = arguments;
            $rootScope.$apply(function () {
              callback.apply(socket, args);
            });
          });
        },
        send: function(data) {
            socket.send(data);
        },
        emit: function (eventName, data, callback) {
          socket.emit(eventName, data, function () {
            var args = arguments;
            $rootScope.$apply(function () {
              if (callback) {
                callback.apply(socket, args);
              }
            });
          })
        }
    };
});

function RemoteCTRL($scope, socket) {
    $scope.timers = {};

    $scope.log = "[START]";
    $scope.img = "iVBORw0KGgoAAAANSUhEUgAAAAIAAAABCAYAAAD0In+KAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3gYDDC8yqsQdOAAAAB1pVFh0Q29tbWVudAAAAAAAQ3JlYXRlZCB3aXRoIEdJTVBkLmUHAAAAC0lEQVQI12NggAIAAAkAAWMqFg0AAAAASUVORK5CYII=";
    $scope.query = "";
    $scope.stream = false;

    $scope.send = function() {
        socket.send($scope.query);
    }

    $scope.getParams = function() {
        socket.send('GET PARAMS');
    };

    $scope.getFrame = function() {
        socket.send('GET IMG');
    };


    socket.on('message', function(data){
        var msg = JSON.parse(data);
        if (msg.param == 'IMG') {
            //$scope.img = "data:image/png;base64,"+msg.value;
            $scope.img = msg.value;

            //document.getElementById("debug_img").src = "data:image/png;base64,"+msg.value;
            if ($scope.stream) {
                socket.send("GET IMG");
                socket.send("GET profile");
            }
        } else if (msg.param == 'profile') {
            $scope.timers = {};
            var timers = msg.value.split("\n");
            for (var i = 0; i < timers.length; i++) {
                var timer = timers[i].split(":");
                if (timer.length == 2) {
                    $scope.timers[timer[0]] = timer[1];
                }
            }
        } else {
            console.log(data);
            $scope.log = data + "\n" + $scope.log;
        }
    });

    socket.on('close', function(){
    });
    socket.on('open', function(){
    });
}
