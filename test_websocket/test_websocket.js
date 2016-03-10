var fs = require('fs');

// Loading index.html displayed to the client
var server = require('http').createServer(function(req, res) {
    fs.readFile('./index.html', 'utf-8', function(error, content) {
        res.writeHead(200, {"Content-Type": "text/html"});
        res.end(content);
    });
    
   fs.readFile('./wallix.png', function(err, buf){
        var image = fs.readFileSync('./wallix.png');
        res.writeHead(200, {'Content-Type': 'image/gif' });
        res.end(image);
        console.log('image file is initialized');
    });
});

var io = require('socket.io').listen(server);
server.listen(8080);
console.log('Server running at  http://localhost:8080');


io.sockets.on('connection', function (socket) {
    console.log('Un client est connecté !');
    socket.emit('info', { msg: 'The world is round, there is no up or down.' });
    socket.on('disconnect', function(){console.log('Un client s\'est déconnecté !');});
    socket.on("close", function () {console.log("Browser gone.");});
    
    
    
    
    
    
 
    /*
    socket.on('called', function(called){
        console.log("Request received." + called.request) ;
        socket.emit('notification', { msg:'Yes still here! Want some data?' });
    });
    */

    socket.emit(fs.createReadStream('wallix.png'));

    socket.on('message', function (str) {
     var ob = JSON.parse(str);
     switch(ob.type) {
     case 'text':
         console.log("Server @text : received from client : " + ob.content)
         var data = '{ "type":"text", "content":"Here is a text the client awaits!"}';
         socket.send({data:data, msg:'Client ? Want some data ?' });
         break;
     case 'image':
         console.log("\nServer @image : received from client : " + ob.content)
         var path ="wallix.png";
         console.log("path is " + path)
         //var data = '{ "type":"image", "path":"' + path + '"}';
        //socket.send({data:data, msg:'Are you still here Client ? Want some data?' }); 
        fs.exists(path, function() {
            console.log(path + " exists ")
            var data = { 
                    "type":"image",
                    "path":"' + path + '",
                    "content":"Here is an image the client awaits!",
                    id:"picture"
            };
            socket.send({data:data, msg:'Client ? Want some data ?' });
        });
         

         // socket.emit('image', {data:data, msg:'Yes still here! Want some data?' }); 
         break;
      }
    });

/*
    socket.on('image',function(){
        console.log("Image asked.");
        var path ="wallix.png";
        console.log("Sending image: " + path);
        fs.exists(path, function() {
            var data = JSON.stringify( {'image' : path } );
            socket.sendText(data); 
        });
    });
*/

});




