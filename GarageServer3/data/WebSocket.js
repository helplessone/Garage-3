var rainbowEnable = false;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);

var YELLOW = "#ffcc00";
var RED = "#ff0000";
var GREEN = "#009900";
var GRAY = "#999999";
var PURPLE = "#3333cc"

connection.onopen = function () {
    connection.send("Connect " + new Date());
};
connection.onerror = function (error) {
    console.log("WebSocket Error ", error);
};
connection.onmessage = function (e) {
    console.log("Server: ", e.data);
};
connection.onclose = function(){
    console.log("WebSocket connection closed");
};

setInterval(getJson, 1000);

function sendRGB() {
    var r = document.getElementById("r").value**2/1023;
    var g = document.getElementById("g").value**2/1023;
    var b = document.getElementById("b").value**2/1023;

    var rgb = r << 20 | g << 10 | b;
    var rgbstr = "#"+ rgb.toString(16);
    console.log("RGB: " + rgbstr);
    connection.send(rgbstr);
}

function rainbowEffect(){
    rainbowEnable = ! rainbowEnable;
    if(rainbowEnable){
        connection.send("R");
        document.getElementById('rainbow').style.backgroundColor = '#00878F';
        document.getElementById('r').className = 'disabled';
        document.getElementById('g').className = 'disabled';
        document.getElementById('b').className = 'disabled';
        document.getElementById('r').disabled = true;
        document.getElementById('g').disabled = true;
        document.getElementById('b').disabled = true;
    } else {
        connection.send("N");
        document.getElementById('rainbow').style.backgroundColor = '#999';
        document.getElementById('r').className = 'enabled';
        document.getElementById('g').className = 'enabled';
        document.getElementById('b').className = 'enabled';
        document.getElementById('r').disabled = false;
        document.getElementById('g').disabled = false;
        document.getElementById('b').disabled = false;
        sendRGB();
    }
}

function buttonOne() {
	connection.send("1");
	console.log("Button One");
}
function buttonTwo() {
	connection.send("2");
	console.log('Button Two');
}
function buttonThree() {
	connection.send("3");
	console.log('Button Three');
}

function addButton (identifier) {

}

function b1function(button) {

	console.log("***** B1 Function ******");
	console.log ("Button.id = " + button.id);
}

function addDoorButton (doorName) {
	var b1 = document.createElement("BUTTON"); // Create Button
	b1.style.width = "95%";
	b1.style.height = "80px";
	b1.style.margin = "5px";
	b1.style.background = "blue";
	b1.style.borderRadius = "10px";
	b1.style.color = "white";
	b1.style.fontSize = "24px";
	b1.id = doorName;

	// Assign text to your button
	b1.textContent = doorName;

	// Register click handlers to call respective functions
	b1.onclick = function() {b1function(this);};

	// Append them in your DOM i.e to show it on your page.
	// Suppose to append it to an existing element in your page with id as "appp".
	var attachTo = document.getElementById("buttons");
	attachTo.appendChild(b1);
}

function loadControls(numDoors) {
	var i;
    var serverJSON = JSON.parse(getJson());
    console.log(serverJSON["d1text"] + " " + serverJSON["d2text"] + " " + serverJSON["d2text"]);
	addDoorButton(serverJSON["d2text"]);
	addDoorButton(serverJSON["d3text"]);
	addDoorButton(serverJSON["d1text"]);
}

function settingsButton() {
	console.log("Settings Button");
}

setInterval(getJson, 2000);

function getJson(){
/*	console.log ("Get JSON"); */
/*  console.log("d1button.value = " + document.getElementById("d1button").innerHTML); */
    var Httpreq = new XMLHttpRequest(); // a new request
    Httpreq.open("GET","/var.json",false);
    Httpreq.send(null);
    console.log("Response " + Httpreq.responseText);
    var serverJSON = JSON.parse(Httpreq.responseText);
    if (serverJSON["d1color"] == 0) document.getElementById("d1button").style.backgroundColor = RED;
    if (serverJSON["d1color"] == 1) document.getElementById("d1button").style.backgroundColor = GREEN;
    if (serverJSON["d1color"] == 2) document.getElementById("d1button").style.backgroundColor = YELLOW;
    if (serverJSON["d1color"] == 3) document.getElementById("d1button").style.backgroundColor = GRAY;
    if (serverJSON["d1color"] == 4) document.getElementById("d1button").style.backgroundColor = PURPLE;
    if (serverJSON["d2color"] == 0) document.getElementById("d2button").style.backgroundColor = RED;
    if (serverJSON["d2color"] == 1) document.getElementById("d2button").style.backgroundColor = GREEN;
    if (serverJSON["d2color"] == 2) document.getElementById("d2button").style.backgroundColor = YELLOW;
    if (serverJSON["d2color"] == 3) document.getElementById("d2button").style.backgroundColor = GRAY;
    if (serverJSON["d2color"] == 4) document.getElementById("d2button").style.backgroundColor = PURPLE;
    if (serverJSON["d3color"] == 0) document.getElementById("d3button").style.backgroundColor = RED;
    if (serverJSON["d3color"] == 1) document.getElementById("d3button").style.backgroundColor = GREEN;
    if (serverJSON["d3color"] == 2) document.getElementById("d3button").style.backgroundColor = YELLOW;
    if (serverJSON["d3color"] == 3) document.getElementById("d3button").style.backgroundColor = GRAY;
    if (serverJSON["d3color"] == 4) document.getElementById("d3button").style.backgroundColor = PURPLE;

    document.getElementById("d1button").innerHTML = serverJSON["d1text"];
    document.getElementById("d2button").innerHTML = serverJSON["d2text"];
    document.getElementById("d3button").innerHTML = serverJSON["d3text"];


	console.log(document.getElementById("d1button").innerHTML);
	console.log(document.getElementById("d2button").innerHTML);
	console.log(document.getElementById("d3button").innerHTML);

    return Httpreq.responseText;
}



