// jshint esversion: 6

var canvas, canvasContext;

const BALLSPEEDX_INIT = 5;
const BALLSPEEDY_INIT = 7;
var ballX=200;
var ballY=600;
var ballSpeedX = BALLSPEEDX_INIT;
var ballSpeedY = BALLSPEEDY_INIT;

const BRICK_W = 80;
const BRICK_H = 20;
const BRICK_COLS = 10;
const BRICK_ROWS = 14;
const BRICK_OFFSET = 2;
var brickGrid = new Array(BRICK_COLS*BRICK_ROWS);
var bricksLeft;

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
        move();
        render();
    },1000/framesPerSecond);

    canvas.addEventListener('mousemove',updateMousePos);

};

function ballReset(){
    ballX = canvas.width/2;
    ballY = canvas.height/2;
}

function render(){
    // Draw the canvas
    drawRect(0,0,canvas.width,canvas.height,'black');

    // Draw the ball
    drawCircle(ballX,ballY,10,'white');

    // Draw the paddle
    drawRect(paddleX,canvas.height-PADDLE_DIST_FROM_EDGE,PADDLE_WIDTH,PADDLE_THICKNESS,'white');

    drawBricks();
}

function ballMove(){
    ballX += ballSpeedX;
    ballY += ballSpeedY;

    if(ballX < 0)
        ballSpeedX *= -1;
    else if(ballX > canvas.width)
        ballSpeedX *= -1;
    if(ballY < 0)
        ballSpeedY *= -1;
    else if(ballY > canvas.height){
        ballReset();
        resetBricks();
    }
}

function isAtBrickPos(col,row){
    if(col >= 0 && col < BRICK_COLS &&
       row >= 0 && row < BRICK_ROWS){

        var brickInd = getInd(col,row);
        return brickGrid[brickInd];
    } else{
        return false;
    }
}

function checkCollision() {
    var ballBrickCol = Math.floor(ballX / BRICK_W);
    var ballBrickRow = Math.floor(ballY / BRICK_H);
    var brickIndexUnderBall = getInd(ballBrickCol, ballBrickRow);

    if(ballBrickCol >= 0 && ballBrickCol < BRICK_COLS &&
        ballBrickRow >= 0 && ballBrickRow < BRICK_ROWS) {

        if(isAtBrickPos( ballBrickCol,ballBrickRow )) {
            brickGrid[brickIndexUnderBall] = false;
            bricksLeft--;
            console.log(bricksLeft);

            var prevBallX = ballX - ballSpeedX;
            var prevBallY = ballY - ballSpeedY;
            var prevBrickCol = Math.floor(prevBallX / BRICK_W);
            var prevBrickRow = Math.floor(prevBallY / BRICK_H);

            var bothTestsFailed = true;

            if(prevBrickCol != ballBrickCol) {
                if(isAtBrickPos(prevBrickCol, ballBrickRow) === false) {
                    ballSpeedX *= -1;
                    bothTestsFailed = false;
                }
            }
            if(prevBrickRow != ballBrickRow) {
                if(isAtBrickPos(ballBrickCol, prevBrickRow) === false) {
                    ballSpeedY *= -1;
                    bothTestsFailed = false;
                }
            }

            if(bothTestsFailed) { // armpit case, prevents ball from going through
                ballSpeedX *= -1;
                ballSpeedY *= -1;
            }

        }
    }
}

function updatePaddle(){
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

        if(bricksLeft === 0)
            resetBricks();
    }
}

function move(){
    ballMove();

    checkCollision();

    updatePaddle();
}

function resetBricks(){
    bricksLeft = 0;
    var i;
    for(i = 0; i < BRICK_COLS * 3; i++){
        brickGrid[i] = false;
    }
    for(; i < BRICK_COLS * BRICK_ROWS; i++){
        brickGrid[i] = true;
        bricksLeft++;
    }
}

function getInd(col,row){
    return BRICK_COLS*row+col;
}

function drawBricks(){
    for(var row = 0; row < BRICK_ROWS; row++){
        for(var col = 0; col < BRICK_COLS; col++){
            if(brickGrid[getInd(col,row)])
                drawRect(BRICK_W*col,BRICK_H*row,BRICK_W-BRICK_OFFSET,BRICK_H-BRICK_OFFSET,'blue');
        }
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

