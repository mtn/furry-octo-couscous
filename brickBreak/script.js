// jshint esversion: 6

var canvas, canvasContext;

const BALLSPEEDX_INIT = 10;
const BALLSPEEDY_INIT = 10;
var ballX=75;
var ballY=75;
var ballSpeedX = BALLSPEEDX_INIT;
var ballSpeedY = BALLSPEEDY_INIT;

const BRICK_W = 100;
const BRICK_H = 50;
const BRICK_COUNT = 4;
const BRICK_ROWS = 2;
const BRICK_COLS = 2;
const BRICK_OFFSET = 2;
var brickGrid = new Array(BRICK_ROWS*BRICK_COLS);

const PADDLE_WIDTH = 100;
const PADDLE_THICKNESS = 10;
const PADDLE_DIST_FROM_EDGE = 60;
var paddleX = 400;

var mouseX;
var mouseY;

function updateMousePos(evt){
    var rect = canvas.getBoundingClientRect();
    var root = document.documentElement;

    mouseX = evt.clientX - rect.left - root.scrollLeft;
    mouseY = evt.clientY - rect.top - root.scrollTop;

    paddleX = mouseX - PADDLE_WIDTH/2;
}

window.onload = function(){
    canvas = document.getElementById('gameCanvas');
    canvasContext = canvas.getContext('2d');
    resetBricks();

    var framesPerSecond = 30;
    setInterval(function() {
        render();
        move();
    },1000/framesPerSecond);

    canvas.addEventListener('mousemove',updateMousePos);

};

function ballReset(){
    ballX = canvas.width/2;
    ballY = canvas.height/2;
    ballSpeedX = BALLSPEEDX_INIT;
    ballSpeedY = BALLSPEEDY_INIT;
}

function render(){
    // Draw the canvas
    drawRect(0,0,canvas.width,canvas.height,'black');

    // Draw the ball
    drawCircle(ballX,ballY,10,'white');

    // Draw the paddle
    drawRect(paddleX,canvas.height-PADDLE_DIST_FROM_EDGE,PADDLE_WIDTH,PADDLE_THICKNESS,'white');

    mouseBrickX = mouseX/BRICK_W;
    mouseBrickY = mouseY/BRICK_H;
    colorText(mouseBrickX+","+mouseBrickY,mouseX,mouseY,'yellow');

    drawBricks();

}

function move(){
    ballX += ballSpeedX;
    ballY += ballSpeedY;

    if(ballX < 0)
        ballSpeedX *= -1;
    else if(ballX > canvas.width)
        ballSpeedX *= -1;
    if(ballY < 0)
        ballSpeedY *= -1;
    else if(ballY > canvas.height)
        ballReset();

    var paddleTopEdgeY = canvas.height - PADDLE_DIST_FROM_EDGE;
    var paddleBottomEdgeY = paddleTopEdgeY + PADDLE_THICKNESS;
    var paddleLeftEdgeX = paddleX;
    var paddleRightEdgeX = paddleLeftEdgeX + PADDLE_WIDTH;

    if(ballY > paddleTopEdgeY &&
       ballY < paddleBottomEdgeY &&
       ballX > paddleLeftEdgeX &&
       ballX < paddleRightEdgeX){

        ballSpeedY *= -1;

        var paddleCenterX = paddleX + PADDLE_WIDTH/2;
        var ballDistFromCenter = ballX - paddleCenterX;

        ballSpeedX = ballDistFromCenter * 0.35;
    }
}

function resetBricks(){
    for(var i = 0; i < BRICK_COUNT; i++){
        brickGrid[i] = true;
    }
}

function drawBricks(){
    for(var i = 0; i < BRICK_COUNT; i++){
        if(brickGrid[i])
            drawRect(BRICK_W*i,0,BRICK_W-BRICK_OFFSET,BRICK_H-BRICK_OFFSET,'blue');
    }
}

function drawRect(xPos,yPos,width,height,color){
    canvasContext.fillStyle = color;
    canvasContext.fillRect(xPos,yPos,width,height);
}

function drawCircle(centerX,centerY,radius,color){
    canvasContext.fillStyle = color;
    canvasContext.beginPath();
    canvasContext.arc(centerX,centerY,radius,0,Math.PI*2,true);
    canvasContext.fill();
}

function colorText(str,xPos,yPos,color){
    canvasContext.fillStyle = color;
    canvasContext.fillText(str,xPos,yPos);
}

