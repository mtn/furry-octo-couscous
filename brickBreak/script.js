/*jshint esversion: 6 */

var canvas;
var canvasContext;

const BALL_RADIUS = 10;
var ballX = 75, ballY = 75;
var INIT_BALLSPEEDX = 5;
var INIT_BALLSPEEDY = 5;
var ballSpeedX = INIT_BALLSPEEDX, ballSpeedY = INIT_BALLSPEEDY;

const PADDLE_WIDTH = 200, PADDLE_HEIGHT = 10, PADDLE_OFFSET = 30;
var paddleCenter;
var paddleX;
var deltaX;

const BRICK_W = 80;
const BRICK_H = 20;
const BRICK_GAP = 2;
const BRICK_COLS = 10;
const BRICK_ROWS = 14;

var brickGrid = new Array(BRICK_COLS*BRICK_ROWS);

window.onload = function(){
    canvas = document.getElementById('gameCanvas');
    canvasContext = canvas.getContext('2d');
    resetBricks();

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

function resetBricks(){
    for(var i= 0; i < BRICK_COLS*BRICK_ROWS; i++){
            if(Math.random() < 0.5)
                brickGrid[i] = 1;
            else
                brickGrid[i] = 0;
    }
}

function moveBall(){
    if(ballX > canvas.width || ballX < 0)
        ballSpeedX *= -1;
    if(ballY > canvas.height)
        ballReset();
    else if(ballY > canvas.height - PADDLE_OFFSET-PADDLE_HEIGHT-BALL_RADIUS){
        if(ballX > paddleX && ballX < paddleX+PADDLE_WIDTH && ballSpeedY > 0){
            ballSpeedY = -ballSpeedY;
            paddleCenter = paddleX + PADDLE_WIDTH/2;
            deltaX = ballX - paddleCenter;
            ballSpeedX = deltaX*0.3;
        }
    }
    if(ballY < BALL_RADIUS)
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
    drawCircle('white',ballX,ballY,BALL_RADIUS);
    drawBricks();
}

function drawBricks(){
    for(var col=0; col < BRICK_COLS; col++){
        for(var row = 0; row < BRICK_ROWS; row++){
            if(brickGrid[to2D(row,col)]){
                var brickLeftEdge = col * BRICK_W;
                var brickTopEdge = row * BRICK_H;
                drawRectangle('blue',brickLeftEdge,brickTopEdge,
                    BRICK_W-BRICK_GAP, BRICK_H-BRICK_GAP);
            }
        }
    }
}

function to2D(row,col){
    return row*col + row;
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

