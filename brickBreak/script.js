var canvas;
var canvasContext;

var ballX = 75, ballY = 75;
var INIT_BALLSPEEDX = 5;
var INIT_BALLSPEEDY = 5;
var ballSpeedX = INIT_BALLSPEEDX, ballSpeedY = INIT_BALLSPEEDY;

var PADDLE_WIDTH = 200, PADDLE_HEIGHT = 10, PADDLE_OFFSET = 5;
var paddleX;
var deltaX;

window.onload = function(){
    canvas = document.getElementById('gameCanvas');
    canvasContext = canvas.getContext('2d');

    canvas.addEventListener('mousemove',function(evt){
        var mousePos = getMousePos(evt);
        paddleX = mousePos.x - (PADDLE_WIDTH/2);
    });

    var framesPerSecond = 30;
    setInterval(function(){
        moveBall();
        render();
    },1000/framesPerSecond);
};

function moveBall(){
    if(ballX > canvas.width || ballX < 0)
        ballSpeedX *= -1;
    if(ballY < PADDLE_OFFSET+PADDLE_HEIGHT){
        if(ballX > paddleX && ballX < paddleX+PADDLE_WIDTH){
          ballSpeedX = -ballSpeedX;
          deltaX = ballX - paddleX+PADDLE_WIDTH/2;
          ballSpeedX = deltaX*0.3;
        }
        else
            ballReset();
    }
    if(ballY > canvas.height)
        ballSpeedY *= -1;

    ballX += ballSpeedX;
    ballY += ballSpeedY;
}

function ballReset(){
    ballSpeedY = INIT_BALLSPEEDY;
    ballSpeedX = INIT_BALLSPEEDX;
    ballX = canvas.width/2;
    ballY = canvas.height/2;
}

function render(){
    drawRectangle('black',0,0,canvas.width,canvas.height);
    drawRectangle('white',
        paddleX,
        canvas.height-PADDLE_HEIGHT-PADDLE_OFFSET,
        PADDLE_WIDTH,
        PADDLE_HEIGHT);
    drawCircle('white',ballX,ballY,10);
}

function drawRectangle(color,posX,posY,width,height){
    canvasContext.fillStyle = color;
    canvasContext.fillRect(posX,posY,width,height);
}

function drawCircle(color,posX,posY,radius){
    canvasContext.fillStyle = color;
    canvasContext.beginPath();
    canvasContext.arc(posX,posY,radius,0,Math.PI*2,true);
    canvasContext.fill();
}

function getMousePos(evt){
    var rect = canvas.getBoundingClientRect();
    var  root = document.documentElement;

    var mouseX = evt.clientX - rect.left - root.scrollLeft;
    var mouseY = evt.clientY - rect.top - root.scrollTop;

    return {
        x : mouseX,
        y: mouseY
    };
}

