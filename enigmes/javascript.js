var canvas, context;

var draw = false;
var x, y;

function my_draw(){
	//on veut dessiner dans la zone du canvas
	canvas = document.getElementById("my_canvas");
	context = canvas.getContext("2d");
	
	//couleur et épaisseur par défault
	context.strokeStyle = "black";
    context.lineWidth = "1";

	//on ajoute des evenements en fonction de ce que l'utilisateur fera
	canvas.addEventListener("mousedown", click, false);
	canvas.addEventListener("mouseup", unclick, false);
	canvas.addEventListener("mousemove", move, false);
}

function click(){
	//si on clique, on commence à dessiner un chemin à partir des coordonnées de la souris (x, y)
	context.beginPath();
	context.moveTo(x, y);

	//si on clique, on commence à dessiner, donc draw passe à true
	draw = true;
}

function getCursorCoord(event){
	//récupère les coordonnées du curseur s'il est dans le canvas	
	var mouseX = 0;
	var mouseY = 0;
	
	while(event && !isNaN(event.offsetLeft) && !isNaN(event.offsetTop)) {
        mouseX += event.offsetLeft - event.scrollLeft;
        mouseY += event.offsetTop - event.scrollTop;
        event = event.offsetParent;
    }
    return { top: mouseY, left: mouseX };
}

function unclick(){
	//si on declique, on arrête de dessiner, donc draw passe à false
	if(draw){
		draw = false;
	}
}

function move(event){
	if(event.offsetX || event.offsetY) {
		x = event.pageX - getCursorCoord(document.getElementById("my_canvas")).left - window.pageXOffset;
		y = event.pageY - getCursorCoord(document.getElementById("my_canvas")).top - window.pageYOffset;
	}
	else if(event.layerX || event.layerY) {
		x = (event.clientX + document.body.scrollLeft + document.documentElement.scrollLeft) - getCursorCoord(document.getElementById("my_canvas")).left - window.pageXOffset;
		y = (event.clientY + document.body.scrollTop + document.documentElement.scrollTop) - getCursorCoord(document.getElementById("my_canvas")).top;
	}			
	// If started is true, then draw a line
	if(draw) {
		context.lineTo(x, y);
		context.stroke();	
	}
}

//supprime le dessin
function clear_canvas(){
	context.clearRect(0, 0, 800, 800);
}

//modifie la couleur
function newColor(color){
	context.strokeStyle = color;
}

//modifie l'épaisseur du trait
function newSize(width){
	context.lineWidth = width;
}
