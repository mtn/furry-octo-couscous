var canvas;
var canvasContext;

var ballX = 75, ballY = 75;
var ballSpeedX = 2, ballSpeedY = 2;

window.onload = function(){
    canvas = document.getElementById('gameCanvas');
    canvasContext = canvas.getContext('2d');

    var framesPerSecond = 30;
    setInterval(function(){
        move();
        render();
    },1000/framesPerSecond);
};

function move(){
    if(ballX > canvas.width || ballX < 0)
        ballSpeedX *= -1;
    if(ballY > canvas.height || ballY < 0)
        ballSpeedY *= -1;

    ballX += ballSpeedX;
    ballY += ballSpeedY;
}

function render(){
    drawRectangle('black',0,0,canvas.width,canvas.height);

    canvasContext.fillStyle = 'white';
    canvasContext.beginPath();
    canvasContext.arc(ballX,ballY,10,0,Math.PI*2,true);
    canvasContext.fill();
}

function drawRectangle(color,posX,posY,width,height){
    canvasContext.fillStyle = color;
    canvasContext.fillRect(posX,posY,width,height);
}


