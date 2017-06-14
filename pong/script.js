/*jshint esversion: 6 */

var canvas;
var canvasContext;
var ballX = 50;
var ballSpeedX = 7;
var ballY = 50;
var ballSpeedY = 5;

var playerScore = 0;
var computerScore = 0;

var paddle1Y = 250;
var paddle2Y = 250;
var paddleCenter;

var dispWin = false;

const WINNING_SCORE = 3;

const INIT_BALLSPEEDY = 5;
const PADDLE_HEIGHT = 100;
const PADDLE_WIDTH = 10;

function calculateMousePosition(evt) {
    var rect = canvas.getBoundingClientRect();
    var root = document.documentElement;
    var mouseX = evt.clientX - rect.left - root.scrollLeft;
    var mouseY = evt.clientY - rect.top - root.scrollTop;
    return {
        x:mouseX,
        y:mouseY
    };
}

window.onload = function(){
  canvas = document.getElementById('gameCanvas');
  canvasContext = canvas.getContext('2d');

  var framesPerSecond = 30;
  setInterval(function(){
    move();
    render();
  },1000/framesPerSecond);

  canvas.addEventListener('mousedown',function(evt){
    if(dispWin){
      playerScore = 0;
      computerScore = 0;
      dispWin = false;
    }
  });

  canvas.addEventListener('mousemove',function(evt){
    var mousePos = calculateMousePosition(evt);
    paddle1Y = mousePos.y - PADDLE_HEIGHT/2;
  });
};

function ballReset(){
  if(playerScore >= WINNING_SCORE || computerScore >= WINNING_SCORE){
    dispWin = true;
  }
  ballSpeedX = -ballSpeedX;
  ballSpeedY = INIT_BALLSPEEDY;
  ballX = canvas.width/2;
  ballY = canvas.height/2;
}

function computerMvt(){
  if(paddle2Y+PADDLE_HEIGHT/2 < ballY+35) paddle2Y += 6;
  else if(paddle2Y+PADDLE_HEIGHT/2 > ballY-35) paddle2Y -= 6;
}

function move(){
  if(dispWin) return;

  computerMvt();

  ballX += ballSpeedX;
  ballY += ballSpeedY;

  var deltaY;
  if(ballX > canvas.width){
    if(ballY > paddle2Y && ballY < paddle2Y+PADDLE_HEIGHT){
      ballSpeedX = -ballSpeedX;
      paddleCenter = paddle2Y+PADDLE_HEIGHT/2;
      deltaY = ballY - paddleCenter;
      ballSpeedY = deltaY*0.3;
    }
    else{
      playerScore++;
      ballReset();
    }
  }
  else if(ballX <= 0){
    if(ballY > paddle1Y && ballY < paddle1Y+PADDLE_HEIGHT){
      ballSpeedX = -ballSpeedX;
      paddleCenter = paddle1Y + PADDLE_HEIGHT/2;
      deltaY = ballY - paddleCenter;
      ballSpeedY = deltaY*0.3;
    }
    else{
      computerScore++;
      ballReset();
    }
}
  if(ballY > canvas.height || ballY < 0) ballSpeedY = -ballSpeedY;
}

function render(){
  drawRect(0,0,canvas.width,canvas.height,'black');
  if(dispWin){
    canvasContext.fillStyle = 'white';
    if(playerScore >= WINNING_SCORE)
      canvasContext.fillText("Left player won!",350,200);
    else
      canvasContext.fillText("Right player won!",350,200);
    canvasContext.fillText("Click to continue.",350,500);
    return;
  }

  drawMidline();
  drawRect(0,paddle1Y,PADDLE_WIDTH,PADDLE_HEIGHT,'white');
  drawRect(canvas.width-PADDLE_WIDTH,paddle2Y,PADDLE_WIDTH,PADDLE_HEIGHT,'white');
  drawCircle(ballX,ballY,10,'white');

  canvasContext.fillText(playerScore,100,100);
  canvasContext.fillText(computerScore,canvas.width-100,100);
}

function drawMidline(){
  for(var i = 0; i < canvas.height; i+=40)
    drawRect(canvas.width/2-1,i,2,20,'white');
}

function drawCircle(centerX,centerY,radius,color) {
    canvasContext.fillStyle = color;
    canvasContext.beginPath();
    canvasContext.arc(centerX, centerY, radius, 0,Math.PI*2,true);
    canvasContext.fill();
}

function drawRect(leftX,topY,width,height,color){
  canvasContext.fillStyle = color;
  canvasContext.fillRect(leftX,topY,width,height);
}

